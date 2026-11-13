/* SPDX-License-Identifier: GPL-3.0-only
 * Per-pattern synth-slot preset in the existing one-byte rsv[voice] field.
 * Never change the project stream layout for an engine choice.
 *
 * Historical encodings are immutable:
 *     0             = no recall
 *     1..81         = Chicago factory (one-based)
 *     129..158      = Miami factory (0x80 | one-based)
 * Drone occupies the formerly unused 0xC1..0xC4 codes. Codes outside these
 * ranges are malformed, not guessed; Juno occupies 0xE1..0xE4. Other codes remain invalid.
 */
#pragma once
#include <stdint.h>
#include "original_sound.h"

static inline unsigned bass_pattern_preset_encode(unsigned kind, unsigned one_based)
{
    if (!one_based) return 0u;
    if (one_based > original_sound_factories(kind)) return 0u;
    if (kind == ORIGINAL_SOUND_JUNO) return 0xE0u | one_based;
    if (kind == ORIGINAL_SOUND_DRONE) return 0xC0u | one_based;
    if (kind == ORIGINAL_SOUND_CHICAGO || kind == ORIGINAL_SOUND_MIAMI)
        return (kind << 7) | one_based;
    return 0u;
}

static inline int bass_pattern_preset_decode(unsigned code, unsigned *kind, unsigned *one_based)
{
    if (!kind || !one_based || code > 255u) return 0;
    if (!code) { *kind = *one_based = 0u; return 1; }
    unsigned juno = (code & 0xE0u) == 0xE0u;
    unsigned drone = (code & 0xC0u) == 0xC0u;
    unsigned k = juno ? ORIGINAL_SOUND_JUNO : drone ? ORIGINAL_SOUND_DRONE : code >> 7;
    unsigned p = juno ? code & 0x1Fu : drone ? code & 0x3Fu : code & 0x7Fu;
    if (!p || p > original_sound_factories(k)) return 0;
    *kind = k; *one_based = p;
    return 1;
}

