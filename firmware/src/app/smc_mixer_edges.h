/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>

/* Apply successive transport presses to the *pending* pattern, not the
 * currently sounding one. -2 is out of range, -1 cancels the pending cue.
 * Returning the playing pattern as another cue would restart it at a bar. */
static inline int smc_pattern_delta(int playing, int pending, int delta, int patterns)
{
    int start = pending >= 0 && pending < patterns ? pending : playing;
    int next = start + delta;
    if (next < 0 || next >= patterns) return -2;
    return next == playing ? -1 : next;
}

/* Millisecond deadline comparisons must survive a wrapping hardware clock;
 * an action still pressed retains its contextual view past the deadline. */
static inline int smc_ui_edge_alive(uint32_t now, uint32_t until, int held)
{
    return held || (int32_t)(now - until) < 0;
}
