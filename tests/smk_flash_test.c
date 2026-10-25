/* SPDX-License-Identifier: GPL-3.0-only
 * SMK-37 Pro port: Copyright (C) 2026 M. Geisler (genkigenki/makkuro) */
/* SMK-37 Pro flash map (build with -DBOARD_SMK37=1):
 * every 4 KiB sector of the 1 MiB part through the app's allow-list (FL_RANGE_OK / FL_GUARD, fm1_flash.h),
 * the update loader's (loader.c) and ldr_core.c's record window: none of them may ever reach
 * the head 0x0-0x3FFF, BTIF / USRTRIM 0xC0000-0xC1FFF, USR 0xF4000-0xFDFFF or key_mac 0xFF000-;
 * X0X's store objects (storage.c), its user sample slots (app/slots.c), the store owner tag and the OTA
 * staging (ota.c) lie inside the allowed windows; the store and the slots do not overlap; the Bluetooth
 * block is one 64 KiB piece of the app area; ranges that wrap or straddle a boundary are refused. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fm1_cc.h"
#if !BOARD_SMK37
#error "build with -DBOARD_SMK37=1"
#endif

#include "fm1_flash_map.h"

static int st_read(uint32_t off, void *dst, uint32_t n) { (void)off; (void)dst; (void)n; return -1; }
static int st_erase(uint32_t off) { (void)off; return -1; }
static int st_prog(uint32_t off, const void *src, uint32_t n) { (void)off; (void)src; (void)n; return -1; }
#include "../firmware/src/app/plat.h"         /* X0X: the object map */
#include "../firmware/src/storage.c"
#define SMP_USER_XIP(k) ((const uint8_t *)0)  /* (slots.c: only its map is used here) */
#include "../firmware/src/app/slots.c"
#include "../firmware/src/app/boot_screen.h"
#define LDR_RANGE_OK(off, n) FL_LDR_RANGE_OK(off, n)       /* what loader.c uses on the SMK */

static int bad;
static void check(const char *what, int ok) { printf("smk flash: %-78s %s\n", what, ok ? "ok" : "FAIL"); bad += !ok; }

static int protected_sector(uint32_t s)
{
    return s < 0x4000u || (s >= 0xC0000u && s < 0xC2000u) || (s >= 0xF4000u && s < 0xFE000u) || s >= 0xFF000u;
}

