/* SPDX-License-Identifier: GPL-3.0-only */
/* BF-032: independent ACCENT and SLIDE edits survive TB-3PO mutation. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "tb3po.h"

static unsigned checks, failures;
static void expect(int ok, const char *label, unsigned seed)
{
    checks++;
    if (ok)
        return;
    if (failures++ < 8u)
        fprintf(stderr, "FAIL seed=%u: %s\n", seed, label);
}

static void preset(bpart_t *b, int next_note, int accented)
{
    memset(b, 0, sizeof *b);
    tb3po_defaults(&b->gen, 17u);
    b->len = 2;
    b->step[0].flags = (uint8_t)(G_NOTE | BS_SLIDE | (accented ? BS_ACCENT : 0));
    b->step[0].note = 60;
    b->step[0].velocity = 111;
    b->step[1].flags = (uint8_t)(next_note ? G_NOTE : G_REST);
    b->step[1].note = 67;
}

/* Matches only the first draw of real tb3po_mutate: the source step is not
 * randomly rewritten. Later draws remain the production algorithm's own. */
static int source_step_unchanged(uint32_t seed)
{
    uint32_t x = seed ? seed : 1u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return (x & 0xFFFFFFu) >= 0x400000u;   /* >= 0.25 */
}

int main(void)
{
    bpart_t b;
    uint32_t rng;
    unsigned eligible = 0, cleared = 0, sustained = 0;

    /* A direct, reproducible consumer case. Step 1 is an accented sliding
     * NOTE; step 2 is a REST. The slide has no target, but its ACCENT does. */
    preset(&b, 0, 1);
    rng = 16;
    tb3po_mutate(&b, &rng);
    expect(b.step[0].flags == (G_NOTE | BS_ACCENT), "orphan slide cleanup preserves ACCENT", 16);
    expect(b.step[0].note == 60 && b.step[0].velocity == 111,
           "manual note and velocity unchanged", 16);
    expect(b.step[1].flags == G_REST, "rest remains silent", 16);
    expect(rng == 1082410067u, "mutation RNG stream unchanged", 16);

    /* Positive preservation: a genuine next NOTE retains both user-edited
     * ACCENT and SLIDE. No slide change is required in this case. */
    preset(&b, 1, 1);
    rng = 16;
    tb3po_mutate(&b, &rng);
    expect(b.step[0].flags == (G_NOTE | BS_ACCENT | BS_SLIDE),
           "valid slide and accent retained", 16);
    expect(b.step[1].flags == G_NOTE, "slide target still sounds", 16);

    /* Normal unaccented slide cleanup still removes only SLIDE. */
    preset(&b, 0, 0);
    rng = 16;
    tb3po_mutate(&b, &rng);
    expect(b.step[0].flags == G_NOTE, "unaccented orphan slide still cleared", 16);

    /* Explore many deterministic mutation streams on the *real C function*.
     * Only assert when the source was not selected and the following step
     * still is REST; no legitimate kind re-roll is mistaken for a defect. */
    for (uint32_t seed = 1; seed <= 4096; seed++) {
        if (!source_step_unchanged(seed))
            continue;
        eligible++;
        preset(&b, 0, 1);
        rng = seed;
        tb3po_mutate(&b, &rng);
        if (b.step[1].flags == G_REST) {
            cleared++;
            expect(b.step[0].flags == (G_NOTE | BS_ACCENT),
                   "ACCENT retained when mutation removes invalid SLIDE", seed);
            expect(b.step[0].note == 60 && b.step[0].velocity == 111,
                   "unselected source step note and velocity stable", seed);
        }
        preset(&b, 1, 1);
        rng = seed;
        tb3po_mutate(&b, &rng);
        if (b.step[1].flags == G_NOTE) {
            sustained++;
            expect(b.step[0].flags == (G_NOTE | BS_ACCENT | BS_SLIDE),
                   "ACCENT plus valid SLIDE remain independent", seed);
        }
    }
    expect(eligible > 1000u && cleared > 100u && sustained > 100u,
           "deterministic cases cover valid and orphan slides", 0);

    /* Generator seeds and settings remain deterministic and produce valid
     * gates: the correction must not enter the generation path. */
    for (uint32_t seed = 1; seed <= 64; seed++) {
        bpart_t first, second;
        memset(&first, 0, sizeof first);
        tb3po_defaults(&first.gen, seed);
        first.len = 16;
        tb3po_generate(&first);
        second = first;
        tb3po_generate(&second);
        expect(memcmp(&first, &second, sizeof first) == 0,
               "seeded generation remains deterministic", seed);
        for (unsigned i = 0; i < first.len; i++)
            expect(bstep_gate(&first.step[i]) <= G_TIE,
                   "generated gate is within the maintained range", seed);
    }

    printf("BF-032: %u/%u checks passed, eligible=%u orphan=%u valid=%u\n",
           checks - failures, checks, eligible, cleared, sustained);
    return failures ? 1 : 0;
}
