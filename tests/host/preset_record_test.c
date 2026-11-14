/* SPDX-License-Identifier: GPL-3.0-only */
/* Host tests for the versioned user-preset record. No device flash, no UI. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../../firmware/src/app/preset_record.h"
#include "../../firmware/src/crc32_nibble.h"
#include "../../firmware/src/app/original_sound.h"
#include "../../firmware/src/app/factory_banks.h"
#include "../../firmware/src/app/bass_pattern_preset.h"

static int failures;
static int ran;

static void expect(int cond, const char *msg)
{
    ran++;
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", msg);
        failures++;
    }
}

static void expect_eq(int got, int want, const char *msg)
{
    ran++;
    if (got != want) {
        fprintf(stderr, "FAIL %s (got %d %s, want %d %s)\n",
                msg, got, preset_record_err_str(got),
                want, preset_record_err_str(want));
        failures++;
    }
}

static void rewrite_crc(uint8_t *buf, uint32_t n)
{
    uint32_t crc;
    buf[16] = buf[17] = buf[18] = buf[19] = 0;
    crc = x0x_crc32(buf, n);
    buf[16] = (uint8_t)crc;
    buf[17] = (uint8_t)(crc >> 8);
    buf[18] = (uint8_t)(crc >> 16);
    buf[19] = (uint8_t)(crc >> 24);
}

static void fill_params(preset_record_t *r)
{
    unsigned i;
    r->schema_version = (uint16_t)PRESET_SCHEMA_V1;
    r->nparams = (uint8_t)preset_record_expected_nparams(r->engine_id, r->schema_version);
    if (preset_record_expected_nsamples(r->engine_id, r->schema_version) >= 0)
        r->nsamples = (uint8_t)preset_record_expected_nsamples(r->engine_id, r->schema_version);
    else
        r->nsamples = 0;
    for (i = 0; i < r->nparams; i++)
        r->params[i] = (uint8_t)(17u + i * 3u);
}

static void fill_sample(preset_sample_ref_t *s, uint8_t source, uint16_t index, uint32_t crc, const char *name)
{
    unsigned i;
    s->source = source;
    s->reserved = 0;
    s->index = index;
    s->crc = crc;
    for (i = 0; i < 8u; i++)
        s->name[i] = 0;
    if (name) {
        for (i = 0; name[i] && i < 7u; i++)
            s->name[i] = (uint8_t)name[i];
    }
}

static void make_user(preset_record_t *r, uint16_t engine, const char *name)
{
    preset_record_clear(r);
    r->engine_id = engine;
    r->original_engine = (uint8_t)engine;
    r->origin_tag = PRESET_ORIGIN_USER;
    r->date = 20261113u;
    preset_record_set_name(r, name);
    preset_record_set_source(r, "CAPTURE");
    fill_params(r);
}

static int roundtrip(const preset_record_t *in, uint16_t expected_engine)
{
    uint8_t buf[PRESET_MAX_BYTES];
    uint32_t n = 0, i;
    preset_record_t out;
    int rc;
    for (i = 0; i < PRESET_MAX_BYTES; i++)
        buf[i] = 0xA5u;
    rc = preset_record_encode(buf, sizeof buf, &n, in);
    if (rc != PRESET_OK)
        return rc;
    if (n < PRESET_HEADER_BYTES || n > sizeof buf)
        return PRESET_ERR_LENGTH;
    preset_record_clear(&out);
    rc = preset_record_decode(&out, buf, n, expected_engine);
    if (rc != PRESET_OK)
        return rc;
    if (!preset_record_equal(in, &out))
        return PRESET_ERR_PAYLOAD;
    return PRESET_OK;
}

static uint32_t file_crc(const char *path)
{
    FILE *f = fopen(path, "rb");
    uint8_t buf[4096];
    uint32_t c, n;
    if (!f)
        return 0;
    c = 0xFFFFFFFFu;
    while ((n = (uint32_t)fread(buf, 1, sizeof buf, f)) > 0)
        c = x0x_crc32_upd(c, buf, n);
    fclose(f);
    return ~c;
}

static uint32_t upd_u32(uint32_t c, uint32_t v)
{
    uint8_t b[4];
    b[0] = (uint8_t)v;
    b[1] = (uint8_t)(v >> 8);
    b[2] = (uint8_t)(v >> 16);
    b[3] = (uint8_t)(v >> 24);
    return x0x_crc32_upd(c, b, 4);
}

static uint32_t upd_str(uint32_t c, const char *s)
{
    if (!s)
        return x0x_crc32_upd(c, (const uint8_t *)"", 1);
    return x0x_crc32_upd(c, (const uint8_t *)s, (uint32_t)strlen(s) + 1u);
}

/* Hash of factory tables via public APIs. Does not copy table bytes into a preset. */
static uint32_t factory_table_crc(unsigned juno_count)
{
    uint32_t c = 0xFFFFFFFFu;
    unsigned i, k;
    uint8_t cell[ORIGINAL_SOUND_BYTES];

    c = upd_u32(c, PRESET_FACTORY_CHICAGO_N);
    for (i = 0; i < PRESET_FACTORY_CHICAGO_N; i++) {
        original_chicago_program p;
        int src = original_sound_factory_source(ORIGINAL_SOUND_CHICAGO, i);
        c = upd_u32(c, (uint32_t)(src + 1));
        c = upd_str(c, original_sound_name(ORIGINAL_SOUND_CHICAGO, i));
        if (!original_chicago_program_init(&p, (unsigned)src))
            return 0;
        c = x0x_crc32_upd(c, (const uint8_t *)p.macro, (uint32_t)sizeof p.macro);
        c = x0x_crc32_upd(c, (const uint8_t *)p.target, (uint32_t)sizeof p.target);
        c = upd_u32(c, p.revision);
    }
    c = upd_u32(c, PRESET_FACTORY_MIAMI_N);
    for (i = 0; i < PRESET_FACTORY_MIAMI_N; i++) {
        original_miami_program p;
        int src = original_sound_factory_source(ORIGINAL_SOUND_MIAMI, i);
        c = upd_u32(c, (uint32_t)(src + 1));
        c = upd_str(c, original_sound_name(ORIGINAL_SOUND_MIAMI, i));
        if (!original_miami_program_init(&p, (unsigned)src))
            return 0;
        c = x0x_crc32_upd(c, (const uint8_t *)p.macro, (uint32_t)sizeof p.macro);
        c = x0x_crc32_upd(c, (const uint8_t *)p.target, (uint32_t)sizeof p.target);
        c = upd_u32(c, p.revision);
    }
    c = upd_u32(c, juno_count);
    for (i = 0; i < juno_count; i++) {
        c = upd_str(c, original_sound_name(ORIGINAL_SOUND_JUNO, i));
        if (!original_sound_init(cell, ORIGINAL_SOUND_JUNO, i))
            return 0;
        c = x0x_crc32_upd(c, cell, ORIGINAL_SOUND_BYTES);
        for (k = 0; k < ORIGINAL_JUNO_NPARAMS; k++) {
            float v = 0.0f;
            if (!original_sound_value(cell, k, &v))
                return 0;
            c = x0x_crc32_upd(c, (const uint8_t *)&v, (uint32_t)sizeof v);
        }
    }
    c = upd_u32(c, PRESET_FACTORY_DRONE_N);
    for (i = 0; i < PRESET_FACTORY_DRONE_N; i++) {
        c = upd_str(c, original_sound_name(ORIGINAL_SOUND_DRONE, i));
        if (!original_sound_init(cell, ORIGINAL_SOUND_DRONE, i))
            return 0;
        c = x0x_crc32_upd(c, cell, ORIGINAL_SOUND_BYTES);
        for (k = 0; k < ORIGINAL_DRONE_NPARAMS; k++) {
            float v = 0.0f;
            if (!original_sound_value(cell, k, &v))
                return 0;
            c = x0x_crc32_upd(c, (const uint8_t *)&v, (uint32_t)sizeof v);
        }
    }
    return ~c;
}

