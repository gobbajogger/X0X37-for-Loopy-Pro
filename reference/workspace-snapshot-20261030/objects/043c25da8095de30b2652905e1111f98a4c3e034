/* SPDX-License-Identifier: GPL-3.0-only */
/* BF-015: mutation must not silence a generated 303 phrase at zero density. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/tb3po.h"

static int active_steps(const bpart_t *b)
{
    int n = 0;
    for (int i = 0; i < b->len; ++i)
        n += bstep_gate(&b->step[i]) != G_REST;
    return n;
}

static int test_generated_anchor(int len, uint32_t seed)
{
    bpart_t b;
    memset(&b, 0, sizeof b);
    b.len = (uint8_t)len;
    tb3po_defaults(&b.gen, seed);
    b.gen.density = 0;
    b.gen.root = 7;
    b.gen.base_oct = 2;
    tb3po_generate(&b);
    if (active_steps(&b) != 1 || bstep_gate(&b.step[0]) != G_NOTE) {
        fprintf(stderr, "generator initial anchor failure len=%d seed=%u\n", len, seed);
        return 1;
    }
    uint32_t rng = seed;
    for (int k = 0; k < 4096; ++k) {
        uint32_t prev_rng = rng;
        tb3po_mutate(&b, &rng);
        if (rng == prev_rng) {
            fprintf(stderr, "mutation did not advance RNG len=%d seed=%u iteration=%d\n", len, seed, k);
            return 1;
        }
        if (active_steps(&b) == 0) {
            fprintf(stderr, "silent generated line len=%d seed=%u mutation=%d\n", len, seed, k);
            return 1;
        }
        if (bstep_gate(&b.step[0]) != G_NOTE || b.step[0].velocity != 0) {
            fprintf(stderr, "generator anchor invalid len=%d seed=%u mutation=%d\n", len, seed, k);
            return 1;
        }
    }
    printf("PASS anchor len=%d seed=%u 4096 mutations\n", len, seed);
    return 0;
}

static int test_manual_rest(void)
{
    bpart_t b;
    memset(&b, 0, sizeof b);
    b.len = 1;
    tb3po_defaults(&b.gen, 91);
    b.gen.density = 0;
    b.step[0].flags = G_REST;    /* user explicitly removed the last note */
    uint32_t rng = 91;
    for (int k = 0; k < 512; ++k) {
        tb3po_mutate(&b, &rng);
        if (active_steps(&b) != 0) {
            fprintf(stderr, "manual rest unexpectedly made audible at mutation %d\n", k);
            return 1;
        }
    }
    puts("PASS manually cleared line remains silent");
    return 0;
}

static int test_dense(void)
{
    bpart_t b;
    memset(&b, 0, sizeof b);
    b.len = NSTEPS;
    tb3po_defaults(&b.gen, 113);
    b.gen.density = 100;
    tb3po_generate(&b);
    uint32_t rng = 113;
    for (int k = 0; k < 512; ++k) {
        tb3po_mutate(&b, &rng);
        if (active_steps(&b) != NSTEPS) {
            fprintf(stderr, "dense line lost note at mutation %d\n", k);
            return 1;
        }
    }
    puts("PASS full-density boundary");
    return 0;
}

int main(void)
{
    if (test_generated_anchor(1, 1) ||
        test_generated_anchor(16, 91) ||
        test_generated_anchor(NSTEPS, 113) ||
        test_manual_rest() || test_dense())
        return 1;
    puts("PASS BF-015 all cases");
    return 0;
}
