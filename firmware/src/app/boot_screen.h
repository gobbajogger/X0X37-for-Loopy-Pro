/* SPDX-License-Identifier: GPL-3.0-only */
/* Boot screen record. One picture and two text lines, committed like the store:
 * the previous copy stays until the new copy's header is written last. A bad,
 * short, or torn record is not shown. Staging uses the existing store buffer
 * (st_buf); this file does not allocate another one.
 *
 * Payload (what the installer sends), little-endian:
 *   0 kind: 1 custom, 2 original words
 *   1 flags: bit 0 = picture follows
 *   2 line0 length (1..12), 3 line1 length (1..21) — custom only; 0 on original
 *   4 line0[12], 16 line1[21], 37 three zero bytes
 *   40 picture width, 42 height (240 and 96, or 0)
 *   44 bitmap, 240*96/8 bytes, MSB left, only when the picture flag is set
 * Sector: 32-byte commit header at 0, payload at 32. Header CRC covers the
 * header except its own CRC; payload CRC covers the payload. */
#ifndef X0X_BOOT_SCREEN_H
#define X0X_BOOT_SCREEN_H
#include <stdint.h>
#include "../crc32_nibble.h"

#define BOOT_MAGIC 0x42583058u              /* "X0XB" */
#define BOOT_VERSION 1u
#define BOOT_KIND_CUSTOM 1u
#define BOOT_KIND_FACTORY 2u
#define BOOT_F_PIC 1u
#define BOOT_LINE0_MAX 12u
#define BOOT_LINE1_MAX 21u
#define BOOT_PIC_W 240u
#define BOOT_PIC_H 96u
#define BOOT_PIC_STRIDE (BOOT_PIC_W / 8u)
#define BOOT_PIC_BYTES (BOOT_PIC_STRIDE * BOOT_PIC_H)   /* 2880 */
#define BOOT_PAY_FIXED 44u
#define BOOT_REC_MAX (BOOT_PAY_FIXED + BOOT_PIC_BYTES)  /* 2924 */
#define BOOT_HDR 32u
#define BOOT_PAY_OFF 32u
#define BOOT_MS 20000u
#define BOOT_OK 0
#define BOOT_BAD 1
#define BOOT_NOFLASH 2
#define BOOT_FLASH 3
#define BOOT_BUSY 4
#define BOOT_SIZE 5
#define BOOT_LATE 6

/* SMK: the two spare sectors after the seven store objects (0xAAFFF), still
 * below the owner tag. FM-1: the gap after the song object, and the unused
 * third globals sector (settings use 0xFC000 and 0xFD000). Each pair is
 * erasable separately, so a torn write keeps the other copy. */
#define BOOT_SMK_SEC0 0x000AB000u
#define BOOT_SMK_SEC1 0x000AC000u
#define BOOT_FM1_SEC0 0x0009F000u
#define BOOT_FM1_SEC1 0x000FE000u
#if BOARD_SMK37
#define BOOT_SEC0 BOOT_SMK_SEC0
#define BOOT_SEC1 BOOT_SMK_SEC1
#else
#define BOOT_SEC0 BOOT_FM1_SEC0
#define BOOT_SEC1 BOOT_FM1_SEC1
#endif

#define BOOT_FACTORY_0 "DnBOX"
#define BOOT_FACTORY_1 "909 808 CHICAGO MIAMI"

_Static_assert(sizeof(BOOT_FACTORY_0) - 1u <= BOOT_LINE0_MAX, "factory title");
_Static_assert(sizeof(BOOT_FACTORY_1) - 1u <= BOOT_LINE1_MAX, "factory subtitle");
_Static_assert(BOOT_REC_MAX <= 3840u, "boot record fits the existing store buffer");
_Static_assert(BOOT_PAY_OFF + BOOT_REC_MAX <= 4096u, "boot record fits one sector");
_Static_assert(BOOT_PIC_W % 8u == 0u, "picture rows are whole bytes");

