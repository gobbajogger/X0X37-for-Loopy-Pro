/* SPDX-License-Identifier: GPL-3.0-only */
/* Versioned user-preset record (on-flash and on-file).
 * This module does not write flash, does not run in an audio callback,
 * and does not touch the UI. Records live in the storage/data region.
 * Code flash and pool must take only this small codec. There is no static
 * record buffer here. The working record is a stack object or a caller
 * buffer (reuse st_buf when the firmware is wired).
 * Miami is not a 303 and must never be labelled 303 or 303B.
 * Factory table bytes are not stored in this record. */
#pragma once
#include <stdint.h>
#include "original_sound.h"

#ifndef DNBOX_PRESET_MEASURE_STAGE
/* Device measurement only: 0 (default) = the codec is not in the image.
 * 1..4 select a linkable subset (load; +validation; +encode/commit; +export).
 * Stage 0 must leave the device image byte-identical to the base. */
#define DNBOX_PRESET_MEASURE_STAGE 0
#endif

#ifndef DNBOX_FACTORY_PRESET_EXPORT
/* Temporary engineering default: OFF. This is not a rights finding.
 * It is the single build-time switch that gates factory (and factory-derived)
 * export. User and imported non-derived export always work. */
#define DNBOX_FACTORY_PRESET_EXPORT 0
#endif

#define PRESET_MAGIC          0x52504E44u /* "DNPR", little endian */
#define PRESET_FORMAT_V1      1u
#define PRESET_SCHEMA_V1      1u

#define PRESET_ENGINE_NONE    0u
#define PRESET_ENGINE_CHICAGO 1u
#define PRESET_ENGINE_MIAMI   2u
#define PRESET_ENGINE_JUNO    3u
#define PRESET_ENGINE_DRONE   4u
#define PRESET_ENGINE_909     5u
#define PRESET_ENGINE_808     6u
#define PRESET_ENGINE_BREAK   7u
#define PRESET_ENGINE_LAST    PRESET_ENGINE_BREAK

/* Factory origin bounds match the selected source bank. Juno's legacy
 * indexes 0-3 stay fixed; append-only additions use the next one-based IDs
 * on wire. Including the declaration header does not link factory bytes. */
#define PRESET_FACTORY_CHICAGO_N 81u
#define PRESET_FACTORY_MIAMI_N   30u
#define PRESET_FACTORY_JUNO_N    ORIGINAL_JUNO_FACTORIES
#define PRESET_FACTORY_DRONE_N   4u

#define PRESET_NAME_MAX            32u
#define PRESET_SOURCE_MAX          32u
#define PRESET_HEADER_BYTES        96u
#define PRESET_SAMPLE_REF_BYTES    16u
#define PRESET_MAX_PARAMS          192u
#define PRESET_MAX_SAMPLES         16u
#define PRESET_PAYLOAD_HEAD        4u
#define PRESET_MAX_BYTES           (PRESET_HEADER_BYTES + PRESET_PAYLOAD_HEAD + \
                                    PRESET_MAX_PARAMS + \
                                    PRESET_SAMPLE_REF_BYTES * PRESET_MAX_SAMPLES)

#define PRESET_CHICAGO_NPARAMS_V1  22u
#define PRESET_MIAMI_NPARAMS_V1    18u
#define PRESET_JUNO_NPARAMS_V1     18u
#define PRESET_DRONE_NPARAMS_V1    8u
#define PRESET_909_NVOICES_V1      12u
#define PRESET_909_VOICE_PARAMS_V1 11u
#define PRESET_909_NPARAMS_V1      (PRESET_909_NVOICES_V1 * PRESET_909_VOICE_PARAMS_V1)
#define PRESET_909_NSAMPLES_V1     4u
#define PRESET_808_NVOICES_V1      12u
#define PRESET_808_VOICE_PARAMS_V1 12u
#define PRESET_808_NPARAMS_V1      (PRESET_808_NVOICES_V1 * PRESET_808_VOICE_PARAMS_V1)
#define PRESET_BREAK_NPARAMS_V1    16u
#define PRESET_BREAK_NSAMPLES_V1   2u

#define PRESET_SAMPLE_NONE  0u
#define PRESET_SAMPLE_ROM   1u
#define PRESET_SAMPLE_PACK  2u
#define PRESET_SAMPLE_USER  3u
#define PRESET_SAMPLE_LAST  PRESET_SAMPLE_USER

#define PRESET_ORIGIN_UNKNOWN  0u
#define PRESET_ORIGIN_FACTORY  1u
#define PRESET_ORIGIN_USER     2u
#define PRESET_ORIGIN_IMPORTED 3u
#define PRESET_ORIGIN_LAST     PRESET_ORIGIN_IMPORTED
#define PRESET_FACTORY_NONE    0u