/* Keep the original four Juno identities pinned even when factories append. */
#ifndef PRESET_FACTORY_TABLE_CRC
#define PRESET_FACTORY_TABLE_CRC 0xd644a5f5u
#endif

static void test_sizes(void)
{
    printf("HOST ONLY sizeof(preset_record_t)=%u\n", (unsigned)sizeof(preset_record_t));
    printf("HOST ONLY sizeof(preset_sample_ref_t)=%u\n", (unsigned)sizeof(preset_sample_ref_t));
    printf("HOST ONLY sizeof(preset_commit_t)=%u\n", (unsigned)sizeof(preset_commit_t));
    printf("HOST ONLY PRESET_HEADER_BYTES=%u PRESET_MAX_BYTES=%u\n",
           (unsigned)PRESET_HEADER_BYTES, (unsigned)PRESET_MAX_BYTES);
    expect(sizeof(preset_sample_ref_t) == 16u, "sample-ref sizeof 16");
    expect(sizeof(preset_record_t) < 800u, "working record stays bounded");
    expect(DNBOX_FACTORY_PRESET_EXPORT == 0 || DNBOX_FACTORY_PRESET_EXPORT == 1,
           "single switch is 0 or 1");
}

static void test_names(void)
{
    const char *mia = preset_record_engine_name(PRESET_ENGINE_MIAMI);
    const char *chi = preset_record_engine_name(PRESET_ENGINE_CHICAGO);
    expect(mia && strcmp(mia, "Miami") == 0, "Miami name");
    expect(chi && strcmp(chi, "Chicago") == 0, "Chicago name");
    expect(strstr(mia, "303") == 0, "Miami is not 303");
    expect(preset_record_origin_name(PRESET_ORIGIN_FACTORY) &&
           strcmp(preset_record_origin_name(PRESET_ORIGIN_FACTORY), "FACTORY") == 0,
           "origin FACTORY");
    expect(preset_record_origin_name(PRESET_ORIGIN_USER) &&
           strcmp(preset_record_origin_name(PRESET_ORIGIN_USER), "USER") == 0,
           "origin USER");
    expect(preset_record_origin_name(PRESET_ORIGIN_IMPORTED) &&
           strcmp(preset_record_origin_name(PRESET_ORIGIN_IMPORTED), "IMP") == 0,
           "origin IMP");
    expect(preset_record_origin_name(PRESET_ORIGIN_UNKNOWN) &&
           strcmp(preset_record_origin_name(PRESET_ORIGIN_UNKNOWN), "UNKNOWN") == 0,
           "origin UNKNOWN");
    expect(preset_import_class(PRESET_ERR_ENGINE) == PRESET_IMPORT_WRONG_ENGINE,
           "class WRONG ENGINE");
    expect(strcmp(preset_import_message(PRESET_ERR_ENGINE), "WRONG ENGINE") == 0,
           "msg WRONG ENGINE");
    expect(strcmp(preset_import_message(PRESET_ERR_CRC), "BAD FILE") == 0, "msg BAD FILE");
    expect(strcmp(preset_import_message(PRESET_ERR_TRUNCATED), "BAD FILE") == 0,
           "truncated is BAD FILE");
    expect(strcmp(preset_import_message(PRESET_ERR_MEMORY_FULL), "MEMORY FULL") == 0,
           "msg MEMORY FULL");
    expect(strcmp(preset_import_message(PRESET_ERR_VERSION), "NEWER DATA") == 0,
           "msg NEWER DATA");
    expect(strcmp(preset_import_message(PRESET_ERR_SCHEMA), "NEWER DATA") == 0,
           "schema is NEWER DATA");
    expect(preset_import_class(PRESET_ERR_OLDER) == PRESET_IMPORT_BAD_FILE,
           "older unknown is BAD FILE");
}

static void add_909_samples(preset_record_t *r)
{
    r->nsamples = PRESET_909_NSAMPLES_V1;
    fill_sample(&r->samples[0], PRESET_SAMPLE_ROM, 0, 0x11111111u, "CH");
    fill_sample(&r->samples[1], PRESET_SAMPLE_ROM, 1, 0x22222222u, "OH");
    fill_sample(&r->samples[2], PRESET_SAMPLE_ROM, 2, 0x33333333u, "CR");
    fill_sample(&r->samples[3], PRESET_SAMPLE_ROM, 3, 0x44444444u, "RD");
}

static void add_break_samples(preset_record_t *r)
{
    r->nsamples = PRESET_BREAK_NSAMPLES_V1;
    fill_sample(&r->samples[0], PRESET_SAMPLE_USER, 0, 0xABCDEFu, "SLOT0");
    fill_sample(&r->samples[1], PRESET_SAMPLE_PACK, 2, 0x55555555u, "PACK2");
}

static void test_roundtrip(void)
{
    preset_record_t r;
    uint16_t engines[7] = {
        PRESET_ENGINE_CHICAGO, PRESET_ENGINE_MIAMI, PRESET_ENGINE_JUNO,
        PRESET_ENGINE_DRONE, PRESET_ENGINE_909, PRESET_ENGINE_808,
        PRESET_ENGINE_BREAK
    };
    const char *names[7] = {
        "User Chi", "User Mia", "User Juno", "User Drone", "User 909", "User 808", "User BRK"
    };
    unsigned i;
    for (i = 0; i < 7u; i++) {
        char msg[64];
        make_user(&r, engines[i], names[i]);
        if (engines[i] == PRESET_ENGINE_909)
            add_909_samples(&r);
        if (engines[i] == PRESET_ENGINE_BREAK)
            add_break_samples(&r);
        snprintf(msg, sizeof msg, "round-trip %s", preset_record_engine_name(engines[i]));
        expect_eq(roundtrip(&r, engines[i]), PRESET_OK, msg);
        expect_eq(roundtrip(&r, 0), PRESET_OK, "round-trip any-engine");
    }
}

