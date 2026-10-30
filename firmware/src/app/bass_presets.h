/* SPDX-License-Identifier: GPL-3.0-only */
/* Exact original factory -> OS cell, or the legacy profile's AU byte decoder.
 * No DSP/storage writes. */
#pragma once
#include "engine.h"
#include "factory_banks.h"
#include "original_sound.h"

typedef const x0x_param_t *(*bass_preset_param_fn)(int bank, int param);
enum { BASS_PRESET_OK = 0, BASS_PRESET_INVALID = -1, BASS_PRESET_UNAVAILABLE = -2 };

static inline int bass_preset_decode_record(int bank, const factory_preset_t *p,
                                            bass_preset_param_fn param, uint8_t out[NPARAMS_MAX])
{
    /* Rounded WBL/legacy Chicago records have no meaning in the new profile. */
    (void)bank; (void)p; (void)param; (void)out;
    return BASS_PRESET_INVALID;
}
static inline int bass_preset_decode(int bank, int index, bass_preset_param_fn param,
                                     uint8_t out[NPARAMS_MAX])
{
    if (bank < 0 || index < 0 || !param || !out) return BASS_PRESET_INVALID;
    unsigned count = original_sound_count((unsigned)bank);
    if (!count || (unsigned)index >= original_sound_factories((unsigned)bank)) return BASS_PRESET_INVALID;
    if (!param(bank, 0)) return BASS_PRESET_UNAVAILABLE;
    if (param(bank, (int)count)) return BASS_PRESET_INVALID;
    for (unsigned i = 0; i < count; ++i) {
        const x0x_param_t *d = param(bank, (int)i);
        if (!d || !d->name || d->max != 127 || d->def > d->max) return BASS_PRESET_INVALID;
    }
    return original_sound_init(out, (unsigned)bank, (unsigned)index) ? BASS_PRESET_OK : BASS_PRESET_INVALID;
}