/* Imported-plugin provenance. 0 = none / unknown. PRESET_PLUGIN_FORMAT is a
 * placeholder until plugin-identity research fills the real id. KORG Gadget is
 * a lead only. This module does not parse a plugin file. */
#define PRESET_PLUGIN_NONE     0u
#define PRESET_PLUGIN_FORMAT   1u

/* Per-param mapping labels (UX §3A). Not stored in format v1: a status array
 * and its encode/decode loops would add code flash, which the 8440 B free
 * budget cannot spare. Future work (a later schema). */
#define PRESET_MAP_EXACT        0u
#define PRESET_MAP_APPROXIMATE  1u
#define PRESET_MAP_UNSUPPORTED  2u

#define PRESET_COMMIT_MAGIC 0x4D434E44u /* "DNCM", little endian */
#define PRESET_COMMIT_BYTES 32u

/* Codec errors. Import maps a subset onto the four visible classes below. */
enum {
    PRESET_OK = 0,
    PRESET_ERR_NULL = -1,
    PRESET_ERR_TRUNCATED = -2,
    PRESET_ERR_MAGIC = -3,
    PRESET_ERR_VERSION = -4,          /* newer than known: NEWER DATA */
    PRESET_ERR_ENGINE = -5,           /* WRONG ENGINE */
    PRESET_ERR_SCHEMA = -6,           /* newer schema: NEWER DATA */
    PRESET_ERR_CRC = -7,
    PRESET_ERR_LENGTH = -8,
    PRESET_ERR_PAYLOAD = -9,
    PRESET_ERR_SAMPLE_CRC = -10,
    PRESET_ERR_NAME = -11,
    PRESET_ERR_EXPORT_FACTORY = -12,  /* factory export refused (switch OFF) */
    PRESET_ERR_BUFFER = -13,
    PRESET_ERR_ORIGIN = -14,
    PRESET_ERR_FLAGS = -15,
    PRESET_ERR_MEMORY_FULL = -16,     /* MEMORY FULL: no empty slot, no eviction */
    PRESET_ERR_OLDER = -17            /* older unknown format: BAD FILE */
};

/* Visible import classes (UX spec §3.5). */
enum {
    PRESET_IMPORT_OK = 0,
    PRESET_IMPORT_WRONG_ENGINE = 1,
    PRESET_IMPORT_BAD_FILE = 2,
    PRESET_IMPORT_MEMORY_FULL = 3,
    PRESET_IMPORT_NEWER_DATA = 4
};

/* Wire sample reference: 16 explicit little-endian bytes. Do not memcpy. */
typedef struct {
    uint8_t source;
    uint8_t reserved;
    uint16_t index;
    uint32_t crc;
    uint8_t name[8];
} preset_sample_ref_t;

/* In-memory working record. Not a wire image. One instance belongs on the
 * stack or in a caller scratch buffer, never as a .bss / pool array of slots. */
typedef struct {
    uint16_t engine_id;
    uint16_t schema_version;
    uint16_t flags;            /* v1: must be 0 */
    uint8_t origin_tag;        /* PRESET_ORIGIN_* */
    uint8_t original_engine;   /* 0 = unknown (use engine_id); else PRESET_ENGINE_* */
    uint16_t factory_index;    /* 0 = none; 1-based factory origin */
    uint32_t date;             /* 0 = unknown; else YYYYMMDD in America/Chicago */
    uint16_t plugin_format;    /* 0 = none; PRESET_PLUGIN_FORMAT = placeholder */
    uint16_t plugin_version;   /* 0 = unknown */
    uint8_t name[PRESET_NAME_MAX];
    uint8_t source_name[PRESET_SOURCE_MAX];
    uint8_t nparams;
    uint8_t nsamples;
    uint8_t params[PRESET_MAX_PARAMS];
    preset_sample_ref_t samples[PRESET_MAX_SAMPLES];
} preset_record_t;

/* Power-loss commit marker. No flash I/O is in this module.
 * Write the new record to the inactive pad, verify its CRC, then write this
 * marker. The valid marker with the highest seq is live. The other pad stays
 * readable so a confirmed overwrite can be undone (restore the previous pad
 * and write a new marker). Firmware must run this from the main loop when
 * stopped and quiet, never from an audio callback. */
typedef struct {
    uint32_t seq;
    uint32_t slot;
    uint32_t record_crc;
    uint8_t copy;              /* 0 = pad A, 1 = pad B */
    uint8_t reserved[3];
} preset_commit_t;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PRESET_RECORD_NO_STRINGS
/* Host-only debug strings. Omitted from a device build that sets
 * PRESET_RECORD_NO_STRINGS. The device uses PRESET_ERR_* and
 * PRESET_IMPORT_* codes. UI words are owned by a later UX step. */
