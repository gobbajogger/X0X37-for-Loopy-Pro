/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_SYNTH_CONTROL_MATH_H
#define ORIGINAL_SYNTH_CONTROL_MATH_H
#include <stdint.h>
/* Explicit helper-entry clock, not a presumed wall-clock scheduler. Full note
 * init seeds counter=Fs then calls helper immediately; partial note entry calls
 * helper without reset. Both are additional to normal audio-sample entries. */
int original_control_tick(uint32_t *counter,uint32_t integer_fs,uint32_t *duration);
typedef struct {float level;uint8_t state;} original_vca;
/* Inputs are already selected finite effective EG/GATE controls, clamped0..1.
 * Level accepts finite [-1,1], including reachable one-tick decay undershoot.
 * The next original state transition handles it; it is not clipped early.
 * Does not claim complete VCF envelope/voice allocator or audio gain ramp.
 * action0 none,1 retire whole voice,2 restart pending note; caller owns action. */
int original_vca_tick(original_vca *s,float fs,uint32_t duration,
 float attack,float decay,float sustain,float release,unsigned *action);
typedef struct {uint16_t source,second,destination;uint8_t has_second;float intensity;} original_patch;
/* Sources must be immutable PREVIOUS-tick snapshots; destination count bounded
 * by caller arrays. All8 sequential contributions clamp individually. All
 * validation precedes output; source and destination arrays may not overlap. */
int original_patch_accumulate(const float *sources,unsigned source_count,
 const original_patch routes[8],float *destinations,unsigned destination_count);
#endif
