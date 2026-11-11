/* SPDX-License-Identifier: GPL-3.0-only */
/* WBL1 Miami knob labels and pot law for UI / SMC (product wobble voice). */
#pragma once
#include "../dsp/x0x_param.h"
#include "../dsp/wobble_config.h"
#if X0X_DEV_WOBBLE
#include "../../../experiments/wobble/wobble.h"

static const char *const W_WAVE_N[] = {"Tri", "Sqr"};
static const char *const W_DAMP_N[] = {"Off", "On"};

/* Indices match wobble_parameter / WOBBLE_EXPERIMENT_NPARAMS. */
static const x0x_param_t WOBBLE_P[W_PARAMETER_COUNT] = {
    { "CrWave", 1, 0, W_WAVE_N },
    { "CrLvl", 127, 89, 0 },
    { "Sub", 127, 0, 0 },
    { "CrPit", 127, 64, 0 },
    { "Glide", 127, 0, 0 },
    { "XmLvl", 127, 0, 0 },
    { "XmWave", 1, 0, W_WAVE_N },
    { "XmTune", 127, 64, 0 },
    { "XmDep", 127, 0, 0 },
    { "Cutoff", 127, 76, 0 },
    { "Crush", 127, 0, 0 },
    { "WobShp", 1, 0, W_WAVE_N },
    { "WobDep", 127, 0, 0 },
    { "WobRat", 127, 0, 0 },
    { "Output", 127, 64, 0 },
    { "Stereo", 127, 0, 0 },
    { "PBend", 127, 64, 0 },
    { "Damper", 1, 0, W_DAMP_N },
    { "AAtt", 127, 0, 0 },
    { "ADec", 127, 0, 0 },
    { "ASus", 127, 127, 0 },
    { "ARel", 127, 0, 0 },
    { "BassSpl", 127, 36, 0 }, /* 36/127 ≈ 0.285; product 60–250 Hz crossover */
};

static inline const x0x_param_t *wobble_param(int i)
{
    return (i >= 0 && i < (int)W_PARAMETER_COUNT) ? &WOBBLE_P[i] : 0;
}
#endif
