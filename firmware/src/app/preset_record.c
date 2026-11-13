/* SPDX-License-Identifier: GPL-3.0-only */
/* User-preset record codec. Explicit little-endian wire, zlib CRC-32.
 * No flash I/O, no audio-path use, no factory table bytes, no static buffers.
 * The working record (HOST sizeof 536) lives on the stack or in caller scratch.
 *
 * CRC is the existing firmware CRC: firmware/src/crc32_nibble.h, symbols
 * x0x_crc32, x0x_crc32_upd, and table X0X_CRC32_NIBBLE_T. A unity build
 * includes that header once (include guards). A separate object that must
 * not duplicate the table is built with PRESET_RECORD_EXT_CRC and linked
 * against the existing provider.
 *
 * Host-only debug strings (err_str, engine_name, origin_name, import_message)
 * are omitted when PRESET_RECORD_NO_STRINGS is set. The device maps errors
 * to the small PRESET_ERR_* / PRESET_IMPORT_* codes. UI words come later. */
#include "preset_record.h"
#include <stddef.h>

#ifdef PRESET_RECORD_EXT_CRC
uint32_t x0x_crc32_upd(uint32_t c, const uint8_t *p, uint32_t n);
uint32_t x0x_crc32(const void *p, uint32_t n);
#else
#include "../crc32_nibble.h"
#endif

_Static_assert(PRESET_HEADER_BYTES == 96u, "header size");
_Static_assert(PRESET_SAMPLE_REF_BYTES == 16u, "sample-ref size");
_Static_assert(PRESET_COMMIT_BYTES == 32u, "commit-marker size");
_Static_assert(PRESET_909_NPARAMS_V1 <= PRESET_MAX_PARAMS, "909 params fit");
_Static_assert(PRESET_808_NPARAMS_V1 <= PRESET_MAX_PARAMS, "808 params fit");
_Static_assert(PRESET_BREAK_NPARAMS_V1 <= PRESET_MAX_PARAMS, "BREAK params fit");
_Static_assert(sizeof(preset_sample_ref_t) == 16u, "sample-ref HOST sizeof");
_Static_assert(PRESET_FACTORY_CHICAGO_N <= 255u, "factory count fits u8");

/* Engine-indexed v1 tables. Index 0 is PRESET_ENGINE_NONE. 0xFF in
 * k_nsamples means "any count up to PRESET_MAX_SAMPLES" (808). */
static const uint8_t k_nparams[8] = {
    0,
    (uint8_t)PRESET_CHICAGO_NPARAMS_V1,
    (uint8_t)PRESET_MIAMI_NPARAMS_V1,
    (uint8_t)PRESET_JUNO_NPARAMS_V1,
    (uint8_t)PRESET_DRONE_NPARAMS_V1,
    (uint8_t)PRESET_909_NPARAMS_V1,
    (uint8_t)PRESET_808_NPARAMS_V1,
    (uint8_t)PRESET_BREAK_NPARAMS_V1
};
static const uint8_t k_nsamples[8] = {
    0, 0, 0, 0, 0,
    (uint8_t)PRESET_909_NSAMPLES_V1,
    0xFFu,
    (uint8_t)PRESET_BREAK_NSAMPLES_V1
};
static const uint8_t k_factory[8] = {
    0,
    (uint8_t)PRESET_FACTORY_CHICAGO_N,
    (uint8_t)PRESET_FACTORY_MIAMI_N,
    (uint8_t)PRESET_FACTORY_JUNO_N,
    (uint8_t)PRESET_FACTORY_DRONE_N,
    0, 0, 0
};

/* Table-driven numeric header: one walk writes or reads every scalar field.
 * Packed as wire_off, width (1/2/4), offsetof(preset_record_t). */
#define HF(wo, w, m) { (uint8_t)(wo), (uint8_t)(w), (uint8_t)offsetof(preset_record_t, m) }
static const uint8_t k_hdr[][3] = {
    HF( 6, 2, engine_id),
    HF( 8, 2, schema_version),
    HF(10, 2, flags),
    HF(20, 1, origin_tag),
    HF(21, 1, original_engine),
    HF(22, 2, factory_index),
    HF(24, 4, date),
    HF(28, 2, plugin_format),
    HF(30, 2, plugin_version)
};
#define K_HDR_N ((unsigned)(sizeof k_hdr / sizeof k_hdr[0]))

