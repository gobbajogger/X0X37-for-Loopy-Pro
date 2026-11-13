/* SPDX-License-Identifier: GPL-3.0-only */
/* User-preset record codec. Explicit little-endian wire, zlib CRC-32.
 * No flash I/O, no audio-path use, no factory table bytes, no static buffers. */
#include "preset_record.h"
#include "../crc32_nibble.h"

_Static_assert(PRESET_HEADER_BYTES == 96u, "header size");
_Static_assert(PRESET_SAMPLE_REF_BYTES == 16u, "sample-ref size");
_Static_assert(PRESET_COMMIT_BYTES == 32u, "commit-marker size");
_Static_assert(PRESET_909_NPARAMS_V1 <= PRESET_MAX_PARAMS, "909 params fit");
_Static_assert(PRESET_808_NPARAMS_V1 <= PRESET_MAX_PARAMS, "808 params fit");
_Static_assert(PRESET_BREAK_NPARAMS_V1 <= PRESET_MAX_PARAMS, "BREAK params fit");
_Static_assert(sizeof(preset_sample_ref_t) == 16u, "sample-ref HOST sizeof");

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

static int pad_canonical(const uint8_t *s, unsigned n)
{
    unsigned i, z = 0;
    for (i = 0; i < n; i++) {
        if (z) {
            if (s[i] != 0)
                return 0;
        } else if (s[i] == 0) {
            z = 1;
        }
    }
    return z ? 1 : 0;
}

