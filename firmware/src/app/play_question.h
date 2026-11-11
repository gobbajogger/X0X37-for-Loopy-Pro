/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef X0X_PLAY_QUESTION_H
#define X0X_PLAY_QUESTION_H

#include <stdint.h>

enum {
    X0X_PQ_ANSWER_NO = 1,
    X0X_PQ_PLAY_TOGGLE = 2,
    X0X_PQ_REDO = 3
};

/* Ordered actions for one PLAY tap. n is 0, 1 or 2. act[0] happens first.
 * This helper does not change audio, transport, the question or song rules.
 * Later UI code performs the actions. */
typedef struct {
    uint8_t n;
    uint8_t act[2];
} x0x_play_question_t;

/* question_open: a question is waiting for an answer.
 * play: PLAY was tapped. Any non-zero value is one tap, not extra toggles.
 * home_held: HOME is down, so the tap is the HOME+PLAY redo chord.
 * A question does not swallow PLAY: answer NO, then one PLAY toggle.
 * HOME+PLAY stays redo and does not answer or toggle.
 * No question: one PLAY toggle. No PLAY tap: no action. */
static inline x0x_play_question_t x0x_play_question_resolve(int question_open,
                                                           int play,
                                                           int home_held)
{
    x0x_play_question_t out = {0, {0, 0}};
    if (!play)
        return out;
    if (home_held) {
        out.n = 1;
        out.act[0] = X0X_PQ_REDO;
        return out;
    }
    if (question_open) {
        out.n = 2;
        out.act[0] = X0X_PQ_ANSWER_NO;
        out.act[1] = X0X_PQ_PLAY_TOGGLE;
        return out;
    }
    out.n = 1;
    out.act[0] = X0X_PQ_PLAY_TOGGLE;
    return out;
}

#endif