static void test_reject(void)
{
    preset_record_t r, out;
    uint8_t buf[PRESET_MAX_BYTES];
    uint32_t n = 0, i;
    int rc;
    make_user(&r, PRESET_ENGINE_CHICAGO, "Cut");
    expect_eq(preset_record_encode(buf, sizeof buf, &n, &r), PRESET_OK, "encode for reject");

    /* truncated */
    expect_eq(preset_record_decode(&out, buf, 0, 0), PRESET_ERR_TRUNCATED, "empty");
    expect_eq(preset_record_decode(&out, buf, PRESET_HEADER_BYTES - 1u, 0),
              PRESET_ERR_TRUNCATED, "short header");
    expect_eq(preset_record_decode(&out, buf, n - 1u, 0), PRESET_ERR_TRUNCATED, "short body");
    expect(preset_import_class(PRESET_ERR_TRUNCATED) == PRESET_IMPORT_BAD_FILE,
           "truncated class");

    /* bad CRC */
    buf[40] ^= 1u;
    expect_eq(preset_record_decode(&out, buf, n, 0), PRESET_ERR_CRC, "bad CRC");
    buf[40] ^= 1u;
    expect(preset_import_class(PRESET_ERR_CRC) == PRESET_IMPORT_BAD_FILE, "crc class");

    /* unknown newer version, CRC valid */
    buf[4] = 99;
    buf[5] = 0;
    {
        uint32_t crc = x0x_crc32(buf, n);
        /* CRC field must be zero during compute — zero, compute, write */
        buf[16] = buf[17] = buf[18] = buf[19] = 0;
        crc = x0x_crc32(buf, n);
        buf[16] = (uint8_t)crc;
        buf[17] = (uint8_t)(crc >> 8);
        buf[18] = (uint8_t)(crc >> 16);
        buf[19] = (uint8_t)(crc >> 24);
    }
    rc = preset_record_decode(&out, buf, n, 0);
    expect_eq(rc, PRESET_ERR_VERSION, "unknown version");
    expect(preset_import_class(rc) == PRESET_IMPORT_NEWER_DATA, "version class");

    /* restore a clean record */
    expect_eq(preset_record_encode(buf, sizeof buf, &n, &r), PRESET_OK, "re-encode");
    /* wrong engine */
    expect_eq(preset_record_decode(&out, buf, n, PRESET_ENGINE_MIAMI),
              PRESET_ERR_ENGINE, "wrong engine decode");
    expect_eq(preset_record_import(&out, buf, n, PRESET_ENGINE_MIAMI),
              PRESET_ERR_ENGINE, "import WRONG ENGINE");
    expect(preset_import_class(PRESET_ERR_ENGINE) == PRESET_IMPORT_WRONG_ENGINE,
           "import class");
    /* Slot-clean behaviour of a failed import is in
     * test_import_failed_leaves_slot_clean. */

    /* newer schema */
    expect_eq(preset_record_encode(buf, sizeof buf, &n, &r), PRESET_OK, "re-encode 2");
    buf[8] = 2;
    buf[9] = 0;
    buf[16] = buf[17] = buf[18] = buf[19] = 0;
    {
        uint32_t crc = x0x_crc32(buf, n);
        buf[16] = (uint8_t)crc;
        buf[17] = (uint8_t)(crc >> 8);
        buf[18] = (uint8_t)(crc >> 16);
        buf[19] = (uint8_t)(crc >> 24);
    }
    rc = preset_record_decode(&out, buf, n, 0);
    expect_eq(rc, PRESET_ERR_SCHEMA, "newer schema");
    expect(preset_import_class(rc) == PRESET_IMPORT_NEWER_DATA, "schema class");

    (void)i;
}

static void test_sample_crc(void)
{
    preset_record_t r;
    uint32_t actual[4] = {0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u};
    make_user(&r, PRESET_ENGINE_909, "Kit");
    add_909_samples(&r);
    expect_eq(preset_record_verify_samples(&r, actual, 4), PRESET_OK, "sample crc ok");
    actual[2] ^= 1u;
    expect_eq(preset_record_verify_samples(&r, actual, 4), PRESET_ERR_SAMPLE_CRC,
              "sample crc mismatch");
    expect_eq(preset_record_verify_sample(&r, 2, 0x33333333u), PRESET_OK, "one sample ok");
    expect_eq(preset_record_verify_sample(&r, 2, 0xdeadbeefu), PRESET_ERR_SAMPLE_CRC,
              "one sample bad");
}

static void test_factory_stable(void)
{
    uint32_t got;
    expect(PRESET_FACTORY_CHICAGO_N == FACTORY_CHICAGO_NPRESETS, "chicago count factory_banks");
    expect(PRESET_FACTORY_CHICAGO_N == ORIGINAL_CHICAGO_FACTORIES, "chicago count original");
    expect(PRESET_FACTORY_MIAMI_N == FACTORY_MIAMI_NPRESETS, "miami count factory_banks");
    expect(PRESET_FACTORY_MIAMI_N == original_sound_factories(ORIGINAL_SOUND_MIAMI),
           "miami count original");
    expect(PRESET_FACTORY_JUNO_N == ORIGINAL_JUNO_FACTORIES, "juno count");
    expect(PRESET_FACTORY_DRONE_N == ORIGINAL_DRONE_FACTORIES, "drone count");
    expect(ORIGINAL_JUNO_FACTORIES == 4u || ORIGINAL_JUNO_FACTORIES == 8u,
           "known Juno bank profile");
    expect(original_sound_factories(ORIGINAL_SOUND_JUNO) == ORIGINAL_JUNO_FACTORIES,
           "juno count original");
    expect(original_sound_factories(ORIGINAL_SOUND_DRONE) == 4u, "drone 4");
    expect(FACTORY_CHICAGO_NPRESETS == 81, "chicago 81");
    expect(FACTORY_MIAMI_NPRESETS == 30, "miami 30");
    got = factory_table_crc(4u);
    printf("factory_table_crc=0x%08x\n", got);
    expect(got != 0, "factory hash nonzero");
#if PRESET_FACTORY_TABLE_CRC
    expect(got == PRESET_FACTORY_TABLE_CRC, "factory table hash pinned");
#endif
    /* The historical pin above still covers Juno/Drone indices 0-3. */
    if (ORIGINAL_JUNO_FACTORIES == 8u) {
        uint32_t full = factory_table_crc(8u);
        printf("expanded_factory_table_crc=0x%08x\n", full);
        expect(full == 0xf4c1b51eu, "expanded factory table hash pinned");
    }
    expect(preset_record_factory_count(PRESET_ENGINE_JUNO) == ORIGINAL_JUNO_FACTORIES,
           "juno origin max matches bank");
    expect(preset_record_factory_count(PRESET_ENGINE_DRONE) == 4u, "drone origin max 4");
}

static void test_project_files(void)
{
    expect(file_crc("firmware/src/app/project.c") == 0xa20e0a98u, "project.c unchanged");
    expect(file_crc("tests/fixtures/project-v3-sound10.json") == 0x750e04dau,
           "fixture project-v3");
    expect(file_crc("tests/fixtures/project-v6-sound10.json") == 0xa26387a4u,
           "fixture project-v6");
    expect(file_crc("tests/fixtures/v0102_store.bin") == 0x20f81f6cu, "fixture v0102_store");
    expect(file_crc("tests/fixtures/v1_store.bin") == 0xf7e6c7e6u, "fixture v1_store");
    expect(file_crc("tests/host/fixtures/bass_identity/project-v3-sound10.json") == 0x750e04dau,
           "host fixture v3");
    expect(file_crc("tests/host/fixtures/bass_identity/project-v6-sound10.json") == 0xa26387a4u,
           "host fixture v6");
    expect(file_crc("firmware/src/app/factory_banks.c") == 0x500d1152u, "factory_banks.c");
    /* original_sound.c moved its Juno arrays into juno_stock_bank.h in J1.
     * Pin actual bank names, cells and values above instead of the text file. */
    expect(file_crc("experiments/original_synth/chicago_program.c") == 0xccc1dcc2u,
           "chicago_program.c");
    expect(file_crc("experiments/original_synth/miami_program.c") == 0x95da36bdu,
           "miami_program.c");
    expect(file_crc("firmware/src/app/bass_pattern_preset.h") == 0x82bd9921u,
           "bass_pattern_preset.h");
}

