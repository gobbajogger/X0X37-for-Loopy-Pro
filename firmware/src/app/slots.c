/* SPDX-License-Identifier: GPL-3.0-only
 * User sample slots: adapted from Felucca's eng_sample.c, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hugelton Instruments. */
/* User sample slots (break loops), uploaded over USB with tools/upload_breaks.py
 * (or the break loops page): 2 slots, read in place through the XIP window. SMK-37 Pro: 80 KiB
 * then 120 KiB (see below). FM-1: 2 slots of 80 KiB at 0xA0000. Each slot has its own flash offset
 * and size. Slot = header (magic, count, name, data length, CRC32) + up to 16 zones + IMA ADPCM
 * data at +512. X0X plays zone 0 of a slot as a loop. */
typedef struct {
    uint32_t off, n, ls, le;     /* byte offset (from the slot data), samples, loop start / end */
    uint32_t rate;               /* source rate / 44100, Q16 */
    int16_t root16;
    int16_t pred;
    uint8_t idx, lo, hi, looped;
} smp_zone_t;
#if BOARD_SMK37 || defined(X0X_HOST)
/* SMK-37 Pro (fm1_flash_map.h; the host simulator, host/x0x_host.c, uses this map too). Slots 1 and 2 keep the flash offsets of the earlier layouts, so
 * their data stays valid: slot 1 0xC2000-0xD5FFF (80 KiB), slot 2 0xD6000-0xF3FFF (120 KiB), both in FL_SMP.
 * The 44 KiB window at 0xB0000-0xBAFFF (FL_SMPV) is not a sample slot. It is not added to these two:
 * protected BTIF / USRTRIM sits between that window and FL_SMP. */
#define SMP_USER_SLOTS 2
#define SMP_USER_OFF(k) ((k) == 0u ? 0xC2000u : 0xD6000u)
#define SMP_USER_SIZE(k) ((k) == 0u ? 0x14000u : 0x1E000u)
#define SMP_USER_MAX 0x1E000u                       /* the largest slot */
/* First 8 KiB of FL_SMPV: per-slot 4 KiB shadow of sector 0 so a mid-WRITE
 * cancel / failed END / power loss can put the last-good header+data back. */
#define SMP_SEC0_BACKUP 1
#define SMP_SEC0_BACKUP_OFF(k) (0xB0000u + (uint32_t)(k) * 0x1000u)
#ifdef FL_SMP_LO
_Static_assert(SMP_USER_SLOTS == 2u, "SMK: two user sample slots");
_Static_assert(SMP_USER_OFF(0u) == FL_SMP_LO && SMP_USER_OFF(0u) + SMP_USER_SIZE(0u) == SMP_USER_OFF(1u) &&
               SMP_USER_OFF(1u) + SMP_USER_SIZE(1u) == FL_SMP_HI, "SMK: slots 1 and 2 must fill FL_SMP");
_Static_assert(SMP_USER_OFF(0u) >= FL_SMPV_HI, "SMK: the sample slots must not use the VM spare");
_Static_assert(SMP_SEC0_BACKUP_OFF(0u) >= FL_SMPV_LO &&
               SMP_SEC0_BACKUP_OFF(SMP_USER_SLOTS - 1u) + 0x1000u <= FL_SMPV_HI,
               "SMK: sector-0 backup fits in FL_SMPV");
#endif
#else
/* FM-1: two slots of 80 KiB at 0xA0000 and 0xB4000. The old third slot at 0xC8000 is not offered,
 * and those bytes are not added to slot 2. */
#define SMP_USER_SLOTS 2
#define SMP_USER_OFF(k) (0xA0000u + (k) * 0x14000u)
#define SMP_USER_SIZE(k) ((void)(k), 0x14000u)
#define SMP_USER_MAX 0x14000u
#define SMP_SEC0_BACKUP 0
_Static_assert(SMP_USER_SLOTS == 2u, "FM-1: two user sample slots");
#endif
#define SMP_USER_DATA 512u
#define SMP_USER_MAGIC 0x504D5346u                  /* "FSMP" */
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint8_t nz, rsv;
    char name[8];
    uint32_t data_len, crc, rsv2[2];
    smp_zone_t zone[16];
} smp_user_hdr_t;                                   /* 32 + 16 x 28 = 480 B, data at +512 */
static uint8_t usr_nz[SMP_USER_SLOTS];

#ifndef SMP_USER_XIP                                /* host: a RAM image of the slots */
#define SMP_USER_XIP(k) fm1_xip_ptr(SMP_USER_OFF(k))
#endif
static const uint8_t *smp_user_xip(uint32_t k) { return SMP_USER_XIP(k); }

#include "../crc32_nibble.h"
/* Same zlib CRC-32 as storage.c st_crc32 / SMP_END — keep scan fail-closed with END. */
#define smp_data_crc x0x_crc32

/* (re)read slot k: a valid header with sane zones and matching data CRC makes it usable.
 * Without the CRC check, a restored sector-0 header over wiped higher-sector data
 * (mid-rewrite abort past 0x1000) would still set usr_nz. */
static void smp_user_scan(uint32_t k)
{
    const smp_user_hdr_t *h = (const smp_user_hdr_t *)smp_user_xip(k);
    uint32_t i;
    usr_nz[k] = 0;
    if (h->magic != SMP_USER_MAGIC || h->version != 1 || !h->nz || h->nz > 16u ||
        h->data_len > SMP_USER_SIZE(k) - SMP_USER_DATA)
        return;
    for (i = 0; i < h->nz; i++) {
        const smp_zone_t *z = &h->zone[i];
        if (z->off > h->data_len || z->n > 2u * SMP_USER_SIZE(k) || z->off + (z->n + 1u) / 2u > h->data_len ||
            z->idx > 88u || !z->rate || z->rate > 4u << 16)
            return;
    }
    if (smp_data_crc(smp_user_xip(k) + SMP_USER_DATA, h->data_len) != h->crc)
        return;
    usr_nz[k] = h->nz;
}
