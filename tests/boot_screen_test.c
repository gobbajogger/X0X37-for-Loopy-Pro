/* SPDX-License-Identifier: GPL-3.0-only */
/* Boot screen record: bad pictures and text never erase flash; a torn write
 * keeps the previous copy; a half-written sector is not what the screen shows. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app/boot_screen.h"

static int bad;
static void check(const char *what, int ok)
{
    printf("%-72s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}

static uint8_t nor[2][4096];
static uint32_t erases, progs;
static int fail_header;                 /* fail the commit header program */
static int fail_read;

static int map_off(uint32_t off, uint8_t **p)
{
    if (off >= BOOT_SMK_SEC0 && off + 1 <= BOOT_SMK_SEC0 + 4096u) {
        *p = nor[0] + (off - BOOT_SMK_SEC0);
        return 0;
    }
    if (off >= BOOT_SMK_SEC1 && off + 1 <= BOOT_SMK_SEC1 + 4096u) {
        *p = nor[1] + (off - BOOT_SMK_SEC1);
        return 1;
    }
    return -1;
}

static int tio_erase(void *ctx, uint32_t off)
{
    uint8_t *p;
    (void)ctx;
    if (map_off(off, &p) < 0 || (off & 0xFFFu))
        return -1;
    memset(p, 0xFF, 4096);
    erases++;
    return 0;
}
static int tio_prog(void *ctx, uint32_t off, const uint8_t *src, uint32_t n)
{
    uint8_t *p;
    uint32_t i;
    (void)ctx;
    if (fail_header && (off & 0xFFFu) == 0)
        return -1;
    if (map_off(off, &p) < 0 || !n || n > 256u || ((off & 0xFFu) + n) > 256u)
        return -1;
    for (i = 0; i < n; i++)
        p[i] &= src[i];
    progs++;
    return 0;
}
static int tio_read(void *ctx, uint32_t off, uint8_t *dst, uint32_t n)
{
    uint8_t *p;
    uint32_t base, i;
    (void)ctx;
    if (fail_read)
        return -1;
    if (map_off(off, &p) < 0)
        return -1;
    base = (p >= nor[1]) ? BOOT_SMK_SEC1 : BOOT_SMK_SEC0;
    if (off + n > base + 4096u)
        return -1;
    for (i = 0; i < n; i++)
        dst[i] = p[i];
    return 0;
}

static boot_io_t io = {tio_erase, tio_prog, tio_read, 0};

static void wipe(void)
{
    memset(nor, 0xFF, sizeof nor);
    erases = progs = 0;
    fail_header = fail_read = 0;
}

static uint32_t put_line(uint8_t *d, uint32_t nmax, const char *s)
{
    uint32_t n = 0;
    while (s[n] && n < nmax) {
        d[n] = (uint8_t)s[n];
        n++;
    }
    return n;
}

/* custom record, or factory when both strings are null. bits null = no picture. */
static uint32_t build(uint8_t *d, const char *a, const char *b, const uint8_t *bits)
{
    uint32_t n0, n1;
    memset(d, 0, BOOT_REC_MAX);
    if (!a && !b) {
        d[0] = BOOT_KIND_FACTORY;
        return BOOT_PAY_FIXED;
    }
    d[0] = BOOT_KIND_CUSTOM;
    n0 = put_line(d + 4, BOOT_LINE0_MAX, a);
    n1 = put_line(d + 16, BOOT_LINE1_MAX, b);
    d[2] = (uint8_t)n0;
    d[3] = (uint8_t)n1;
    if (bits) {
        d[1] = BOOT_F_PIC;
        boot_p16(d + 40, BOOT_PIC_W);
        boot_p16(d + 42, BOOT_PIC_H);
        memcpy(d + BOOT_PAY_FIXED, bits, BOOT_PIC_BYTES);
        return BOOT_PAY_FIXED + BOOT_PIC_BYTES;
    }
    return BOOT_PAY_FIXED;
}

static int saved_is(const char *a, const char *b)
{
    int w = boot_pick(nor[0], nor[1]);
    char l0[16], l1[24];
    const uint8_t *pay;
    if (w < 0)
        return a == 0;
    pay = nor[w] + BOOT_PAY_OFF;
    boot_lines(pay, l0, l1);
    if (a == 0)
        return pay[0] == BOOT_KIND_FACTORY && !strcmp(l0, BOOT_FACTORY_0) && !strcmp(l1, BOOT_FACTORY_1);
    return !strcmp(l0, a) && !strcmp(l1, b);
}

