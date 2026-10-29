/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_GLOBAL_STAGE_H
#define ORIGINAL_CHICAGO_GLOBAL_STAGE_H
#include "chicago_global_mg.h"
#include "chicago_allocator.h"
typedef struct {
 original_chicago_global_mg *state;
 const original_chicago_global_mg_input *input;
 chicago_arp_clock *clock1,*clock2;
 const original_chicago_allocator *allocator;
 uint32_t helper_entries,full_head_seeds;
 int status;
} original_chicago_selected_global_stage;
/* Exact selected all81/all22 context:44100Hz and native52=1, never written by
 * a public macro. Explicit state/inputs/clocks are borrowed without resets.
 * No global constructor or saved-context defaults are guessed. Free MG2 is
 * independently implemented by global_mg.c; its selector side effects require
 * a different inter-tail owner and are not admitted by this selected bridge.
 * The disjoint selected G/T read-write proof allows inner global tail after
 * final resume's local tail, immediately before any next initializer/render.
 * No side-effectful table readers or intervening observers are permitted. */
int original_chicago_selected_global_bind(original_chicago_selected_global_stage *,original_chicago_global_mg *,const original_chicago_global_mg_input *,chicago_arp_clock *,chicago_arp_clock *,const original_chicago_allocator *);
void original_chicago_selected_global_seed(original_chicago_selected_global_stage *,unsigned voice_id);
int original_chicago_selected_global_exit(original_chicago_selected_global_stage *);
#endif
