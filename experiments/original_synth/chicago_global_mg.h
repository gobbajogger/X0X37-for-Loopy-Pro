/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_GLOBAL_MG_H
#define ORIGINAL_CHICAGO_GLOBAL_MG_H
#include <stdint.h>
#include "chicago_arp_control.h"
/* Logical snapshot of global b4cc..b50c only; no original pointer ABI.
 * Clocks remain borrowed R3 states: accumulator/threshold/key_reset.
 * RNG snapshots preserve32 bits. Advancing uses low16 and writes zero-high32;
 * non-RNG paths preserve the entire word.
 * Per-voice MG copies and A088 resolver ordering have a different owner. */
typedef struct {float phase,output,held;uint32_t wrapped;uint32_t random;} original_chicago_global_mg_osc;
typedef struct {int32_t counter;original_chicago_global_mg_osc mg1,mg2;} original_chicago_global_mg;
typedef struct {
 int32_t sample_rate_integer;float sample_rate,rate1,rate2;
 uint32_t waveform1,waveform2,tempo_sync1,tempo_sync2;
} original_chicago_global_mg_input;
/* No initialization/defaults: caller supplies exact converted inputs and
 * finite original global snapshots. All five waveform IDs0..4 are retained.
 * Original other waveform IDs preserve output and still run phase logic. */
/* Actual55e64..55f38, after queued events/gate tick, before MG2 R3 clock.
 * Consume key_reset, add current BPM, subtract threshold ONCE. Only sync==1
 * performs notification: wrapped=0, shape-specific output, phase=-1. */
void original_chicago_global_mg1_tick(original_chicago_global_mg_osc *,chicago_arp_clock *,uint32_t waveform,uint32_t tempo_sync,float bpm);
/* Actual55f84..56004, called only by the existing R3 MG2 tempo notification
 * edge. It does not tick the MG2 clock or call selector; R3 owns those edges. */
void original_chicago_global_mg2_notify(original_chicago_global_mg_osc *,uint32_t waveform);
/* Actual57010 GLOBAL tail5942c..597a0, AFTER assigned-voice control for each
 * genuine57010 entry. Caller must run it once at that original entry boundary,
 * including any actual forced-helper entries, never once per individual voice.
 * Adds690 to signed32 counter; on Fs crossing subtracts ONCE, computes all
 * five waveform outputs from OLD phase, clears wrapped, then advances phase.
 * MG1 phase update precedes MG2 waveform evaluation. MG2 free wrap invokes
 * selector exactly once AFTER both outputs/phase updates. Sync phase uses
 * accumulator/threshold; free increment uses original cubic rate and ceil
 * scheduling. This does not dispatch R3 clock or replace assigned-voice MG.
 * Explicit resources: no allocation, table approximation or sample memory.
 * Return0 when not due,1 when due. Required step callback is synchronous/total;
 * owner preserves selector failure separately. No gate-expiry NoteOff added.
 * sample_rate_integer>0; counter and all arithmetic operands must describe a
 * qualified finite original snapshot. Compile strictly (-ffp-contract=off).
 */
unsigned original_chicago_global_mg_after_voices(original_chicago_global_mg *,const original_chicago_global_mg_input *,const chicago_arp_clock *mg1_clock,const chicago_arp_clock *mg2_clock,void (*selector_step)(void *),void *);
/* Number of entries before the next waveform/phase update. Qualified selected
 * Chicago counters are nonnegative; a seeded or due counter returns zero. */
uint32_t original_chicago_global_mg_quiet_frames(const original_chicago_global_mg *,const original_chicago_global_mg_input *);
#endif
