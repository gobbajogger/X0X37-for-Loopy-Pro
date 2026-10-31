/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_DYNAMICS_H
#define ORIGINAL_CHICAGO_DYNAMICS_H
#include "chicago_output.h"
/* These are logical caller-owned snapshots, not original C++ layout or a
 * project serialization format. Source/program/event/render ownership is serial.
 * Event records are the exact same canonical type as R3 Drive; callers retain
 * immutable sorted storage until consumed/rebound. No allocation or queue cap. */
typedef struct {const original_chicago_drive_event *events;uint32_t count,cursor;} original_chicago_dyn_queue;
typedef struct {
 int32_t target_sensitivity,sensitivity,attack;
 int32_t previous_input[2],highpass[2],peak,envelope,control;
 int32_t target_gain,gain;
 uint8_t parameter[3],activity;
 original_chicago_dyn_queue queue;
} original_chicago_mastercomp;
typedef struct {
 int32_t coefficient[5]; /* b0,b1,b2,a1,a2, native SCVTF-rounded Q24 */
 int32_t history[2][4]; /* y1,y2,x1,x2; raw y precedes output saturation */
 float alpha;
 uint8_t parameter[3],activity; /* Fc,Gain,Q; fixed Fs44100 */
 original_chicago_dyn_queue queue;
} original_chicago_peaking;
int original_chicago_mastercomp_init(original_chicago_mastercomp *s);
int original_chicago_peaking_init(original_chicago_peaking *s);
/* Native kernel0Sens/1Attack/2hiddenGain (MasterCompressor),0Fc/1Gain/2Q
 * (PeakingEQ). Public wrapper Edit1/Edit2 map to kernel0/1. Hidden defaults
 * Gain100/Q64 are retained. Master masks value&127, as original. EQ accepts
 * canonical0..127 (actual wrapper metadata domain); no invented clamp. */
int original_chicago_mastercomp_set(original_chicago_mastercomp *s,unsigned parameter,int32_t value);
int original_chicago_peaking_set(original_chicago_peaking *s,unsigned parameter,unsigned canonical_value);
void original_chicago_mastercomp_reset(original_chicago_mastercomp *s);
/* Master reset retains both highpass output histories, as native29A30. */
void original_chicago_peaking_reset(original_chicago_peaking *s);
void original_chicago_mastercomp_activity(original_chicago_mastercomp *s,uint8_t active);
void original_chicago_peaking_activity(original_chicago_peaking *s,uint8_t active);
int original_chicago_mastercomp_bind_events(original_chicago_mastercomp *s,const original_chicago_drive_event *events,uint32_t count);
int original_chicago_peaking_bind_events(original_chicago_peaking *s,const original_chicago_drive_event *events,uint32_t count);
/* Direct native virtual58 resets on type5 only; queued type4 resets before
 * local sample, other queue types are consumed. Reset preserves controls,
 * activity and remaining queue. Empty processing never drains or latches.
 * Tempo virtual20 is no-op for both selected adapters, NOT sample-rate setup. */
void original_chicago_mastercomp_direct_event(original_chicago_mastercomp *s,unsigned type);
void original_chicago_peaking_direct_event(original_chicago_peaking *s,unsigned type);
/* Disjoint in-place stereo arrays; other state/input/event overlap unsupported.
 * Count describes one real effect invocation; local index restarts0. Native
 * outer wrapper wet102/type transitions/master remain separate owner work. */
int original_chicago_mastercomp_process(original_chicago_mastercomp *s,int16_t *left,int16_t *right,uint32_t frames);
int original_chicago_peaking_process(original_chicago_peaking *s,int16_t *left,int16_t *right,uint32_t frames);
/* Exact zero-input spans; return0 without mutation if a history, pending
 * event or evolving detector/control prevents a proof of unchanged state. */
int original_chicago_mastercomp_advance_zero(original_chicago_mastercomp *,uint32_t);
/* EQ admits an exact fixed point, including a nonzero quantized residue.
 * The returned sample is the original saturated output; history is retained. */
int original_chicago_peaking_advance_zero(const original_chicago_peaking *,uint32_t,int16_t constant[2]);
#endif
