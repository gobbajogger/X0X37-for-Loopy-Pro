/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_ENGINE_OUTPUT_H
#define ORIGINAL_CHICAGO_ENGINE_OUTPUT_H
#include "chicago_fx2_wrapper.h"
#include "chicago_program_output.h"
typedef struct {original_chicago_output output;original_chicago_fx2_wrapper fx2;} original_chicago_engine_output;
/* Joins the real owner-supplied original FX2 objects/type wrapper with R3's
 * actual Drive/conversion/master graph. Histories, events and reset-all/arena
 * closure remain the owner's explicit admission contracts, not test callbacks.
 * No host encoding, startup, persistent project adoption or selector activation.
 * Control/render is serial; all callbacks and resources must remain total. */
int original_chicago_engine_output_init(original_chicago_engine_output *,const original_chicago_routing *,const original_chicago_fx2_owner *);
/* Explicit firmware cold-selection composition; no original all30 reset or
 * cross-type arena continuity is claimed by this entry point. */
int original_chicago_engine_output_init_cold(original_chicago_engine_output *,const original_chicago_routing *,const original_chicago_fx2_cold_owner *);
int original_chicago_engine_output_apply(original_chicago_engine_output *,const original_chicago_routing *,unsigned changed_macro);
/* Scratch is caller-owned, with capacity>=frames, up to4096. It may be reused
 * between the Drive and FX2 stages since their calls do not overlap. The four
 * FX2 scratch channels must remain disjoint from converted L/R and each other.
 * Routing/master comes from the same immutable snapshot as the voices. */
int original_chicago_engine_output_process(original_chicago_engine_output *,original_chicago_output_workspace *,original_chicago_fx2_workspace *,const original_chicago_routing *,const uint32_t *,const uint32_t *,uint32_t *,uint32_t *,uint32_t frames,unsigned fz);
/* Serial idle-owner entry: both input buffers were just zero-filled by the
 * caller. Retains all state/geometry checks and the unsettled-state fallback. */
int original_chicago_engine_output_process_zero(original_chicago_engine_output *,original_chicago_output_workspace *,original_chicago_fx2_workspace *,const original_chicago_routing *,const uint32_t *,const uint32_t *,uint32_t *,uint32_t *,uint32_t frames,unsigned fz);
#endif
