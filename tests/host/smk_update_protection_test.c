/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the actual SMK allowlists and storage owner-migration path. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fm1_flash_map.h"
#include "../../firmware/src/app/plat.h"
static uint8_t flash[0x100000], before[0x100000];
static unsigned forbidden_writes;
static int st_read(uint32_t off, void *dst, uint32_t n)
{ memcpy(dst, flash + off, n); return 0; }
static int st_erase(uint32_t off)
{
    if (off >= 0xAB000u && off < 0xAF000u) forbidden_writes++;
    memset(flash + off, 255, 4096); return 0;
}
static int st_prog(uint32_t off, const void *src, uint32_t n)
{ assert(FL_STORE_OK(off, n)); memcpy(flash + off, src, n); return 0; }
#include "../../firmware/src/storage.c"
static void refused(uint32_t off, uint32_t n)
{
    assert(!FL_STORE_OK(off, n)); assert(!FL_RANGE_OK(off, n));
    assert(!FL_GUARD(off, n)); assert(!FL_LDR_RANGE_OK(off, n));
}
int main(void)
{
    uint32_t s;
    for (s = 0; s < sizeof flash; s += 4096u)
        if (s < 0x4000u || (s >= 0xC0000u && s < 0xC2000u) ||
            (s >= 0xF4000u && s < 0xFE000u) || s >= 0xFF000u) {
            refused(s, 4096); refused(s, 256); refused(s + 3840, 256);
            assert(FL_PROTECTED(s, 4096));
        }
    refused(0xBF000u, 8192); refused(0x3FFFu, 2);
    refused(0xF3FFFu, 2); refused(0xFEFFFu, 2);
    refused(0xBB000u, 0xFFFFF000u); refused(0xFFFFF000u, 8192);
    /* 1006085b: immutable external app/vendor data in the TEST profile. */
    for (s = 0xAB000u; s < 0xAF000u; s += 4096u) {
        refused(s, 4096); refused(s, 256); refused(s + 4095, 1);
    }
    refused(0xAAFFFu, 2); refused(0xAEFFFu, 2);
    assert(FL_STORE_OK(0xAA000u, 4096)); assert(FL_STORE_OK(0xAF000u, 4096));
    assert(FL_GUARD(0xBB000u, 0x5000u));
    assert(FL_LDR_RANGE_OK(0x4000u, 0x99000u));
    assert(FL_LDR_RANGE_OK(0xAF000u, 0x11000u));
    assert(FL_STORE_OK(0xC2000u, 0x32000u));
    for (unsigned erased = 0; erased < 2; erased++) {
        memset(flash, 0x5A, sizeof flash);
        if (erased) memset(flash + 0xAF000u, 255, 4096);
        memcpy(before, flash, sizeof flash); forbidden_writes = 0;
        st_owner_check(); assert(!forbidden_writes);
        for (s = 0; s < sizeof flash; s++)
            if (!((s >= 0x9D000u && s < 0xAB000u) ||
                  (s >= 0xAF000u && s < 0xB0000u))) assert(flash[s] == before[s]);
        assert(st_save(OBJ_SET, "x", 1) == 0);
        { char c; assert(st_load(OBJ_SET, &c, 1) == 1 && c == 'x'); }
    }
    puts("SMK TEST update protection: hardware/immutable sectors refused; owner migration preserves bytes PASS");
    return 0;
}
