/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * X0X: object map changed (see below). */
/* Persistent storage on the SPI NOR.
 *
 * Every object has an A/B sector pair. A save goes to the copy that is not
 * the current one: erase the sector, program the payload (from offset 256),
 * then the 32-byte header at offset 0 last. The header is the commit record;
 * on load the valid copy with the highest seq wins, so a torn write leaves
 * the previous copy in charge.
 *
 * Flash access goes through three hooks (also used by the host test):
 *   st_read(off, dst, n)   st_erase(off)   st_prog(off, src, n)
 */
#ifndef ST_MAINTENANCE_ADMIT
#define ST_MAINTENANCE_ADMIT() 1
#endif

#define ST_MAGIC 0x30583058u                   /* "X0X0": never mistaken for a Felucca object */
#define ST_SECTOR 4096u
#define ST_PAYLOAD_OFF 256u
#define ST_PAYLOAD_MAX (ST_SECTOR - ST_PAYLOAD_OFF)

/* flash map, X0X (plat.h OBJ_*): settings 0xFC000 (FL_GLOB); sound, patterns 1-8, patterns 9-16,
 * the song at 0x97000.. (2 sectors each, below the user sample slots at 0xA0000); the panel
 * calibration at 0xDC000 and motion at 0xDE000 (Felucca's user-preset area, unused by X0X). Nothing
 * above 0xDFFFF: 0xE0000.. is the update loader's staging area. */
typedef struct {
    uint32_t magic;
    uint16_t type, slot;
    uint32_t seq, len, crc, rsv[2];
    uint32_t hcrc;
} st_hdr_t;

static int st_read(uint32_t off, void *dst, uint32_t n);
static int st_erase(uint32_t off);
static int st_prog(uint32_t off, const void *src, uint32_t n);

#include "crc32_nibble.h"              /* nibble table + x0x_crc32 shared with project/slots/boot */
#define st_crc32 x0x_crc32             /* zlib CRC-32, 4 bits per step */

#if BOARD_SMK37
/* SMK-37 Pro: every object in FL_DATA 0x9D000..0xAFFFF (the stock VM), A/B
 * neighbours in plat.h order: settings 0x9D000, sound 0x9F000, patterns 1-8 0xA1000, patterns 9-16 0xA3000,
 * panel 0xA5000, song 0xA7000, motion 0xA9000 (..0xAAFFF); boot screen A/B at 0xAB000 and
 * 0xAC000 (app/boot_screen.h). A foreign-store clear erases those two with the
 * rest of the store. 0xAD000..0xAEFFF stays spare. The owner tag is at
 * 0xAF000 (st_owner_check). 0xB0000..0xBAFFF is not a sample slot and not the store: the
 * owner check never clears it. Never near BTIF / USRTRIM (0xC0000..0xC1FFF) or key_mac (0xFF000). */
#define ST_SMK_BASE 0x9D000u
_Static_assert(ST_SMK_BASE + 2u * OBJ_NOBJ * ST_SECTOR <= 0xAF000u, "SMK store: objects past the owner tag");
_Static_assert(ST_SMK_BASE + 2u * OBJ_NOBJ * ST_SECTOR == 0xAB000u, "boot screen sectors must follow the objects");
static uint32_t st_sector(uint32_t obj, uint32_t copy)  /* flash offset of copy A (0) / B (1) */
{
    return ST_SMK_BASE + ((obj % OBJ_NOBJ) * 2u + (copy & 1u)) * ST_SECTOR;
}

/* The owner of the store. Several FM-1 firmwares are ported to the SMK-37 Pro with the same board layer and
 * this same store area, each with its own object formats: a firmware must never parse another's data. The
 * sector 0xAF000 holds "SMK37DATA" + the firmware family (FELUCCA_DATA_FAMILY, build.py: X0X). Ours: load as
 * ever. None (erased): data of unknown origin, kept only if FELUCCA_DATA_UNTAGGED_OK (X0X: 0, cleared).
 * Another family's: clear the whole store area below the tag (0x9D000..0xAEFFF, the other firmware's objects
 * included) and take it. Samples (FL_SMP) validate themselves. Calibration, BT pairing and key_mac are outside
 * FL_DATA: never touched. Called once at boot, before the first store read (app/main_fm1.c). */
