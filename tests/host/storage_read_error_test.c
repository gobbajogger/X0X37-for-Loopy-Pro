/* SPDX-License-Identifier: GPL-3.0-only */
/* Focused save-selection regressions against the actual storage.c and RAM NOR.
 * No hardware, platform flash driver, or complete project stream is exercised. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../firmware/src/app/plat.h"
#include "../../firmware/src/storage.c"

static uint8_t nor[0x100000], before[sizeof nor];
static uint8_t payload[3][600], staged[ST_PAYLOAD_MAX];
static uint32_t fault_off, allowed_base;
static int read_fault, read_hits, erases, programs, fail_erase, fail_program;
static unsigned checks, cases;
static const char *context;
static uint32_t context_obj;

static void require(int ok, const char *what, int line)
{
    ++checks;
    if (!ok) {
        fprintf(stderr, "FAIL board=%d object=%u case=%s line=%d: %s\n",
                BOARD_SMK37, context_obj, context, line, what);
        exit(1);
    }
}
#define REQUIRE(x) require(!!(x), #x, __LINE__)

static int st_read(uint32_t off, void *dst, uint32_t n)
{
    REQUIRE(off <= sizeof nor && n <= sizeof nor - off);
    if (read_fault && off <= fault_off && fault_off - off < n) {
        ++read_hits;
        return -6;                 /* storage normalizes any read error to -2 */
    }
    memcpy(dst, nor + off, n);
    return 0;
}

static int st_erase(uint32_t off)
{
    REQUIRE(off == allowed_base && off + ST_SECTOR <= sizeof nor);
    ++erases;
    if (fail_erase) return -5;
    memset(nor + off, 0xff, ST_SECTOR);
    return 0;
}

static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *bytes = src;
    int fail = programs++ == fail_program;
    REQUIRE(off >= allowed_base && off + n <= allowed_base + ST_SECTOR);
    REQUIRE((off & 255u) + n <= 256u);
    /* A failed program may partially clear bits, including in the header. */
    for (uint32_t i = 0; i < (fail ? n / 2u : n); ++i)
        nor[off + i] &= bytes[i];
    return fail ? -6 : 0;
}

enum damage { GOOD, BAD_HEADER, BAD_BODY };
struct fixture {
    const char *name;
    uint32_t seq[2];               /* zero means erased */
    enum damage damage[2];
    int current;
    unsigned bodies;              /* bodies visited in newest-valid selection */
    int empty_result;
};
static const struct fixture fixtures[] = {
    {"blank",              {0, 0}, {GOOD, GOOD},             -1, 0, -1},
    {"sole-A",             {4, 0}, {GOOD, GOOD},              0, 1,  0},
    {"sole-B",             {0, 4}, {GOOD, GOOD},              1, 2,  0},
    {"both-A-newer",       {5, 4}, {GOOD, GOOD},              0, 1,  0},
    {"both-B-newer",       {4, 5}, {GOOD, GOOD},              1, 2,  0},
    {"tie-prefers-A",      {4, 4}, {GOOD, GOOD},              0, 1,  0},
    {"newer-A-body-bad",   {5, 4}, {BAD_BODY, GOOD},          1, 3,  0},
    {"newer-B-body-bad",   {4, 5}, {GOOD, BAD_BODY},          0, 3,  0},
    {"both-bodies-bad",    {5, 4}, {BAD_BODY, BAD_BODY},     -1, 3, -3},
    {"both-headers-bad",   {5, 4}, {BAD_HEADER, BAD_HEADER}, -1, 0, -3},
    {"A-header-bad",       {5, 4}, {BAD_HEADER, GOOD},        1, 2,  0},
    {"B-header-bad",       {4, 5}, {GOOD, BAD_HEADER},        0, 1,  0},
};

static void reset_hooks(void)
{
    read_fault = read_hits = erases = programs = fail_erase = 0;
    fail_program = -1;
}