static void test_import_memory_export(void)
{
    preset_record_t r, out;
    uint8_t buf[PRESET_MAX_BYTES], guard[PRESET_MAX_BYTES];
    uint32_t n = 0, i, w;
    uint8_t used[4] = {1, 1, 1, 1};
    unsigned slot = 99;
    int rc;

    make_user(&r, PRESET_ENGINE_JUNO, "ImpSrc");
    r.origin_tag = PRESET_ORIGIN_IMPORTED;
    preset_record_set_source(&r, "file.dnpr");
    expect_eq(preset_record_encode(buf, sizeof buf, &n, &r), PRESET_OK, "encode imported");
    expect_eq(preset_record_import(&out, buf, n, PRESET_ENGINE_JUNO), PRESET_OK,
              "import matching engine");
    expect(out.origin_tag == PRESET_ORIGIN_IMPORTED, "imported origin kept");
    expect_eq(preset_record_import(&out, buf, n, PRESET_ENGINE_CHICAGO), PRESET_ERR_ENGINE,
              "Juno onto Chicago");

    expect_eq(preset_slot_find_empty(used, 4, &slot), PRESET_ERR_MEMORY_FULL, "bank full");
    expect(preset_import_class(PRESET_ERR_MEMORY_FULL) == PRESET_IMPORT_MEMORY_FULL,
           "MEMORY FULL class");
    used[2] = 0;
    expect_eq(preset_slot_find_empty(used, 4, &slot), PRESET_OK, "empty slot found");
    expect(slot == 2, "first empty is 2");

    /* unknown provenance loads */
    make_user(&r, PRESET_ENGINE_DRONE, "Old");
    r.origin_tag = PRESET_ORIGIN_UNKNOWN;
    r.original_engine = 0;
    r.date = 0;
    r.factory_index = 0;
    preset_record_set_source(&r, 0);
    expect_eq(roundtrip(&r, 0), PRESET_OK, "unknown origin loads");
    expect_eq(preset_record_encode(buf, sizeof buf, &n, &r), PRESET_OK, "encode unknown");
    expect_eq(preset_record_import(&out, buf, n, PRESET_ENGINE_DRONE), PRESET_OK,
              "import unknown origin uses engine_id");

    /* export user, switch OFF or ON: succeeds */
    make_user(&r, PRESET_ENGINE_MIAMI, "Mine");
    for (i = 0; i < sizeof guard; i++)
        guard[i] = 0xAAu;
    w = 0;
    rc = preset_record_export(guard, sizeof guard, &w, &r);
    expect_eq(rc, PRESET_OK, "user export");
    expect(w > PRESET_HEADER_BYTES, "user export wrote bytes");
    /* payload is small: not a factory program dump */
    expect(w < 200u, "user record has no factory table bytes");

    /* P5: imported, not factory-derived, exports with the switch OFF or ON. */
    make_user(&r, PRESET_ENGINE_MIAMI, "Brought");
    r.origin_tag = PRESET_ORIGIN_IMPORTED;
    r.factory_index = 0;
    preset_record_set_source(&r, "brought.dnpr");
    w = 0;
    rc = preset_record_export(guard, sizeof guard, &w, &r);
    expect_eq(rc, PRESET_OK, "P5 imported export");
    expect(w > PRESET_HEADER_BYTES, "P5 wrote bytes");
    expect(guard[20] == PRESET_ORIGIN_IMPORTED, "P5 origin in file");

    /* factory export gate */
    make_user(&r, PRESET_ENGINE_CHICAGO, "Fac");
    r.origin_tag = PRESET_ORIGIN_FACTORY;
    r.factory_index = 5;
    preset_record_set_source(&r, "factory");
    for (i = 0; i < sizeof guard; i++)
        guard[i] = 0xAAu;
    w = 99;
    rc = preset_record_export(guard, sizeof guard, &w, &r);
#if DNBOX_FACTORY_PRESET_EXPORT
    expect_eq(rc, PRESET_OK, "P7 factory export ON");
    expect(w > PRESET_HEADER_BYTES, "P7 wrote a file");
#else
    expect_eq(rc, PRESET_ERR_EXPORT_FACTORY, "P6 factory export OFF");
    expect(w == 99, "P6 written unchanged");
    for (i = 0; i < sizeof guard; i++)
        if (guard[i] != 0xAAu) {
            expect(0, "P6 no partial file");
            break;
        }
#endif

    /* factory-derived user capture, same gate */
    make_user(&r, PRESET_ENGINE_CHICAGO, "FromFac");
    r.origin_tag = PRESET_ORIGIN_USER;
    r.factory_index = 12;
    preset_record_set_source(&r, "Chicago 12");
    for (i = 0; i < sizeof guard; i++)
        guard[i] = 0xAAu;
    w = 0;
    rc = preset_record_export(guard, sizeof guard, &w, &r);
#if DNBOX_FACTORY_PRESET_EXPORT
    expect_eq(rc, PRESET_OK, "derived export ON");
#else
    expect_eq(rc, PRESET_ERR_EXPORT_FACTORY, "P8 derived export OFF");
    for (i = 0; i < 16u; i++)
        expect(guard[i] == 0xAAu, "P8 no partial");
#endif

    /* too-small buffer: no partial write */
    make_user(&r, PRESET_ENGINE_808, "Tiny");
    for (i = 0; i < 16u; i++)
        guard[i] = 0xAAu;
    w = 0;
    rc = preset_record_export(guard, 16, &w, &r);
    expect_eq(rc, PRESET_ERR_BUFFER, "export buffer");
    for (i = 0; i < 16u; i++)
        expect(guard[i] == 0xAAu, "buffer short: no partial");
}

static void test_plugin_room(void)
{
    preset_record_t r;
    make_user(&r, PRESET_ENGINE_MIAMI, "Plug");
    r.origin_tag = PRESET_ORIGIN_IMPORTED;
    r.plugin_format = PRESET_PLUGIN_FORMAT;
    r.plugin_version = 0x0102;
    preset_record_set_source(&r, "plugin-file");
    expect_eq(roundtrip(&r, PRESET_ENGINE_MIAMI), PRESET_OK, "plugin placeholder round-trip");
    expect(PRESET_PLUGIN_FORMAT != 0, "placeholder id is not none");
    /* absent plugin fields (zeros) still load */
    r.plugin_format = PRESET_PLUGIN_NONE;
    r.plugin_version = 0;
    expect_eq(roundtrip(&r, 0), PRESET_OK, "no plugin fields loads");
}

static void test_commit_undo(void)
{
    preset_commit_t a, b, d;
    uint8_t ba[PRESET_COMMIT_BYTES], bb[PRESET_COMMIT_BYTES], torn[PRESET_COMMIT_BYTES];
    uint32_t na, nb, i;
    int win, prev;
    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    a.seq = 1;
    a.slot = 3;
    a.record_crc = 0x1010;
    a.copy = 0;
    b.seq = 2;
    b.slot = 3;
    b.record_crc = 0x2020;
    b.copy = 1;
    expect_eq(preset_commit_encode(ba, sizeof ba, &na, &a), PRESET_OK, "commit A");
    expect_eq(preset_commit_encode(bb, sizeof bb, &nb, &b), PRESET_OK, "commit B");
    win = preset_commit_winner(ba, na, bb, nb);
    expect(win == 1, "higher seq B wins");
    prev = preset_commit_previous(ba, na, bb, nb);
    expect(prev == 0, "previous is A (undo)");
    expect_eq(preset_commit_decode(&d, ba, na), PRESET_OK, "decode previous");
    expect(d.record_crc == 0x1010, "undo restores previous record crc");
    /* torn B: A still wins, no previous */
    for (i = 0; i < PRESET_COMMIT_BYTES; i++)
        torn[i] = bb[i];
    torn[8] ^= 1u;
    win = preset_commit_winner(ba, na, torn, nb);
    expect(win == 0, "torn B falls back to A");
    prev = preset_commit_previous(ba, na, torn, nb);
    expect(prev < 0, "no undo pad if only one valid");
}

