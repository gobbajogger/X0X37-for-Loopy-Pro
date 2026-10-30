/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_VOICE_SNAPSHOT_H
#define ORIGINAL_MIAMI_VOICE_SNAPSHOT_H
#include "miami_voice_owner.h"
#include <stddef.h>
typedef struct {
 original_miami_selected_voice voice;
 original_miami_pending_note pending;
 original_miami_extra_sources extra;
 uint32_t program_revision,voice_id;
} original_miami_voice_snapshot;
#define ORIGINAL_MIAMI_VOICE_SNAPSHOT_WORDS 130u
#define ORIGINAL_MIAMI_VOICE_SNAPSHOT_BYTES (16u+4u*ORIGINAL_MIAMI_VOICE_SNAPSHOT_WORDS)
/* Complete owned per-voice value snapshot: all DSP/filter/gain/phase/MG/cache,
 * counter/action2 continuation, pending masks/notes, all modulation sources,
 * program revision and stable voice ID. No pointer or C padding is encoded.
 * Explicit v1 field order, little-endian words, original binary32 bits and
 * FNV integrity. Byte fields widen losslessly to words; signed fields retain
 * their exact two's-complement values. Captures require a quiescent serial
 * owner; no capture or application may interleave a pending continuation.
 * Decode stages a value object, never edits live voices/lists. Existing owner
 * must validate matching program/resource identity and atomically adopt the
 * WHOLE allocator/global/clocks/FX/queue/history context. This codec is not a
 * project format, automatic migration, or complete engine-state acceptance.
 * Caller owns output/staging storage. No allocation or table reconstruction.
 */
int original_miami_voice_snapshot_capture(original_miami_voice_snapshot *,const original_miami_voice_owner *,unsigned voice_id);
int original_miami_voice_snapshot_encode(const original_miami_voice_snapshot *,void *,size_t capacity);
int original_miami_voice_snapshot_decode(original_miami_voice_snapshot *,const void *,size_t length);
#endif