static void fixture(uint32_t obj, const struct fixture *f)
{
    context = f->name;
    context_obj = obj;
    ++cases;
    reset_hooks();
    st_stage_hold = 0;
    /* Mark all other objects/reserved regions, then clear only this pair. */
    memset(nor, 0xa5, sizeof nor);
    for (unsigned bank = 0; bank < 2; ++bank) {
        uint32_t base = st_sector(obj, bank);
        st_hdr_t h = {0};
        memset(nor + base, 0xff, ST_SECTOR);
        if (!f->seq[bank]) continue;
        h.magic = ST_MAGIC;
        h.type = (uint16_t)obj;
        h.slot = (uint16_t)bank;
        h.seq = f->seq[bank];
        h.len = sizeof payload[bank];
        h.crc = st_crc32(payload[bank], h.len);
        h.rsv[0] = h.rsv[1] = 0xffffffffu;
        h.hcrc = st_crc32(&h, sizeof h - 4u);
        if (f->damage[bank] == BAD_HEADER) h.hcrc ^= 1;
        memcpy(nor + base, &h, sizeof h);
        memcpy(nor + base + ST_PAYLOAD_OFF, payload[bank], h.len);
        if (f->damage[bank] == BAD_BODY) nor[base + ST_PAYLOAD_OFF + 300] ^= 1;
    }
    allowed_base = st_sector(obj, f->current == 0 ? 1u : 0u);
    memset(st_buf, 0xc5, sizeof st_buf);
    memcpy(st_buf, payload[2], sizeof payload[2]);
    memcpy(staged, st_buf, sizeof staged);
    memcpy(before, nor, sizeof before);
}

static void unchanged_outside_target(void)
{
    REQUIRE(!memcmp(nor, before, allowed_base));
    REQUIRE(!memcmp(nor + allowed_base + ST_SECTOR,
                    before + allowed_base + ST_SECTOR,
                    sizeof nor - allowed_base - ST_SECTOR));
}

static void load_original(uint32_t obj, const struct fixture *f)
{
    uint8_t got[sizeof payload[0]];
    int rc = st_load(obj, got, sizeof got);
    if (f->current < 0) REQUIRE(rc == f->empty_result);
    else {
        REQUIRE(rc == (int)sizeof got);
        REQUIRE(!memcmp(got, payload[f->current], sizeof got));
    }
}

static void save_recovered(uint32_t obj, const struct fixture *f)
{
    uint8_t got[sizeof payload[0]];
    st_hdr_t h;
    uint32_t expected_seq = f->current < 0 ? 1u : f->seq[f->current] + 1u;
    reset_hooks();
    memcpy(st_buf, staged, sizeof st_buf);
    REQUIRE(st_save(obj, st_buf, sizeof payload[2]) == 0);
    REQUIRE(erases == 1 && programs == 4);
    unchanged_outside_target();
    memcpy(&h, nor + allowed_base, sizeof h);
    REQUIRE(h.seq == expected_seq && h.len == sizeof got);
    REQUIRE(h.slot == (f->current == 0 ? 1u : 0u));
    REQUIRE(h.rsv[0] == 0xffffffffu && h.rsv[1] == 0xffffffffu);
    REQUIRE(st_load(obj, got, sizeof got) == (int)sizeof got);
    REQUIRE(!memcmp(got, payload[2], sizeof got));
}

