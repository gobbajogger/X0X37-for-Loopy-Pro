/* SPDX-License-Identifier: GPL-3.0-only */
/*
 * Chicago AU Env ADSR → filter EG dual-drive.
 *
 * Chicago Acid AU 3.1.6 exposes one Env Attack/Decay/Sustain/Release. native303
 * splits amp ADSR (AAtt..ARel) from the TeeBee filter EG (FAttack/Decay/
 * FSustain/FRelease + FEnv). Leaving filter Decay at the Open303 default (64)
 * with FEnv=303 makes Gate-mode factory presets (amp held open) ignore Env
 * Attack/Sustain — the audible envelope is then only the wrong filter sweep.
 *
 * AU compare and factory decode mirror Env into both amp ADSR and filter ADSR
 * (FEnv = ADSR). Product UI pages stay independent; this helper only runs when
 * applying AU/factory Env bytes.
 */
#ifndef CHICAGO_ENV_H
#define CHICAGO_ENV_H

#include <stdint.h>
#include "../dsp/bass_param_ids.h"

/* AU/factory 0..255 → 0..max_pot (matches chicago_au_map._pot_from_byte). */
static inline uint8_t chi_env_pot_from_byte(uint8_t v, uint8_t max_pot)
{
    if (max_pot == 0)
        return 0;
    return (uint8_t)(((uint32_t)v * (uint32_t)max_pot + 127u) / 255u);
}

/*
 * After amp ADSR pots (AAtt/ADec/ASus/ARel) are set from AU Env bytes, mirror
 * them onto the filter EG and select FEnv ADSR mode.
 */
static inline void chi_env_mirror_amp_to_filter(uint8_t *voice_pots)
{
    if (!voice_pots)
        return;
    voice_pots[BASS303_FATTACK] = voice_pots[BASS303_AATTACK];
    voice_pots[BASS303_DECAY] = voice_pots[BASS303_ADECAY];
    voice_pots[BASS303_FSUSTAIN] = voice_pots[BASS303_ASUSTAIN];
    voice_pots[BASS303_FRELEASE] = voice_pots[BASS303_ARELEASE];
    voice_pots[BASS303_FENV] = 1; /* ADSR */
}

/* Apply AU Env Attack/Decay/Sustain/Release bytes (factory pots[7..10]) to amp
 * ADSR and mirrored filter EG on a 303 voice cell. */
static inline void chi_env_apply_factory_voice(uint8_t *voice_pots, const uint8_t env_bytes[4])
{
    if (!voice_pots || !env_bytes)
        return;
    voice_pots[BASS303_AATTACK] = chi_env_pot_from_byte(env_bytes[0], 127);
    voice_pots[BASS303_ADECAY] = chi_env_pot_from_byte(env_bytes[1], 127);
    voice_pots[BASS303_ASUSTAIN] = chi_env_pot_from_byte(env_bytes[2], 127);
    voice_pots[BASS303_ARELEASE] = chi_env_pot_from_byte(env_bytes[3], 127);
    chi_env_mirror_amp_to_filter(voice_pots);
}

#endif /* CHICAGO_ENV_H */
