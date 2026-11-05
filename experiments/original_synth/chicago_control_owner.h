/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_CONTROL_OWNER_H
#define ORIGINAL_CHICAGO_CONTROL_OWNER_H
#include "chicago_engine_output.h"
#include "chicago_voice.h"
#include "chicago_scheduler.h"
#include "chicago_global_mg.h"
typedef struct {
 original_chicago_program *program;
 original_chicago_voice_controls *voice;
 original_chicago_engine_output *output;
 original_chicago_scheduler *scheduler;
 original_chicago_global_mg_input *global_input;
 int status;
} original_chicago_control_owner;
/* Borrows one serial owner's already initialized, corresponding snapshot and
 * real processors. Held voices read this stable compiled view. No restore,
 * geometry, table, queue, clock residual or cold-state reset is performed.
 * Initial factory/routing/selector/global contexts must already correspond.
 * The current explicit sample-rate/tempo remain with their established owner. */
int original_chicago_control_owner_bind(original_chicago_control_owner *,original_chicago_program *,original_chicago_voice_controls *,original_chicago_engine_output *,original_chicago_scheduler *,original_chicago_global_mg_input *);
/* One actual public macro edit, all targets compiled once, then exact active
 * output writes and scheduler/global converted views. All22 endpoints reach
 * their audible/control owners. IDs128/129/131/132/133 remain original no-ops.
 * Native108 alone among these setter targets immediately runs cleanup+54d94,
 * including an identical pedal-up write. Native62 enable and speed/range/mode
 * stores do NOT add a prepare/reset: block54d94 and clock threshold updates
 * stay at their original caller boundaries. No per-sample getters/conversion.
 * Invalid normalized input leaves everything unchanged. All accepted callbacks
 * must be total; a downstream contract failure becomes sticky status0, not a
 * successful silent edit. No engine activation follows from this composition. */
int original_chicago_control_owner_edit(original_chicago_control_owner *,unsigned macro,float normalized);
#endif
