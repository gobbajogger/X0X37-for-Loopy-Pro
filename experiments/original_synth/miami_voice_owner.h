/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_VOICE_OWNER_H
#define ORIGINAL_MIAMI_VOICE_OWNER_H
#include "miami_selected_voice.h"
#include "chicago_allocator.h"
/* The portable allocator/list/descriptor implementation is shared only after
 * exact original Miami/Chicago operand/CFG equivalence at their numeric-note
 * seam. See MIAMI-LIFECYCLE-PROOF.json. The BFD0 chord projection is separately
 * qualified by its readonly/import relocation proof and miami_geometry caller.
 * This alias alone does not perform geometry/reset ownership. No duplicate list pool. */
typedef original_chicago_allocator original_miami_allocator;
typedef original_pending_note original_miami_pending_note;
typedef struct {float velocity_bipolar,key_unipolar,key_bipolar;} original_miami_extra_sources;
typedef struct {
 void *context;
 int (*tune)(void *,unsigned voice_id,int32_t raw_note,float *pitch);
 int (*full_mg)(void *,unsigned voice_id,original_miami_voice_mg out[2]);
 void (*full_helper_seed)(void *,unsigned voice_id);
 int (*helper_exit)(void *);
} original_miami_note_producers;
typedef struct {
 original_miami_selected_voice *voices;
 original_miami_pending_note *pending;
 original_miami_extra_sources *extra;
 original_miami_allocator *allocator;
 const original_miami_voice_controls *controls;
 const original_miami_voice_tables *tables;
 original_miami_note_producers producers;
 uint32_t count;
 float bpm;
 int status;
} original_miami_voice_owner;
/* Bind existing cold/restored resources only; no reset, constructor snapshot,
 * active selector, MIDI mapping or persistence write. Selected canonical note
 * domain0..127/velocity0..127; all30 factories have unison2/SINGLE. Runtime
 * callbacks are total and serial, after source/pitch publication. A full MG
 * producer supplies the actual ordered-head/global snapshot. In the closed
 * selected52=1 context helper_exit only owns the separate global tail; it may
 * not observe/mutate local voice tails or allocator lists. Wider free-sync
 * needs a distinct inter-tail owner. Descriptor bits are never cleared here. */
int original_miami_voice_owner_bind(original_miami_voice_owner *,original_miami_selected_voice *,original_miami_pending_note *,original_miami_extra_sources *,unsigned count,original_miami_allocator *,const original_miami_voice_controls *,const original_miami_voice_tables *,const original_miami_note_producers *,float bpm);
const original_allocator_callbacks *original_miami_voice_owner_callbacks(void);
/* One normal B2EB0(-1) entry, ascending configured voices then global once.
 * Render assigned lanes afterward using render_frame, without another helper.
 * Every forced/pending initializer has its own exact helper/global entry. */
int original_miami_voice_owner_control_all(original_miami_voice_owner *);
/* Internal backend: admit the event-free block once, then preserve controls,
 * tables, BPM and ready tokens through all prevalidated control/render calls. */
int original_miami_voice_owner_block_valid(const original_miami_voice_owner *);
int original_miami_voice_owner_control_prevalidated(original_miami_voice_owner *);
#endif
