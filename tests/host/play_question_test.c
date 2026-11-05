/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include "app/play_question.h"

typedef struct {
    int question_open;
    int play;
    int home_held;
    uint8_t n;
    uint8_t act0;
    uint8_t act1;
} step_t;

static unsigned checks;
static unsigned failures;

static void check(const char *trace, unsigned index, int question_open, int play,
                  int home_held, uint8_t n, uint8_t act0, uint8_t act1)
{
    x0x_play_question_t actual =
        x0x_play_question_resolve(question_open, play, home_held);
    ++checks;
    if (actual.n != n || actual.act[0] != act0 || actual.act[1] != act1) {
        ++failures;
        fprintf(stderr,
                "%s[%u]: question=%d play=%d home=%d "
                "expected n=%u [%u,%u] actual n=%u [%u,%u]\n",
                trace, index, question_open, play, home_held,
                n, act0, act1, actual.n, actual.act[0], actual.act[1]);
    }
}

static void trace(const char *name, const step_t *steps, unsigned count)
{
    unsigned i;
    for (i = 0; i < count; ++i)
        check(name, i, steps[i].question_open, steps[i].play, steps[i].home_held,
              steps[i].n, steps[i].act0, steps[i].act1);
}

#define TRACE(a) trace(#a, a, (unsigned)(sizeof(a) / sizeof((a)[0])))

/* Count of one action, and whether ANSWER_NO is strictly before PLAY_TOGGLE. */
static void check_order(const char *trace, int question_open, int play, int home_held,
                        unsigned expect_no, unsigned expect_toggle, unsigned expect_redo,
                        int no_before_toggle)
{
    x0x_play_question_t actual =
        x0x_play_question_resolve(question_open, play, home_held);
    unsigned no = 0, toggle = 0, redo = 0, i;
    int no_at = -1, toggle_at = -1;
    ++checks;
    for (i = 0; i < actual.n && i < 2u; ++i) {
        if (actual.act[i] == X0X_PQ_ANSWER_NO) {
            ++no;
            no_at = (int)i;
        } else if (actual.act[i] == X0X_PQ_PLAY_TOGGLE) {
            ++toggle;
            toggle_at = (int)i;
        } else if (actual.act[i] == X0X_PQ_REDO) {
            ++redo;
        }
    }
    if (no != expect_no || toggle != expect_toggle || redo != expect_redo ||
        (no_before_toggle && !(no_at >= 0 && toggle_at > no_at)) ||
        (!no_before_toggle && no_at >= 0 && toggle_at >= 0 && no_at > toggle_at &&
         expect_no && expect_toggle)) {
        ++failures;
        fprintf(stderr,
                "%s: question=%d play=%d home=%d "
                "no=%u toggle=%u redo=%u no_at=%d toggle_at=%d\n",
                trace, question_open, play, home_held,
                no, toggle, redo, no_at, toggle_at);
    }
}

int main(void)
{
    static const step_t question_play_answers_no_then_one_toggle[] = {
        {1, 1, 0, 2, X0X_PQ_ANSWER_NO, X0X_PQ_PLAY_TOGGLE}
    };
    static const step_t home_play_is_redo_with_question[] = {
        {1, 1, 1, 1, X0X_PQ_REDO, 0}
    };
    static const step_t home_play_is_redo_without_question[] = {
        {0, 1, 1, 1, X0X_PQ_REDO, 0}
    };
    static const step_t no_question_play_toggles_once[] = {
        {0, 1, 0, 1, X0X_PQ_PLAY_TOGGLE, 0}
    };
    static const step_t play_absent_does_nothing[] = {
        {1, 0, 0, 0, 0, 0},
        {1, 0, 1, 0, 0, 0},
        {0, 0, 1, 0, 0, 0},
        {0, 0, 0, 0, 0, 0}
    };
    static const step_t nonzero_play_is_still_one_toggle[] = {
        {0, 2, 0, 1, X0X_PQ_PLAY_TOGGLE, 0},
        {1, 2, 0, 2, X0X_PQ_ANSWER_NO, X0X_PQ_PLAY_TOGGLE},
        {1, 2, 4, 1, X0X_PQ_REDO, 0}
    };

    TRACE(question_play_answers_no_then_one_toggle);
    TRACE(home_play_is_redo_with_question);
    TRACE(home_play_is_redo_without_question);
    TRACE(no_question_play_toggles_once);
    TRACE(play_absent_does_nothing);
    TRACE(nonzero_play_is_still_one_toggle);

    check_order("no-before-toggle", 1, 1, 0, 1, 1, 0, 1);
    check_order("redo-keeps-question-chord", 1, 1, 1, 0, 0, 1, 0);
    check_order("plain-play-one-toggle", 0, 1, 0, 0, 1, 0, 0);

    /* A second identical call stays one toggle. The helper holds no memory. */
    check("repeat", 0, 0, 1, 0, 1, X0X_PQ_PLAY_TOGGLE, 0);
    check("repeat", 1, 0, 1, 0, 1, X0X_PQ_PLAY_TOGGLE, 0);
    check("repeat-question", 0, 1, 1, 0, 2, X0X_PQ_ANSWER_NO, X0X_PQ_PLAY_TOGGLE);
    check("repeat-question", 1, 1, 1, 0, 2, X0X_PQ_ANSWER_NO, X0X_PQ_PLAY_TOGGLE);

    printf("play question: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