static void uncertain_read(uint32_t obj, const struct fixture *f,
                           unsigned bank, int window)
{
    uint8_t got[sizeof payload[0]], untouched[sizeof got];
    fixture(obj, f);
    fault_off = st_sector(obj, bank) + (window < 0 ? 0u : ST_PAYLOAD_OFF + 256u * (unsigned)window);
    read_fault = 1;
    REQUIRE(st_save(obj, st_buf, sizeof payload[2]) == -2);
    REQUIRE(read_hits == 1 && erases == 0 && programs == 0);
    REQUIRE(!memcmp(nor, before, sizeof nor));
    REQUIRE(!memcmp(st_buf, staged, sizeof st_buf));
    /* Load's existing I/O result and destination preservation remain intact. */
    memset(got, 0x7c, sizeof got);
    memcpy(untouched, got, sizeof got);
    REQUIRE(st_load(obj, got, sizeof got) == -2);
    REQUIRE(!memcmp(got, untouched, sizeof got));
    read_fault = 0;
    load_original(obj, f);
    save_recovered(obj, f);
}

static void failed_write(uint32_t obj, const struct fixture *f, int program)
{
    fixture(obj, f);
    if (program < 0) fail_erase = 1;
    else fail_program = program;
    REQUIRE(st_save(obj, st_buf, sizeof payload[2]) == (program < 0 ? -5 : -6));
    REQUIRE(erases == 1 && programs == (program < 0 ? 0 : program + 1));
    unchanged_outside_target();
    reset_hooks();
    load_original(obj, f);
    save_recovered(obj, f);
}

int main(void)
{
    unsigned uncertain = 0, write_failures = 0, normal = 0, skipped_older = 0;
    const int failures[] = {-1, 0, 1, 3}; /* erase, first/later body page, commit header */
    for (unsigned i = 0; i < sizeof payload[0]; ++i)
        for (unsigned p = 0; p < 3; ++p) payload[p][i] = (uint8_t)(i * 17u + p * 37u);
    for (uint32_t obj = 0; obj < OBJ_NOBJ; ++obj) {
        for (unsigned i = 0; i < sizeof fixtures / sizeof fixtures[0]; ++i) {
            const struct fixture *f = &fixtures[i];
            fixture(obj, f);
            load_original(obj, f);
            save_recovered(obj, f);
            ++normal;
            for (unsigned bank = 0; bank < 2; ++bank) {
                uncertain_read(obj, f, bank, -1);
                ++uncertain;
                if (f->bodies & (1u << bank)) {
                    for (int window = 0; window < 3; ++window) {
                        uncertain_read(obj, f, bank, window);
                        ++uncertain;
                    }
                }
            }
        }
        /* A fully validated newest copy does not require reading the older
         * payload. Preserve that lazy selection even if the old body is unreadable. */
        for (unsigned i = 3; i <= 5; ++i) {
            const struct fixture *f = &fixtures[i];
            uint32_t seq = 0;
            uint8_t got[sizeof payload[0]];
            fixture(obj, f);
            unsigned older = (unsigned)(1 - f->current);
            fault_off = st_sector(obj, older) + ST_PAYLOAD_OFF;
            read_fault = 1;
            REQUIRE(st_pick(obj, &seq) == f->current && seq == f->seq[f->current]);
            REQUIRE(read_hits == 0 && erases == 0 && programs == 0);
            REQUIRE(!memcmp(st_buf, staged, sizeof st_buf));
            read_fault = 0;
            int rc = st_load(obj | PLAT_STORE_PREVIOUS, got, sizeof got);
            if (i == 5) REQUIRE(rc == -1); /* equal sequence is not previous */
            else {
                REQUIRE(rc == (int)sizeof got);
                REQUIRE(!memcmp(got, payload[older], sizeof got));
            }
            ++skipped_older;
        }
        for (unsigned i = 1; i <= 4; ++i)
            for (unsigned j = 0; j < sizeof failures / sizeof failures[0]; ++j) {
                failed_write(obj, &fixtures[i], failures[j]);
                ++write_failures;
            }
    }
    printf("PASS BOARD_SMK37=%d objects=%u fixtures=%u uncertain_reads=%u "
           "failed_writes=%u normal=%u lazy_selection=%u checks=%u\n",
           BOARD_SMK37, (unsigned)OBJ_NOBJ, cases, uncertain, write_failures,
           normal, skipped_older, checks);
    return 0;
}
