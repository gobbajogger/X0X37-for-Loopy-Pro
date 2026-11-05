/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_OSCILLATOR_MOTION_H
#define ORIGINAL_OSCILLATOR_MOTION_H
#include <stdint.h>
/* Explicit phases, increments, current gains and native ordinal selectors.
 * This is not a note initializer, complete pitch binding or preset loader.
 * Only the original 44.1 kHz phase-table context is presently evidenced. */
typedef struct {uint32_t phase[4];} original_oscillators;
/* Curve0 original level normalization is binary32 bits3faaa993. Physical
 * oscillator gains can exceed normalized1, including stock factory programs.
 * This is a recovered signal domain, not an enlarged firmware resource limit. */
#define ORIGINAL_OSCILLATOR_GAIN_MAX 1.333299994468689f
typedef struct {uint32_t increment[4];float gain[4],width[4];uint8_t shape[4];float xmod;} original_osc_controls;
typedef struct {original_osc_controls controls;float correction[4],triangle[4];uint8_t ready;} original_prepared_oscillators;
/* Original control-rate preparation caches increment-derived coefficients.
 * Reprepare after any control/increment change. Prepared state is caller-owned
 * and immutable during a sample; direct field mutation is unsupported. */
int original_oscillator_prepare(original_prepared_oscillators *out,const original_osc_controls *controls);
int original_oscillator_sample_prepared(original_oscillators *s,const original_prepared_oscillators *c,unsigned sync,float *output);
/* Same immutable waveform preparation, with current audio-rate gain ramps.
 * Updating gains alone must not repeat increment-derived divisions or waveform
 * preparation. Inputs are physical [0, ORIGINAL_OSCILLATOR_GAIN_MAX] gains. */
int original_oscillator_sample_gains(original_oscillators *s,const original_prepared_oscillators *c,const float current_gain[4],unsigned sync,float *output);
/* Convenience checked preparation+sample reference; not the real-time route. */
/* sync=0 Miami double XMod, sync=1 Chicago SYNC/XMod; each independently traced.
 * Controls immutable for the call; output/state unchanged on invalid input. */
int original_oscillator_sample(original_oscillators *s,const original_osc_controls *c,unsigned sync,float *output);
int original_waveform(unsigned shape,float phase,uint32_t increment,float width,float *out);
/* Native public rate ordinal 0..7. Read waveform from old phase before advance.
 * Explicit input phase avoids guessing original unison initialization order. */
int original_mg_tempo(float *phase,unsigned rate,float bpm,float fs,uint32_t duration,unsigned *wrapped);
/* Four explicit current/target glide pitches; finite 0..127 (MIDI notes; final oscillator pitch clamps to 120). */
int original_glide(float current[4],const float target[4],float portamento,float modulation,float fs,uint32_t duration);
/* Original signed integer lookup requires verified 128-entry table. */
int original_pitch_increment(float pitch,const int32_t table[128],uint32_t *out);
int original_xmod_coefficient(float native_depth,float modulation,float *out);
int original_pw_width(unsigned shape,float intensity,float source,float shadow_width,float mod_intensity,float mod_width,float *width);
#endif