const char *preset_record_err_str(int err);
const char *preset_record_engine_name(uint16_t engine_id);
const char *preset_record_origin_name(uint8_t origin_tag);
const char *preset_import_message(int err);
#endif

/* Import class (UX spec §3.5). This module does not show a toast. */
int preset_import_class(int err);

int preset_record_expected_nparams(uint16_t engine_id, uint16_t schema);
int preset_record_expected_nsamples(uint16_t engine_id, uint16_t schema);
unsigned preset_record_factory_count(uint16_t engine_id);

void preset_record_clear(preset_record_t *rec);
int preset_record_set_name(preset_record_t *rec, const char *name);
int preset_record_set_source(preset_record_t *rec, const char *source);
int preset_record_equal(const preset_record_t *a, const preset_record_t *b);

/* 1 if origin is factory, or factory_index is set (derived user / re-import). */
int preset_record_factory_derived(const preset_record_t *rec);

/* Encode a valid record to wire bytes. Does not apply the export gate.
 * If out is 0, *written is the size and the record is only checked.
 * On a buffer error, out and *written are left unchanged. *written is
 * set only after the buffer-size check passes, or on a size-only query.
 * The record does not contain a chain. */
int preset_record_encode(uint8_t *out, uint32_t cap, uint32_t *written,
                         const preset_record_t *rec);

/* File export. User and non-derived imported records always succeed.
 * Factory and factory-derived records succeed only when
 * DNBOX_FACTORY_PRESET_EXPORT is 1. Otherwise PRESET_ERR_EXPORT_FACTORY,
 * no bytes written. */
int preset_record_export(uint8_t *out, uint32_t cap, uint32_t *written,
                         const preset_record_t *rec);

/* Structural load: magic, length, CRC, format version, and flags only.
 * Does not check engine, schema, names, origin, param counts, or sample
 * refs. On error, *rec is unchanged. A load-only link can drop the
 * validation code. The 536 B HOST working record is a stack local here. */
int preset_record_load(preset_record_t *rec, const uint8_t *buf, uint32_t len);

/* Decode one record: load, then the semantic checks (engine, schema,
 * names, origin, counts, sample refs). Trailing bytes after total_len
 * are ignored. If expected_engine is not 0, a different engine id is
 * PRESET_ERR_ENGINE and original_engine is not rewritten. A missing
 * provenance field (zeros) loads as origin unknown and does not fail.
 * A newer format or schema is PRESET_ERR_VERSION or PRESET_ERR_SCHEMA.
 * No input crashes the decoder. */
int preset_record_decode(preset_record_t *rec, const uint8_t *buf, uint32_t len,
                         uint16_t expected_engine);

/* Import into a target engine. The encoded engine_id owns the parameter
 * schema and must match target_engine. original_engine is provenance only,
 * preserved on success (even if different). Decode is into scratch:
 * on any failure, including WRONG ENGINE, *rec stays unchanged. */
int preset_record_import(preset_record_t *rec, const uint8_t *buf, uint32_t len,
                         uint16_t target_engine);

/* Strict file/transport entry point. Requires the declared total length to
 * equal the supplied file length (no hidden suffix), then uses the native
 * import validation. A failure never mutates *rec. Firmware integration
 * must still provide stopped/quiet gating and durable storage separately. */
int preset_record_import_file(preset_record_t *rec, const uint8_t *buf,
                              uint32_t len, uint16_t target_engine);

int preset_record_verify_sample(const preset_record_t *rec, unsigned index,
                                uint32_t actual_crc);
int preset_record_verify_samples(const preset_record_t *rec,
                                 const uint32_t *actual_crc, unsigned n);

/* Find an unused slot in a caller-owned occupancy map (0 empty, nonzero used).
 * Returns PRESET_ERR_MEMORY_FULL when none is free. Does not evict. */
int preset_slot_find_empty(const uint8_t *used, unsigned nslots, unsigned *slot);

/* *written is set only after the buffer-size check, or when out is 0. */
int preset_commit_encode(uint8_t *out, uint32_t cap, uint32_t *written,
                         const preset_commit_t *mark);
int preset_commit_decode(preset_commit_t *mark, const uint8_t *buf, uint32_t len);
/* 0 = A wins, 1 = B wins, or a negative error if neither is valid.
 * Higher seq wins. On a seq tie, A wins. */
int preset_commit_winner(const uint8_t *a, uint32_t a_len,
                         const uint8_t *b, uint32_t b_len);
/* The non-winning valid pad, for overwrite undo. Negative if none. */
int preset_commit_previous(const uint8_t *a, uint32_t a_len,
                           const uint8_t *b, uint32_t b_len);

#ifdef __cplusplus
}
#endif
