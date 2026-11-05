/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual pinned master versus candidate: compare every state and audio byte. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "dsp/master.h"
void reference_master_init(master_t *);
void reference_master_set(master_t *, int, int);
void reference_master_key(master_t *, int, float);
void reference_master_process(master_t *, float *, int, float);
void reference_master_process_st(master_t *, float *, float *, int, float);
void reference_limit_step(master_t *, float);
void candidate_limit_step(master_t *, float);
static uint32_t seed = 12345;
static uint32_t random_word(void) { seed = seed * 1664525u + 1013904223u; return seed; }
static float sample(void) { return (float)(int32_t)random_word() * (4.0f / 2147483648.0f); }
static void same(const master_t *a, const master_t *b) { assert(memcmp(a, b, sizeof *a) == 0); }
int main(void)
{
    master_t a, b;
    /* Values on both sides of 0.8, signed zero, tiny inputs and changing holds. */
    const uint32_t bits[] = {0, 0x80000000u, 1, 0x00800000u, 0x3f4cccccu,
                            0x3f4ccccdu, 0x3f4cccceu, 0x3f800000u, 0x40800000u};
    for (unsigned i = 0; i < sizeof bits / sizeof *bits; ++i) {
        float x;
        memcpy(&x, &bits[i], sizeof x);
        reference_master_init(&a); master_init(&b);
        for (int j = 0; j < 200; ++j) {
            reference_limit_step(&a, x); candidate_limit_step(&b, x); same(&a, &b);
        }
    }
    for (int stereo = 0; stereo < 2; ++stereo)
    for (int mode = 0; mode < 4; ++mode)
    for (int comp = 0; comp < 2; ++comp)
    for (int limiter = 0; limiter < 2; ++limiter) {
        reference_master_init(&a); master_init(&b);
        int params[] = {MST_MODE, MST_COMP1, MST_LIMIT, MST_PUMP, MST_MIX};
        int values[] = {mode, comp ? 110 : 0, limiter, comp ? 90 : 0, 77};
        for (unsigned j = 0; j < sizeof params / sizeof *params; ++j) {
            reference_master_set(&a, params[j], values[j]); master_set(&b, params[j], values[j]);
        }
        for (int block = 0; block < 200; ++block) {
            float l[256], r[256], cl[256], cr[256];
            int sizes[] = {0, 1, 3, 4, 63, 64, 65, 255, 256};
            int n = sizes[block % 9];
            /* Silence-in, signal, long tails, then a fresh attack. */
            for (int i = 0; i < 256; ++i) {
                l[i] = (block >= 10 && block < 45) || block >= 170 ? sample() : (i & 1 ? -0.0f : 0.0f);
                r[i] = (block >= 10 && block < 45) || block >= 170 ? sample() : 0.0f;
            }
            if (block == 10 || block == 170) { reference_master_key(&a, 0, 0.9f); master_key(&b, 0, 0.9f); }
            if (block == 25 || block == 160) {
                int cutoff = block == 25 ? 10 : 120;
                reference_master_set(&a, MST_CUTOFF, cutoff); master_set(&b, MST_CUTOFF, cutoff);
            }
            if (comp && block % 17 == 0) {
                /* Exercise complement rounding across the full pot range. */
                const int ids[] = {MST_ATTACK, MST_RELEASE, MST_PUMP, MST_MIX, MST_MAKEUP};
                for (unsigned j = 0; j < sizeof ids / sizeof *ids; ++j) {
                    int value = (int)(random_word() & 127u);
                    reference_master_set(&a, ids[j], value); master_set(&b, ids[j], value);
                }
            }
            memcpy(cl, l, sizeof l); memcpy(cr, r, sizeof r);
            float vol = block < 80 ? 0.9f : block < 160 ? 0.2f : 1.1f;
            if (stereo) {
                reference_master_process_st(&a, l, r, n, vol); master_process_st(&b, cl, cr, n, vol);
            } else { reference_master_process(&a, l, n, vol); master_process(&b, cl, n, vol); }
            assert(!memcmp(l, cl, sizeof l)); assert(!memcmp(r, cr, sizeof r)); same(&a, &b);
        }
    }
    puts("master state/audio silence-tail-restart equivalence: PASS");
}