static void wr_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void wr_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static uint16_t rd_u16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* Width-dispatch used only by the header table walk. */
static void wr_w(uint8_t *p, unsigned w, uint32_t v)
{
    if (w == 1u)
        p[0] = (uint8_t)v;
    else if (w == 2u)
        wr_u16(p, (uint16_t)v);
    else
        wr_u32(p, v);
}

static uint32_t rd_w(const uint8_t *p, unsigned w)
{
    if (w == 1u)
        return p[0];
    if (w == 2u)
        return rd_u16(p);
    return rd_u32(p);
}

static void copy_n(uint8_t *d, const uint8_t *s, unsigned n)
{
    unsigned i;
    for (i = 0; i < n; i++)
        d[i] = s[i];
}

static void zero_n(uint8_t *d, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++)
        d[i] = 0;
}

static int same_n(const uint8_t *a, const uint8_t *b, unsigned n)
{
    unsigned i;
    for (i = 0; i < n; i++) {
        if (a[i] != b[i])
            return 0;
    }
    return 1;
}

static int pad_canonical(const uint8_t *s, unsigned n)
{
    unsigned i, z = 0;
    for (i = 0; i < n; i++) {
        if (s[i] == 0)
            z = 1;
        else if (z)
            return 0;
    }
    return z ? 1 : 0;
}

static int set_pad(uint8_t *dst, unsigned n, const char *src)
{
    unsigned i;
    zero_n(dst, n);
    if (!src)
        return PRESET_OK;
    for (i = 0; src[i] != 0; i++) {
        if (i + 1u >= n)
            return PRESET_ERR_NAME;
        dst[i] = (uint8_t)src[i];
    }
    return PRESET_OK;
}

static uint32_t crc_zero_field(const uint8_t *p, uint32_t n, uint32_t off, uint32_t flen)
{
    uint8_t z[4];
    uint32_t c;
    zero_n(z, 4u);
    c = x0x_crc32_upd(0xFFFFFFFFu, p, off);
    c = x0x_crc32_upd(c, z, flen);
    c = x0x_crc32_upd(c, p + off + flen, n - off - flen);
    return ~c;
}

static uint32_t record_size(uint8_t nparams, uint8_t nsamples)
{
    return PRESET_HEADER_BYTES + PRESET_PAYLOAD_HEAD + (uint32_t)nparams +
           (uint32_t)nsamples * PRESET_SAMPLE_REF_BYTES;
}

static int engine_ok(uint16_t id)
{
    return id >= (uint16_t)PRESET_ENGINE_CHICAGO && id <= (uint16_t)PRESET_ENGINE_LAST;
}

static int tab_u8(uint16_t id, const uint8_t *t)
{
    if (id > (uint16_t)PRESET_ENGINE_LAST)
        return 0;
    return (int)t[id];
}

static void wr_sample(uint8_t *p, const preset_sample_ref_t *s)
{
    p[0] = s->source;
    p[1] = s->reserved;
    wr_u16(p + 2, s->index);
    wr_u32(p + 4, s->crc);
    copy_n(p + 8, s->name, 8u);
}

static void rd_sample(preset_sample_ref_t *s, const uint8_t *p)
{
    s->source = p[0];
    s->reserved = p[1];
    s->index = rd_u16(p + 2);
    s->crc = rd_u32(p + 4);
    copy_n(s->name, p + 8, 8u);
}

static void hdr_wr(uint8_t *out, const preset_record_t *rec)
{
    unsigned i;
    const uint8_t *rb = (const uint8_t *)rec;
    for (i = 0; i < K_HDR_N; i++) {
        const uint8_t *f = k_hdr[i];
        wr_w(out + f[0], f[1], rd_w(rb + f[2], f[1]));
    }
}

static void hdr_rd(preset_record_t *rec, const uint8_t *in)
{
    unsigned i;
    uint8_t *rb = (uint8_t *)rec;
    for (i = 0; i < K_HDR_N; i++) {
        const uint8_t *f = k_hdr[i];
        wr_w(rb + f[2], f[1], rd_w(in + f[0], f[1]));
    }
}