typedef struct boot_io {
    int (*erase)(void *ctx, uint32_t off);   /* 0 = ok; one 4 KiB sector */
    int (*prog)(void *ctx, uint32_t off, const uint8_t *src, uint32_t n);
    int (*read)(void *ctx, uint32_t off, uint8_t *dst, uint32_t n);
    void *ctx;
} boot_io_t;

typedef struct boot_stage {
    uint8_t *buf;
    uint32_t cap, fill, t0;
    uint8_t armed;
    uint8_t committed; /* END already committed; a lost reply may repeat END */
} boot_stage_t;

static uint32_t boot_u16(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8;
}
static uint32_t boot_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void boot_p16(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}
static void boot_p32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

#define boot_crc_upd x0x_crc32_upd
#define boot_crc     x0x_crc32

/* 1 = the bytes are a whole record the keyboard may store. Anything else
 * (empty lines, a clipped line, a picture of the wrong size) is refused. */
static int boot_payload_ok(const uint8_t *p, uint32_t n)
{
    uint32_t i, n0, n1, kind, flags, w, h;
    if (!p || n < BOOT_PAY_FIXED || n > BOOT_REC_MAX)
        return 0;
    kind = p[0];
    flags = p[1];
    n0 = p[2];
    n1 = p[3];
    if (p[37] || p[38] || p[39])
        return 0;
    w = boot_u16(p + 40);
    h = boot_u16(p + 42);
    if (kind == BOOT_KIND_FACTORY) {
        if (flags || n0 || n1 || w || h || n != BOOT_PAY_FIXED)
            return 0;
        for (i = 4; i < 37u; i++)
            if (p[i])
                return 0;
        return 1;
    }
    if (kind != BOOT_KIND_CUSTOM || (flags & ~BOOT_F_PIC))
        return 0;
    if (n0 < 1u || n0 > BOOT_LINE0_MAX || n1 < 1u || n1 > BOOT_LINE1_MAX)
        return 0;
    for (i = 0; i < BOOT_LINE0_MAX; i++) {
        if (i < n0) {
            if (p[4u + i] < 32u || p[4u + i] > 126u)
                return 0;
        } else if (p[4u + i])
            return 0;
    }
    for (i = 0; i < BOOT_LINE1_MAX; i++) {
        if (i < n1) {
            if (p[16u + i] < 32u || p[16u + i] > 126u)
                return 0;
        } else if (p[16u + i])
            return 0;
    }
    if (flags & BOOT_F_PIC)
        return w == BOOT_PIC_W && h == BOOT_PIC_H && n == BOOT_PAY_FIXED + BOOT_PIC_BYTES;
    return !w && !h && n == BOOT_PAY_FIXED;
}

static int boot_pix(const uint8_t *bits, uint32_t x, uint32_t y)
{
    return (bits[y * BOOT_PIC_STRIDE + (x >> 3)] >> (7u - (x & 7u))) & 1;
}

/* 1 valid, 0 not a boot record, -1 the read failed (caller must not erase). */
static int boot_copy_state(boot_io_t *io, uint32_t sec, uint32_t *seq_out)
{
    uint8_t h[BOOT_HDR], head[BOOT_PAY_FIXED], tmp[256];
    uint32_t len, c, off, n;
    if (io->read(io->ctx, sec, h, BOOT_HDR))
        return -1;
    if (boot_u32(h) != BOOT_MAGIC || boot_u16(h + 4) != BOOT_VERSION)
        return 0;
    if (boot_u32(h + 24) || boot_u32(h + 28))
        return 0;
    if (boot_crc(h, 20) != boot_u32(h + 20))
        return 0;
    len = boot_u32(h + 12);
    if (len < BOOT_PAY_FIXED || len > BOOT_REC_MAX)
        return 0;
    c = 0xFFFFFFFFu;
    for (off = 0; off < len;) {
        n = len - off > sizeof tmp ? (uint32_t)sizeof tmp : len - off;
        if (io->read(io->ctx, sec + BOOT_PAY_OFF + off, tmp, n))
            return -1;
        if (off == 0) {
            uint32_t k;
            for (k = 0; k < BOOT_PAY_FIXED; k++)
                head[k] = tmp[k];
        }
        c = boot_crc_upd(c, tmp, n);
        off += n;
    }
    if (~c != boot_u32(h + 16) || !boot_payload_ok(head, len))
        return 0;
    if (seq_out)
        *seq_out = boot_u32(h + 8);
    return 1;
}