static void test_origin_range(void)
{
    preset_record_t r;
    uint8_t buf[PRESET_MAX_BYTES];
    uint32_t n;
    make_user(&r, PRESET_ENGINE_CHICAGO, "O");
    r.factory_index = 82;
    expect_eq(preset_record_encode(buf, sizeof buf, &n, &r), PRESET_ERR_ORIGIN,
              "chicago origin 82 rejected");
    r.factory_index = 81;
    expect_eq(preset_record_encode(buf, sizeof buf, &n, &r), PRESET_OK, "chicago origin 81 ok");
    r.engine_id = PRESET_ENGINE_JUNO;
    r.original_engine = PRESET_ENGINE_JUNO;
    fill_params(&r);
    r.factory_index = (uint16_t)(ORIGINAL_JUNO_FACTORIES + 1u);
    expect_eq(preset_record_encode(buf, sizeof buf, &n, &r), PRESET_ERR_ORIGIN,
              "juno origin beyond bank rejected");
    r.factory_index = (uint16_t)ORIGINAL_JUNO_FACTORIES;
    expect_eq(preset_record_encode(buf, sizeof buf, &n, &r), PRESET_OK,
              "last juno origin accepted");
}

static void test_juno_invalid_origin(unsigned index)
{
    preset_record_t r, out, snapshot;
    uint8_t buf[PRESET_MAX_BYTES], guard[PRESET_MAX_BYTES];
    uint32_t n = 0, written = 123u;

    make_user(&r, PRESET_ENGINE_JUNO, "Bad origin");
    r.factory_index = 1u;
    expect_eq(preset_record_encode(buf, sizeof buf, &n, &r), PRESET_OK,
              "juno malformed-origin fixture");
    buf[22] = (uint8_t)index;
    buf[23] = (uint8_t)(index >> 8);
    rewrite_crc(buf, n);
    memset(&out, 0xA5, sizeof out);
    memcpy(&snapshot, &out, sizeof out);
    expect_eq(preset_record_decode(&out, buf, n, PRESET_ENGINE_JUNO), PRESET_ERR_ORIGIN,
              "juno invalid wire origin rejected");
    expect(memcmp(&out, &snapshot, sizeof out) == 0,
           "juno invalid decode preserves destination");
    expect_eq(preset_record_import_file(&out, buf, n, PRESET_ENGINE_JUNO), PRESET_ERR_ORIGIN,
              "juno invalid file origin rejected");
    expect(memcmp(&out, &snapshot, sizeof out) == 0,
           "juno invalid import preserves destination");

    r.factory_index = (uint16_t)index;
    memset(buf, 0xA5, sizeof buf);
    memcpy(guard, buf, sizeof buf);
    expect_eq(preset_record_encode(buf, sizeof buf, &written, &r), PRESET_ERR_ORIGIN,
              "juno invalid record origin rejected");
    expect(written == 123u && memcmp(buf, guard, sizeof buf) == 0,
           "juno invalid encode preserves buffer and length");
}

static void test_juno_factory_origins(void)
{
    static const uint8_t origins[] = {
        PRESET_ORIGIN_FACTORY, PRESET_ORIGIN_USER, PRESET_ORIGIN_IMPORTED
    };
    preset_record_t r, out;
    uint8_t cell[ORIGINAL_SOUND_BYTES], buf[PRESET_MAX_BYTES], guard[PRESET_MAX_BYTES];
    uint32_t n, written;
    unsigned index, tag;
    int rc;

    for (index = 0; index < ORIGINAL_JUNO_FACTORIES; index++) {
        expect(original_sound_init(cell, ORIGINAL_SOUND_JUNO, index),
               "juno origin fixture bank entry exists");
        expect(cell[30] == index, "juno bank index remains zero-based");
        for (tag = 0; tag < sizeof origins / sizeof origins[0]; tag++) {
            make_user(&r, PRESET_ENGINE_JUNO, original_sound_name(ORIGINAL_SOUND_JUNO, index));
            r.origin_tag = origins[tag];
            /* Synthetic codec fixture: a future runtime adapter must preserve
             * this one-based provenance when capturing or editing the sound. */
            r.factory_index = (uint16_t)(index + 1u);
            r.params[0] ^= 1u;
            preset_record_set_source(&r, "edited Juno");
            n = 0;
            rc = preset_record_encode(buf, sizeof buf, &n, &r);
            expect_eq(rc, PRESET_OK, "juno supported origin encodes");
            if (rc != PRESET_OK)
                continue;
            expect(buf[22] == index + 1u && buf[23] == 0u,
                   "juno wire origin remains one-based");
            expect_eq(preset_record_import_file(&out, buf, n, PRESET_ENGINE_JUNO), PRESET_OK,
                      "juno supported origin imports");
            expect(preset_record_equal(&r, &out),
                   "juno edited record preserves provenance and parameters");
            expect(preset_record_factory_derived(&out), "juno derived status retained");

            memset(buf, 0xA5, sizeof buf);
            memcpy(guard, buf, sizeof buf);
            written = 123u;
            rc = preset_record_export(buf, sizeof buf, &written, &out);
#if DNBOX_FACTORY_PRESET_EXPORT
            expect_eq(rc, PRESET_OK, "juno factory-derived export ON");
            expect_eq(preset_record_import_file(&out, buf, written, PRESET_ENGINE_JUNO), PRESET_OK,
                      "juno exported origin imports");
            expect(preset_record_equal(&r, &out), "juno exported provenance unchanged");
#else
            expect_eq(rc, PRESET_ERR_EXPORT_FACTORY, "juno factory-derived export OFF");
            expect(written == 123u && memcmp(buf, guard, sizeof buf) == 0,
                   "juno blocked export preserves buffer and length");
#endif
        }
    }
    test_juno_invalid_origin(ORIGINAL_JUNO_FACTORIES + 1u);
    if (ORIGINAL_JUNO_FACTORIES + 1u != 9u)
        test_juno_invalid_origin(9u);
}


/* Golden v1 wire bytes, captured from the encoder at 9bda58dd (tree
 * cf846cd3) before the size-optimization pass. A later encoder must
 * emit these exact bytes for the same inputs. */
static const uint8_t GOLDEN_CHI[] = {
    0x44, 0x4e, 0x50, 0x52, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7a, 0x00, 0x00, 0x00,
    0x7f, 0x21, 0x90, 0x92, 0x02, 0x01, 0x00, 0x00, 0xf9, 0x28, 0x35, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x47, 0x6f, 0x6c, 0x64, 0x65, 0x6e, 0x20, 0x43, 0x68, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x43, 0x41, 0x50, 0x54, 0x55, 0x52, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x16, 0x00, 0x00, 0x00, 0x11, 0x14, 0x17, 0x1a, 0x1d, 0x20, 0x23, 0x26, 0x29, 0x2c, 0x2f, 0x32,
    0x35, 0x38, 0x3b, 0x3e, 0x41, 0x44, 0x47, 0x4a, 0x4d, 0x50
};

