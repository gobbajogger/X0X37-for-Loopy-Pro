/* SPDX-License-Identifier: GPL-3.0-only */
/* Product-sound identity codec. Callers own storage transactions. The original
 * profile accepts only OCH1/OMI1 schema 1 and their 22/18-control cells; legacy
 * profiles retain their own product identity rules. The integration owner
 * supplies the registry from actual production descriptors. */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "original_sound.h"

#define BASS_IDENTITY_BYTES 8u
#define BASS_IDENTITY_PRODUCT_SCHEMA 1u
#define BASS_IDENTITY_LAST_UNTAGGED_SOUND_REV 10u

enum { BASS_IDENTITY_UNKNOWN = 0, BASS_IDENTITY_KNOWN = 1 };
enum {
    BASS_IDENTITY_READY = 0,
    BASS_IDENTITY_CHOICE_REQUIRED,
    BASS_IDENTITY_UNSUPPORTED,
    BASS_IDENTITY_UNAVAILABLE,
    BASS_IDENTITY_INVALID
};
typedef struct {
    uint32_t engine;
    uint16_t schema;
    uint8_t count, state;
} bass_identity_t;
typedef struct {
    uint32_t engine;
    uint16_t schema;
    uint8_t count, available;
} bass_identity_entry_t;

/* Count/schema mismatches and conflicting registry rows never admit a sound.
 * An unavailable known engine remains distinct from an invalid or unknown tag. */
static inline int bass_identity_check(const bass_identity_t *tag,
                                     const bass_identity_entry_t *registry, size_t n)
{
    const bass_identity_entry_t *match = NULL;
    size_t i;
    if (!tag || (!registry && n)) return BASS_IDENTITY_INVALID;
    if (tag->state == BASS_IDENTITY_UNKNOWN) return BASS_IDENTITY_UNSUPPORTED;
    if (tag->engine != ORIGINAL_SOUND_CHICAGO_ID && tag->engine != ORIGINAL_SOUND_MIAMI_ID)
        return BASS_IDENTITY_UNSUPPORTED;
    if (tag->schema != ORIGINAL_SOUND_SCHEMA) return BASS_IDENTITY_UNSUPPORTED;
    if (tag->count != original_sound_count(tag->engine == ORIGINAL_SOUND_MIAMI_ID))
        return BASS_IDENTITY_INVALID;
    if (tag->state == BASS_IDENTITY_UNKNOWN)
        return !tag->engine && !tag->schema && !tag->count ?
            BASS_IDENTITY_CHOICE_REQUIRED : BASS_IDENTITY_INVALID;
    if (tag->state != BASS_IDENTITY_KNOWN || !tag->engine || !tag->schema || !tag->count)
        return BASS_IDENTITY_INVALID;
    for (i = 0; i < n; i++) {
        const bass_identity_entry_t *row = &registry[i];
        if (!row->engine || !row->schema || !row->count || row->available > 1)
            return BASS_IDENTITY_INVALID;
        if (row->engine != tag->engine || row->schema != tag->schema) continue;
        if (match && (match->count != row->count || match->available != row->available))
            return BASS_IDENTITY_INVALID;
        match = row;
    }
    if (!match) return BASS_IDENTITY_UNSUPPORTED;
    if (tag->count != match->count) return BASS_IDENTITY_INVALID;
    return match->available ? BASS_IDENTITY_READY : BASS_IDENTITY_UNAVAILABLE;
}

static inline int bass_identity_decode(bass_identity_t *out, const uint8_t *wire, size_t bytes,
                                      const bass_identity_entry_t *registry, size_t n)
{
    bass_identity_t tag;
    int status;
    if (!out || !wire || bytes != BASS_IDENTITY_BYTES) return BASS_IDENTITY_INVALID;
    tag.engine = (uint32_t)wire[0] | (uint32_t)wire[1] << 8 |
                 (uint32_t)wire[2] << 16 | (uint32_t)wire[3] << 24;
    tag.schema = (uint16_t)((uint16_t)wire[4] | (uint16_t)wire[5] << 8);
    tag.count = wire[6]; tag.state = wire[7];
    status = bass_identity_check(&tag, registry, n);
    if (status != BASS_IDENTITY_INVALID) *out = tag;
    return status;
}

/* Unknown future tags are retained as their original bytes, never rewritten by
 * this encoder. Write only canonical known/known-unavailable or empty tags. */
