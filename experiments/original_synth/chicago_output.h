/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_OUTPUT_H
#define ORIGINAL_CHICAGO_OUTPUT_H
#include "chicago_drive.h"
#include "master.h"
#include <stdint.h>
/* Canonical internal event records. The caller resolves host encoding and
 * supplies immutable records sorted by unsigned offset, preserving tie order.
 * No event allocation/capacity limit is introduced; the caller retains storage
 * until consumed or replaced. Bind replaces the previous queue, cursor=0. */
typedef struct {uint32_t offset,type;} original_chicago_drive_event;
typedef struct {
 const original_chicago_drive_event *events;
 uint32_t count,cursor;
} original_chicago_drive_queue;
typedef struct {
 original_chicago_drive drive;
 original_chicago_drive_queue queue;
 int32_t wet_target,wet_accumulator,progress,activity_counter;
 uint8_t edit1,edit2,switch_current,switch_pending,switch_dirty;
 uint8_t initial_selection_pending,selected,transitioning;
} original_chicago_drive_wrapper;
/* Fixed initial selection: native constructor's null active object -> Drive19.
 * The first wrapper invocation resets constructor targets BEFORE applying
 * cached canonical edits, then starts fast smoothing and null->Drive fade.
 * This is not a selector for unrestricted algorithms or Drive reselection. */
int original_chicago_drive_wrapper_init(original_chicago_drive_wrapper *s,
 unsigned edit1,unsigned edit2,unsigned switch_on);
int original_chicago_drive_wrapper_set(original_chicago_drive_wrapper *s,
 unsigned parameter,unsigned canonical_value);
int original_chicago_drive_wrapper_events(original_chicago_drive_wrapper *s,
 const original_chicago_drive_event *events,uint32_t count);
/* Virtual58 immediate event5 resets Drive; other types have no effect.
 * Queued event4 resets before its local frame; other queue types are consumed.
 * Reset preserves queue, activity counter/flag, cached controls and pending
 * initial selection. It clears type-transition flag and snaps wet gain. */
void original_chicago_drive_wrapper_direct_event(original_chicago_drive_wrapper *s,unsigned type);
void original_chicago_drive_wrapper_reset(original_chicago_drive_wrapper *s);
/* Wrapper invocation boundaries are semantic. Native transition chunk size
 * is 743*(max(progress/743,1)+1), bounded by remaining frames. Drive event
 * indices restart0 at each chunk. The fast flag clears only after the full
 * wrapper invocation reaches cumulative44 frames. Scratch holds2*n int16.
 * All pointers/channels/scratch/state disjoint, except L/R are in-place outputs.
 * Empty inner calls commit dirty switches/initial selection, as native4fc08.
 * The enclosing output n0 path skips wrappers altogether. */
int original_chicago_drive_wrapper_process(original_chicago_drive_wrapper *s,
 int16_t *left,int16_t *right,int16_t *scratch_left,int16_t *scratch_right,
 uint32_t frames,uint32_t capacity);

typedef void (*original_chicago_fx_process)(void *context,int16_t *left,int16_t *right,uint32_t frames);
typedef void (*original_chicago_fx_reset)(void *context);
typedef struct {void *context;original_chicago_fx_process process;original_chicago_fx_reset reset;} original_chicago_fx2;
typedef struct {
 original_chicago_drive_wrapper fx1;
 original_master master;
 int32_t reset_countdown[2];
 uint8_t process_slot[2],enable_slot[2];
} original_chicago_output;
typedef struct {
 int16_t *left,*right,*wet_left,*wet_right;
 uint32_t capacity;
} original_chicago_output_workspace;
/* 8dc90 initializes both countdowns-1;8fe50 makes process/enable flags1.
 * This initializes the bounded fixed-Drive stage and core master constructor,
 * not complete original VST restore/warmup or a native engine activation. */
int original_chicago_output_init(original_chicago_output *s,unsigned drive_edit1,unsigned drive_edit2,unsigned drive_switch);
/* Actual944fc/9456c body: nonzero suspends processing and sets reset countdown
 * trunc_u32(float32(float32(Fs*20)/5)) & ~15; zero resumes and stores-1.
 * At44100 this is176400 frames. Enable bytes are independent and unchanged.
 * Its original external caller/host event reachability is not inferred. */
int original_chicago_output_suspend(original_chicago_output *s,unsigned slot,unsigned suspend,float original_sample_rate);
/* Explicit outer snapshot edit, not a claimed reachable public control.
 * Enable0 skips both countdown handling and processing for that slot. */
int original_chicago_output_enable(original_chicago_output *s,unsigned slot,unsigned enabled);
void original_chicago_output_mute(original_chicago_output *s,uint8_t pending_mute);
/* Pre-FX stereo core float bits -> native ordered cleanup -> FMUL32767 /
 * FCVTZS / low16 -> conditional fixed Drive wrapper -> caller FX2 wrapper ->
 * exact FDIV32767 -> master ramp/cleanup. master_squared is ALREADY converted
 * native96, no extra square. fz explicitly selects observed FMUL flush mode.
 * FX2 callback must implement the selected original full wrapper; this module
 * supplies no fake FX2 algorithm, type transition, host queue or post92228
 * lifecycle. The owner invokes its post-render lifecycle after success.
 * Positive frames require0..4096 native-wrapper capacity and total/infallible
 * callbacks when their slot will process/reset. No heap or frame-sized stack.
 * Matching input/output aliases allowed; all cross-channel/state/workspace
 * aliases unsupported. Empty outer calls are complete no-ops. */
int original_chicago_output_process(original_chicago_output *s,
 original_chicago_output_workspace *workspace,const original_chicago_fx2 *fx2,
 float master_squared,const uint32_t *in_left,const uint32_t *in_right,
 uint32_t *out_left,uint32_t *out_right,uint32_t frames,unsigned fz);
/* Read-only admission for the enclosing exact-zero FX2 path. Requires an
 * ordinary positive, valid invocation, settled Drive/wrapper state and zero
 * signal history. FX2 proves its own state separately before any mutation. */
int original_chicago_output_zero_ready(const original_chicago_output *,
 const original_chicago_output_workspace *,float master_squared,
 const uint32_t *in_left,const uint32_t *in_right,
 uint32_t *out_left,uint32_t *out_right,uint32_t frames,unsigned fz);
/* Same state/geometry admission without reading input samples. The serial
 * owner must have just filled both inputs with zero and must prevent edits
 * until processing completes. This does not prove arbitrary buffers silent. */
int original_chicago_output_zero_state_ready(const original_chicago_output *,
 const original_chicago_output_workspace *,float master_squared,
 const uint32_t *in_left,const uint32_t *in_right,
 uint32_t *out_left,uint32_t *out_right,uint32_t frames,unsigned fz);
#endif