#ifndef FELUCCA_DATA_FAMILY
#define FELUCCA_DATA_FAMILY "X0X"
#endif
#ifndef FELUCCA_DATA_UNTAGGED_OK
#define FELUCCA_DATA_UNTAGGED_OK 0
#endif
#define ST_OWNER_SECTOR 0xAF000u
#ifndef ST_IDLE
#define ST_IDLE() ((void)0)                     /* (x0x.c: feed the watchdog between erases) */
#endif
_Static_assert(ST_OWNER_SECTOR + ST_SECTOR <= 0xB0000u, "SMK store owner: past FL_DATA (the VM spare follows)");
static uint8_t st_owner_cleared;                /* (the UI says so at boot) */
static void st_owner_check(void)
{
    static const char FAM[] = FELUCCA_DATA_FAMILY;
    uint8_t tag[16], want[16];
    uint32_t i, erased = 1, ours = 1, s;
    for (i = 0; i < 16u; i++)
        want[i] = i < 9u ? (uint8_t)"SMK37DATA"[i] : i - 9u < sizeof FAM - 1u ? (uint8_t)FAM[i - 9u] : 0u;
    if (st_read(ST_OWNER_SECTOR, tag, 16))
        return;
    for (i = 0; i < 16u; i++) {
        erased &= tag[i] == 0xFFu;
        ours &= tag[i] == want[i];
    }
    if (ours)
        return;
    if (!erased || !FELUCCA_DATA_UNTAGGED_OK) {     /* another firmware's store (or unknown): clear it */
        for (s = ST_SMK_BASE; s < ST_OWNER_SECTOR; s += ST_SECTOR) {
            st_erase(s);
            ST_IDLE();
        }
        st_owner_cleared = 1;
    }
    st_erase(ST_OWNER_SECTOR);
    st_prog(ST_OWNER_SECTOR, want, 16);
}
#else
static uint32_t st_sector(uint32_t obj, uint32_t copy)  /* flash offset of copy A (0) / B (1) */
{
    if (obj == OBJ_SET)
        return 0xFC000u + copy * ST_SECTOR;
    if (obj == OBJ_PANEL)
        return 0xDC000u + copy * ST_SECTOR;
    if (obj == OBJ_MOTION)
        return 0xDE000u + copy * ST_SECTOR;
    if (obj == OBJ_SONG)
        return 0x9D000u + copy * ST_SECTOR;
    return 0x97000u + (obj - OBJ_SOUND) * 2u * ST_SECTOR + copy * ST_SECTOR;   /* SOUND, PAT0, PAT1 */
}
#endif

static uint8_t st_buf[ST_PAYLOAD_MAX] __attribute__((aligned(4)));
static uint8_t st_stage_hold;             /* boot screen is filling st_buf; saves must not clobber it */

/* The boot screen stages its record in this existing buffer. A second buffer
 * would not fit. While held, load and save fail and leave the bytes alone. */
uint8_t *store_stage_lock(uint32_t *cap)
{
    if (!ST_MAINTENANCE_ADMIT())
        return 0;
    if (st_stage_hold)
        return 0;
    st_stage_hold = 1;
    if (cap)
        *cap = (uint32_t)sizeof st_buf;
    return st_buf;
}
void store_stage_unlock(void) { st_stage_hold = 0; }
int store_stage_held(void) { return st_stage_hold; }

static int st_head(uint32_t obj, uint32_t copy, st_hdr_t *h)   /* commit record valid: 0 */
{
    if (st_read(st_sector(obj, copy), h, sizeof *h))
        return -1;
    if (h->magic != ST_MAGIC || h->type != obj || h->len > ST_PAYLOAD_MAX ||
        h->hcrc != st_crc32(h, sizeof *h - 4u))
        return -1;
    return 0;
}

static int st_body(uint32_t obj, uint32_t copy, const st_hdr_t *h)   /* payload -> st_buf, CRC ok: 0 */
{
    if (st_read(st_sector(obj, copy) + ST_PAYLOAD_OFF, st_buf, h->len) || st_crc32(st_buf, h->len) != h->crc)
        return -1;
    return 0;
}

/* the current copy: the valid one with the highest seq (A on a tie), -1 when
 * neither is valid. Headers first, so only the winner's payload is read (it
 * is left in st_buf); *h gets its header. */
