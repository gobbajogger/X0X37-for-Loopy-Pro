/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago pitch bend + damper (AU ids 20–21) — performance aux on native303.
 *
 * Stored in sound_t after SEND FX (CHI_PBEND / CHI_DAMPER). Pitch bend is a
 * live ±2 semitone offset around centre pot 64 — it does not rewrite TUNE.
 * The SMK pitch wheel is the live source: while it is off centre it writes
 * CHI_PBEND (not TUNE) and on release the saved pot returns. Damper (sustain)
 * defers live/MIDI note-off while pot ≥ 64; pattern gates ignore it.
 * Miami aux stays on WBL1 — not mapped here. */
#pragma once
#include <stdint.h>
#include "../dsp/x0x_param.h"
#include "chicago_fx.h" /* CHI_FX_END == 30 */

enum {
    CHI_PBEND = CHI_FX_END, /* 30 */
    CHI_DAMPER,
    CHI_AUX_END /* engine_nparams(T_303) == 32; fills NPARAMS_MAX */
};

enum { CHI_DAMP_OFF = 0, CHI_DAMP_ON = 1 };
/* Damper engages at pot ≥ 64 (AU 0.5). Factory banks leave damper at 0. */
#define CHI_DAMP_THRESH 64

/* Tables live in chicago_param_tables.c (one copy for engine.o + x0x.o). */
extern const x0x_param_t CHI_AUX_PARAMS[2];

/* AU / factory 0..255 → 0..127. */
static inline uint8_t chi_aux_from_byte(uint8_t v)
{
    return (uint8_t)(((uint32_t)v * 127u + 127u) / 255u);
}

static inline void chi_aux_decode_factory(const uint8_t pots[2], uint8_t out[2])
{
    out[0] = chi_aux_from_byte(pots[0]);
    out[1] = chi_aux_from_byte(pots[1]);
}

static inline void chi_aux_apply_factory_voice(uint8_t *voice_pots, const uint8_t factory_pots[2])
{
    uint8_t d[2];
    chi_aux_decode_factory(factory_pots, d);
    voice_pots[CHI_PBEND] = d[0];
    voice_pots[CHI_DAMPER] = d[1];
}

static inline int chi_damper_on(uint8_t pot)
{
    return pot >= CHI_DAMP_THRESH;
}

/* Semitone offset for pot 0..127 (±2 at the ends). */
static inline float chi_pbend_semitones(uint8_t pot)
{
    return ((float)((int)pot - 64) / 64.0f) * 2.0f;
}

/* Pitch-wheel deflection −63..63 (0 = spring centre) → CHI_PBEND pot.
 * 64 is no bend. The wheel does not add this onto TUNE. */
static inline int chi_pbend_from_wheel(int bend)
{
    if (bend < -63)
        bend = -63;
    if (bend > 63)
        bend = 63;
    return 64 + bend;
}