static int commit_buf(const uint8_t *rec, uint32_t n)
{
    boot_stage_t s;
    uint8_t buf[3840];
    uint32_t i;
    memset(&s, 0, sizeof s);
    memset(buf, 0, sizeof buf);
    s.buf = buf;
    s.cap = sizeof buf;
    if (boot_begin(&s, 1000) != BOOT_OK)
        return -1;
    for (i = 0; i < n;) {
        uint32_t c = n - i > 256u ? 256u : n - i;
        int rc = boot_write(&s, i, rec + i, c, 1000 + i);
        if (rc != BOOT_OK)
            return rc;
        i += c;
    }
    return boot_end(&s, &io, BOOT_SMK_SEC0, BOOT_SMK_SEC1, 1000u + n + 1u);
}

int main(void)
{
    uint8_t rec[BOOT_REC_MAX], bits[BOOT_PIC_BYTES];
    uint32_t n, er0;
    boot_stage_t s;
    uint8_t buf[3840];
    char l0[16], l1[24];
    int w;

    check("factory subtitle fits the record", strlen(BOOT_FACTORY_1) <= BOOT_LINE1_MAX);
    check("sectors differ and are 4 KiB", BOOT_SMK_SEC0 + 0x1000u == BOOT_SMK_SEC1 && BOOT_FM1_SEC0 != BOOT_FM1_SEC1);
    check("FM-1 boot sectors are not the settings pair", BOOT_FM1_SEC0 != 0xFC000u && BOOT_FM1_SEC1 != 0xFC000u &&
          BOOT_FM1_SEC0 != 0xFD000u && BOOT_FM1_SEC1 != 0xFD000u);
    check("FM-1 picture sector is not a sample slot", BOOT_FM1_SEC0 < 0xA0000u || BOOT_FM1_SEC0 >= 0xDC000u);
    check("SMK boot sectors sit in the spare after the objects", BOOT_SMK_SEC0 == 0xAB000u && BOOT_SMK_SEC1 == 0xAC000u &&
          BOOT_SMK_SEC1 + 0x1000u < 0xAF000u);

    memset(rec, 0, sizeof rec);
    check("empty buffer is not a boot record", !boot_payload_ok(rec, 0) && !boot_payload_ok(rec, BOOT_PAY_FIXED));
    n = build(rec, 0, 0, 0);
    check("original-words record is accepted", n == BOOT_PAY_FIXED && boot_payload_ok(rec, n));
    n = build(rec, "", "SECOND LINE HERE", 0);
    check("empty top line is refused", !boot_payload_ok(rec, BOOT_PAY_FIXED));
    n = build(rec, "HELLO", "", 0);
    check("empty second line is refused", !boot_payload_ok(rec, BOOT_PAY_FIXED));
    n = build(rec, "THIS TITLE IS TOO LONG", "OK", 0);
    rec[2] = 13;                               /* the field itself is only 12, so a longer claim is a lie */
    check("top line over 12 characters is refused", !boot_payload_ok(rec, BOOT_PAY_FIXED));
    {
        char big[32];
        memset(big, 'B', 22);
        big[22] = 0;
        n = build(rec, "OK", big, 0);
        /* put_line stops at 21, so the builder clips; force a 22nd claimed length */
        rec[3] = 22;
        check("second line over 21 characters is refused", !boot_payload_ok(rec, BOOT_PAY_FIXED));
    }
    n = build(rec, "Hi", "THERE", 0);          /* 'i' is allowed; lowercase is 32..126 */
    check("ordinary letters are accepted", boot_payload_ok(rec, n));
    rec[4] = 10;
    check("a control character in the text is refused", !boot_payload_ok(rec, n));
    n = build(rec, "HELLO", "THERE", 0);
    rec[1] = BOOT_F_PIC;
    boot_p16(rec + 40, 16);
    boot_p16(rec + 42, 16);
    check("wrong picture size is refused", !boot_payload_ok(rec, n));
    check("picture flag with no bitmap bytes is refused", !boot_payload_ok(rec, BOOT_PAY_FIXED + 10));
    memset(bits, 0, sizeof bits);
    bits[0] = 0x80;                            /* pixel (0,0) */
    n = build(rec, "HELLO", "THERE", bits);
    check("240 by 96 picture is accepted", n == BOOT_REC_MAX && boot_payload_ok(rec, n) && boot_pix(rec + BOOT_PAY_FIXED, 0, 0) &&
          !boot_pix(rec + BOOT_PAY_FIXED, 1, 0));
    check("a longer buffer than the picture is refused", !boot_payload_ok(rec, n + 1));

    wipe();
    memset(&s, 0, sizeof s);
    s.buf = buf;
    s.cap = sizeof buf;
    n = build(rec, "", "THERE", 0);
    check("begin does not erase", boot_begin(&s, 10) == BOOT_OK && erases == 0);
    check("writing an empty-line record does not erase", boot_write(&s, 0, rec, BOOT_PAY_FIXED, 11) == BOOT_OK);
    er0 = erases;
    check("end of a bad record does not touch flash", boot_end(&s, &io, BOOT_SMK_SEC0, BOOT_SMK_SEC1, 12) == BOOT_BAD && erases == er0);
    check("blank flash shows the original words", boot_pick(nor[0], nor[1]) < 0);
    boot_lines(0, l0, l1);
    check("factory words are DnBOX and the engine line", !strcmp(l0, "DnBOX") && !strcmp(l1, BOOT_FACTORY_1));

    n = build(rec, "MORN", "SHIFT", 0);
    check("a skipped chunk is refused and disarms", boot_begin(&s, 20) == BOOT_OK && boot_write(&s, 10, rec, 4, 21) == BOOT_BAD && !s.armed);
    check("end after a skipped chunk does not erase", boot_end(&s, &io, BOOT_SMK_SEC0, BOOT_SMK_SEC1, 22) == BOOT_BAD && erases == 0);

    {
        uint32_t i, rc = BOOT_OK;
        check("begin before an overrun does not erase", boot_begin(&s, 30) == BOOT_OK && erases == 0);
        for (i = 0; i < BOOT_REC_MAX && rc == BOOT_OK; i += 256u) {
            uint32_t c = BOOT_REC_MAX - i > 256u ? 256u : BOOT_REC_MAX - i;
            rc = (uint32_t)boot_write(&s, i, rec + i, c, 31);
        }
        check("filling the record does not erase", rc == BOOT_OK && erases == 0);
        check("a one-byte overrun is refused", boot_write(&s, BOOT_REC_MAX, rec, 1, 32) == BOOT_SIZE && erases == 0);
    }

    n = build(rec, "MORN", "SHIFT", 0);
    check("first save stores the lines", commit_buf(rec, n) == BOOT_OK && saved_is("MORN", "SHIFT") && erases == 1);
    n = build(rec, "LATE", "SHOW", bits);
    check("second save keeps a picture and new lines", commit_buf(rec, n) == BOOT_OK && saved_is("LATE", "SHOW"));
    w = boot_pick(nor[0], nor[1]);
    check("the newer copy is the one shown", w == 1 && boot_pix(nor[1] + BOOT_PAY_OFF + BOOT_PAY_FIXED, 0, 0));

    fail_header = 1;
    n = build(rec, "NEW", "WORDS", 0);
    er0 = erases;
    check("header program failure reports flash and keeps the previous lines",
          commit_buf(rec, n) == BOOT_FLASH && saved_is("LATE", "SHOW"));
    check("the failed copy is not a valid boot record", boot_image_state(nor[0], 0) == 0);
    (void)er0;
    fail_header = 0;

    /* half-written image: payload bytes without a header must not win */
    memset(nor[0], 0xA5, 4096);
    check("a sector full of payload and no header is ignored", boot_pick(nor[0], nor[1]) == 1 && saved_is("LATE", "SHOW"));

    n = build(rec, 0, 0, 0);
    check("original-words save replaces the picture", commit_buf(rec, n) == BOOT_OK && saved_is(0, 0));
    w = boot_pick(nor[0], nor[1]);
    check("original-words record has no picture flag", w >= 0 && !(nor[w][BOOT_PAY_OFF + 1] & BOOT_F_PIC));

    fail_read = 1;
    n = build(rec, "NO", "READ", 0);
    er0 = erases;
    check("unreadable flash is refused before erase", commit_buf(rec, n) == BOOT_FLASH && erases == er0);
    fail_read = 0;
    check("previous original-words record still wins", saved_is(0, 0));

    check("abort drops the stage and does not erase", boot_begin(&s, 50) == BOOT_OK && (boot_abort(&s), !s.armed) && erases == er0);
    check("a stalled upload times out without erasing", boot_begin(&s, 100) == BOOT_OK && boot_poll(&s, 100 + BOOT_MS + 1) == BOOT_LATE &&
          !s.armed && erases == er0);
    check("a write after the timeout does not erase", boot_write(&s, 0, rec, 8, 100 + BOOT_MS + 2) == BOOT_BAD && erases == er0);

    n = build(rec, "REDO", "WRITE", 0);
    check("repeating an accepted write chunk is ok", boot_begin(&s, 60) == BOOT_OK && boot_write(&s, 0, rec, 8, 61) == BOOT_OK &&
          boot_write(&s, 0, rec, 8, 62) == BOOT_OK && boot_write(&s, 8, rec + 8, n - 8, 63) == BOOT_OK);
    check("a repeated end after commit is ok", boot_end(&s, &io, BOOT_SMK_SEC0, BOOT_SMK_SEC1, 64) == BOOT_OK &&
          boot_end(&s, &io, BOOT_SMK_SEC0, BOOT_SMK_SEC1, 65) == BOOT_OK && saved_is("REDO", "WRITE"));

    check("CRC-32 matches zlib's check value", boot_crc((const uint8_t *)"123456789", 9) == 0xCBF43926u);

    printf("%s\n", bad ? "BOOT SCREEN TEST FAILED" : "boot screen test passed");
    return bad != 0;
}