static int set_pad(uint8_t *dst, unsigned n, const char *src)
{
    unsigned i;
    for (i = 0; i < n; i++)
        dst[i] = 0;
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
    uint32_t i, c;
    for (i = 0; i < 4u; i++)
        z[i] = 0;
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

int preset_record_expected_nparams(uint16_t engine_id, uint16_t schema)
{
    if (schema != PRESET_SCHEMA_V1)
        return 0;
    switch (engine_id) {
    case PRESET_ENGINE_CHICAGO: return (int)PRESET_CHICAGO_NPARAMS_V1;
    case PRESET_ENGINE_MIAMI: return (int)PRESET_MIAMI_NPARAMS_V1;
    case PRESET_ENGINE_JUNO: return (int)PRESET_JUNO_NPARAMS_V1;
    case PRESET_ENGINE_DRONE: return (int)PRESET_DRONE_NPARAMS_V1;
    case PRESET_ENGINE_909: return (int)PRESET_909_NPARAMS_V1;
    case PRESET_ENGINE_808: return (int)PRESET_808_NPARAMS_V1;
    case PRESET_ENGINE_BREAK: return (int)PRESET_BREAK_NPARAMS_V1;
    default: return 0;
    }
}

int preset_record_expected_nsamples(uint16_t engine_id, uint16_t schema)
{
    if (schema != PRESET_SCHEMA_V1)
        return 0;
    switch (engine_id) {
    case PRESET_ENGINE_CHICAGO:
    case PRESET_ENGINE_MIAMI:
    case PRESET_ENGINE_JUNO:
    case PRESET_ENGINE_DRONE:
        return 0;
    case PRESET_ENGINE_909: return (int)PRESET_909_NSAMPLES_V1;
    case PRESET_ENGINE_808: return -1;
    case PRESET_ENGINE_BREAK: return (int)PRESET_BREAK_NSAMPLES_V1;
    default: return 0;
    }
}

unsigned preset_record_factory_count(uint16_t engine_id)
{
    switch (engine_id) {
    case PRESET_ENGINE_CHICAGO: return PRESET_FACTORY_CHICAGO_N;
    case PRESET_ENGINE_MIAMI: return PRESET_FACTORY_MIAMI_N;
    case PRESET_ENGINE_JUNO: return PRESET_FACTORY_JUNO_N;
    case PRESET_ENGINE_DRONE: return PRESET_FACTORY_DRONE_N;
    default: return 0;
    }
}

void preset_record_clear(preset_record_t *rec)
{
    uint8_t *p;
    uint32_t i;
    if (!rec)
        return;
    p = (uint8_t *)rec;
    for (i = 0; i < (uint32_t)sizeof *rec; i++)
        p[i] = 0;
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
    unsigned i, k;
    if (!a || !b)
        return 0;
    if (a->engine_id != b->engine_id || a->schema_version != b->schema_version ||
        a->flags != b->flags || a->origin_tag != b->origin_tag ||
        a->original_engine != b->original_engine ||
        a->factory_index != b->factory_index || a->date != b->date ||
        a->plugin_format != b->plugin_format || a->plugin_version != b->plugin_version ||
        a->nparams != b->nparams || a->nsamples != b->nsamples)
        return 0;
    for (i = 0; i < PRESET_NAME_MAX; i++)
        if (a->name[i] != b->name[i])
            return 0;
    for (i = 0; i < PRESET_SOURCE_MAX; i++)
        if (a->source_name[i] != b->source_name[i])
            return 0;
    for (i = 0; i < a->nparams; i++)
        if (a->params[i] != b->params[i])
            return 0;
    for (i = 0; i < a->nsamples; i++) {
        const preset_sample_ref_t *sa = &a->samples[i], *sb = &b->samples[i];
        if (sa->source != sb->source || sa->reserved != sb->reserved ||
            sa->index != sb->index || sa->crc != sb->crc)
            return 0;
        for (k = 0; k < 8u; k++)
            if (sa->name[k] != sb->name[k])
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

static int sample_name_ok(const uint8_t name[8])
{
    unsigned i, allz = 1, z = 0;
    for (i = 0; i < 8u; i++)
        if (name[i] != 0)
            allz = 0;
    if (allz)
        return 1;
    for (i = 0; i < 8u; i++) {
        if (z) {
            if (name[i] != 0)
                return 0;
        } else if (name[i] == 0) {
            z = 1;
        }
    }
    return z ? 1 : 0;
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
        if (!sample_name_ok(s->name))
            return PRESET_ERR_PAYLOAD;
    }
    return PRESET_OK;
}

static int check_record(const preset_record_t *rec)
{
    int nparams, nsamples;
    unsigned max_origin;
    if (!rec)
        return PRESET_ERR_NULL;
    if (rec->flags != 0)
        return PRESET_ERR_FLAGS;
    if (!preset_record_engine_name(rec->engine_id))
        return PRESET_ERR_ENGINE;
    if (rec->schema_version != PRESET_SCHEMA_V1)
        return PRESET_ERR_SCHEMA;
    if (rec->origin_tag > PRESET_ORIGIN_LAST)
        return PRESET_ERR_ORIGIN;
    if (rec->original_engine != 0 && !preset_record_engine_name(rec->original_engine))
        return PRESET_ERR_ENGINE;
    nparams = preset_record_expected_nparams(rec->engine_id, rec->schema_version);
    nsamples = preset_record_expected_nsamples(rec->engine_id, rec->schema_version);
    if (nparams <= 0 || rec->nparams != (uint8_t)nparams)
        return PRESET_ERR_PAYLOAD;
    if (nsamples < 0) {
        if (rec->nsamples > PRESET_MAX_SAMPLES)
            return PRESET_ERR_PAYLOAD;
    } else if (rec->nsamples != (uint8_t)nsamples) {
        return PRESET_ERR_PAYLOAD;
    }
    if (!pad_canonical(rec->name, PRESET_NAME_MAX) ||
        !pad_canonical(rec->source_name, PRESET_SOURCE_MAX))
        return PRESET_ERR_NAME;
    max_origin = preset_record_factory_count(rec->engine_id);
    if (rec->factory_index != PRESET_FACTORY_NONE) {
        if (max_origin == 0 || rec->factory_index > max_origin)
            return PRESET_ERR_ORIGIN;
    } else if (rec->origin_tag == PRESET_ORIGIN_FACTORY) {
        return PRESET_ERR_ORIGIN;
    }
    return check_samples(rec);
}

int preset_record_encode(uint8_t *out, uint32_t cap, uint32_t *written,
                         const preset_record_t *rec)
{
    uint32_t total, i, k, off, crc;
    int rc = check_record(rec);
    if (rc != PRESET_OK)
        return rc;
    total = record_size(rec->nparams, rec->nsamples);
    if (written)
        *written = total;
    if (!out)
        return PRESET_OK;
    if (cap < total)
        return PRESET_ERR_BUFFER;
    for (i = 0; i < total; i++)
        out[i] = 0;
    wr_u32(out + 0, PRESET_MAGIC);
    wr_u16(out + 4, (uint16_t)PRESET_FORMAT_V1);
    wr_u16(out + 6, rec->engine_id);
    wr_u16(out + 8, rec->schema_version);
    wr_u16(out + 10, rec->flags);
    wr_u32(out + 12, total);
    wr_u32(out + 16, 0);
    out[20] = rec->origin_tag;
    out[21] = rec->original_engine;
    wr_u16(out + 22, rec->factory_index);
    wr_u32(out + 24, rec->date);
    wr_u16(out + 28, rec->plugin_format);
    wr_u16(out + 30, rec->plugin_version);
    for (i = 0; i < PRESET_NAME_MAX; i++)
        out[32 + i] = rec->name[i];
    for (i = 0; i < PRESET_SOURCE_MAX; i++)
        out[64 + i] = rec->source_name[i];
    out[96] = rec->nparams;
    out[97] = rec->nsamples;
    wr_u16(out + 98, 0);
    off = PRESET_HEADER_BYTES + PRESET_PAYLOAD_HEAD;
    for (i = 0; i < rec->nparams; i++)
        out[off + i] = rec->params[i];
    off += rec->nparams;
    for (i = 0; i < rec->nsamples; i++) {
        const preset_sample_ref_t *s = &rec->samples[i];
        out[off + 0] = s->source;
        out[off + 1] = s->reserved;
        wr_u16(out + off + 2, s->index);
        wr_u32(out + off + 4, s->crc);
        for (k = 0; k < 8u; k++)
            out[off + 8 + k] = s->name[k];
        off += PRESET_SAMPLE_REF_BYTES;
    }
    crc = x0x_crc32(out, total);
    wr_u32(out + 16, crc);
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

int preset_record_decode(preset_record_t *rec, const uint8_t *buf, uint32_t len,
                         uint16_t expected_engine)
{
    uint32_t total, i, k, off;
    uint16_t format, engine, schema, flags, findex, pfmt, pver;
    uint8_t nparams, nsamples, origin, orig_eng;
    int rsv;
    int rc, expect_np, expect_ns;
    preset_record_t out;

    if (!rec || !buf)
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

    format = rd_u16(buf + 4);
    if (format > PRESET_FORMAT_V1)
        return PRESET_ERR_VERSION;
    if (format < PRESET_FORMAT_V1)
        return PRESET_ERR_OLDER;
    engine = rd_u16(buf + 6);
    schema = rd_u16(buf + 8);
    flags = rd_u16(buf + 10);
    origin = buf[20];
    orig_eng = buf[21];
    findex = rd_u16(buf + 22);
    pfmt = rd_u16(buf + 28);
    pver = rd_u16(buf + 30);
    rsv = 0;
    if (len >= PRESET_HEADER_BYTES + PRESET_PAYLOAD_HEAD) {
        nparams = buf[96];
        nsamples = buf[97];
        if (rd_u16(buf + 98) != 0)
            rsv = 1;
    } else {
        return PRESET_ERR_TRUNCATED;
    }
    if (rsv || flags != 0)
        return PRESET_ERR_FLAGS;
    if (record_size(nparams, nsamples) != total)
        return PRESET_ERR_LENGTH;
    if (schema > PRESET_SCHEMA_V1)
        return PRESET_ERR_SCHEMA;
    if (schema < PRESET_SCHEMA_V1)
        return PRESET_ERR_OLDER;
    if (!preset_record_engine_name(engine))
        return PRESET_ERR_ENGINE;
    if (expected_engine != 0 && engine != expected_engine)
        return PRESET_ERR_ENGINE;
    expect_np = preset_record_expected_nparams(engine, schema);
    expect_ns = preset_record_expected_nsamples(engine, schema);
    if (expect_np <= 0 || nparams != (uint8_t)expect_np)
        return PRESET_ERR_PAYLOAD;
    if (expect_ns < 0) {
        if (nsamples > PRESET_MAX_SAMPLES)
            return PRESET_ERR_PAYLOAD;
    } else if (nsamples != (uint8_t)expect_ns) {
        return PRESET_ERR_PAYLOAD;
    }
    if (!pad_canonical(buf + 32, PRESET_NAME_MAX) ||
        !pad_canonical(buf + 64, PRESET_SOURCE_MAX))
        return PRESET_ERR_NAME;
    if (origin > PRESET_ORIGIN_LAST)
        return PRESET_ERR_ORIGIN;
    if (orig_eng != 0 && !preset_record_engine_name(orig_eng))
        return PRESET_ERR_ENGINE;
    if (findex != PRESET_FACTORY_NONE) {
        unsigned max_o = preset_record_factory_count(engine);
        if (max_o == 0 || findex > max_o)
            return PRESET_ERR_ORIGIN;
    } else if (origin == PRESET_ORIGIN_FACTORY) {
        return PRESET_ERR_ORIGIN;
    }

    preset_record_clear(&out);
    out.engine_id = engine;
    out.schema_version = schema;
    out.flags = flags;
    out.origin_tag = origin;
    out.original_engine = orig_eng;
    out.factory_index = findex;
    out.date = rd_u32(buf + 24);
    out.plugin_format = pfmt;
    out.plugin_version = pver;
    for (i = 0; i < PRESET_NAME_MAX; i++)
        out.name[i] = buf[32 + i];
    for (i = 0; i < PRESET_SOURCE_MAX; i++)
        out.source_name[i] = buf[64 + i];
    out.nparams = nparams;
    out.nsamples = nsamples;
    off = PRESET_HEADER_BYTES + PRESET_PAYLOAD_HEAD;
    for (i = 0; i < nparams; i++)
        out.params[i] = buf[off + i];
    off += nparams;
    for (i = 0; i < nsamples; i++) {
        preset_sample_ref_t *s = &out.samples[i];
        s->source = buf[off + 0];
        s->reserved = buf[off + 1];
        s->index = rd_u16(buf + off + 2);
        s->crc = rd_u32(buf + off + 4);
        for (k = 0; k < 8u; k++)
            s->name[k] = buf[off + 8 + k];
        off += PRESET_SAMPLE_REF_BYTES;
    }
    rc = check_samples(&out);
    if (rc != PRESET_OK)
        return rc;
    *rec = out;
    return PRESET_OK;
}

int preset_record_import(preset_record_t *rec, const uint8_t *buf, uint32_t len,
                         uint16_t target_engine)
{
    int rc;
    uint16_t have;
    if (!target_engine)
        return PRESET_ERR_ENGINE;
    rc = preset_record_decode(rec, buf, len, 0);
    if (rc != PRESET_OK)
        return rc;
    have = rec->original_engine ? rec->original_engine : rec->engine_id;
    if (have != target_engine)
        return PRESET_ERR_ENGINE;
    return PRESET_OK;
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
    uint32_t i;
    if (!mark)
        return PRESET_ERR_NULL;
    if (mark->copy > 1u || mark->reserved[0] || mark->reserved[1] || mark->reserved[2])
        return PRESET_ERR_PAYLOAD;
    if (written)
        *written = PRESET_COMMIT_BYTES;
    if (!out)
        return PRESET_OK;
    if (cap < PRESET_COMMIT_BYTES)
        return PRESET_ERR_BUFFER;
    for (i = 0; i < PRESET_COMMIT_BYTES; i++)
        out[i] = 0;
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
    for (i = 17; i < 28u; i++)
        if (buf[i] != 0)
            return PRESET_ERR_FLAGS;
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