#ifndef PRESET_RECORD_NO_STRINGS
const char *preset_record_err_str(int err)
{
    switch (err) {
    case PRESET_OK: return "ok";
    case PRESET_ERR_NULL: return "null";
    case PRESET_ERR_TRUNCATED: return "truncated";
    case PRESET_ERR_MAGIC: return "magic";
    case PRESET_ERR_VERSION: return "version";
    case PRESET_ERR_ENGINE: return "engine";
    case PRESET_ERR_SCHEMA: return "schema";
    case PRESET_ERR_CRC: return "crc";
    case PRESET_ERR_LENGTH: return "length";
    case PRESET_ERR_PAYLOAD: return "payload";
    case PRESET_ERR_SAMPLE_CRC: return "sample-crc";
    case PRESET_ERR_NAME: return "name";
    case PRESET_ERR_EXPORT_FACTORY: return "export-factory";
    case PRESET_ERR_BUFFER: return "buffer";
    case PRESET_ERR_ORIGIN: return "origin";
    case PRESET_ERR_FLAGS: return "flags";
    case PRESET_ERR_MEMORY_FULL: return "memory-full";
    case PRESET_ERR_OLDER: return "older";
    default: return "unknown";
    }
}

const char *preset_record_engine_name(uint16_t engine_id)
{
    switch (engine_id) {
    case PRESET_ENGINE_CHICAGO: return "Chicago";
    case PRESET_ENGINE_MIAMI: return "Miami";
    case PRESET_ENGINE_JUNO: return "Juno";
    case PRESET_ENGINE_DRONE: return "Drone";
    case PRESET_ENGINE_909: return "909";
    case PRESET_ENGINE_808: return "808";
    case PRESET_ENGINE_BREAK: return "BREAK";
    default: return 0;
    }
}

const char *preset_record_origin_name(uint8_t origin_tag)
{
    switch (origin_tag) {
    case PRESET_ORIGIN_UNKNOWN: return "UNKNOWN";
    case PRESET_ORIGIN_FACTORY: return "FACTORY";
    case PRESET_ORIGIN_USER: return "USER";
    case PRESET_ORIGIN_IMPORTED: return "IMP";
    default: return 0;
    }
}
#endif

int preset_import_class(int err)
{
    if (err == PRESET_OK)
        return PRESET_IMPORT_OK;
    if (err == PRESET_ERR_ENGINE)
        return PRESET_IMPORT_WRONG_ENGINE;
    if (err == PRESET_ERR_MEMORY_FULL)
        return PRESET_IMPORT_MEMORY_FULL;
    if (err == PRESET_ERR_VERSION || err == PRESET_ERR_SCHEMA)
        return PRESET_IMPORT_NEWER_DATA;
    return PRESET_IMPORT_BAD_FILE;
}

#ifndef PRESET_RECORD_NO_STRINGS
const char *preset_import_message(int err)
{
    switch (preset_import_class(err)) {
    case PRESET_IMPORT_OK: return 0;
    case PRESET_IMPORT_WRONG_ENGINE: return "WRONG ENGINE";
    case PRESET_IMPORT_BAD_FILE: return "BAD FILE";
    case PRESET_IMPORT_MEMORY_FULL: return "MEMORY FULL";
    case PRESET_IMPORT_NEWER_DATA: return "NEWER DATA";
    default: return "BAD FILE";
    }
}
#endif

int preset_record_expected_nparams(uint16_t engine_id, uint16_t schema)
{
    if (schema != PRESET_SCHEMA_V1)
        return 0;
    return tab_u8(engine_id, k_nparams);
}

int preset_record_expected_nsamples(uint16_t engine_id, uint16_t schema)
{
    int n;
    if (schema != PRESET_SCHEMA_V1)
        return 0;
    n = tab_u8(engine_id, k_nsamples);
    return (n == 0xFF) ? -1 : n;
}

unsigned preset_record_factory_count(uint16_t engine_id)
{
    return (unsigned)tab_u8(engine_id, k_factory);
}

void preset_record_clear(preset_record_t *rec)
{
    if (!rec)
        return;
    zero_n((uint8_t *)rec, (uint32_t)sizeof *rec);
}

int preset_record_set_name(preset_record_t *rec, const char *name)
{
    if (!rec)
        return PRESET_ERR_NULL;
    return set_pad(rec->name, PRESET_NAME_MAX, name);
}

int preset_record_set_source(preset_record_t *rec, const char *source)
{
    if (!rec)
        return PRESET_ERR_NULL;
    return set_pad(rec->source_name, PRESET_SOURCE_MAX, source);
}

