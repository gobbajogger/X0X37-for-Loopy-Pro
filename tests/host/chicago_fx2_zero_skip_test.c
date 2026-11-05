/* SPDX-License-Identifier: GPL-3.0-only */
/* Restored Chicago inserts (3/12/13/15/16): the exact silence skip must leave
 * every arena byte and every output sample identical to ordinary processing,
 * across random edits, tempo, bursts, tails and silence->signal transitions. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__SSE__) || defined(__x86_64__)
#include <xmmintrin.h>
#include <pmmintrin.h>
#define HAVE_FTZ 1
#endif
#include "../../experiments/original_synth/chicago_fx2_inserts.h"

static uint32_t rng;
static uint32_t rnd(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }

static chi_fx2_insert_arena A, B;

static unsigned long run(unsigned type, uint32_t seed, unsigned long *skips)
{
    static int16_t la[4096], ra[4096], lb[4096], rb[4096];
    unsigned long frames = 0;
    rng = seed * 2654435761u + 1u;
    chi_fx2_insert_init(&A, type);
    uint8_t e1 = (uint8_t)(rnd() % 128), e2 = (uint8_t)(rnd() % 128);
    int32_t bpm = 200 + (int32_t)(rnd() % 2800);
    int set_late = rnd() & 1;          /* some runs start unconfigured (wet 0) */
    if (!set_late) chi_fx2_insert_set(&A, type, e1, e2, bpm);
    memcpy(&B, &A, sizeof A);
    for (int seg = 0; seg < 24; ++seg) {
        int loud = seg & 1 ? 0 : (rnd() % 3 != 0);
        uint32_t len = loud ? 64u + rnd() % 6000u : (rnd() % 4 ? 200u + rnd() % 30000u : 150000u + rnd() % 150000u);
        int amp = 1 + (int)(rnd() % 32767u);
        if (set_late && seg == 3) { chi_fx2_insert_set(&A, type, e1, e2, bpm); chi_fx2_insert_set(&B, type, e1, e2, bpm); }
        if (type == 13 && seg == 9) { bpm = 200 + (int32_t)(rnd() % 2800); chi_fx2_insert_set(&A, 13, e1, e2, bpm); chi_fx2_insert_set(&B, 13, e1, e2, bpm); }
        while (len) {
            uint32_t n = rnd() % 16u ? 1u + rnd() % 512u : 1u + rnd() % 4096u, i;
            if (n > len) n = len;
            int zero = 1;
            for (i = 0; i < n; ++i) {
                if (loud) { la[i] = (int16_t)((int)(rnd() % (2u * amp + 1u)) - amp); ra[i] = (int16_t)((int)(rnd() % (2u * amp + 1u)) - amp); }
                else la[i] = ra[i] = 0;
                if (la[i] | ra[i]) zero = 0;
            }
            memcpy(lb, la, n * 2); memcpy(rb, ra, n * 2);
            chi_fx2_insert_process(&A, type, la, ra, n);
            if (zero && chi_fx2_insert_advance_zero(&B, type, n)) { memset(lb, 0, n * 2); memset(rb, 0, n * 2); ++*skips; }
            else chi_fx2_insert_process(&B, type, lb, rb, n);
            if (memcmp(la, lb, n * 2) || memcmp(ra, rb, n * 2) || memcmp(&A, &B, sizeof A)) {
                printf("FAIL type %u seed %u seg %d frame %lu\n", type, seed, seg, frames); exit(1);
            }
            frames += n; len -= n;
        }
    }
    return frames;
}

int main(int argc, char **argv)
{
    static const unsigned types[5] = { 3, 12, 13, 15, 16 };
    uint32_t seeds = argc > 1 ? (uint32_t)atoi(argv[1]) : 40;
    for (int mode = 0; mode < 2; ++mode) {
#ifdef HAVE_FTZ
        if (mode) { _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON); _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON); }
#else
        if (mode) break;
#endif
        for (int t = 0; t < 5; ++t) {
            unsigned long skips = 0, frames = 0;
            for (uint32_t s = 1; s <= seeds; ++s) frames += run(types[t], s, &skips);
            printf("%s type %2u: %u seeds, %lu frames identical, %lu skipped blocks\n",
                mode ? "ftz" : "ieee", types[t], seeds, frames, skips);
            if (!skips) { printf("FAIL type %u: skip never admitted\n", types[t]); return 1; }
        }
    }
    puts("chicago_fx2_zero_skip_test: OK");
    return 0;
}
