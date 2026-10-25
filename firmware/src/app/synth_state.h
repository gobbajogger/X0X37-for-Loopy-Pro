/* SPDX-License-Identifier: GPL-3.0-only */
/* Experimental engine envelope. No project_t layout or existing X0XP v1 bytes change.
 * The legacy payload is opaque: validate it with the existing project reader before
 * activation. Views borrow input bytes; the caller owns their lifetime. Unknown
 * selected engine IDs, retained-bank IDs and setting schemas survive unchanged. This codec grants
 * no permission to activate them. All legacy T_303 lanes retain 303 identity.
 * Encode source and destination must not overlap. No heap or global storage. */
#pragma once
#include <stddef.h>
#include <stdint.h>

#define SYNTH_STATE_PARTS 2u
#define SYNTH_STATE_MAX_SETTINGS 128u
#define SYNTH_STATE_303_ID 1u
#define SYNTH_STATE_303_SCHEMA 1u
#define SYNTH_STATE_HEADER 32u
#define SYNTH_STATE_RECORD 12u
#define SYNTH_STATE_MAGIC 0x4553584cu /* LXSE, LE */
#define SYNTH_STATE_VERSION 1u

typedef struct {
    uint32_t selected_engine_id; /* independent of the retained settings bank */
    uint32_t engine_id;
    uint16_t schema, size;
    const uint8_t *bytes;
} synth_state_part_t;
typedef struct {
    const uint8_t *legacy;
    uint32_t legacy_size;
    uint32_t legacy_lock_engine;
    synth_state_part_t part[SYNTH_STATE_PARTS];
} synth_state_view_t;
enum { SYNTH_STATE_OK = 0, SYNTH_STATE_INVALID = -1,
       SYNTH_STATE_VERSION_ERROR = -2, SYNTH_STATE_SPACE = -3,
       SYNTH_STATE_CRC_ERROR = -4 };

static inline uint32_t synth_state_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static inline uint16_t synth_state_u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | (uint16_t)p[1] << 8);
}
static inline void synth_state_put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static inline void synth_state_put16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}
static inline uint32_t synth_state_crc(const uint8_t *p, size_t n)
{
    uint32_t c = 0xffffffffu;
    while (n--) {
        unsigned b;
        c ^= *p++;
        for (b = 0; b < 8; b++) c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1u)));
    }
    return ~c;
}

/* Explicit legacy migration; never guess raw ABI by size alone. expected_size must
 * come from the pinned legacy format's ABI, after its normal header/CRC validation.
 * Empty 303 blobs mean use exactly the legacy sound.v[T_303] settings, not defaults. */
static inline int synth_state_migrate_legacy(const void *raw, size_t n, uint32_t expected_size,
                                            synth_state_view_t *out)
{
    synth_state_view_t v;
    unsigned i;
    if (!raw || !out || !expected_size || n != expected_size) return SYNTH_STATE_INVALID;
    __builtin_memset(&v, 0, sizeof v);
    v.legacy = (const uint8_t *)raw;
    v.legacy_size = expected_size;
    v.legacy_lock_engine = SYNTH_STATE_303_ID;
    for (i = 0; i < SYNTH_STATE_PARTS; i++) {
        v.part[i].selected_engine_id = SYNTH_STATE_303_ID;
        v.part[i].engine_id = SYNTH_STATE_303_ID;
        v.part[i].schema = SYNTH_STATE_303_SCHEMA;
    }
    *out = v;
    return SYNTH_STATE_OK;
}

/* Header: magic/version/total/legacy-size/legacy-lock-id/parts/reserved/CRC.
 * CRC covers bytes 0..27 followed by payload; CRC field itself is excluded.
 * Version 1 requires 303 legacy lock identity; alternate locks need a new explicit
 * schema rather than reusing old (target, voice, parameter-index) tuples. */