static inline int bass_identity_encode(uint8_t *wire, size_t bytes, const bass_identity_t *tag,
                                      const bass_identity_entry_t *registry, size_t n)
{
    int status = bass_identity_check(tag, registry, n);
    if (!wire || bytes != BASS_IDENTITY_BYTES) return BASS_IDENTITY_INVALID;
    if (status != BASS_IDENTITY_READY && status != BASS_IDENTITY_UNAVAILABLE &&
        status != BASS_IDENTITY_CHOICE_REQUIRED) return status;
    wire[0] = (uint8_t)tag->engine; wire[1] = (uint8_t)(tag->engine >> 8);
    wire[2] = (uint8_t)(tag->engine >> 16); wire[3] = (uint8_t)(tag->engine >> 24);
    wire[4] = (uint8_t)tag->schema; wire[5] = (uint8_t)(tag->schema >> 8);
    wire[6] = tag->count; wire[7] = tag->state;
    return status;
}

/* No voice-index, value-range, producer-version or snapshot inference. The caller
 * owns a separate tag output; the original sound/export is not an argument. */
static inline int bass_identity_legacy(unsigned sound_revision, bass_identity_t *out)
{
    (void)sound_revision; (void)out;
    return BASS_IDENTITY_UNSUPPORTED;
}

/* Eight canonical identity bytes, LE preset index, flags, reserved zero.
 * Revision 12 requires the original cell's factory base, even after edits.
 * Legacy revision 11 also allowed an unknown tag or a standalone edited sound. */
#define BASS_SOUND_TAG_BYTES 12u
#define BASS_SOUND_NO_PRESET 0xffffu
#define BASS_SOUND_EDITED 1u
typedef struct { uint8_t bytes[BASS_SOUND_TAG_BYTES]; } bass_sound_tag_t;
typedef struct {
    bass_identity_t identity;
    uint16_t preset;
    uint8_t flags;
} bass_sound_info_t;
static inline int bass_sound_tag_decode(bass_sound_info_t *out,const bass_sound_tag_t *wire,
                                       const bass_identity_entry_t *registry,size_t n)
{
    bass_sound_info_t info;
    int rc;
    if(!out || !wire)return BASS_IDENTITY_INVALID;
    rc=bass_identity_decode(&info.identity,wire->bytes,BASS_IDENTITY_BYTES,registry,n);
    if(rc==BASS_IDENTITY_INVALID || rc==BASS_IDENTITY_UNSUPPORTED)return rc;
    info.preset=(uint16_t)(wire->bytes[8]|((uint16_t)wire->bytes[9]<<8));
    info.flags=wire->bytes[10];
    if(wire->bytes[11] || (info.flags&~BASS_SOUND_EDITED))return BASS_IDENTITY_INVALID;
    if (info.preset >= original_sound_factories(info.identity.engine == ORIGINAL_SOUND_MIAMI_ID))
        return BASS_IDENTITY_INVALID;
    if(rc==BASS_IDENTITY_CHOICE_REQUIRED) {
        if(info.preset || info.flags)return BASS_IDENTITY_INVALID;
    } else if(info.preset==BASS_SOUND_NO_PRESET && info.flags!=BASS_SOUND_EDITED)return BASS_IDENTITY_INVALID;
    *out=info;return rc;
}
static inline int bass_sound_tag_encode(bass_sound_tag_t *out,const bass_sound_info_t *info,
                                       const bass_identity_entry_t *registry,size_t n)
{
    bass_sound_tag_t wire={{0}};bass_sound_info_t check;
    if(!out || !info)return BASS_IDENTITY_INVALID;
    int rc=bass_identity_encode(wire.bytes,BASS_IDENTITY_BYTES,&info->identity,registry,n);
    if(rc!=BASS_IDENTITY_READY && rc!=BASS_IDENTITY_UNAVAILABLE && rc!=BASS_IDENTITY_CHOICE_REQUIRED)return rc;
    wire.bytes[8]=(uint8_t)info->preset;wire.bytes[9]=(uint8_t)(info->preset>>8);wire.bytes[10]=info->flags;
    int valid=bass_sound_tag_decode(&check,&wire,registry,n);
    if(valid==BASS_IDENTITY_INVALID)return valid;
    *out=wire;return rc;
}
