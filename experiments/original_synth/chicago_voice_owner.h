/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_VOICE_OWNER_H
#define ORIGINAL_CHICAGO_VOICE_OWNER_H
#include "chicago_voice.h"
#include "chicago_scheduler.h"
typedef struct {
 void *context;
 /* Immediate A088/B200 has already published enabled/key/raw/unison/source
  * fields. Resolve the actual original global scale/tuned target here, once
  * for each initializer. Partial must not consume full-note MG producers. */
 int (*tune)(void *,unsigned voice_id,const original_voice_record *,unsigned full,int32_t last_note,float *tuned_pitch,float *current_pitch_anchor);
 /* Called only for full initialization, AFTER the masked target/current/
  * activation writes. Resolve actual ordered-head/global MG copies/resets.
  * Existing list ownership and projected voice fields are observable here. */
 int (*full_mg)(void *,unsigned voice_id,original_chicago_voice_mg out[2]);
 /* Publish the other original source arrays as well: selected renderer only
  * retains velocity_unipolar and key_bipolar. No allocation or re-entry. */
 void (*sources)(void *,unsigned voice_id,const original_chicago_voice_sources *);
 /* Full initialization seeds the separate core-global counter only for the
  * ordered active head, after MG selection and before the forced helper.
  * helper_exit runs once per real forced/normal57010 entry. In this selected
  * composition it must not mutate/read per-voice state or allocator lists:
  * exact native52=1 profile makes the global tail's selector branch inactive.
  * This qualifies commuting final inner-global/outer-local tails; unrestricted
  * free-sync callbacks require a separate between-tail orchestration seam. */
 void (*full_helper_seed)(void *,unsigned voice_id);
 int (*helper_exit)(void *);
} original_chicago_note_producers;
typedef struct {
 original_chicago_voice *voices;
 original_pending_note *pending;
 uint32_t count;
 original_chicago_allocator *allocator;
 const original_chicago_voice_controls *controls;
 const original_chicago_voice_tables *tables;
 original_chicago_note_producers producers;
 float bpm;
 int status;
} original_chicago_voice_owner;
/* Binds already initialized/restored owned states without changing them.
 * Explicit producer callbacks are not constructor/tuning placeholders and
 * must be exact/total before full engine admission. No snapshot pointers are
 * serialized; legacy storage/startup adoption stays with its source owner. */
int original_chicago_voice_owner_bind(original_chicago_voice_owner *,original_chicago_voice *,original_pending_note *,uint32_t count,original_chicago_allocator *,const original_chicago_voice_controls *,const original_chicago_voice_tables *,const original_chicago_note_producers *,float bpm);
const original_allocator_callbacks *original_chicago_voice_owner_callbacks(void);
/* Exact same-frame helper/action2 restart and render for one original voice
 * ID. Action1 returns A818 after helper tail; action2 never returns its record.
 * No audio sample/helper is inserted between pending parts. A producer error
 * sets sticky status0 and is returned as failure, never successful silence.
 * Voice summation/group reduction order remains with the original renderer. */
int original_chicago_voice_owner_sample(original_chicago_voice_owner *,unsigned voice_id,float *left,float *right);
/* Actual normal57010 entry: prepare eligible IDs0..capacity-1 in ascending
 * order, then one separate core-global tail. Does NOT render any oscillator
 * sample. The caller renders every assigned lane afterward, retaining native
 * group/sum order. Forced note/pending entries run their global tails inside
 * the actual note callback, never once per arbitrary sampled voice. */
int original_chicago_voice_owner_control_all(original_chicago_voice_owner *);
/* Selected single-voice, event-free serial block. Resources are validated
 * once; scheduler callbacks and every due control/global tick retain their
 * sample order. No direct API call may interleave this operation. */
int original_chicago_voice_owner_render_block(original_chicago_voice_owner *,original_chicago_scheduler *,uint32_t *left,uint32_t *right,uint32_t frames);
#endif
