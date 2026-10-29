/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_MS20_H
#define ORIGINAL_MIAMI_MS20_H
#include <stdint.h>
typedef struct {float current,target,delta;} original_ramp;
typedef struct {
 float history[5],b,cap;
 original_ramp a,resonance,noise_gain;
 uint32_t noise;
 uint8_t prepared;
} original_miami_ms20;
/* Authentic sample-rate-specific A/B/R and correctedcap arrays must each have
 *129finite entries. This unit does NOT invent platform libm table bits. */
typedef struct {const float *a,*b,*r,*cap;} original_ms20_tables;
int original_miami_ms20_init(original_miami_ms20 *s,unsigned voice_index,float a0,float b0,float cap0);
/* Full note reset preserves noise history/gain; next prepare is mandatory. */
int original_miami_ms20_note_reset(original_miami_ms20 *s,float a0);
int original_miami_ms20_prepare(original_miami_ms20 *s,const original_ms20_tables *tables,
 float cutoff,float resonance,float env_depth,float env,float mod_depth,float mod_cutoff,
 float mod_resonance,float noise_level,float mod_noise,uint32_t duration);
int original_miami_ms20_sample(original_miami_ms20 *s,float authentic_osc_mix,float vco_gain,float *out);
/* Finite checked numerical probes for exact original N and MGshape equation.
 * MGphase advance/rate is NOT implemented by the waveform evaluator. */
int original_miami_nonlinear(float input,float *out);
int original_miami_mg_shape(unsigned ordinal,float phase,float *bipolar,float *unipolar);
#endif
