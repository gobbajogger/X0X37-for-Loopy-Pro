/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_PITCH_H
#define ORIGINAL_MIAMI_PITCH_H
#include "oscillator_motion.h"
typedef struct {original_oscillators oscillator;float current[4],target[4];} original_miami_pitch;
typedef struct {
 float master_tune,unison_coordinate,unison_detune,mod_master_pitch,bend_range,bend;
 int transpose,semitone[4];
 float fine_tune[4],mod_pitch[4],detune,mod_detune,frequency_mod,mod_slave_pitch;
 float portamento,mod_portamento;
 uint8_t modulation_on;
} original_miami_pitch_controls;
/* Constructor alone clears all four phases. Selected Equal Temperament notes
 * supply MIDI0..127; original global Analog/MasterTune influence remains open. */
int original_miami_pitch_init(original_miami_pitch *s,unsigned last_note);
/* Full note uses explicit previous last-note anchor, partial preserves current.
 * Both preserve oscillator phases. This does not replace the voice allocator. */
int original_miami_pitch_note(original_miami_pitch *s,unsigned note,unsigned last_note,unsigned mask,unsigned full);
/* Selected Miami modulation Mode0: VCO2/4 get slave modulation, VCO3 does not.
 * Output increments feed accepted prepared oscillator state, not active engine.
 * Complete controls are immutable during this call; output/state transactional. */
int original_miami_pitch_prepare(original_miami_pitch *s,const original_miami_pitch_controls *c,const int32_t table[128],float fs,uint32_t duration,uint32_t out[4]);
#endif
