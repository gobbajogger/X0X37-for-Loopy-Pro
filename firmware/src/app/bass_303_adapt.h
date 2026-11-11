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

static inline void bass_303_accent_output(int accent, float *output)
{
    if (!accent || !output)
        return;
    /* The Chicago voice gain stage rejects output > 1 (voice_gain.c), and a
     * rejected render faults the part: keep the accent inside 0..1. */
    *output = fm_clampf(*output * bass_303_accent_gain(1), 0.0f, 1.0f);
}

static inline void bass_303_accent_controls(int accent, float *cutoff, float *env_depth)
{
    if (!accent || !cutoff || !env_depth)
        return;
    *env_depth = fm_clampf(*env_depth + BASS_303_ACCENT_AMT * 0.5f, -1.0f, 1.0f);
    *cutoff = fm_clampf(*cutoff + BASS_303_ACCENT_AMT * 0.08f, 0.0f, 1.0f);
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
