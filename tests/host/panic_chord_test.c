/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include "app/panic_chord.h"

typedef struct {
    unsigned held;
    int mode;
    unsigned expected;
} step_t;

static unsigned checks;
static unsigned failures;

static void check(const char *trace, unsigned index, x0x_panic_chord_t *state,
                  unsigned held, int mode, unsigned expected)
{
    unsigned actual = x0x_panic_chord_update(state, held, mode);
    ++checks;
    if (actual != expected) {
        ++failures;
        fprintf(stderr, "%s[%u]: held=%u mode=%d expected=%u actual=%u\n",
                trace, index, held, mode, expected, actual);
    }
}

static void trace(const char *name, const step_t *steps, unsigned count)
{
    x0x_panic_chord_t state = {0};
    unsigned i;
    for (i = 0; i < count; ++i)
        check(name, i, &state, steps[i].held, steps[i].mode, steps[i].expected);
}

#define TRACE(a) trace(#a, a, (unsigned)(sizeof(a) / sizeof((a)[0])))

int main(void)
{
    static const step_t keyboard_first_then_second[] = {
        {0, X0X_PANIC_KEYBOARD, 0},
        {1, X0X_PANIC_KEYBOARD, 0},
        {1, X0X_PANIC_KEYBOARD, 0},
        {3, X0X_PANIC_KEYBOARD, 3},
        {3, X0X_PANIC_KEYBOARD, 2},
        {2, X0X_PANIC_KEYBOARD, 2},
        {0, X0X_PANIC_KEYBOARD, 2},
        {0, X0X_PANIC_KEYBOARD, 0}
    };
    static const step_t daw_second_then_first[] = {
        {2, X0X_PANIC_SMC_DAW, 0},
        {3, X0X_PANIC_SMC_DAW, 3},
        {1, X0X_PANIC_SMC_DAW, 2},
        {0, X0X_PANIC_SMC_DAW, 2},
        {0, X0X_PANIC_SMC_DAW, 0}
    };
    static const step_t rearm_requires_both_up[] = {
        {3, X0X_PANIC_KEYBOARD, 3},
        {1, X0X_PANIC_KEYBOARD, 2},
        {3, X0X_PANIC_KEYBOARD, 2},
        {2, X0X_PANIC_KEYBOARD, 2},
        {3, X0X_PANIC_KEYBOARD, 2},
        {0, X0X_PANIC_KEYBOARD, 2},
        {3, X0X_PANIC_KEYBOARD, 3},
        {0, X0X_PANIC_KEYBOARD, 2}
    };
    static const step_t single_members_pass[] = {
        {1, X0X_PANIC_KEYBOARD, 0},
        {0, X0X_PANIC_KEYBOARD, 0},
        {2, X0X_PANIC_KEYBOARD, 0},
        {0, X0X_PANIC_KEYBOARD, 0},
        {1, X0X_PANIC_SMC_DAW, 0},
        {0, X0X_PANIC_SMC_DAW, 0},
        {2, X0X_PANIC_SMC_DAW, 0},
        {0, X0X_PANIC_SMC_DAW, 0}
    };
    static const step_t user_and_mmc_never_fire[] = {
        {1, X0X_PANIC_SMC_USER, 0},
        {3, X0X_PANIC_SMC_USER, 0},
        {3, X0X_PANIC_SMC_USER, 0},
        {0, X0X_PANIC_SMC_USER, 0},
        {2, X0X_PANIC_SMC_MMC, 0},
        {3, X0X_PANIC_SMC_MMC, 0},
        {0, X0X_PANIC_SMC_MMC, 0}
    };
    static const step_t disabled_hold_cannot_become_daw_chord[] = {
        {1, X0X_PANIC_SMC_USER, 0},
        {1, X0X_PANIC_SMC_DAW, 0},
        {3, X0X_PANIC_SMC_DAW, 0},
        {2, X0X_PANIC_SMC_DAW, 0},
        {3, X0X_PANIC_SMC_DAW, 0},
        {0, X0X_PANIC_SMC_DAW, 0},
        {3, X0X_PANIC_SMC_DAW, 3},
        {0, X0X_PANIC_SMC_DAW, 2}
    };
    static const step_t disabled_mode_cancels_consumption[] = {
        {3, X0X_PANIC_SMC_DAW, 3},
        {3, X0X_PANIC_SMC_MMC, 0},
        {1, X0X_PANIC_SMC_DAW, 0},
        {3, X0X_PANIC_SMC_DAW, 0},
        {0, X0X_PANIC_SMC_MMC, 0},
        {3, X0X_PANIC_SMC_DAW, 3},
        {0, X0X_PANIC_SMC_USER, 0},
        {0, X0X_PANIC_SMC_DAW, 0},
        {3, X0X_PANIC_SMC_DAW, 3}
    };
    static const step_t invalid_modes_are_disabled[] = {
        {3, -1, 0},
        {3, X0X_PANIC_KEYBOARD, 0},
        {0, -1, 0},
        {3, X0X_PANIC_KEYBOARD, 3},
        {1, 999, 0},
        {3, X0X_PANIC_KEYBOARD, 0},
        {0, 999, 0},
        {3, X0X_PANIC_KEYBOARD, 3}
    };
    static const step_t unrelated_bits_are_ignored[] = {
        {4, X0X_PANIC_KEYBOARD, 0},
        {5, X0X_PANIC_KEYBOARD, 0},
        {7, X0X_PANIC_KEYBOARD, 3},
        {255, X0X_PANIC_KEYBOARD, 2},
        {6, X0X_PANIC_KEYBOARD, 2},
        {4, X0X_PANIC_KEYBOARD, 2},
        {4, X0X_PANIC_KEYBOARD, 0},
        {7, X0X_PANIC_KEYBOARD, 3}
    };
    static const step_t disabled_unrelated_bits_do_not_disarm[] = {
        {4, X0X_PANIC_SMC_USER, 0},
        {7, X0X_PANIC_SMC_DAW, 3}
    };
    x0x_panic_chord_t peer_a = {0}, peer_b = {0};

    TRACE(keyboard_first_then_second);
    TRACE(daw_second_then_first);
    TRACE(rearm_requires_both_up);
    TRACE(single_members_pass);
    TRACE(user_and_mmc_never_fire);
    TRACE(disabled_hold_cannot_become_daw_chord);
    TRACE(disabled_mode_cancels_consumption);
    TRACE(invalid_modes_are_disabled);
    TRACE(unrelated_bits_are_ignored);
    TRACE(disabled_unrelated_bits_do_not_disarm);

    check("peer-a", 0, &peer_a, 1, X0X_PANIC_SMC_DAW, 0);
    check("peer-b", 0, &peer_b, 2, X0X_PANIC_SMC_DAW, 0);
    check("peer-a", 1, &peer_a, 1, X0X_PANIC_SMC_DAW, 0);
    check("peer-b", 1, &peer_b, 2, X0X_PANIC_SMC_DAW, 0);
    check("peer-a", 2, &peer_a, 3, X0X_PANIC_SMC_DAW, 3);
    check("peer-b", 2, &peer_b, 3, X0X_PANIC_SMC_DAW, 3);
    check("peer-a", 3, &peer_a, 0, X0X_PANIC_SMC_DAW, 2);
    check("peer-b", 3, &peer_b, 3, X0X_PANIC_SMC_DAW, 2);
    check("peer-a", 4, &peer_a, 3, X0X_PANIC_SMC_DAW, 3);
    check("peer-b", 4, &peer_b, 0, X0X_PANIC_SMC_DAW, 2);

    printf("panic chord: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