static const uint8_t GOLDEN_909[] = {
    0x44, 0x4e, 0x50, 0x52, 0x01, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x28, 0x01, 0x00, 0x00,
    0x66, 0x8b, 0x3b, 0x20, 0x03, 0x05, 0x00, 0x00, 0xf9, 0x28, 0x35, 0x01, 0x01, 0x00, 0x02, 0x01,
    0x4b, 0x69, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x66, 0x69, 0x6c, 0x65, 0x2e, 0x64, 0x6e, 0x70, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x84, 0x04, 0x00, 0x00, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16,
    0x18, 0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36,
    0x38, 0x3a, 0x3c, 0x3e, 0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e, 0x50, 0x52, 0x54, 0x56,
    0x58, 0x5a, 0x5c, 0x5e, 0x60, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e, 0x70, 0x72, 0x74, 0x76,
    0x78, 0x7a, 0x7c, 0x7e, 0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e, 0x90, 0x92, 0x94, 0x96,
    0x98, 0x9a, 0x9c, 0x9e, 0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2, 0xb4, 0xb6,
    0xb8, 0xba, 0xbc, 0xbe, 0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xd0, 0xd2, 0xd4, 0xd6,
    0xd8, 0xda, 0xdc, 0xde, 0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee, 0xf0, 0xf2, 0xf4, 0xf6,
    0xf8, 0xfa, 0xfc, 0xfe, 0x00, 0x02, 0x04, 0x06, 0x01, 0x00, 0x00, 0x00, 0x11, 0x11, 0x11, 0x11,
    0x43, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x22, 0x22, 0x22, 0x22,
    0x4f, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x33, 0x33, 0x33, 0x33,
    0x43, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x44, 0x44, 0x44, 0x44,
    0x52, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t GOLDEN_COMMIT[] = {
    0x44, 0x4e, 0x43, 0x4d, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x20, 0x20, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xac, 0x34, 0xf0, 0xba
};

static void expect_bytes(const uint8_t *got, const uint8_t *want, uint32_t n, const char *msg)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        if (got[i] != want[i]) {
            fprintf(stderr, "FAIL %s at %u (got 0x%02x, want 0x%02x)\n",
                    msg, i, got[i], want[i]);
            failures++;
            ran++;
            return;
        }
    }
    ran++;
}

static void test_golden_bytes(void)
{
    preset_record_t r, out;
    preset_commit_t m, d;
    uint8_t buf[PRESET_MAX_BYTES];
    uint32_t n = 0, i;
    int rc;

    /* Chicago user "Golden Chi", captured from the 9bda58dd encoder. */
    preset_record_clear(&r);
    r.engine_id = PRESET_ENGINE_CHICAGO;
    r.schema_version = PRESET_SCHEMA_V1;
    r.origin_tag = PRESET_ORIGIN_USER;
    r.original_engine = PRESET_ENGINE_CHICAGO;
    r.factory_index = 0;
    r.date = 20261113u;
    r.plugin_format = 0;
    r.plugin_version = 0;
    r.nparams = (uint8_t)PRESET_CHICAGO_NPARAMS_V1;
    r.nsamples = 0;
    preset_record_set_name(&r, "Golden Chi");
    preset_record_set_source(&r, "CAPTURE");
    for (i = 0; i < r.nparams; i++)
        r.params[i] = (uint8_t)(17u + i * 3u);
    memset(buf, 0xA5, sizeof buf);
    rc = preset_record_export(buf, (uint32_t)sizeof buf, &n, &r);
    expect_eq(rc, PRESET_OK, "golden chi encode");
    expect(n == (uint32_t)sizeof GOLDEN_CHI, "golden chi length 122");
    expect_bytes(buf, GOLDEN_CHI, n, "golden chi bytes");
    expect_eq(preset_record_decode(&out, GOLDEN_CHI, (uint32_t)sizeof GOLDEN_CHI,
                                   PRESET_ENGINE_CHICAGO), PRESET_OK,
              "golden chi decode");
    expect(preset_record_equal(&r, &out), "golden chi equal");

    /* 909 imported kit with plugin format 1, version 0x0102, four ROM refs. */
    preset_record_clear(&r);
    r.engine_id = PRESET_ENGINE_909;
    r.schema_version = PRESET_SCHEMA_V1;
    r.origin_tag = PRESET_ORIGIN_IMPORTED;
    r.original_engine = PRESET_ENGINE_909;
    r.date = 20261113u;
    r.plugin_format = PRESET_PLUGIN_FORMAT;
    r.plugin_version = 0x0102;
    r.nparams = (uint8_t)PRESET_909_NPARAMS_V1;
    r.nsamples = (uint8_t)PRESET_909_NSAMPLES_V1;
    preset_record_set_name(&r, "Kit");
    preset_record_set_source(&r, "file.dnpr");
    for (i = 0; i < r.nparams; i++)
        r.params[i] = (uint8_t)(i * 2u);
    fill_sample(&r.samples[0], PRESET_SAMPLE_ROM, 0, 0x11111111u, "CH");
    fill_sample(&r.samples[1], PRESET_SAMPLE_ROM, 1, 0x22222222u, "OH");
    fill_sample(&r.samples[2], PRESET_SAMPLE_ROM, 2, 0x33333333u, "CR");
    fill_sample(&r.samples[3], PRESET_SAMPLE_ROM, 3, 0x44444444u, "RD");
    rc = preset_record_encode(buf, (uint32_t)sizeof buf, &n, &r);
    expect_eq(rc, PRESET_OK, "golden 909 encode");
    expect(n == (uint32_t)sizeof GOLDEN_909, "golden 909 length 296");
    expect_bytes(buf, GOLDEN_909, n, "golden 909 bytes");
    expect_eq(preset_record_decode(&out, GOLDEN_909, (uint32_t)sizeof GOLDEN_909,
                                   PRESET_ENGINE_909), PRESET_OK,
              "golden 909 decode");
    expect(preset_record_equal(&r, &out), "golden 909 equal");

    /* Commit marker B: seq 2, slot 3, record_crc 0x2020, copy 1. */
    memset(&m, 0, sizeof m);
    m.seq = 2;
    m.slot = 3;
    m.record_crc = 0x2020;
    m.copy = 1;
    rc = preset_commit_encode(buf, PRESET_COMMIT_BYTES, &n, &m);
    expect_eq(rc, PRESET_OK, "golden commit encode");
    expect(n == (uint32_t)sizeof GOLDEN_COMMIT, "golden commit length 32");
    expect_bytes(buf, GOLDEN_COMMIT, n, "golden commit bytes");
    expect_eq(preset_commit_decode(&d, GOLDEN_COMMIT, (uint32_t)sizeof GOLDEN_COMMIT),
              PRESET_OK, "golden commit decode");
    expect(d.seq == 2 && d.slot == 3 && d.record_crc == 0x2020 && d.copy == 1,
           "golden commit fields");

    /* load() is the structural path: it accepts a record decode() rejects
     * for a bad sample name, so a load-only link can drop check_samples. */
    {
        uint8_t dirty[sizeof GOLDEN_909];
        uint32_t crc;
        memcpy(dirty, GOLDEN_909, sizeof GOLDEN_909);
        dirty[sizeof GOLDEN_909 - 1] = 0x7F; /* non-canonical pad in last name */
        memset(dirty + 16, 0, 4);
        crc = x0x_crc32(dirty, (uint32_t)sizeof GOLDEN_909);
        dirty[16] = (uint8_t)crc;
        dirty[17] = (uint8_t)(crc >> 8);
        dirty[18] = (uint8_t)(crc >> 16);
        dirty[19] = (uint8_t)(crc >> 24);
        expect_eq(preset_record_load(&out, dirty, (uint32_t)sizeof dirty),
                  PRESET_OK, "load skips sample-name check");
        expect_eq(preset_record_decode(&out, dirty, (uint32_t)sizeof dirty, 0),
                  PRESET_ERR_PAYLOAD, "decode still checks samples");
    }
}


