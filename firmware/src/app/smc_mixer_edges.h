/* SPDX-License-Identifier: GPL-3.0-only */
/* Pure SMC input edges shared by the app and native host regressions.
 * No audio/engine or external-header dependencies. */
#pragma once
#include <stdint.h>

/* A new press follows the pending cue until it returns to the playing
 * pattern. -1 means clear cue; -2 means boundary (no change). */
static inline int smc_pattern_delta(int playing, int pending, int delta, int patterns)
{
    int from = pending >= 0 && pending < patterns ? pending : playing;
    int to = from + delta;
    if (to < 0 || to >= patterns) return -2;
    return to == playing ? -1 : to;
}

/* Physical SMC buttons have note-based down/up pairing. Exclude CC and
 * unsupplied notes, and retain ownership until actual key-up or loss. */
static inline int smc_hold_note(unsigned note)
{
    return note < 32u || (note >= 0x2Eu && note <= 0x31u) ||
           (note >= 0x5Bu && note <= 0x63u);
}

static inline int smc_hold_press(uint8_t held[16], unsigned note)
{
    if (!smc_hold_note(note)) return 1;
    uint8_t bit = (uint8_t)(1u << (note & 7u));
    uint8_t *slot = &held[note >> 3];
    if (*slot & bit) return 0;
    *slot |= bit;
    return 1;
}

static inline int smc_hold_release(uint8_t held[16], unsigned note)
{
    if (!smc_hold_note(note)) return 0;
    uint8_t bit = (uint8_t)(1u << (note & 7u));
    uint8_t *slot = &held[note >> 3];
    int was_held = !!(*slot & bit);
    *slot &= (uint8_t)~bit;
    return was_held;
}

static inline int smc_hold_any(const uint8_t held[16])
{
    for (unsigned i = 0; i < 16; i++) if (held[i]) return 1;
    return 0;
}

/* A full input ring may lose key-up. Treat every tracked held note as a
 * release class so main-loop loss recovery clears stale UI and button state. */
static inline int smc_tracked_release(uint8_t st, uint8_t note, uint8_t velocity)
{
    uint8_t type = st & 0xF0u;
    return !(note & 0x80u) && !(velocity & 0x80u) &&
           smc_hold_note(note) &&
           (type == 0x80u || (type == 0x90u && !velocity));
}

/* Millisecond inactivity deadline, including a wrapping clock. */
static inline int smc_ui_edge_alive(uint32_t now, uint32_t until, int holding)
{
    return holding || (int32_t)(now - until) < 0;
}