int preset_record_equal(const preset_record_t *a, const preset_record_t *b)
{
    unsigned i;
    if (!a || !b)
        return 0;
    /* Field-wise, not a raw struct memcmp: padding and unused tails differ. */
    if (a->engine_id != b->engine_id || a->schema_version != b->schema_version ||
        a->flags != b->flags || a->origin_tag != b->origin_tag ||
        a->original_engine != b->original_engine ||
        a->factory_index != b->factory_index || a->date != b->date ||
        a->plugin_format != b->plugin_format || a->plugin_version != b->plugin_version ||
        a->nparams != b->nparams || a->nsamples != b->nsamples)
        return 0;
    if (!same_n(a->name, b->name, PRESET_NAME_MAX) ||
        !same_n(a->source_name, b->source_name, PRESET_SOURCE_MAX) ||
        !same_n(a->params, b->params, a->nparams))
        return 0;
    for (i = 0; i < a->nsamples; i++) {
        if (!same_n((const uint8_t *)&a->samples[i],
                    (const uint8_t *)&b->samples[i], 16u))
            return 0;
    }
    return 1;
}

int preset_record_factory_derived(const preset_record_t *rec)
{
    if (!rec)
        return 0;
    if (rec->origin_tag == PRESET_ORIGIN_FACTORY)
        return 1;
    if (rec->factory_index != PRESET_FACTORY_NONE)
        return 1;
    return 0;
}

static int check_samples(const preset_record_t *rec)
{
    unsigned i;
    for (i = 0; i < rec->nsamples; i++) {
        const preset_sample_ref_t *s = &rec->samples[i];
        if (s->reserved != 0 || s->source > PRESET_SAMPLE_LAST)
            return PRESET_ERR_PAYLOAD;
        if (s->source == PRESET_SAMPLE_NONE && (s->index != 0 || s->crc != 0))
            return PRESET_ERR_PAYLOAD;
        if (!pad_canonical(s->name, 8u))
            return PRESET_ERR_PAYLOAD;
    }
    return PRESET_OK;
}

static int check_counts(const preset_record_t *rec)
{
    uint16_t id = rec->engine_id;
    int nparams, nsamples;
    /* Direct table walk so the public expected_* wrappers can be dropped
     * by gc-sections when only the codec is linked. */
    if (rec->schema_version != PRESET_SCHEMA_V1 || id > (uint16_t)PRESET_ENGINE_LAST)
        return PRESET_ERR_PAYLOAD;
    nparams = (int)k_nparams[id];
    nsamples = (int)k_nsamples[id];
    if (nparams <= 0 || rec->nparams != (uint8_t)nparams)
        return PRESET_ERR_PAYLOAD;
    if (nsamples == 0xFF) {
        if (rec->nsamples > PRESET_MAX_SAMPLES)
            return PRESET_ERR_PAYLOAD;
    } else if (rec->nsamples != (uint8_t)nsamples) {
        return PRESET_ERR_PAYLOAD;
    }
    return PRESET_OK;
}

static int check_origin(const preset_record_t *rec)
{
    unsigned max_origin;
    uint16_t id = rec->engine_id;
    if (rec->origin_tag > PRESET_ORIGIN_LAST)
        return PRESET_ERR_ORIGIN;
    if (rec->original_engine != 0 && !engine_ok(rec->original_engine))
        return PRESET_ERR_ENGINE;
    max_origin = (id > (uint16_t)PRESET_ENGINE_LAST) ? 0u : (unsigned)k_factory[id];
    if (rec->factory_index != PRESET_FACTORY_NONE) {
        if (max_origin == 0 || rec->factory_index > max_origin)
            return PRESET_ERR_ORIGIN;
    } else if (rec->origin_tag == PRESET_ORIGIN_FACTORY) {
        return PRESET_ERR_ORIGIN;
    }
    return PRESET_OK;
}

/* Encode-side check. Schema not equal to v1 is SCHEMA (not OLDER). */
static int check_record(const preset_record_t *rec)
{
    int rc;
    if (!rec)
        return PRESET_ERR_NULL;
    if (rec->flags != 0)
        return PRESET_ERR_FLAGS;
    if (!engine_ok(rec->engine_id))
        return PRESET_ERR_ENGINE;
    if (rec->schema_version != PRESET_SCHEMA_V1)
        return PRESET_ERR_SCHEMA;
    rc = check_origin(rec);
    if (rc != PRESET_OK)
        return rc;
    rc = check_counts(rec);
    if (rc != PRESET_OK)
        return rc;
    if (!pad_canonical(rec->name, PRESET_NAME_MAX) ||
        !pad_canonical(rec->source_name, PRESET_SOURCE_MAX))
        return PRESET_ERR_NAME;
    return check_samples(rec);
}