static int boot_prog_span(boot_io_t *io, uint32_t off, const uint8_t *src, uint32_t n)
{
    while (n) {
        uint32_t chunk = 256u - (off & 0xFFu);
        if (chunk > n)
            chunk = n;
        if (io->prog(io->ctx, off, src, chunk))
            return -1;
        off += chunk;
        src += chunk;
        n -= chunk;
    }
    return 0;
}

/* Write the inactive copy. Header last. Returns BOOT_OK or BOOT_FLASH.
 * A refused record never gets here. */
static int boot_commit(boot_io_t *io, uint32_t sec0, uint32_t sec1, const uint8_t *rec, uint32_t n)
{
    uint32_t seq0 = 0, seq1 = 0, dest, seq, i;
    uint8_t hdr[BOOT_HDR];
    int s0, s1;
    if (!boot_payload_ok(rec, n))
        return BOOT_BAD;
    s0 = boot_copy_state(io, sec0, &seq0);
    s1 = boot_copy_state(io, sec1, &seq1);
    if (s0 < 0 || s1 < 0)
        return BOOT_FLASH;
    if (s0 > 0 && (s1 <= 0 || seq0 >= seq1)) {
        dest = sec1;
        seq = seq0 + 1u;
    } else if (s1 > 0) {
        dest = sec0;
        seq = seq1 + 1u;
    } else {
        dest = sec0;
        seq = 1u;
    }
    if (io->erase(io->ctx, dest))
        return BOOT_FLASH;
    if (boot_prog_span(io, dest + BOOT_PAY_OFF, rec, n))
        return BOOT_FLASH;
    for (i = 0; i < BOOT_HDR; i++)
        hdr[i] = 0;
    boot_p32(hdr, BOOT_MAGIC);
    boot_p16(hdr + 4, BOOT_VERSION);
    boot_p16(hdr + 6, dest == sec1 ? 1u : 0u);
    boot_p32(hdr + 8, seq);
    boot_p32(hdr + 12, n);
    boot_p32(hdr + 16, boot_crc(rec, n));
    boot_p32(hdr + 20, boot_crc(hdr, 20));
    if (io->prog(io->ctx, dest, hdr, BOOT_HDR))
        return BOOT_FLASH;
    s0 = boot_copy_state(io, dest, &seq0);
    if (s0 != 1 || seq0 != seq)
        return BOOT_FLASH;
    return BOOT_OK;
}

static int boot_begin(boot_stage_t *s, uint32_t now)
{
    uint32_t i, n;
    if (!s || !s->buf || s->cap < BOOT_REC_MAX)
        return BOOT_SIZE;
    n = s->cap < BOOT_REC_MAX ? s->cap : BOOT_REC_MAX;
    for (i = 0; i < n; i++)
        s->buf[i] = 0;
    s->fill = 0;
    s->armed = 1;
    s->committed = 0;
    s->t0 = now;
    return BOOT_OK;
}

