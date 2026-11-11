/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef X0X_PANIC_CHORD_H
#define X0X_PANIC_CHORD_H

#include <stdint.h>

enum {
    X0X_PANIC_FIRST = 1u, X0X_PANIC_SECOND = 2u,
    X0X_PANIC_FIRE = 1u, X0X_PANIC_CONSUME = 2u
};
enum {
    X0X_PANIC_KEYBOARD, X0X_PANIC_SMC_DAW,
    X0X_PANIC_SMC_USER, X0X_PANIC_SMC_MMC
};

/* Zero-initialize one state per physical keyboard/controller peer. */
typedef struct {
    uint8_t active, inhibited;
} x0x_panic_chord_t;

/* held is a two-bit snapshot: FX/SEQ for keyboard, BANK L/R for SMC.
 * Only keyboard and explicit DAW mode are eligible. SMC User raw NoteOn is
 * still User mode. Update on every member edge and mode change, even when
 * other routing consumed the event. Ignore unrelated held controls.
 * FIRE requests global notes-off; this helper never changes audio/transport.
 * CONSUME suppresses normal chord handling, including the final release.
 * Both members must be released before the next chord can fire. */
static inline unsigned x0x_panic_chord_update(x0x_panic_chord_t *state,
                                              unsigned held, int mode)
{
    unsigned consumed = state->active ? X0X_PANIC_CONSUME : 0u;
    held &= X0X_PANIC_FIRST | X0X_PANIC_SECOND;
    if (mode != X0X_PANIC_KEYBOARD && mode != X0X_PANIC_SMC_DAW) {
        state->active = 0;
        state->inhibited = (uint8_t)(held != 0u);
        return 0;
    }
    if (!held) {
        state->active = state->inhibited = 0;
        return consumed;
    }
    if (state->inhibited)
        return 0;
    if (state->active)
        return X0X_PANIC_CONSUME;
    if (held == (X0X_PANIC_FIRST | X0X_PANIC_SECOND)) {
        state->active = 1;
        return X0X_PANIC_FIRE | X0X_PANIC_CONSUME;
    }
    return 0;
}

#endif