int preset_record_encode(uint8_t *out, uint32_t cap, uint32_t *written,
                         const preset_record_t *rec)
{
    uint32_t total, i, off;
    int rc = check_record(rec);
    if (rc != PRESET_OK)
        return rc;
    total = record_size(rec->nparams, rec->nsamples);
    if (out && cap < total)
        return PRESET_ERR_BUFFER;
    if (written)
        *written = total;
    if (!out)
        return PRESET_OK;
    zero_n(out, total);
    wr_u32(out + 0, PRESET_MAGIC);
    wr_u16(out + 4, (uint16_t)PRESET_FORMAT_V1);
    hdr_wr(out, rec);
    wr_u32(out + 12, total);
    copy_n(out + 32, rec->name, PRESET_NAME_MAX);
    copy_n(out + 64, rec->source_name, PRESET_SOURCE_MAX);
    out[96] = rec->nparams;
    out[97] = rec->nsamples;
    off = PRESET_HEADER_BYTES + PRESET_PAYLOAD_HEAD;
    copy_n(out + off, rec->params, rec->nparams);
    off += rec->nparams;
    for (i = 0; i < rec->nsamples; i++) {
        wr_sample(out + off, &rec->samples[i]);
        off += PRESET_SAMPLE_REF_BYTES;
    }
    wr_u32(out + 16, x0x_crc32(out, total));
    return PRESET_OK;
}

int preset_record_export(uint8_t *out, uint32_t cap, uint32_t *written,
                         const preset_record_t *rec)
{
    if (!rec)
        return PRESET_ERR_NULL;
    if (preset_record_factory_derived(rec) && !DNBOX_FACTORY_PRESET_EXPORT)
        return PRESET_ERR_EXPORT_FACTORY;
    return preset_record_encode(out, cap, written, rec);
}

/* Structural load: magic, length, CRC, format version, flags, reserved,
 * length consistency. Copies fields. Does not check engine, schema, names,
 * origin, param counts, or sample refs. On error, *rec is unchanged. */
static int load_into(preset_record_t *out, const uint8_t *buf, uint32_t len)
{
    uint32_t total, i, off;
    uint8_t nparams, nsamples;

    if (!out || !buf)
        return PRESET_ERR_NULL;
    if (len < PRESET_HEADER_BYTES)
        return PRESET_ERR_TRUNCATED;
    if (rd_u32(buf + 0) != PRESET_MAGIC)
        return PRESET_ERR_MAGIC;
    total = rd_u32(buf + 12);
    if (total < PRESET_HEADER_BYTES + PRESET_PAYLOAD_HEAD || total > PRESET_MAX_BYTES)
        return PRESET_ERR_LENGTH;
    if (len < total)
        return PRESET_ERR_TRUNCATED;
    if (crc_zero_field(buf, total, 16u, 4u) != rd_u32(buf + 16))
        return PRESET_ERR_CRC;
    {
        uint16_t format = rd_u16(buf + 4);
        if (format > PRESET_FORMAT_V1)
            return PRESET_ERR_VERSION;
        if (format < PRESET_FORMAT_V1)
            return PRESET_ERR_OLDER;
    }
    nparams = buf[96];
    nsamples = buf[97];
    /* An envelope can have a valid CRC and fit PRESET_MAX_BYTES while
     * declaring more elements than the in-memory arrays can hold. Reject
     * before copy_n / rd_sample, including the structural load-only path. */
    if (nparams > PRESET_MAX_PARAMS || nsamples > PRESET_MAX_SAMPLES)
        return PRESET_ERR_PAYLOAD;
    if (rd_u16(buf + 98) != 0 || rd_u16(buf + 10) != 0)
        return PRESET_ERR_FLAGS;
    if (record_size(nparams, nsamples) != total)
        return PRESET_ERR_LENGTH;

    zero_n((uint8_t *)out, (uint32_t)sizeof *out);
    hdr_rd(out, buf);
    copy_n(out->name, buf + 32, PRESET_NAME_MAX);
    copy_n(out->source_name, buf + 64, PRESET_SOURCE_MAX);
    out->nparams = nparams;
    out->nsamples = nsamples;
    off = PRESET_HEADER_BYTES + PRESET_PAYLOAD_HEAD;
    copy_n(out->params, buf + off, nparams);
    off += nparams;
    for (i = 0; i < nsamples; i++) {
        rd_sample(&out->samples[i], buf + off);
        off += PRESET_SAMPLE_REF_BYTES;
    }
    return PRESET_OK;
}