static int boot_write(boot_stage_t *s, uint32_t off, const uint8_t *p, uint32_t n, uint32_t now)
{
    uint32_t i;
    if (!s->armed)
        return BOOT_BAD;
    if (now - s->t0 > BOOT_MS) {
        s->armed = 0;
        return BOOT_LATE;
    }
    if (!n || !p || n > 256u) {
        s->armed = 0;
        return BOOT_BAD;
    }
    if (off != s->fill) {
        uint32_t j;
        if (off + n > s->fill) {
            s->armed = 0;
            return BOOT_BAD;
        }
        for (j = 0; j < n; j++) {
            if (s->buf[off + j] != p[j]) {
                s->armed = 0;
                return BOOT_BAD;
            }
        }
        s->t0 = now;
        return BOOT_OK;
    }
    if (off + n > BOOT_REC_MAX || off + n > s->cap) {
        s->armed = 0;
        return BOOT_SIZE;
    }
    for (i = 0; i < n; i++)
        s->buf[off + i] = p[i];
    s->fill = off + n;
    s->t0 = now;
    return BOOT_OK;
}

static int boot_end(boot_stage_t *s, boot_io_t *io, uint32_t sec0, uint32_t sec1, uint32_t now)
{
    int rc;
    if (!s->armed) {
        if (s->committed)
            return BOOT_OK;
        return BOOT_BAD;
    }
    if (now - s->t0 > BOOT_MS) {
        s->armed = 0;
        return BOOT_LATE;
    }
    if (!boot_payload_ok(s->buf, s->fill)) {
        s->armed = 0;
        return BOOT_BAD;
    }
    rc = boot_commit(io, sec0, sec1, s->buf, s->fill);
    s->armed = 0;
    if (rc == BOOT_OK)
        s->committed = 1;
    return rc;
}

static void boot_abort(boot_stage_t *s)
{
    if (s) {
        s->armed = 0;
        s->committed = 0;
        s->fill = 0;
    }
}

static int boot_poll(boot_stage_t *s, uint32_t now)
{
    if (s && s->armed && now - s->t0 > BOOT_MS) {
        s->armed = 0;
        return BOOT_LATE;
    }
    return BOOT_OK;
}

static void boot_lines(const uint8_t *pay, char *a, char *b)
{
    uint32_t i, n0, n1;
    const char *f0 = BOOT_FACTORY_0, *f1 = BOOT_FACTORY_1;
    if (!pay || pay[0] != BOOT_KIND_CUSTOM) {
        for (i = 0; f0[i]; i++)
            a[i] = f0[i];
        a[i] = 0;
        for (i = 0; f1[i]; i++)
            b[i] = f1[i];
        b[i] = 0;
        return;
    }
    n0 = pay[2];
    n1 = pay[3];
    for (i = 0; i < n0; i++)
        a[i] = (char)pay[4u + i];
    a[i] = 0;
    for (i = 0; i < n1; i++)
        b[i] = (char)pay[16u + i];
    b[i] = 0;
}

/* 1 if this 4 KiB image is a complete boot record. A missing header, a bad
 * CRC, or a half-written payload is 0 — the other copy, or the original words. */
static int boot_image_state(const uint8_t *sec, uint32_t *seq_out)
{
    uint32_t len;
    if (!sec || boot_u32(sec) != BOOT_MAGIC || boot_u16(sec + 4) != BOOT_VERSION)
        return 0;
    if (boot_u32(sec + 24) || boot_u32(sec + 28))
        return 0;
    if (boot_crc(sec, 20) != boot_u32(sec + 20))
        return 0;
    len = boot_u32(sec + 12);
    if (len < BOOT_PAY_FIXED || len > BOOT_REC_MAX)
        return 0;
    if (boot_crc(sec + BOOT_PAY_OFF, len) != boot_u32(sec + 16))
        return 0;
    if (!boot_payload_ok(sec + BOOT_PAY_OFF, len))
        return 0;
    if (seq_out)
        *seq_out = boot_u32(sec + 8);
    return 1;
}

/* 0 or 1, or -1 when neither copy is valid. The higher sequence wins. */
static int boot_pick(const uint8_t *sec0, const uint8_t *sec1)
{
    uint32_t a = 0, b = 0;
    int va = boot_image_state(sec0, &a);
    int vb = boot_image_state(sec1, &b);
    if (va && (!vb || a >= b))
        return 0;
    if (vb)
        return 1;
    return -1;
}
#endif
