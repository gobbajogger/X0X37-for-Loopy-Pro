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
static uint32_t factory_table_crc(void)
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
    c = upd_u32(c, PRESET_FACTORY_JUNO_N);
    for (i = 0; i < PRESET_FACTORY_JUNO_N; i++) {
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

/* Pinned after the first host run on this source. */
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
    /* engine is not relabelled: a failed import leaves caller buffer only if we
     * decode first; import returns before changing? Our import decodes then
     * checks, so out may be filled. Check original_engine was not rewritten. */
    expect(out.engine_id == PRESET_ENGINE_CHICAGO, "no relabel engine id");
    expect(out.original_engine == PRESET_ENGINE_CHICAGO, "no relabel original");

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
    expect(original_sound_factories(ORIGINAL_SOUND_JUNO) == 4u, "juno 4");
    expect(original_sound_factories(ORIGINAL_SOUND_DRONE) == 4u, "drone 4");
    expect(FACTORY_CHICAGO_NPRESETS == 81, "chicago 81");
    expect(FACTORY_MIAMI_NPRESETS == 30, "miami 30");
    got = factory_table_crc();
    printf("factory_table_crc=0x%08x\n", got);
    expect(got != 0, "factory hash nonzero");
#if PRESET_FACTORY_TABLE_CRC
    expect(got == PRESET_FACTORY_TABLE_CRC, "factory table hash pinned");
#endif
    /* Juno/Drone append-only: indices 0-3 are in the hash. A change fails. */
    expect(preset_record_factory_count(PRESET_ENGINE_JUNO) == 4u, "juno origin max 4");
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
    expect(file_crc("firmware/src/app/original_sound.c") == 0x07ec7595u, "original_sound.c");
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
    r.factory_index = 5;
    expect_eq(preset_record_encode(buf, sizeof buf, &n, &r), PRESET_ERR_ORIGIN,
              "juno origin 5 rejected (append not in this step)");
    r.factory_index = 4;
    expect_eq(preset_record_encode(buf, sizeof buf, &n, &r), PRESET_OK, "juno origin 4 ok");
}

int main(void)
{
    printf("DNBOX_FACTORY_PRESET_EXPORT=%d\n", (int)DNBOX_FACTORY_PRESET_EXPORT);
    test_sizes();
    test_names();
    test_roundtrip();
    test_reject();
    test_sample_crc();
    test_factory_stable();
    test_project_files();
    test_import_memory_export();
    test_plugin_room();
    test_commit_undo();
    test_origin_range();
    if (failures) {
        fprintf(stderr, "%d failure(s) in %d checks\n", failures, ran);
        return 1;
    }
    printf("PASS checks=%d chicago=%u miami=%u juno=%u drone=%u\n",
           ran, (unsigned)PRESET_FACTORY_CHICAGO_N, (unsigned)PRESET_FACTORY_MIAMI_N,
           (unsigned)PRESET_FACTORY_JUNO_N, (unsigned)PRESET_FACTORY_DRONE_N);
    return 0;
}
