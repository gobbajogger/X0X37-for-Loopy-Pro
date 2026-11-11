/* SPDX-License-Identifier: GPL-3.0-only */
/* Real storage implementation, shared incoming RAM and NOR fault boundaries. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/app/plat.h"
static uint8_t nor[0x100000], snapshot[sizeof nor], expected[3840], got[3840];
static int fault, event, partial, armed, persistent;
static unsigned checks;
static int hit(void) { return armed && (persistent ? event++ >= fault : event++ == fault); }
static int st_read(uint32_t off, void *dst, uint32_t n)
{
    assert(off + n <= sizeof nor);
    if (hit()) { if (partial) memcpy(dst, nor + off, n / 2); return -9; }
    memcpy(dst, nor + off, n); return 0;
}
static int st_erase(uint32_t off)
{
    assert(off >= 0x9D000 && off + 4096 <= 0xAB000 && !(off & 4095));
    if (hit()) { if (partial) memset(nor + off, 255, 2048); return -9; }
    memset(nor + off, 255, 4096); return 0;
}
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    assert(off >= 0x9D000 && off + n <= 0xAB000 && (off & 255) + n <= 256);
    int failed = hit();
    uint32_t count = failed ? (partial ? n / 2 : 0) : n;
    for (uint32_t i = 0; i < count; i++) nor[off + i] &= ((const uint8_t *)src)[i];
    return failed ? -9 : 0;
}
#define BOARD_SMK37 1
#include "../firmware/src/storage.c"
#define CHECK(x) do { checks++; assert(x); } while (0)
static void outside(void)
{
    uint32_t base = st_sector(OBJ_SOUND, 0);
    CHECK(!memcmp(nor, snapshot, base));
    CHECK(!memcmp(nor + base + 8192, snapshot + base + 8192, sizeof nor - base - 8192));
}
int main(void)
{
    static const unsigned lengths[] = {0,1,255,256,257,3839,3840};
    for (unsigned k = 0; k < sizeof lengths / sizeof *lengths; k++) {
        unsigned n = lengths[k];
        memset(nor, 255, sizeof nor);
        memset(got, 'A', sizeof got);
        CHECK(st_save(OBJ_SOUND, got, n) == 0);
        memset(got, 'B', sizeof got);
        CHECK(st_save(OBJ_SOUND, got, n) == 0);
        memcpy(snapshot, nor, sizeof nor);
        for (unsigned i = 0; i < sizeof expected; i++) expected[i] = (uint8_t)(i * 71u + k);
        /* Successful saves preserve incoming shared payload through readback. */
        memcpy(st_buf, expected, sizeof expected);
        event = 0; armed = 1; fault = -1;
        CHECK(st_save(OBJ_SOUND, st_buf, n) == 0);
        unsigned events = event;
        armed = 0;
        CHECK(!memcmp(st_buf, expected, sizeof expected));
        CHECK(st_load(OBJ_SOUND, got, sizeof got) == (int)n && !memcmp(got, expected, n));
        outside();
        /* Every read/erase/program event, including partial mutation. A failed
         * current-copy read has the historical fallback semantics; do not
         * pretend all transient read failures make save return failure. */
        for (partial = 0; partial <= 1; partial++) for (unsigned f = 0; f <= events; f++) {
            memcpy(nor, snapshot, sizeof nor);
            memcpy(st_buf, expected, sizeof expected);
            event = 0; fault = (int)f; armed = 1;
            (void)st_save(OBJ_SOUND, st_buf, n);
            armed = 0;
            CHECK(!memcmp(st_buf, expected, sizeof expected));
            int count = st_load(OBJ_SOUND, got, sizeof got);
            CHECK(count == (int)n);
            CHECK(!memcmp(got, expected, n) || !memcmp(got, snapshot + st_sector(OBJ_SOUND, 1) + 256, n));
            outside();
        }
        memcpy(nor, snapshot, sizeof nor);
        event = 0; fault = -1; armed = 1;
        CHECK(st_load(OBJ_SOUND, got, sizeof got) == (int)n);
        events = event; armed = 0;
        for (partial = 0; partial <= 1; partial++) for (unsigned f = 0; f < events; f++) {
            memset(got, 0x5A, sizeof got);
            event = 0; fault = (int)f; armed = 1;
            int rc = st_load(OBJ_SOUND, got, sizeof got);
            armed = 0;
            if (rc < 0) for (unsigned i = 0; i < sizeof got; i++) CHECK(got[i] == 0x5A);
            outside();
        }
        /* Persistent read failure exhausts fallback; partial reads must never
         * reach the external destination on error. */
        for (partial = 0; partial <= 1; partial++) for (unsigned f = 0; f < events; f++) {
            memset(got, 0x5A, sizeof got);
            event = 0; fault = (int)f; armed = persistent = 1;
            CHECK(st_load(OBJ_SOUND, got, sizeof got) < 0);
            armed = persistent = 0;
            for (unsigned i = 0; i < sizeof got; i++) CHECK(got[i] == 0x5A);
            outside();
        }
        /* Newest body rot falls back; malformed length/header never reaches read. */
        nor[st_sector(OBJ_SOUND, 1) + 256] ^= 1;
        if (n) CHECK(st_load(OBJ_SOUND, got, sizeof got) == (int)n && got[0] == 'A');
        st_hdr_t *h = (st_hdr_t *)(void *)(nor + st_sector(OBJ_SOUND, 1));
        h->len = 3841; h->hcrc = st_crc32(h, sizeof *h - 4);
        CHECK(st_load(OBJ_SOUND, got, sizeof got) == (int)n);
        CHECK(st_save(OBJ_SOUND, expected, 3841) < 0);
        /* Equal valid sequence numbers choose A, exactly as before. */
        memcpy(nor, snapshot, sizeof nor);
        st_hdr_t *ha = (st_hdr_t *)(void *)(nor + st_sector(OBJ_SOUND, 0));
        h = (st_hdr_t *)(void *)(nor + st_sector(OBJ_SOUND, 1));
        h->seq = ha->seq; h->hcrc = st_crc32(h, sizeof *h - 4);
        CHECK(st_load(OBJ_SOUND, got, sizeof got) == (int)n);
        if (n) CHECK(got[0] == 'A');
        /* Sources residing in flash are copied to RAM before erase/program. */
        memcpy(nor, snapshot, sizeof nor);
        CHECK(st_save(OBJ_SOUND, nor + st_sector(OBJ_SOUND, 0) + 256, n) == 0);
        CHECK(st_load(OBJ_SOUND, got, sizeof got) == (int)n);
        if (n) CHECK(got[0] == 'A');
    }
    printf("storage scratch: %u checks; shared source, exhaustive hook faults, partial I/O, unchanged external failure destination and protected ranges PASS\n", checks);
}