/* R1: a failed import must not look like a filled slot. */
static void test_import_failed_leaves_slot_clean(void)
{
    preset_record_t r, slot;
    uint8_t buf[PRESET_MAX_BYTES];
    uint32_t n = 0;
    unsigned i;
    const uint8_t *p, *q;

    make_user(&r, PRESET_ENGINE_CHICAGO, "Src");
    expect_eq(preset_record_encode(buf, (uint32_t)sizeof buf, &n, &r), PRESET_OK,
              "R1 encode");

    /* Sentinel: a previously empty / unknown slot. */
    preset_record_clear(&slot);
    slot.engine_id = PRESET_ENGINE_NONE;
    slot.origin_tag = PRESET_ORIGIN_UNKNOWN;
    slot.nparams = 0;
    slot.nsamples = 0;
    preset_record_set_name(&slot, "EMPTY");

    expect_eq(preset_record_import(&slot, buf, n, PRESET_ENGINE_MIAMI),
              PRESET_ERR_ENGINE, "R1 WRONG ENGINE");

    expect(slot.engine_id == PRESET_ENGINE_NONE, "R1 engine_id unchanged");
    expect(slot.origin_tag == PRESET_ORIGIN_UNKNOWN, "R1 origin unchanged");
    expect(slot.nparams == 0, "R1 nparams unchanged");
    expect(slot.name[0] == 'E' && slot.name[1] == 'M', "R1 name unchanged");
    /* Not relabelled to the file's engine. */
    expect(slot.engine_id != PRESET_ENGINE_CHICAGO, "R1 not filled as Chicago");
    expect(slot.original_engine != PRESET_ENGINE_CHICAGO, "R1 original not filled");

    /* A second failure must not clobber a slot already filled by a good import. */
    expect_eq(preset_record_import(&slot, buf, n, PRESET_ENGINE_CHICAGO), PRESET_OK,
              "R1 matching import fills");
    {
        preset_record_t saved = slot;
        expect_eq(preset_record_import(&slot, buf, n, PRESET_ENGINE_MIAMI),
                  PRESET_ERR_ENGINE, "R1 second WRONG ENGINE");
        p = (const uint8_t *)&slot;
        q = (const uint8_t *)&saved;
        for (i = 0; i < (unsigned)sizeof slot; i++) {
            if (p[i] != q[i]) {
                expect(0, "R1 failed import did not clobber a good slot");
                return;
            }
        }
        expect(1, "R1 good slot left intact");
    }
}

/* R2: wrong magic and older-than-v1 format / schema. */
static void test_reject_bad_magic(void)
{
    preset_record_t r, out;
    uint8_t buf[PRESET_MAX_BYTES];
    uint32_t n = 0;
    int rc;

    make_user(&r, PRESET_ENGINE_CHICAGO, "Mag");
    expect_eq(preset_record_encode(buf, (uint32_t)sizeof buf, &n, &r), PRESET_OK,
              "R2 magic encode");
    preset_record_clear(&out);
    buf[0] ^= 1u;
    rc = preset_record_decode(&out, buf, n, 0);
    expect_eq(rc, PRESET_ERR_MAGIC, "R2 bad magic");
    expect(preset_import_class(rc) == PRESET_IMPORT_BAD_FILE, "R2 magic class BAD FILE");
    expect(out.engine_id == 0 && out.nparams == 0, "R2 bad magic leaves output clean");
    buf[0] ^= 1u;
    expect_eq(preset_record_decode(&out, buf, n, 0), PRESET_OK, "R2 restored decodes");
}

static void test_reject_older_version(void)
{
    preset_record_t r, out;
    uint8_t buf[PRESET_MAX_BYTES];
    uint32_t n = 0;
    int rc;

    make_user(&r, PRESET_ENGINE_CHICAGO, "Old");
    expect_eq(preset_record_encode(buf, (uint32_t)sizeof buf, &n, &r), PRESET_OK,
              "R2 older encode");

    /* Older format, CRC valid. */
    buf[4] = 0;
    buf[5] = 0;
    rewrite_crc(buf, n);
    preset_record_clear(&out);
    rc = preset_record_decode(&out, buf, n, 0);
    expect_eq(rc, PRESET_ERR_OLDER, "R2 older format");
    expect(preset_import_class(rc) == PRESET_IMPORT_BAD_FILE, "R2 older format BAD FILE");
    expect(out.engine_id == 0 && out.nparams == 0, "R2 older format leaves output clean");

    /* Restore, then older schema, CRC valid. */
    expect_eq(preset_record_encode(buf, (uint32_t)sizeof buf, &n, &r), PRESET_OK,
              "R2 older re-encode");
    buf[8] = 0;
    buf[9] = 0;
    rewrite_crc(buf, n);
    preset_record_clear(&out);
    rc = preset_record_decode(&out, buf, n, 0);
    expect_eq(rc, PRESET_ERR_OLDER, "R2 older schema");
    expect(preset_import_class(rc) == PRESET_IMPORT_BAD_FILE, "R2 older schema BAD FILE");
    expect(out.engine_id == 0 && out.nparams == 0, "R2 older schema leaves output clean");
}

/* R3: *written is set only after the buffer-size check passes. */
static void test_encode_written_after_buffer_check(void)
{
    preset_record_t r;
    preset_commit_t m;
    uint8_t guard[16], buf[PRESET_MAX_BYTES];
    uint32_t w, n = 0, i;
    int rc;

    make_user(&r, PRESET_ENGINE_CHICAGO, "Cap");

    w = 99;
    rc = preset_record_encode(0, 0, &w, &r);
    expect_eq(rc, PRESET_OK, "R3 encode size query");
    expect(w == (PRESET_HEADER_BYTES + PRESET_PAYLOAD_HEAD + PRESET_CHICAGO_NPARAMS_V1),
           "R3 size query sets written");

    for (i = 0; i < 16u; i++)
        guard[i] = 0xAAu;
    w = 99;
    rc = preset_record_encode(guard, 16, &w, &r);
    expect_eq(rc, PRESET_ERR_BUFFER, "R3 encode short buffer");
    expect(w == 99, "R3 encode written unchanged on BUFFER");
    for (i = 0; i < 16u; i++)
        expect(guard[i] == 0xAAu, "R3 encode no partial");

    w = 99;
    rc = preset_record_export(guard, 16, &w, &r);
    expect_eq(rc, PRESET_ERR_BUFFER, "R3 export short buffer");
    expect(w == 99, "R3 export written unchanged on BUFFER");

    memset(&m, 0, sizeof m);
    m.seq = 1;
    m.slot = 0;
    m.copy = 0;
    w = 99;
    rc = preset_commit_encode(guard, 16, &w, &m);
    expect_eq(rc, PRESET_ERR_BUFFER, "R3 commit short buffer");
    expect(w == 99, "R3 commit written unchanged on BUFFER");

    w = 99;
    rc = preset_record_encode(buf, (uint32_t)sizeof buf, &w, &r);
    expect_eq(rc, PRESET_OK, "R3 encode success");
    expect(w == (PRESET_HEADER_BYTES + PRESET_PAYLOAD_HEAD + PRESET_CHICAGO_NPARAMS_V1),
           "R3 success sets written");
    n = w;
    w = 99;
    rc = preset_commit_encode(buf, PRESET_COMMIT_BYTES, &w, &m);
    expect_eq(rc, PRESET_OK, "R3 commit success");
    expect(w == PRESET_COMMIT_BYTES, "R3 commit success sets written");
    (void)n;
}

/* R5: 808 allows a variable sample-ref count; round-trip nsamples > 0
 * and reject a CRC mismatch. */