int preset_record_load(preset_record_t *rec, const uint8_t *buf, uint32_t len)
{
    preset_record_t out;
    int rc;
    rc = load_into(&out, buf, len);
    if (rc != PRESET_OK)
        return rc;
    *rec = out;
    return PRESET_OK;
}

/* Semantic checks for a loaded record. Order matches the original decoder
 * so a multi-fault input still reports the same first error. */
static int check_loaded(const preset_record_t *rec, uint16_t expected_engine)
{
    int rc;
    uint16_t schema = rec->schema_version;
    if (schema > PRESET_SCHEMA_V1)
        return PRESET_ERR_SCHEMA;
    if (schema < PRESET_SCHEMA_V1)
        return PRESET_ERR_OLDER;
    if (!engine_ok(rec->engine_id))
        return PRESET_ERR_ENGINE;
    if (expected_engine != 0 && rec->engine_id != expected_engine)
        return PRESET_ERR_ENGINE;
    rc = check_counts(rec);
    if (rc != PRESET_OK)
        return rc;
    if (!pad_canonical(rec->name, PRESET_NAME_MAX) ||
        !pad_canonical(rec->source_name, PRESET_SOURCE_MAX))
        return PRESET_ERR_NAME;
    rc = check_origin(rec);
    if (rc != PRESET_OK)
        return rc;
    return check_samples(rec);
}

int preset_record_decode(preset_record_t *rec, const uint8_t *buf, uint32_t len,
                         uint16_t expected_engine)
{
    preset_record_t out;
    int rc;
    if (!rec)
        return PRESET_ERR_NULL;
    rc = load_into(&out, buf, len);
    if (rc != PRESET_OK)
        return rc;
    rc = check_loaded(&out, expected_engine);
    if (rc != PRESET_OK)
        return rc;
    *rec = out;
    return PRESET_OK;
}

int preset_record_import(preset_record_t *rec, const uint8_t *buf, uint32_t len,
                         uint16_t target_engine)
{
    preset_record_t tmp;
    int rc;
    uint16_t have;
    if (!target_engine)
        return PRESET_ERR_ENGINE;
    if (!rec)
        return PRESET_ERR_NULL;
    /* Decode into scratch. Assign *rec only when the target matches, so a
     * WRONG ENGINE (or any other) failure never looks like a filled slot. */
    rc = load_into(&tmp, buf, len);
    if (rc != PRESET_OK)
        return rc;
    rc = check_loaded(&tmp, 0);
    if (rc != PRESET_OK)
        return rc;
    have = tmp.original_engine ? tmp.original_engine : tmp.engine_id;
    if (have != target_engine)
        return PRESET_ERR_ENGINE;
    *rec = tmp;
    return PRESET_OK;
}

/* File/transport boundary: unlike the internal decoder, do not accept
 * arbitrary trailing bytes after a complete DNPR record. No slot writes. */
int preset_record_import_file(preset_record_t *rec, const uint8_t *buf,
                              uint32_t len, uint16_t target_engine)
{
    if (!rec || !buf)
        return PRESET_ERR_NULL;
    if (len < PRESET_HEADER_BYTES)
        return PRESET_ERR_TRUNCATED;
    if (len > PRESET_MAX_BYTES || rd_u32(buf + 12) != len)
        return PRESET_ERR_LENGTH;
    return preset_record_import(rec, buf, len, target_engine);
}

int preset_record_verify_sample(const preset_record_t *rec, unsigned index,
                                uint32_t actual_crc)
{
    if (!rec)
        return PRESET_ERR_NULL;
    if (index >= rec->nsamples)
        return PRESET_ERR_PAYLOAD;
    if (rec->samples[index].crc != actual_crc)
        return PRESET_ERR_SAMPLE_CRC;
    return PRESET_OK;
}

