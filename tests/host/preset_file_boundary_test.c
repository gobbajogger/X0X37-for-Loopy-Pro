/* SPDX-License-Identifier: GPL-3.0-only */
/* Standalone exact-codec contract checks. Does not read/write device flash. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/preset_record.h"
#include "../../firmware/src/crc32_nibble.h"

static unsigned checks, failures;
static void ck(int ok, const char *what)
{
    checks++;
    if (!ok) {
        failures++;
        fprintf(stderr, "FAIL %s\n", what);
    }
}
static void crc_fix(uint8_t *p, uint32_t n)
{
    uint32_t v;
    memset(p + 16, 0, 4);
    v = x0x_crc32(p, n);
    p[16] = (uint8_t)v;
    p[17] = (uint8_t)(v >> 8);
    p[18] = (uint8_t)(v >> 16);
    p[19] = (uint8_t)(v >> 24);
}
static void le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}
static void user(preset_record_t *r, uint16_t id, uint8_t n808)
{
    unsigned i;
    preset_record_clear(r);
    r->engine_id = id;
    r->schema_version = PRESET_SCHEMA_V1;
    r->original_engine = (uint8_t)id;
    r->origin_tag = PRESET_ORIGIN_USER;
    r->nparams = (uint8_t)preset_record_expected_nparams(id, PRESET_SCHEMA_V1);
    r->nsamples = id == PRESET_ENGINE_808 ? n808 :
                   id == PRESET_ENGINE_909 ? PRESET_909_NSAMPLES_V1 :
                   id == PRESET_ENGINE_BREAK ? PRESET_BREAK_NSAMPLES_V1 : 0;
    (void)preset_record_set_name(r, "host-user");
    (void)preset_record_set_source(r, "contract");
    for (i = 0; i < r->nparams; ++i)
        r->params[i] = (uint8_t)(i * 7u + 3u);
    for (i = 0; i < r->nsamples; ++i) {
        r->samples[i].source = PRESET_SAMPLE_ROM;
        r->samples[i].index = (uint16_t)i;
        r->samples[i].crc = 0xAABBCC00u + i;
    }
}
static void roundtrips(void)
{
    static const struct { uint16_t engine; uint8_t refs; } cases[] = {
        { PRESET_ENGINE_CHICAGO, 0 }, { PRESET_ENGINE_MIAMI, 0 },
        { PRESET_ENGINE_JUNO, 0 }, { PRESET_ENGINE_DRONE, 0 },
        { PRESET_ENGINE_909, 0 }, { PRESET_ENGINE_808, 0 },
        { PRESET_ENGINE_808, 16 }, { PRESET_ENGINE_BREAK, 0 }
    };
    unsigned i;
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        preset_record_t original, imported;
        uint8_t in[PRESET_MAX_BYTES], out[PRESET_MAX_BYTES];
        uint32_t n = 0, m = 0;
        int rc;
        user(&original, cases[i].engine, cases[i].refs);
        rc = preset_record_encode(in, sizeof in, &n, &original);
        ck(rc == PRESET_OK, "A1 encode engine");
        if (rc != PRESET_OK)
            continue;
        memset(&imported, 0x5C, sizeof imported);
        ck(preset_record_import_file(&imported, in, n, cases[i].engine) == PRESET_OK,
           "A1 strict import engine");
        ck(preset_record_equal(&original, &imported), "A1 state roundtrip engine");
        ck(preset_record_export(out, sizeof out, &m, &imported) == PRESET_OK,
           "A1 export user engine");
        ck(n == m && memcmp(in, out, n) == 0, "A1 byte identity engine");
    }
}
static void rejected_envelopes(void)
{
    preset_record_t base, dest, prior;
    uint8_t good[PRESET_MAX_BYTES + 1u], p[PRESET_MAX_BYTES + 1u];
    uint32_t n = 0;
    int rc;
    user(&base, PRESET_ENGINE_CHICAGO, 0);
    ck(preset_record_encode(good, PRESET_MAX_BYTES, &n, &base) == PRESET_OK,
       "A2 setup");
    memset(&dest, 0x7B, sizeof dest);
    prior = dest;
#define NEG(CODE, EXPECT, LABEL) do { \
    rc = (CODE); \
    ck(rc == (EXPECT), LABEL); \
    ck(memcmp(&dest, &prior, sizeof dest) == 0, LABEL " unchanged"); \
} while (0)
    memcpy(p, good, n);
    p[100] ^= 0x55u;
    NEG(preset_record_import_file(&dest, p, n, PRESET_ENGINE_CHICAGO),
        PRESET_ERR_CRC, "A2 invalid CRC");
    NEG(preset_record_import_file(&dest, good, n - 1u, PRESET_ENGINE_CHICAGO),
        PRESET_ERR_LENGTH, "A2 truncation strict length");
    memcpy(p, good, n);
    p[0] ^= 1u;
    crc_fix(p, n);
    NEG(preset_record_import_file(&dest, p, n, PRESET_ENGINE_CHICAGO),
        PRESET_ERR_MAGIC, "A2 invalid magic");
    memcpy(p, good, n);
    le32(p + 12, n - 1u);
    crc_fix(p, n);
    NEG(preset_record_import_file(&dest, p, n, PRESET_ENGINE_CHICAGO),
        PRESET_ERR_LENGTH, "A2 invalid total");
    memcpy(p, good, n);
    p[n] = 0;
    NEG(preset_record_import_file(&dest, p, n + 1u, PRESET_ENGINE_CHICAGO),
        PRESET_ERR_LENGTH, "A2 trailing byte");
    memcpy(p, good, n);
    p[4] = 2u;
    crc_fix(p, n);
    NEG(preset_record_import_file(&dest, p, n, PRESET_ENGINE_CHICAGO),
        PRESET_ERR_VERSION, "A3 newer format");
    ck(preset_import_class(PRESET_ERR_VERSION) == PRESET_IMPORT_NEWER_DATA,
       "A3 newer format class");
    memcpy(p, good, n);
    p[8] = 2u;
    crc_fix(p, n);
    NEG(preset_record_import_file(&dest, p, n, PRESET_ENGINE_CHICAGO),
        PRESET_ERR_SCHEMA, "A3 newer schema");
    ck(preset_import_class(PRESET_ERR_SCHEMA) == PRESET_IMPORT_NEWER_DATA,
       "A3 newer schema class");
    NEG(preset_record_import_file(&dest, good, n, PRESET_ENGINE_MIAMI),
        PRESET_ERR_ENGINE, "A4 wrong Chicago->Miami");
    ck(preset_import_class(PRESET_ERR_ENGINE) == PRESET_IMPORT_WRONG_ENGINE,
       "A4 wrong engine class");
#undef NEG
}
static void capacity_guard(void)
{
    static const struct { uint8_t np, ns; } cases[] = {
        { 193, 0 }, { 255, 0 }, { 0, 17 }, { 0, 25 }, { 144, 17 }
    };
    struct guarded { uint8_t a[16]; preset_record_t rec; uint8_t b[64]; } d, prior;
    preset_record_t r;
    uint8_t p[PRESET_MAX_BYTES];
    uint32_t n = 0, total;
    unsigned i;
    user(&r, PRESET_ENGINE_808, 0);
    ck(preset_record_encode(p, sizeof p, &n, &r) == PRESET_OK, "array setup");
    for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        total = PRESET_HEADER_BYTES + PRESET_PAYLOAD_HEAD +
                cases[i].np + PRESET_SAMPLE_REF_BYTES * cases[i].ns;
        ck(total <= sizeof p, "array hostile input bounded wire");
        if (total > sizeof p)
            continue;
        memset(p + PRESET_HEADER_BYTES, 0, sizeof p - PRESET_HEADER_BYTES);
        p[96] = cases[i].np;
        p[97] = cases[i].ns;
        le32(p + 12, total);
        crc_fix(p, total);
        memset(&d, 0xA5, sizeof d);
        prior = d;
        ck(preset_record_load(&d.rec, p, total) == PRESET_ERR_PAYLOAD,
           "array structural reject before copy");
        ck(memcmp(&d, &prior, sizeof d) == 0, "array structural no mutation");
        ck(preset_record_decode(&d.rec, p, total, 0) == PRESET_ERR_PAYLOAD,
           "array decoder reject");
        ck(memcmp(&d, &prior, sizeof d) == 0, "array decoder no mutation");
        ck(preset_record_import_file(&d.rec, p, total, PRESET_ENGINE_808) ==
           PRESET_ERR_PAYLOAD, "array strict file reject");
        ck(memcmp(&d, &prior, sizeof d) == 0, "array strict no mutation");
    }
}
static void origins_samples_capacity(void)
{
    preset_record_t r;
    uint8_t b[PRESET_MAX_BYTES];
    uint8_t occupied[] = { 1u, 1u, 1u };
    unsigned slot = 99;
    uint32_t n = 0;
    user(&r, PRESET_ENGINE_909, 0);
    ck(preset_record_verify_sample(&r, 0, 0xAABBCC00u) == PRESET_OK,
       "A6 matching sample reference");
    ck(preset_record_verify_sample(&r, 0, 0) == PRESET_ERR_SAMPLE_CRC,
       "A6 changed/missing sample warning input");
    ck(preset_slot_find_empty(occupied, 3u, &slot) == PRESET_ERR_MEMORY_FULL &&
       slot == 99, "A5 no-eviction full capacity");
    ck(preset_record_export(b, sizeof b, &n, &r) == PRESET_OK,
       "A7 nonfactory user exported");
    r.engine_id = PRESET_ENGINE_CHICAGO;
    r.original_engine = PRESET_ENGINE_CHICAGO;
    r.nparams = PRESET_CHICAGO_NPARAMS_V1;
    r.nsamples = 0;
    r.origin_tag = PRESET_ORIGIN_FACTORY;
    r.factory_index = 1;
    ck(preset_record_export(b, sizeof b, &n, &r) ==
       (DNBOX_FACTORY_PRESET_EXPORT ? PRESET_OK : PRESET_ERR_EXPORT_FACTORY),
       "A8 factory gated by switch");
    r.origin_tag = PRESET_ORIGIN_USER;
    ck(preset_record_export(b, sizeof b, &n, &r) ==
       (DNBOX_FACTORY_PRESET_EXPORT ? PRESET_OK : PRESET_ERR_EXPORT_FACTORY),
       "A8 factory-derived gated by switch");
}
int main(void)
{
    roundtrips();
    rejected_envelopes();
    capacity_guard();
    origins_samples_capacity();
    if (failures) {
        fprintf(stderr, "FAIL %u/%u\n", failures, checks);
        return 1;
    }
    printf("PASS preset-file-boundary checks=%u factory_export=%d\n",
           checks, DNBOX_FACTORY_PRESET_EXPORT);
    return 0;
}