static void test_808_nsamples_and_crc(void)
{
    preset_record_t r, out;
    uint8_t buf[PRESET_MAX_BYTES];
    uint32_t n = 0, actual[2];

    make_user(&r, PRESET_ENGINE_808, "808kit");
    r.nsamples = 2;
    fill_sample(&r.samples[0], PRESET_SAMPLE_ROM, 0, 0xaaaaaaaau, "BD");
    fill_sample(&r.samples[1], PRESET_SAMPLE_ROM, 1, 0xbbbbbbbbu, "SD");
    expect_eq(roundtrip(&r, PRESET_ENGINE_808), PRESET_OK, "R5 808 samples round-trip");

    expect_eq(preset_record_encode(buf, (uint32_t)sizeof buf, &n, &r), PRESET_OK,
              "R5 encode");
    expect(n == (PRESET_HEADER_BYTES + PRESET_PAYLOAD_HEAD +
                 PRESET_808_NPARAMS_V1 + 2u * PRESET_SAMPLE_REF_BYTES),
           "R5 808 wire length");
    expect_eq(preset_record_decode(&out, buf, n, PRESET_ENGINE_808), PRESET_OK,
              "R5 decode");
    expect(out.nsamples == 2, "R5 nsamples is 2");
    actual[0] = 0xaaaaaaaau;
    actual[1] = 0xbbbbbbbbu;
    expect_eq(preset_record_verify_samples(&out, actual, 2), PRESET_OK,
              "R5 sample crc ok");
    actual[1] = 0xdeadbeefu;
    expect_eq(preset_record_verify_samples(&out, actual, 2), PRESET_ERR_SAMPLE_CRC,
              "R5 sample crc mismatch");
    expect_eq(preset_record_verify_sample(&out, 1, 0xdeadbeefu), PRESET_ERR_SAMPLE_CRC,
              "R5 one sample mismatch");
    expect_eq(preset_record_verify_sample(&out, 1, 0xbbbbbbbbu), PRESET_OK,
              "R5 one sample ok");
}


/* Capacity boundaries: a valid CRC and short total length must not permit
 * malicious nparams/nsamples to overrun preset_record_t. Both structural
 * and semantic entry points preserve the destination on any rejection. */
static void test_untrusted_array_counts(void)
{
    static const struct { uint8_t np, ns; } cases[] = {
        { 193u, 0u }, { 255u, 0u }, { 0u, 17u },
        { 0u, 25u }, { 144u, 17u }
    };
    preset_record_t user;
    uint8_t buf[PRESET_MAX_BYTES];
    uint32_t encoded = 0;
    unsigned i;
    struct guarded_record {
        uint8_t before[16];
        preset_record_t rec;
        uint8_t after[64];
    } got, saved;

    make_user(&user, PRESET_ENGINE_808, "bounds");
    expect_eq(preset_record_encode(buf, sizeof buf, &encoded, &user),
              PRESET_OK, "bounds seed encode");
    for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        uint32_t total = PRESET_HEADER_BYTES + PRESET_PAYLOAD_HEAD +
                         (uint32_t)cases[i].np +
                         (uint32_t)cases[i].ns * PRESET_SAMPLE_REF_BYTES;
        expect(total <= sizeof buf, "bounds malicious envelope fits max");
        if (total > sizeof buf)
            continue;
        memset(buf + PRESET_HEADER_BYTES, 0,
               sizeof buf - PRESET_HEADER_BYTES);
        buf[96] = cases[i].np;
        buf[97] = cases[i].ns;
        buf[12] = (uint8_t)total;
        buf[13] = (uint8_t)(total >> 8);
        buf[14] = (uint8_t)(total >> 16);
        buf[15] = (uint8_t)(total >> 24);
        rewrite_crc(buf, total);
        memset(&got, 0xA5, sizeof got);
        saved = got;
        expect_eq(preset_record_load(&got.rec, buf, total),
                  PRESET_ERR_PAYLOAD, "bounds structural load rejects before copy");
        expect(memcmp(&got, &saved, sizeof got) == 0,
               "bounds structural load preserves record and canaries");
        expect_eq(preset_record_decode(&got.rec, buf, total, 0),
                  PRESET_ERR_PAYLOAD, "bounds decode rejects before copy");
        expect(memcmp(&got, &saved, sizeof got) == 0,
               "bounds decode preserves record and canaries");
        expect_eq(preset_record_import_file(&got.rec, buf, total, PRESET_ENGINE_808),
                  PRESET_ERR_PAYLOAD, "bounds strict file import rejects");
        expect(memcmp(&got, &saved, sizeof got) == 0,
               "bounds file import preserves record and canaries");
    }
}

/* The wire record is the file: surplus bytes, a wrong engine, and a newer
 * schema are refused before any import destination can be changed. */
static void test_strict_file_import(void)
{
    preset_record_t user, got, snapshot;
    uint8_t buf[PRESET_MAX_BYTES + 1u], export_buf[PRESET_MAX_BYTES];
    uint32_t n = 0, m = 0;
    make_user(&user, PRESET_ENGINE_CHICAGO, "strict");
    expect_eq(preset_record_encode(buf, PRESET_MAX_BYTES, &n, &user),
              PRESET_OK, "strict file encode");
    preset_record_clear(&got);
    expect_eq(preset_record_import_file(&got, buf, n, PRESET_ENGINE_CHICAGO),
              PRESET_OK, "strict file import matches engine");
    expect(preset_record_equal(&got, &user), "strict file round-trip state");
    expect_eq(preset_record_export(export_buf, sizeof export_buf, &m, &got),
              PRESET_OK, "strict file export nonfactory");
    expect(m == n && memcmp(buf, export_buf, n) == 0,
           "strict file byte-identical export");
    snapshot = got;
    buf[n] = 0;
    expect_eq(preset_record_import_file(&got, buf, n + 1u, PRESET_ENGINE_CHICAGO),
              PRESET_ERR_LENGTH, "strict file trailing bytes rejected");
    expect(memcmp(&got, &snapshot, sizeof got) == 0,
           "strict file suffix leaves destination unchanged");
    expect_eq(preset_record_import_file(&got, buf, n, PRESET_ENGINE_MIAMI),
              PRESET_ERR_ENGINE, "strict file wrong engine rejected");
    expect(memcmp(&got, &snapshot, sizeof got) == 0,
           "strict file engine leaves destination unchanged");
    buf[8] = 2u;
    rewrite_crc(buf, n);
    expect_eq(preset_record_import_file(&got, buf, n, PRESET_ENGINE_CHICAGO),
              PRESET_ERR_SCHEMA, "strict file future schema rejected");
    expect(memcmp(&got, &snapshot, sizeof got) == 0,
           "strict file future schema leaves destination unchanged");
}

int main(void)
{
    printf("DNBOX_FACTORY_PRESET_EXPORT=%d\n", (int)DNBOX_FACTORY_PRESET_EXPORT);
    test_sizes();
    test_names();
    test_roundtrip();
    test_reject();
    test_import_failed_leaves_slot_clean();
    test_reject_bad_magic();
    test_reject_older_version();
    test_encode_written_after_buffer_check();
    test_808_nsamples_and_crc();
    test_untrusted_array_counts();
    test_strict_file_import();
    test_sample_crc();
    test_factory_stable();
    test_project_files();
    test_import_memory_export();
    test_plugin_room();
    test_commit_undo();
    test_origin_range();
    test_juno_factory_origins();
    test_golden_bytes();
    if (failures) {
        fprintf(stderr, "%d failure(s) in %d checks\n", failures, ran);
        return 1;
    }
    printf("PASS checks=%d chicago=%u miami=%u juno=%u drone=%u\n",
           ran, (unsigned)PRESET_FACTORY_CHICAGO_N, (unsigned)PRESET_FACTORY_MIAMI_N,
           (unsigned)PRESET_FACTORY_JUNO_N, (unsigned)PRESET_FACTORY_DRONE_N);
    return 0;
}
