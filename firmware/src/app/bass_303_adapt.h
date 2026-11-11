/* SPDX-License-Identifier: GPL-3.0-only */
/* 303-style accent, slide and global Tune helpers, adapted from bass303
 * (c4ecd248). No new Chicago param slots (the 32-byte cell already uses 22).
 * Velocity-to-cutoff is not implemented: the real Chicago does not have it. */
#pragma once
#include <stdint.h>
#include "../dsp/fastmath.h"

enum { BASS_303_ACCENT_POT = 64 };
#define BASS_303_ACCENT_AMT ((float)BASS_303_ACCENT_POT * (1.0f / 127.0f))
/* Chicago portamento that slews about 60 ms (stock bass303 Slide 21) in a
 * typical 64-sample span. Only applied when the step's slide flag is set. */
#define BASS_303_SLIDE_PORTA 0.22f

/* Fixed VCA/output scale, adapted from bass303 env add 0.45+4*acc. */
static inline float bass_303_accent_gain(int accent)
{
    if (!accent)
        return 1.0f;
    return 1.0f + 0.45f + 4.0f * BASS_303_ACCENT_AMT;
}

/* Accent works like a velocity layer: an accented note is always
 * bass_303_accent_gain(1) louder than the same note unaccented, on every
 * preset. The Chicago voice gain stage takes output 0..1 only (voice_gain.c;
 * more faults the part), so the part of the accent that fits stays in output
 * (bit-exact with the plain multiply) and the rest is returned as make-up gain
 * for after the voice. Unaccented notes: output unchanged, make-up 1. */
static inline float bass_303_accent_output(int accent, float *output)
{
    float want;
    if (!accent || !output)
        return 1.0f;
    want = *output * bass_303_accent_gain(1);
    if (want <= 1.0f) {
        *output = want;
        return 1.0f;
    }
    *output = 1.0f;
    return want;            /* want / 1.0f */
}

/* Apply make-up gain after the voice. *gain slews linearly to target by at
 * most BASS_303_MAKEUP_SLEW per sample (the full accent range in about 64
 * samples, ~1.5 ms; no step at a sub-block edge of any length), then holds.
 * Settled at 1: no work, output bit-exact. */
#define BASS_303_MAKEUP_SLEW ((bass_303_accent_gain(1) - 1.0f) * (1.0f / 64.0f))
static inline void bass_303_accent_makeup(float *left, float *right, unsigned n,
                                          float *gain, float target)
{
    unsigned i = 0;
    float g;
    if (!left || !right || !gain)
        return;
    g = *gain;
    if (g == 1.0f && target == 1.0f)
        return;
    for (; i < n && g != target; ++i) {
        float d = target - g;
        g = d > BASS_303_MAKEUP_SLEW ? g + BASS_303_MAKEUP_SLEW
          : d < -BASS_303_MAKEUP_SLEW ? g - BASS_303_MAKEUP_SLEW : target;
        left[i] *= g;
        right[i] *= g;
    }
    if (g != 1.0f)
        for (; i < n; ++i) {
            left[i] *= g;
            right[i] *= g;
        }
    *gain = g;
}

static inline void bass_303_accent_controls(int accent, float *cutoff, float *env_depth)
{
    if (!accent || !cutoff || !env_depth)
        return;
    *env_depth = fm_clampf(*env_depth + BASS_303_ACCENT_AMT * 0.5f, -1.0f, 1.0f);
    *cutoff = fm_clampf(*cutoff + BASS_303_ACCENT_AMT * 0.08f, 0.0f, 1.0f);
}

/* Accent decay. TB-303: the main (filter) envelope decays in 200 ms..2 s by the
 * DECAY pot on unaccented notes; on accented notes it is fixed at 200 ms, the
 * short end (Devil Fish manual, R. Whittle,
 * https://www.firstpr.com.au/rwi/dfish/Devil-Fish-Manual.pdf). Chicago's filter
 * EG decay d gives a time constant of 25*d^4 + 0.001 s (chicago_voice.c
 * vcf_tick); d = 0.29873 is 0.200 s. A preset whose decay is already shorter
 * keeps it. Unaccented notes: untouched. */
#define BASS_303_ACCENT_DECAY 0.29873f
static inline void bass_303_accent_decay(int accent, float *decay)
{
    if (accent && decay && *decay > BASS_303_ACCENT_DECAY)
        *decay = BASS_303_ACCENT_DECAY;
}

static inline float bass_303_tune_semitones(int8_t steps)
{
    if (!steps)
        return 0.0f;
    return 12.0f * fm_log2f((440.0f + (float)steps * 0.625f) * (1.0f / 440.0f));
}

static inline void bass_303_slide_porta(int slide, float *portamento)
{
    if (!slide || !portamento)
        return;
    if (*portamento < BASS_303_SLIDE_PORTA)
        *portamento = BASS_303_SLIDE_PORTA;
}