int main(void)
{
    uint32_t s, o, c, k, leak_app = 0, leak_ldr = 0, leak_guard = 0, allowed = 0, in_store = 1, slots_ok = 1;
    for (s = 0; s < 0x100000u; s += 0x1000u) {
        if (!protected_sector(s))
            continue;
        leak_app += FL_RANGE_OK(s, 0x1000u) || FL_STORE_OK(s, 0x1000u) || FL_RANGE_OK(s, 256u) || FL_RANGE_OK(s + 0xF00u, 256u);
        leak_ldr += LDR_RANGE_OK(s, 0x1000u) || LDR_RANGE_OK(s + 0xF00u, 256u);
        leak_guard += !FL_PROTECTED(s, 0x1000u) || !FL_PROTECTED(s + 0xF00u, 256u);
    }
    for (s = 0; s < 0x100000u; s += 0x1000u)
        allowed += FL_GUARD(s, 0x1000u);
    check("no protected sector passes the app's allow-list (erase, page)", leak_app == 0);
    check("no protected sector passes the loader's allow-list", leak_ldr == 0);
    check("the RAM driver's own net (FL_PROTECTED) covers every protected sector", leak_guard == 0);
    check("straddling ranges refused: 0xBF000+0x2000, 0xF3000+0x2000, 0x3000+0x2000",
          !FL_GUARD(0xBF000u, 0x2000u) && !FL_GUARD(0xF3000u, 0x2000u) && !LDR_RANGE_OK(0xBF000u, 0x2000u) &&
          !LDR_RANGE_OK(0x3000u, 0x2000u) && FL_PROTECTED(0xBF000u, 0x2000u) && FL_PROTECTED(0x3F00u, 0x200u));
    check("wrapping ranges refused (off + n past 4 GiB)",
          !FL_GUARD(0xBB000u, 0xFFFFF000u) && !LDR_RANGE_OK(0x9D000u, 0xFFFFFF00u) && FL_PROTECTED(0xBB000u, 0xFFFFF000u));
    check("app windows: FL_DATA 0x9D000-0xAFFFF, FL_SMPV 0xB0000-0xBAFFF, FL_OTA 0xBB000-0xBFFFF, FL_SMP 0xC2000-0xF3FFF",
          FL_GUARD(0x9D000u, 0x1000u) && FL_GUARD(0xAF000u, 0x1000u) && FL_GUARD(0xB0000u, 0x1000u) &&
          FL_GUARD(0xBA000u, 0x1000u) && FL_GUARD(0xBF000u, 0x1000u) && FL_GUARD(0xC2000u, 0x1000u) &&
          FL_GUARD(0xF3000u, 0x1000u) && !FL_GUARD(0xF4000u, 0x1000u) && !FL_GUARD(0x9C000u, 0x1000u) &&
          FL_DATA_HI == 0xB0000u && allowed == (0xC0000u - 0x9D000u + 0xF4000u - 0xC2000u) / 0x1000u);
    check("FL_DATA (the store) and FL_SMPV (VM spare) do not overlap: no store sector in 0xB0000-0xBAFFF",
          FL_IN(0xAF000u, 0x1000u, FL_DATA_LO, FL_DATA_HI) && !FL_IN(0xB0000u, 0x1000u, FL_DATA_LO, FL_DATA_HI) &&
          FL_IN(0xB0000u, 0xB000u, FL_SMPV_LO, FL_SMPV_HI));
    check("Bluetooth block 0x8B000-0x9AFFF: 64 KiB, one piece, sector aligned, in FL_APP below the app slot end",
          FL_BLE_LO == 0x8B000u && FL_BLE_HI == 0x9B000u && FL_BLE_HI - FL_BLE_LO == 0x10000u &&
          FL_LDR_RANGE_OK(FL_BLE_LO, FL_BLE_HI - FL_BLE_LO) && !FL_GUARD(FL_BLE_LO, 0x1000u) &&
          FL_BLE_HI <= 0x4120u + 0x97400u && FL_APP_CODE_MAX == 0x86EE0u);
    check("loader: app area 0x4000-0x9CFFF, records 0x9D000-0xBFFFF only",
          LDR_RANGE_OK(0x4000u, 0x1000u) && LDR_RANGE_OK(0x9C000u, 0x1000u) && LDR_RANGE_OK(0xBF000u, 0x1000u) &&
          !LDR_RANGE_OK(0xC0000u, 0x1000u) && FL_REC_HI == 0xC0000u);
    for (o = 0; o < OBJ_NOBJ; o++)
        for (c = 0; c < 2u; c++)
            in_store &= FL_STORE_OK(st_sector(o, c), 0x1000u) && st_sector(o, c) < ST_OWNER_SECTOR;
    {   /* every object's two copies distinct */
        uint32_t o2, c2, distinct = 1;
        for (o = 0; o < OBJ_NOBJ; o++)
            for (c = 0; c < 2u; c++)
                for (o2 = 0; o2 < OBJ_NOBJ; o2++)
                    for (c2 = 0; c2 < 2u; c2++)
                        if ((o != o2 || c != c2) && st_sector(o, c) == st_sector(o2, c2))
                            distinct = 0;
        check("storage.c: every object (A/B) in FL_DATA below the owner tag, no two share a sector", in_store && distinct);
    }
    check("storage.c: settings 0x9D000/0x9E000, motion up to 0xAAFFF",
          st_sector(OBJ_SET, 0) == 0x9D000u && st_sector(OBJ_SET, 1) == 0x9E000u &&
          st_sector(OBJ_MOTION, 1) == 0xAA000u);
    check("boot screen A/B follows the objects, inside the store, short of the owner tag",
          BOOT_SEC0 == 0xAB000u && BOOT_SEC1 == 0xAC000u && st_sector(OBJ_MOTION, 1) + 0x1000u == BOOT_SEC0 &&
          FL_GUARD(BOOT_SEC0, 0x1000u) && FL_GUARD(BOOT_SEC1, 0x1000u) && BOOT_SEC1 + 0x1000u < ST_OWNER_SECTOR &&
          !FL_GUARD(0xFF000u, 0x1000u));
    check("store owner tag 0xAF000: in FL_DATA, the convention of every SMK port",
          ST_OWNER_SECTOR == 0xAF000u && FL_GUARD(ST_OWNER_SECTOR, 0x1000u));
    for (k = 0; k < SMP_USER_SLOTS; k++) {
        uint32_t k2;
        slots_ok &= FL_GUARD(SMP_USER_OFF(k), SMP_USER_SIZE(k)) && SMP_USER_OFF(k) % 0x1000u == 0u &&
                    SMP_USER_SIZE(k) % 0x1000u == 0u && SMP_USER_SIZE(k) <= SMP_USER_MAX &&
                    !FL_IN(SMP_USER_OFF(k), 0x1000u, FL_DATA_LO, FL_DATA_HI) &&
                    (SMP_USER_OFF(k) >= ST_OWNER_SECTOR + 0x1000u || SMP_USER_OFF(k) + SMP_USER_SIZE(k) <= ST_SMK_BASE);
        for (k2 = 0; k2 < SMP_USER_SLOTS; k2++)
            slots_ok &= k == k2 || SMP_USER_OFF(k) + SMP_USER_SIZE(k) <= SMP_USER_OFF(k2) ||
                        SMP_USER_OFF(k2) + SMP_USER_SIZE(k2) <= SMP_USER_OFF(k);
    }
    check("slots.c: 2 slots, 0xC2000 80 KiB, 0xD6000 120 KiB: allowed, apart, outside the store",
          SMP_USER_SLOTS == 2 && SMP_USER_OFF(0u) == 0xC2000u && SMP_USER_SIZE(0u) == 0x14000u &&
          SMP_USER_OFF(1u) == 0xD6000u && SMP_USER_SIZE(1u) == 0x1E000u && slots_ok);
    check("slots 1 and 2 keep the offsets of the 2 x 80 KiB layout (their data stays valid)",
          SMP_USER_OFF(0u) == 0xC2000u && SMP_USER_OFF(1u) == 0xC2000u + 0x14000u);
    check("no slot in the install range [0x4000, 0x9D000) or in the VM spare 0xB0000-0xBAFFF",
          SMP_USER_OFF(0u) >= 0x9D000u && SMP_USER_OFF(0u) >= 0xBB000u && SMP_USER_OFF(1u) >= 0xBB000u);
    check("FL_PLAIN_LO = the VM start 0x9D000", FL_PLAIN_LO == 0x9D000u);
    check("FL_JEDEC_OK: the SMK part 0xEB6014 and 0x856014 only", FL_JEDEC_OK(0xEB6014u) && FL_JEDEC_OK(0x856014u) &&
          !FL_JEDEC_OK(0u) && !FL_JEDEC_OK(0xFFFFFFu) && !FL_JEDEC_OK(0xEB6015u));
    printf("%s\n", bad ? "SMK FLASH TEST FAILED" : "smk flash test passed");
    return bad != 0;
}
