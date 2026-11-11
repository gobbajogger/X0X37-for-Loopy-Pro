/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_MODULATION_H
#define ORIGINAL_CHICAGO_MODULATION_H
#include <stdint.h>
#include <stddef.h>
#include "chicago_output.h"
typedef original_chicago_drive_event original_mod_event;
typedef struct {const original_mod_event *events;uint32_t count,cursor;} original_mod_queue;
/* One serial owner. Queues are immutable and borrowed until consumed/replaced.
 * Offsets are EFFECT-INVOCATION LOCAL after the owner's actual retiming. This
 * supplies no MIDI interpretation or wrapper type-switch/outer event policy.
 * Queued4 resets before sample; other types are consumed. Direct5 resets all; direct2 also resets Chorus/Ensemble.
 * n0 leaves all state unchanged. Channels/state/histories disjoint. */
int original_mod_events(original_mod_queue *,const original_mod_event *,uint32_t);
typedef struct {
 int32_t pending_frequency,pending_wet,frequency,frequency_acc,wet,wet_acc,phase,dry_gain,wet_gain;
 uint8_t edit1,edit2,dirty,activity;
 original_mod_queue queue;
} original_chicago_ring;
void original_chicago_ring_init(original_chicago_ring *);
void original_chicago_ring_reset(original_chicago_ring *);
void original_chicago_ring_direct_event(original_chicago_ring *,unsigned type);
int original_chicago_ring_set(original_chicago_ring *,unsigned parameter,unsigned value);
int original_chicago_ring_process(original_chicago_ring *,int16_t *,int16_t *,uint32_t);
typedef struct {
 int32_t pending_speed,pending_feedback,pending_wet,speed,feedback,wet,feedback_acc,wet_acc,phase,modulator[2];
 uint32_t head;
 uint8_t edit1,edit2,dirty,activity;
 int16_t *left,*right; /* Exactly1024 int16 per channel;4096B total. */
 original_mod_queue queue;
} original_chicago_chorus;
int original_chicago_chorus_init(original_chicago_chorus *,int16_t left[1024],int16_t right[1024]);
void original_chicago_chorus_reset(original_chicago_chorus *);
void original_chicago_chorus_direct_event(original_chicago_chorus *,unsigned type);
int original_chicago_chorus_set(original_chicago_chorus *,unsigned parameter,unsigned value);
int original_chicago_chorus_process(original_chicago_chorus *,int16_t *,int16_t *,uint32_t);
typedef struct {
 int32_t speed,wet,wet_acc,phase,feedback,feedback_acc,modulation,modulation_acc,depth,depth_acc,history[5][2];
 uint8_t edit1,edit2,activity;
 original_mod_queue queue;
} original_chicago_phaser;
void original_chicago_phaser_init(original_chicago_phaser *);
void original_chicago_phaser_reset(original_chicago_phaser *);
void original_chicago_phaser_direct_event(original_chicago_phaser *,unsigned type);
int original_chicago_phaser_set(original_chicago_phaser *,unsigned parameter,unsigned value);
int original_chicago_phaser_process(original_chicago_phaser *,int16_t *,int16_t *,uint32_t);
/* Optional exact zero-input spans. Refusal is read-only. Requires consumed
 * queues and settled controls, and proves all signal history is zero.
 * Musical/effect phase and Chorus modulator recurrences still advance. */
int original_chicago_ring_advance_zero(original_chicago_ring *,uint32_t);
int original_chicago_chorus_advance_zero(original_chicago_chorus *,uint32_t);
int original_chicago_phaser_advance_zero(original_chicago_phaser *,uint32_t);
typedef struct {
 float phase[2][4],right_phase[2][4];
 float depth,spread,delta_depth,delta_spread,pending_depth,pending_spread,unused_rate;
 float *left,*right;
 uint32_t head,written,slots;
 uint8_t edit1,edit2,dirty;
 original_mod_queue queue;
} original_chicago_ensemble;
/* Fixed original public type3/mode3 at constructor Fs44100. The other original
 * Polysix modes are different non-public FX types, not selected by this API.
 * slots4096 uses dense logical rings; slots1024 uses512 recent ages +512 fixed
 * virtual low indices per channel (8192B total). It preserves the original4096
 * virtual head, float coordinate rounding and tiny-negative fitted-sine taps.
 * Producer checked; independent review pending. Neither mode allocates a pool.
 * Reset is O(1)logical-zero; stale physical bytes are not canonical state.
 * Export snapshot samples by virtual index through history(), not raw bytes. */
int original_chicago_ensemble_init(original_chicago_ensemble *,float *,float *,uint32_t slots);
void original_chicago_ensemble_reset(original_chicago_ensemble *);
void original_chicago_ensemble_direct_event(original_chicago_ensemble *,unsigned type);
int original_chicago_ensemble_set(original_chicago_ensemble *,unsigned,unsigned);
int original_chicago_ensemble_process(original_chicago_ensemble *,int16_t *,int16_t *,uint32_t);
/* Only currently live logical indices are available for compact snapshots:
 * recent ages0..511 plus fixed virtual indices0..511. Caller must preserve
 * head/written/parameters/phases too; this is not a legacy persistent format. */
int original_chicago_ensemble_history(const original_chicago_ensemble *,unsigned channel,uint32_t index,float *);
/* Public parameter IDs are adapter2=Edit1,3=Edit2, both canonical0..127.
 * activity is the original virtual50 byte; zero=>102,nonzero=>32767. Caller
 * owns exact wrapper notification timing. Tempo setter is original no-op.
 * These kernels do not include outer wet/type crossfades or master. */
#endif
