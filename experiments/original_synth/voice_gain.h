/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_VOICE_GAIN_H
#define ORIGINAL_VOICE_GAIN_H
#include "miami_ms20.h"
#include "oscillator_motion.h"
typedef struct {original_ramp oscillator[4],stereo[2];} original_voice_gain;
typedef struct {
 float level[4],mod_level,mod_specific[4];
 float envelope,mod_gain,output,velocity,pan,mod_pan,spread;
 uint8_t curve,unison_flag,ms20;
 float filter_gain;
} original_voice_gain_controls;
/* Construct/reset only: clears all gains. A Miami full note clears stereo
 * gains but retains oscillator currents. A partial note retains both. */
void original_voice_gain_init(original_voice_gain *s);
void original_voice_gain_miami_full_note(original_voice_gain *s);
/* output is the already converted native94 (normalized squared); pan is
 * converted bipolar93. filter_gain comes from the selected filter preparation:
 * 1 for Miami MS20, MP4's returned postgain for Chicago (can reach4).
 * Explicit effective controls, no guessed native modes.
 * duration comes from the actual original voice clock, not fixed64. */
int original_voice_gain_prepare(original_voice_gain *s,const original_voice_gain_controls *controls,const int32_t activation[4],uint32_t duration);
/* Connect current ramps to the prepared four-oscillator renderer, then advance
 * oscillator gain state AFTER mixing for the next sample. Stereo gains advance
 * separately BEFORE multiplying the current filtered sample. */
int original_voice_gain_oscillator(original_voice_gain *s,original_oscillators *phase,const original_prepared_oscillators *prepared,unsigned sync,float *out);
int original_voice_gain_stereo(original_voice_gain *s,float filtered,float *left,float *right);
#endif