int preset_record_verify_samples(const preset_record_t *rec,
                                 const uint32_t *actual_crc, unsigned n)
{
    unsigned i;
    if (!rec || (!actual_crc && n))
        return PRESET_ERR_NULL;
    if (n != rec->nsamples)
        return PRESET_ERR_PAYLOAD;
    for (i = 0; i < n; i++) {
        int rc = preset_record_verify_sample(rec, i, actual_crc[i]);
        if (rc != PRESET_OK)
            return rc;
    }
    return PRESET_OK;
}

int preset_slot_find_empty(const uint8_t *used, unsigned nslots, unsigned *slot)
{
    unsigned i;
    if (!used || !slot)
        return PRESET_ERR_NULL;
    for (i = 0; i < nslots; i++) {
        if (used[i] == 0) {
            *slot = i;
            return PRESET_OK;
        }
    }
    return PRESET_ERR_MEMORY_FULL;
}

int preset_commit_encode(uint8_t *out, uint32_t cap, uint32_t *written,
                         const preset_commit_t *mark)
{
    if (!mark)
        return PRESET_ERR_NULL;
    if (mark->copy > 1u || mark->reserved[0] || mark->reserved[1] || mark->reserved[2])
        return PRESET_ERR_PAYLOAD;
    if (out && cap < PRESET_COMMIT_BYTES)
        return PRESET_ERR_BUFFER;
    if (written)
        *written = PRESET_COMMIT_BYTES;
    if (!out)
        return PRESET_OK;
    zero_n(out, PRESET_COMMIT_BYTES);
    wr_u32(out + 0, PRESET_COMMIT_MAGIC);
    wr_u32(out + 4, mark->seq);
    wr_u32(out + 8, mark->slot);
    wr_u32(out + 12, mark->record_crc);
    out[16] = mark->copy;
    wr_u32(out + 28, x0x_crc32(out, 28u));
    return PRESET_OK;
}

int preset_commit_decode(preset_commit_t *mark, const uint8_t *buf, uint32_t len)
{
    uint32_t i;
    if (!mark || !buf)
        return PRESET_ERR_NULL;
    if (len < PRESET_COMMIT_BYTES)
        return PRESET_ERR_TRUNCATED;
    if (rd_u32(buf + 0) != PRESET_COMMIT_MAGIC)
        return PRESET_ERR_MAGIC;
    if (x0x_crc32(buf, 28u) != rd_u32(buf + 28))
        return PRESET_ERR_CRC;
    for (i = 17; i < 28u; i++) {
        if (buf[i] != 0)
            return PRESET_ERR_FLAGS;
    }
    if (buf[16] > 1u)
        return PRESET_ERR_PAYLOAD;
    mark->seq = rd_u32(buf + 4);
    mark->slot = rd_u32(buf + 8);
    mark->record_crc = rd_u32(buf + 12);
    mark->copy = buf[16];
    mark->reserved[0] = mark->reserved[1] = mark->reserved[2] = 0;
    return PRESET_OK;
}

static int decode_ab(preset_commit_t *ca, int *va,
                     preset_commit_t *cb, int *vb,
                     const uint8_t *a, uint32_t a_len,
                     const uint8_t *b, uint32_t b_len)
{
    *va = (a && a_len) ? (preset_commit_decode(ca, a, a_len) == PRESET_OK) : 0;
    *vb = (b && b_len) ? (preset_commit_decode(cb, b, b_len) == PRESET_OK) : 0;
    if (*va && *vb)
        return (cb->seq > ca->seq) ? 1 : 0;
    if (*va)
        return 0;
    if (*vb)
        return 1;
    return PRESET_ERR_CRC;
}

int preset_commit_winner(const uint8_t *a, uint32_t a_len,
                         const uint8_t *b, uint32_t b_len)
{
    preset_commit_t ca, cb;
    int va, vb;
    return decode_ab(&ca, &va, &cb, &vb, a, a_len, b, b_len);
}

int preset_commit_previous(const uint8_t *a, uint32_t a_len,
                           const uint8_t *b, uint32_t b_len)
{
    preset_commit_t ca, cb;
    int va, vb, win;
    win = decode_ab(&ca, &va, &cb, &vb, a, a_len, b, b_len);
    if (win < 0)
        return win;
    if (va && vb)
        return win ? 0 : 1;
    return PRESET_ERR_CRC;
}
