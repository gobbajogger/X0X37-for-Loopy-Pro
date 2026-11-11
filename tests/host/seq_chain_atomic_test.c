/* SPDX-License-Identifier: GPL-3.0-only */
/* Interrupt the real chain writer and run the real ISR bar boundary. A torn
 * cancellation 3..6 -> 0..0 at pattern 6 must never select pattern 0. */
#include <stdio.h>
#include "../../firmware/src/seq/sequencer.h"
static void interrupt_store(seq_t *s);
#define SEQ_CHAIN_STORE_HOOK(s) interrupt_store(s)
#include "../../firmware/src/seq/sequencer.c"

static seq_t state;
static pattern_t patterns[NPAT];
static int armed, observed, fails;

static void interrupt_store(seq_t *s)
{
    if (!armed) return;
    armed = 0;
    (void)bar_end(s, 0);
    observed = s->ppat[TRK_DRUM];
}

int main(void)
{
    static const struct {
        int old_a, old_b, new_a, new_b, current, old_next, new_next;
    } cases[] = {
        {3, 6, 0, 0, 6, 3, 6}, /* cancel at upper endpoint: mixed 0..6 selects 0 */
        {0, 0, 3, 6, 0, 0, 3}, /* enable: mixed 3..0 selects 1 */
        {6, 3, 0, 0, 6, 3, 6}, /* reversed old chain */
        {3, 6, 8, 10, 6, 3, 8},
        {3, 6, 10, 8, 6, 3, 8}, /* reversed new chain */
    };
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        seq_init(&state, patterns);
        seq_chain(&state, cases[i].old_a, cases[i].old_b);
        state.playing = 1;
        for (int t = 0; t < NTRACKS; ++t) state.ppat[t] = (uint8_t)cases[i].current;
        armed = 1;
        observed = -1;
        seq_chain(&state, cases[i].new_a, cases[i].new_b);
        if (armed || (observed != cases[i].old_next && observed != cases[i].new_next)) {
            printf("FAIL case %u: interrupted next pattern %d; expected old %d or new %d\n",
                   i, observed, cases[i].old_next, cases[i].new_next);
            ++fails;
        }
        /* Once published, every part follows the new selection. */
        for (int t = 0; t < NTRACKS; ++t) state.ppat[t] = (uint8_t)cases[i].current;
        (void)bar_end(&state, 0);
        for (int t = 0; t < NTRACKS; ++t) {
            if (state.ppat[t] != cases[i].new_next) {
                printf("FAIL case %u track %d: new selection %u, expected %d\n",
                       i, t, state.ppat[t], cases[i].new_next);
                ++fails;
            }
        }
    }
    printf("chain atomicity: %d failures\n", fails);
    return fails != 0;
}
