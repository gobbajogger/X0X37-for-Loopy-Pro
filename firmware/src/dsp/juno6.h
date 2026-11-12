/* SPDX-License-Identifier: GPL-3.0-only */
/* Six-voice Juno-style subtractive engine and one shared short-delay chorus.
 * Compact original equations informed by the Plaits analog oscillator/filter
 * design; no Plaits source is copied. */
#pragma once
#include <stdint.h>
#define JUNO6_VOICES 6u
#define JUNO6_CHORUS_BYTES 512u
#define JUNO6_CHORUS_SAMPLES 256u
typedef struct {
 float phase,increment,envelope,filter,filter_state,velocity;
 uint8_t note,gate,stage,reserved;
 uint32_t age;
} juno6_voice_t;
typedef struct {
 juno6_voice_t voice[JUNO6_VOICES];
 uint32_t age;
 float sample_rate,attack,decay,sustain,release,cutoff,resonance,pulse_width,sub_level;
 uint8_t waveform;
} juno6_t;
typedef struct {
 int16_t delay[JUNO6_CHORUS_SAMPLES];
 uint16_t write;
 uint32_t phase;
 float sample_rate,depth_ms,rate_hz,mix;
} juno6_chorus_t;
void juno6_init(juno6_t*,float);
int juno6_note_on(juno6_t*,uint8_t,uint8_t);
void juno6_note_off(juno6_t*,uint8_t);
void juno6_all_off(juno6_t*);
void juno6_render(juno6_t*,float*,unsigned);
void juno6_chorus_init(juno6_chorus_t*,float);
void juno6_chorus_set(juno6_chorus_t*,float,float,float);
void juno6_chorus_process(juno6_chorus_t*,float*,float*,unsigned);
