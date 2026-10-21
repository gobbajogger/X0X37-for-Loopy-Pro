/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "drum808.h"
void baseline_drum808_init(drum808_t *);
void baseline_drum808_set(drum808_t *, int, int, int);
void baseline_drum808_trigger(drum808_t *, int, float);
void baseline_drum808_render_st(drum808_t *, float *, float *, float *, float *, int);
int main(void)
{
    unsigned cases = 0;
    for (int track = 0; track < D8_NUM; ++track)
    for (int param = 0; param < drum808_nparams(track); ++param)
    for (int value = 0; value <= drum808_param(track, param)->max; ++value) {
        drum808_t a, b;
        float out_a[4][64] = {{0}}, out_b[4][64] = {{0}};
        memset(&a, 0, sizeof a); memset(&b, 0, sizeof b);
        baseline_drum808_init(&a); drum808_init(&b);
        baseline_drum808_set(&a, track, param, value); drum808_set(&b, track, param, value);
        baseline_drum808_trigger(&a, track, 0.75f); drum808_trigger(&b, track, 0.75f);
        assert(!memcmp(&a, &b, sizeof a));
        baseline_drum808_render_st(&a, out_a[0], out_a[1], out_a[2], out_a[3], 64);
        drum808_render_st(&b, out_b[0], out_b[1], out_b[2], out_b[3], 64);
        assert(!memcmp(out_a, out_b, sizeof out_a));
        assert(!memcmp(&a, &b, sizeof a));
        ++cases;
    }
    /* Follow envelope transitions and tails, not just the trigger block.
     * Exercise short/mid/long decay, both switched sounds, and retriggers. */
    unsigned sustained = 0;
    for (int track = 0; track < D8_NUM; ++track)
    for (int setting = 0; setting < 3; ++setting)
    for (int alternate = 0; alternate < 2; ++alternate) {
        drum808_t a, b;
        memset(&a, 0, sizeof a); memset(&b, 0, sizeof b);
        baseline_drum808_init(&a); drum808_init(&b);
        for (int param = 0; param < drum808_nparams(track); ++param) {
            const x0x_param_t *p = drum808_param(track, param);
            int value;
            if (!strcmp(p->name, "Decay")) value = setting * p->max / 2;
            else if (!strcmp(p->name, "Sound")) value = alternate;
            else continue;
            baseline_drum808_set(&a, track, param, value);
            drum808_set(&b, track, param, value);
        }
        for (int block = 0; block < 2048; ++block) {
            float out_a[4][64] = {{0}}, out_b[4][64] = {{0}};
            if (block == 0 || block == 512 || block == 1024) {
                float velocity = block == 512 ? 0.25f : 1.0f;
                baseline_drum808_trigger(&a, track, velocity);
                drum808_trigger(&b, track, velocity);
            }
            baseline_drum808_render_st(&a, out_a[0], out_a[1], out_a[2], out_a[3], 64);
            drum808_render_st(&b, out_b[0], out_b[1], out_b[2], out_b[3], 64);
            assert(!memcmp(out_a, out_b, sizeof out_a));
            assert(!memcmp(&a, &b, sizeof a));
        }
        ++sustained;
    }
    printf("PASS: %u sustained 131072-sample envelope/retrigger cases bit-identical\n", sustained);
    printf("PASS: %u 808 parameter/trigger/render cases bit-identical with outlining\n", cases);
    return 0;
}