static int st_current(uint32_t obj, st_hdr_t *h)
{
    st_hdr_t a, b;
    int va = st_head(obj, 0, &a) == 0, vb = st_head(obj, 1, &b) == 0;
    if (vb && (!va || b.seq > a.seq)) {
        if (st_body(obj, 1, &b) == 0) {
            *h = b;
            return 1;
        }
        vb = 0;
    }
    if (va && st_body(obj, 0, &a) == 0) {
        *h = a;
        return 0;
    }
    if (vb && st_body(obj, 1, &b) == 0) {
        *h = b;
        return 1;
    }
    return -1;
}

/* Body CRC of one copy, through a stack window. Does not write st_buf: the
 * project object buffer is that same array, and it already holds the bytes
 * about to be saved. */
static int st_body_ok(uint32_t obj, uint32_t copy, const st_hdr_t *h)
{
    uint8_t tmp[256];
    uint32_t off = 0, c = 0xFFFFFFFFu;
    while (off < h->len) {
        uint32_t n = h->len - off;
        if (n > sizeof tmp)
            n = (uint32_t)sizeof tmp;
        if (st_read(st_sector(obj, copy) + ST_PAYLOAD_OFF + off, tmp, n))
            return 0;
        c = x0x_crc32_upd(c, tmp, n);
        off += n;
    }
    return (~c) == h->crc;
}

/* The current copy (0 or 1), or -1. Same rule as st_current: the valid body
 * with the highest seq, copy A on a tie. *seq is that copy's seq. */
static int st_pick(uint32_t obj, uint32_t *seq)
{
    st_hdr_t a, b;
    int va = st_head(obj, 0, &a) == 0, vb = st_head(obj, 1, &b) == 0;
    if (vb && (!va || b.seq > a.seq)) {
        if (st_body_ok(obj, 1, &b)) {
            *seq = b.seq;
            return 1;
        }
        vb = 0;
    }
    if (va && st_body_ok(obj, 0, &a)) {
        *seq = a.seq;
        return 0;
    }
    if (vb && st_body_ok(obj, 1, &b)) {
        *seq = b.seq;
        return 1;
    }
    return -1;
}

/* load object into dst (up to max bytes); returns the length, or -1 */
static int st_load(uint32_t obj, void *dst, uint32_t max)
{
    st_hdr_t h;
    if (!ST_MAINTENANCE_ADMIT())
        return -1;
    if (st_stage_hold)                            /* do not wipe a boot record being built */
        return -1;
    if (st_current(obj, &h) < 0)
        return -1;
    if (h.len > max)
        h.len = max;
    __builtin_memcpy(dst, st_buf, h.len);
    return (int)h.len;
}

static int st_save(uint32_t obj, const void *src, uint32_t len)
{
    uint32_t seq = 0, base, off;
    int cur, rc;
    st_hdr_t h;
    if (!ST_MAINTENANCE_ADMIT())
        return -8;
    if (st_stage_hold)                            /* refuse; the previous object stays on flash */
        return -8;
    if (len > ST_PAYLOAD_MAX)
        return -1;
    cur = st_pick(obj, &seq);                         /* leaves st_buf alone */
    if (cur < 0)
        seq = 0;
    base = st_sector(obj, cur == 0 ? 1u : 0u);       /* write the other copy */
    if ((const void *)src != (const void *)st_buf)
        __builtin_memcpy(st_buf, src, len);               /* the driver wants RAM sources */
    if ((rc = st_erase(base)) != 0)
        return rc;
    for (off = 0; off < len; off += 256u) {
        uint32_t n = len - off > 256u ? 256u : len - off;
        if ((rc = st_prog(base + ST_PAYLOAD_OFF + off, st_buf + off, n)) != 0)
            return rc;
    }
    h.magic = ST_MAGIC;
    h.type = (uint16_t)obj;
    h.slot = (uint16_t)(cur == 0 ? 1 : 0);
    h.seq = seq + 1u;
    h.len = len;
    h.crc = st_crc32(st_buf, len);
    h.rsv[0] = h.rsv[1] = 0xFFFFFFFFu;
    h.hcrc = st_crc32(&h, sizeof h - 4u);
    if ((rc = st_prog(base, &h, sizeof h)) != 0)       /* the commit record, last */
        return rc;
    {   /* read back: a write-protected or failing part must not report SAVED */
        st_hdr_t chk;
        uint32_t c = cur == 0 ? 1u : 0u;
        if (st_head(obj, c, &chk) || chk.seq != h.seq || st_body(obj, c, &chk))
            return -7;
    }
    return 0;
}
