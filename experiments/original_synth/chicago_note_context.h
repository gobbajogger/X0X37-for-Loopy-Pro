/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_NOTE_CONTEXT_H
#define ORIGINAL_CHICAGO_NOTE_CONTEXT_H
#include "chicago_voice_owner.h"
#include "chicago_global_stage.h"
#include "chicago_tuning.h"
#include "chicago_full_mg.h"
/* The other two source words already live exactly in voice.velocity and
 * voice.keyboard_source. Keep only the missing words, avoiding duplicate
 * source arrays while retaining every source bit through pending staging. */
typedef struct {float velocity_bipolar,key_unipolar;} original_chicago_extra_sources;
typedef struct {
 original_chicago_voice_owner *owner;
 original_chicago_tuning *tuning;
 original_chicago_selected_global_stage *global;
 original_chicago_extra_sources *extra;
 uint8_t key_sync[2];
 int status;
} original_chicago_note_context;
/* Obtain callbacks, bind voice_owner with them, then bind this context before
 * any event/control/render. Context binding changes only this descriptor.
 * All existing voice/global/source snapshots and the process-lifetime shared
 * tuning RNG remain unchanged. Extra-source storage is caller-owned and must
 * contain its exact cold/restored words, not fabricated constructor defaults.
 * Primary45/51 key-sync and both original triangle shapes are the selected
 * all81/all22 domain. No callback asks an unbound constructor to fill gaps. */
original_chicago_note_producers original_chicago_note_context_callbacks(original_chicago_note_context *);
/* Actual MG1 frame clock and MG2 tempo notification behind scheduler's
 * established callbacks. Pass the same voice_owner as scheduler context and
 * retain the returned callback value for the scheduler's full lifetime. */
original_chicago_scheduler_callbacks original_chicago_note_context_scheduler_callbacks(void);
int original_chicago_note_context_bind(original_chicago_note_context *,original_chicago_voice_owner *,original_chicago_tuning *,original_chicago_selected_global_stage *,original_chicago_extra_sources *,unsigned key_sync1,unsigned key_sync2);
/* Full logical source snapshot, including the source words not retained by
 * the audible selected slice. No recomputation, quantization or MIDI clamp. */
int original_chicago_note_context_sources(const original_chicago_note_context *,unsigned voice_id,original_chicago_voice_sources *out);
#endif