static inline uint32_t synth_state_envelope_crc(const uint8_t *p, size_t n)
{
    uint32_t c = 0xffffffffu;
    size_t k;
    for (k = 0; k < n; k++) {
        unsigned b;
        if (k >= 28u && k < SYNTH_STATE_HEADER) continue;
        c ^= p[k];
        for (b = 0; b < 8; b++) c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1u)));
    }
    return ~c;
}
static inline int synth_state_encode(void *dst, size_t capacity, const synth_state_view_t *v,
                                    size_t *written)
{
    uint8_t *p = (uint8_t *)dst;
    size_t n, pos;
    unsigned i;
    if (!p || !v || !written || !v->legacy || !v->legacy_size ||
        v->legacy_lock_engine != SYNTH_STATE_303_ID) return SYNTH_STATE_INVALID;
    n = SYNTH_STATE_HEADER + (size_t)v->legacy_size;
    if (n < v->legacy_size) return SYNTH_STATE_INVALID;
    for (i = 0; i < SYNTH_STATE_PARTS; i++) {
        const synth_state_part_t *s = &v->part[i];
        if (!s->selected_engine_id || !s->engine_id || !s->schema || s->size > SYNTH_STATE_MAX_SETTINGS ||
            (s->size && !s->bytes)) return SYNTH_STATE_INVALID;
        if (n > UINT32_MAX - SYNTH_STATE_RECORD - s->size) return SYNTH_STATE_INVALID;
        n += SYNTH_STATE_RECORD + s->size;
    }
    if (capacity < n) return SYNTH_STATE_SPACE;
    __builtin_memset(p, 0, SYNTH_STATE_HEADER);
    synth_state_put32(p, SYNTH_STATE_MAGIC);
    synth_state_put32(p + 4, SYNTH_STATE_VERSION);
    synth_state_put32(p + 8, (uint32_t)n);
    synth_state_put32(p + 12, v->legacy_size);
    synth_state_put32(p + 16, v->legacy_lock_engine);
    synth_state_put32(p + 20, SYNTH_STATE_PARTS);
    __builtin_memcpy(p + SYNTH_STATE_HEADER, v->legacy, v->legacy_size);
    pos = SYNTH_STATE_HEADER + v->legacy_size;
    for (i = 0; i < SYNTH_STATE_PARTS; i++) {
        const synth_state_part_t *s = &v->part[i];
        synth_state_put32(p + pos, s->selected_engine_id);
        synth_state_put32(p + pos + 4, s->engine_id);
        synth_state_put16(p + pos + 8, s->schema);
        synth_state_put16(p + pos + 10, s->size);
        pos += SYNTH_STATE_RECORD;
        if (s->size) __builtin_memcpy(p + pos, s->bytes, s->size);
        pos += s->size;
    }
    synth_state_put32(p + 28, synth_state_envelope_crc(p, n));
    *written = n;
    return SYNTH_STATE_OK;
}
static inline int synth_state_decode(const void *src, size_t n, uint32_t expected_legacy_size,
                                    synth_state_view_t *out)
{
    const uint8_t *p = (const uint8_t *)src;
    synth_state_view_t v;
    size_t pos;
    unsigned i;
    if (!p || !out || n < SYNTH_STATE_HEADER || n > UINT32_MAX ||
        synth_state_u32(p) != SYNTH_STATE_MAGIC) return SYNTH_STATE_INVALID;
    if (synth_state_u32(p + 4) != SYNTH_STATE_VERSION) return SYNTH_STATE_VERSION_ERROR;
    if (!expected_legacy_size || synth_state_u32(p + 8) != n ||
        synth_state_u32(p + 12) != expected_legacy_size ||
        synth_state_u32(p + 16) != SYNTH_STATE_303_ID ||
        synth_state_u32(p + 20) != SYNTH_STATE_PARTS || synth_state_u32(p + 24) ||
        expected_legacy_size > n - SYNTH_STATE_HEADER) return SYNTH_STATE_INVALID;
    if (synth_state_u32(p + 28) != synth_state_envelope_crc(p, n)) return SYNTH_STATE_CRC_ERROR;
    __builtin_memset(&v, 0, sizeof v);
    v.legacy = p + SYNTH_STATE_HEADER;
    v.legacy_size = expected_legacy_size;
    v.legacy_lock_engine = SYNTH_STATE_303_ID;
    pos = SYNTH_STATE_HEADER + (size_t)expected_legacy_size;
    for (i = 0; i < SYNTH_STATE_PARTS; i++) {
        synth_state_part_t *s = &v.part[i];
        if (n - pos < SYNTH_STATE_RECORD) return SYNTH_STATE_INVALID;
        s->selected_engine_id = synth_state_u32(p + pos);
        s->engine_id = synth_state_u32(p + pos + 4);
        s->schema = synth_state_u16(p + pos + 8);
        s->size = synth_state_u16(p + pos + 10);
        pos += SYNTH_STATE_RECORD;
        if (!s->selected_engine_id || !s->engine_id || !s->schema || s->size > SYNTH_STATE_MAX_SETTINGS ||
            s->size > n - pos) return SYNTH_STATE_INVALID;
        s->bytes = p + pos;
        pos += s->size;
    }
    if (pos != n) return SYNTH_STATE_INVALID;
    *out = v; /* atomic validation: output unchanged on all failures */
    return SYNTH_STATE_OK;
}
