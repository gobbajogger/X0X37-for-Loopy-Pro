/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "dsp/drum808.h"
void reference_drum808_init(drum808_t *);
void reference_drum808_trigger(drum808_t *, int, float);
void reference_drum808_render_st(drum808_t *, float *, float *, float *, float *, int);
void reference_tail_step(drum808_t *, int, const float *, int);
void candidate_tail_step(drum808_t *, int, const float *, int);
static drum808_t a, b;
static uint32_t rng = 123;
static uint32_t next(void) { rng = rng * 1664525u + 1013904223u; return rng; }
static void same(void) { assert(!memcmp(&a, &b, sizeof a)); }
int main(void)
{
    /* Exercise tail thresholds with new peaks, exact ties, tiny values and silence. */
    for (int lane = 0; lane < 13; ++lane) {
        drum808_init(&a); drum808_init(&b);
        for (int block = 0; block < 500; ++block) {
            float x[32];
            for (int i = 0; i < 32; ++i) {
                uint32_t bits = next();
                /* All finite normal and subnormal magnitudes, both signs. */
                bits = (bits & 0x807fffffu) | ((next() % 150u) << 23);
                memcpy(&x[i], &bits, sizeof bits);
                if (block % 3 == 0) x[i] = a.lane[lane].pk * 1e-3f;
                if (block % 5 == 0) x[i] = i & 1 ? -0.0f : 0.0f;
            }
            reference_tail_step(&a, lane, x, block % 33);
            candidate_tail_step(&b, lane, x, block % 33); same();
        }
    }
    for (int track = 0; track < D8_NUM; ++track) {
        reference_drum808_init(&a); drum808_init(&b);
        for (int block = 0; block < 1200; ++block) {
            float l[256] = {0}, r[256] = {0}, rev[256] = {0}, dly[256] = {0};
            float cl[256] = {0}, cr[256] = {0}, cv[256] = {0}, cd[256] = {0};
            int sizes[] = {0, 1, 31, 32, 33, 63, 64, 127, 256};
            int n = sizes[block % 9];
            /* Initial silence, full tail to idle, then a lower velocity restart. */
            if (block == 4 || block == 1100) {
                float velocity = block == 4 ? 1.0f : 0.3f;
                reference_drum808_trigger(&a, track, velocity); drum808_trigger(&b, track, velocity);
            }
            reference_drum808_render_st(&a, l, r, rev, dly, n);
            drum808_render_st(&b, cl, cr, cv, cd, n);
            assert(!memcmp(l, cl, sizeof l) && !memcmp(r, cr, sizeof r));
            assert(!memcmp(rev, cv, sizeof rev) && !memcmp(dly, cd, sizeof dly)); same();
        }
    }
    puts("808 threshold/state/audio silence-tail-restart equivalence: PASS");
}
