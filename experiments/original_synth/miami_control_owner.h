/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_CONTROL_OWNER_H
#define ORIGINAL_MIAMI_CONTROL_OWNER_H
#include "miami_note_context.h"
#include "miami_output.h"
typedef struct {
 original_miami_program *program;
 original_miami_voice_controls *controls;
 original_miami_output *output;
 original_chicago_scheduler *scheduler;
 original_chicago_global_mg_input *global_input;
 int status;
} original_miami_control_owner;
/* Bind corresponding initialized/restored objects without changing them.
 * Existing event/render ownership stays serial; no project adoption/reset.
 * Shared54d94 control projection is independently equivalent to MiamiB0C34. */
int original_miami_control_owner_bind(original_miami_control_owner *,original_miami_program *,original_miami_voice_controls *,original_miami_output *,original_chicago_scheduler *,original_chicago_global_mg_input *);
/* All18 macro endpoints: publish one compiled snapshot, actual FX10 edits,
 * global shape inputs, and immediate108 damper cleanup/prepare for macro17,
 * even if unchanged. Tempo threshold preparation stays at block entry. */
int original_miami_control_owner_edit(original_miami_control_owner *,unsigned macro,float normalized);
int original_miami_control_owner_block_prepare(original_miami_control_owner *,chicago_arp_clock *clock1);
#endif
