/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_VOICE_SNAPSHOT_H
#define ORIGINAL_CHICAGO_VOICE_SNAPSHOT_H
#include "chicago_note_context.h"
#include <stddef.h>
typedef struct {
 original_chicago_voice voice;
 original_pending_note pending;
 original_chicago_extra_sources extra;
 uint32_t program_revision,voice_id;
} original_chicago_voice_snapshot;
#define ORIGINAL_CHICAGO_VOICE_SNAPSHOT_WORDS 135u
#define ORIGINAL_CHICAGO_VOICE_SNAPSHOT_BYTES (16u+4u*ORIGINAL_CHICAGO_VOICE_SNAPSHOT_WORDS)
/* Full owned per-voice MP4/oscillator/gain/noise/MG/cache/action2/pending/source
 * value projection. Same pointer-free v1 word codec as Miami, backend2 tag.
 * Every binary32/signed32/full32noise bit is retained without C padding or
 * derived-control reconstruction. Caller captures only at a serial quiescent
 * boundary. Decode stages values; whole allocator/global/FX/history/queue and
 * matching program/resource adoption remain with the existing storage owner.
 * Shared-arena history backend and complete engine persistence are NOT supplied
 * by this per-voice envelope. Legacy bytes keep their existing backend.
 */
int original_chicago_voice_snapshot_capture(original_chicago_voice_snapshot *,const original_chicago_note_context *,unsigned voice_id);
int original_chicago_voice_snapshot_encode(const original_chicago_voice_snapshot *,void *,size_t capacity);
int original_chicago_voice_snapshot_decode(original_chicago_voice_snapshot *,const void *,size_t length);
#endif
