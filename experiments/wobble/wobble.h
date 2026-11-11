/* SPDX-License-Identifier: GPL-3.0-only */
/* Original host experiment; not selected by any firmware build. */
#ifndef X0X_EXPERIMENT_WOBBLE_H
#define X0X_EXPERIMENT_WOBBLE_H
#include <stddef.h>
#include <stdint.h>
enum wobble_parameter {
    W_CARRIER_WAVE, W_CARRIER_LEVEL, W_SUB_LEVEL, W_CARRIER_PITCH,
    W_GLIDE, W_XMOD_LEVEL, W_XMOD_WAVE, W_XMOD_TUNE, W_XMOD_DEPTH,
    W_CUTOFF, W_CRUSH, W_WOBBLE_SHAPE, W_WOBBLE_DEPTH, W_WOBBLE_RATE,
    W_OUTPUT, W_STEREO, W_PITCH_BEND, W_DAMPER,
    /* Owner amp ADSR (not in Miami AU 3.1.6); defaults ≈ legacy 2 ms / hold / 30 ms */
    W_ATTACK, W_DECAY, W_SUSTAIN, W_RELEASE,
    W_BASS_SPLIT,
    W_PARAMETER_COUNT
};
typedef struct {
    float sample_rate, tempo_bpm, p[W_PARAMETER_COUNT];
    float carrier_phase, mod_phase, sub_phase, lfo_phase;
    float hz, target_hz, glide_step, lfo_step;
    float envelope, velocity, attack_step, decay_step, release_step, sustain_level;
    uint8_t env_stage; /* 0 attack 1 decay 2 sustain 3 release */
    float filter_low, filter_band, stereo_memory, prefilt_lp;
    float split_low, split_band, split2_low, split2_band;
    uint32_t active_samples;
    uint8_t note, gate, active, deferred_off;
} wobble_t;
void wobble_init(wobble_t *s, float sample_rate);
/* Reset restores fresh DSP state, preserving current parameter values. */
void wobble_reset(wobble_t *s);
/* Effective sequencer tempo, 20..300 BPM. Invalid input keeps the last tempo.
 * Defaults to 120 BPM; changes preserve phase, gates and all preset controls. */
int wobble_set_tempo(wobble_t *s, float bpm);
/* Public normalized controls; nonfinite values are rejected; values clamp 0..1. */
int wobble_set(wobble_t *s, unsigned parameter, float value);
void wobble_note_on(wobble_t *s, uint8_t note, uint8_t velocity);
/* Slide preserves amp envelope/stage only while an active gate is held.
 * Pitch follows W_GLIDE; the latest note and velocity take ownership. */
void wobble_note_on_slide(wobble_t *s, uint8_t note, uint8_t velocity, int slide);
/* Last-note ownership: unmatched old note-off is ignored; no held-note stack. */
void wobble_note_off(wobble_t *s, uint8_t note);
void wobble_all_notes_off(wobble_t *s);
/* Stereo planar buffers, no aliasing between left and right. n may be 0..256.
 * Mono downmix is exactly (left+right)*0.5; stereo=0 gives left==right.
 * No allocation, global state, random source, libm, clocks or device access. */
void wobble_render(wobble_t *s, float *left, float *right, size_t n);
float wobble_bass_split_hz(float norm);
float wobble_bass_split_alpha(float norm);
#endif
