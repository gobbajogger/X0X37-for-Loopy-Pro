/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X motion lanes (firmware/src/seq/motion.c): record a sweep, play it back, ramp between steps,
 * a gap filled, a single touch, the knob winning while it moves and for a pass, a pattern
 * change, stop, and a reused slot. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/motion.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf("  FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); fails++; } } while (0)

static lane_t L[NLANE];
static motion_t M;
static int knob = 64, eng = 64, applies;   /* one parameter: the knob (base) and the engine's value */

static int base(void *x, int t, int v, int i) { (void)x; (void)t; (void)v; (void)i; return knob; }
static void apply(void *x, int t, int v, int i, int val) { (void)x; (void)t; (void)v; (void)i; eng = val; applies++; }

/* one pass of a 16-step part, `per` samples a step; with f, the knob is turned to f(step) before
 * each step, recording (lane k) */
static void pass(int pat, int per, int (*f)(int), int k)
{
    int s;
    for (s = 0; s < 16; s++) {
        if (f) {
            eng = knob = f(s);                         /* the knob turned: the engine has it at once */
            motion_req_rec(&M, k);
        }
        motion_tick(&M, (uint32_t)per / 2u);
        motion_step(&M, 0, pat, s, (s + 1) % 16, 16);
        motion_tick(&M, (uint32_t)per / 2u);
    }
}
static int sweep(int s) { return s * 8; }

/* Observe each target independently: restoring a cleared lane must never use
 * the target of a new lane that happens to reuse its slot. */
static int values[2][2][3];
static int target_base(void *x, int t, int v, int i)
{
    (void)x;
    return t == 1 && v == 1 && i == 2 ? 100 : 60 + t * 10 + v * 5 + i;
}
static void target_apply(void *x, int t, int v, int i, int val)
{
    (void)x;
    values[t][v][i] = val;
}
static void target_fixture(void)
{
    int t, v, i;
    memset(L, 0, sizeof L);
    motion_init(&M, L);
    M.base = target_base;
    M.apply = target_apply;
    for (t = 0; t < 2; t++)
        for (v = 0; v < 2; v++)
            for (i = 0; i < 3; i++)
                values[t][v][i] = target_base(0, t, v, i);
}

static void test_pattern_ownership(void)
{
    int order, equal, k[2], s;
    for (order = 0; order < 2; order++)
        for (equal = 0; equal < 2; equal++) {
            target_fixture();
            k[order] = motion_alloc(L, order, 0, 0, 0, 0);
            k[1 - order] = motion_alloc(L, 1 - order, 0, 0, 0, 0);
            for (s = 0; s < NSTEPS; s++) {
                L[k[0]].val[s] = 20;
                L[k[1]].val[s] = (uint8_t)(equal ? 20 : 100);
            }
            motion_step(&M, 0, 1, 0, 1, 16);
            CHECK(values[0][0][0] == (equal ? 20 : 100), "outgoing pattern applies");
            for (s = 0; s < 16; s++) {
                motion_step(&M, 0, 0, s, (s + 1) % 16, 16);
                CHECK(values[0][0][0] == 20, "incoming pattern owns target, order=%d equal=%d step=%d actual=%d",
                      order, equal, s, values[0][0][0]);
            }
            motion_step(&M, 0, 1, 0, 1, 16);
            CHECK(values[0][0][0] == (equal ? 20 : 100), "return pattern owns target, order=%d equal=%d", order, equal);
        }
}

static void test_clear_and_reuse(void)
{
    int k, replacement, how;
    for (how = 0; how < 3; how++) {
        target_fixture();
        k = motion_alloc(L, 0, 0, 0, 1, 2);
        L[k].val[0] = 5;
        motion_step(&M, 0, 0, 0, 1, 16);
        CHECK(values[0][1][2] == 5, "old target applied before clear");
        motion_clear(L, k);
        if (how == 0)
            motion_tick(&M, 256);                      /* no new step required */
        else if (how == 1)
            motion_release(&M);                        /* Stop before a block sees the deletion */
        else
            motion_step(&M, 1, 0, 0, 1, 16);           /* another part fires first */
        CHECK(values[0][1][2] == 67 && motion_value(&M, k) < 0, "clear restores original base, path=%d", how);
    }
    for (how = 0; how < 2; how++) {
        target_fixture();
        k = motion_alloc(L, 0, 0, 0, 1, 2);
        L[k].val[0] = 5;
        motion_step(&M, 0, 0, 0, 1, 16);
        motion_clear(L, k);
        replacement = motion_alloc(L, 0, 1, 1, 0, 1);
        CHECK(replacement == k, "reuse the applied slot before audio observes its deletion");
        L[replacement].val[0] = 5;
        if (how == 0) {
            motion_tick(&M, 256);
            CHECK(values[0][1][2] == 67 && values[1][0][1] == 71, "tick releases only the retired target");
        }
        motion_step(&M, 1, 0, 0, 1, 16);
        CHECK(values[0][1][2] == 67 && values[1][0][1] == 5, "reuse releases old target then applies new, path=%d", how);
        motion_release(&M);
        CHECK(values[0][1][2] == 67 && values[1][0][1] == 71, "Stop releases the new target");
    }
    target_fixture();
    k = motion_alloc(L, 0, 0, 1, 1, 2);                /* probability: the base is 100 */
    L[k].val[0] = 5;
    motion_step(&M, 0, 0, 0, 1, 16);
    CHECK(values[1][1][2] == 5, "probability lock applied");
    motion_clear(L, k);
    motion_tick(&M, 256);
    CHECK(values[1][1][2] == 100, "deleting last probability lock restores 100");
}

static void test_ramp_parity(void)
{
    int k, s;
    target_fixture();
    k = motion_alloc(L, 0, 0, 0, 0, 0);
    for (s = 0; s < 5; s++)
        L[k].val[s] = (uint8_t)(s & 1 ? 100 : 0);
    motion_step(&M, 0, 0, 0, 1, 16);
    motion_tick(&M, 1000);
    motion_step(&M, 0, 0, 1, 2, 16);                  /* observed even step: 1000 samples */
    motion_tick(&M, 2000);
    motion_step(&M, 0, 0, 2, 3, 16);                  /* observed odd step: 2000 samples */
    motion_tick(&M, 500);
    CHECK(values[0][0][0] == 50, "even ramp is halfway after 500 samples");
    motion_tick(&M, 500);
    motion_step(&M, 0, 0, 3, 4, 16);
    motion_tick(&M, 500);
    CHECK(values[0][0][0] == 75, "odd ramp is one quarter through after 500 samples");
    motion_tick(&M, 500);
    CHECK(values[0][0][0] == 50, "odd ramp is halfway after 1000 samples");
}

int main(void)
{
    int k, k2, v;
    memset(L, 0, sizeof L);
    motion_init(&M, L);
    M.base = base;
    M.apply = apply;

    k = motion_alloc(L, 0, 0, 2, 0, 0);
    CHECK(k >= 0 && motion_count(L) == 1, "allocated");
    pass(0, 1000, 0, k);                               /* an empty lane plays the knob */
    CHECK(eng == 64, "an empty lane leaves the knob's value (%d)", eng);

    pass(0, 1000, sweep, k);                           /* record: a sweep over one pass */
    for (v = 0; v < 16; v++)
        CHECK(L[k].val[v] == v * 8, "recorded step %d = %d", v, L[k].val[v]);
    for (v = 0; v < MOT_GAP; v++)                      /* the gesture lasts MOT_GAP steps: the knob */
        motion_step(&M, 0, 0, v, v + 1, 16);
    CHECK(eng == 120, "just after the gesture the knob is heard (%d)", eng);

    knob = 5;                                          /* let go of the knob elsewhere */
    motion_tick(&M, 500);
    motion_step(&M, 0, 0, 0, 1, 16);
    CHECK(eng == 0, "playback: step 1 plays 0, not the knob (%d)", eng);
    motion_tick(&M, 500);
    CHECK(eng == 4, "ramp: halfway to step 2's 8 (%d)", eng);
    motion_tick(&M, 500);
    motion_step(&M, 0, 0, 1, 2, 16);
    CHECK(eng == 8, "step 2 (%d)", eng);

    L[k].val[2] = MOT_NONE;                            /* a step without a value: the knob */
    motion_step(&M, 0, 0, 2, 3, 16);
    CHECK(eng == 5, "a step with no value plays the knob (%d)", eng);

    motion_req_hold(&M, k);                            /* the knob wins for a pass */
    eng = knob = 99;
    for (v = 3; v < 16; v++) {
        motion_step(&M, 0, 0, v, (v + 1) % 16, 16);
        CHECK(eng == 99, "held: step %d plays the knob (%d)", v, eng);
    }
    for (v = 0; v < 3; v++)
        motion_step(&M, 0, 0, v, v + 1, 16);
    CHECK(eng == 99, "held: the knob's value through the pass (%d)", eng);
    motion_step(&M, 0, 0, 3, 4, 16);
    CHECK(eng == 24, "after the pass the lane again (%d)", eng);

    motion_step(&M, 0, 1, 4, 5, 16);                   /* the part changed pattern */
    CHECK(eng == 99, "another pattern: the knob (%d)", eng);
    motion_step(&M, 0, 0, 5, 6, 16);
    CHECK(eng == 40, "back on the pattern: the lane (%d)", eng);
    motion_step(&M, 1, 0, 6, 7, 16);
    CHECK(eng == 40, "another part's step does not touch it (%d)", eng);

    motion_release(&M);                                /* stop */
    CHECK(eng == 99 && motion_value(&M, k) < 0, "stop: the knob (%d)", eng);

    motion_step(&M, 0, 0, 6, 7, 16);
    CHECK(eng == 48, "playing again (%d)", eng);
    motion_clear(L, k);                                /* cleared while applying; the slot reused */
    k2 = motion_alloc(L, 0, 0, 2, 0, 1);
    CHECK(k2 == k, "the slot is reused");
    L[k2].val[7] = 48;                                 /* the stale runtime applied 48: must still apply */
    applies = 0;
    motion_step(&M, 0, 0, 7, 8, 16);
    CHECK(applies == 2 && eng == 48, "a reused slot releases the old target then starts fresh (applies %d)", applies);

    /* a gap: touches on steps 2 and 5 fill 3 and 4; a lone touch writes one step */
    k2 = motion_alloc(L, 2, 0, 2, 0, 2);
    eng = knob = 30;
    motion_req_rec(&M, k2);
    motion_step(&M, 0, 2, 2, 3, 16);
    motion_step(&M, 0, 2, 3, 4, 16);
    motion_step(&M, 0, 2, 4, 5, 16);
    eng = knob = 60;
    motion_req_rec(&M, k2);
    motion_step(&M, 0, 2, 5, 6, 16);
    CHECK(L[k2].val[2] == 30 && L[k2].val[3] == 40 && L[k2].val[4] == 50 && L[k2].val[5] == 60,
          "gap filled: %d %d %d %d", L[k2].val[2], L[k2].val[3], L[k2].val[4], L[k2].val[5]);
    for (v = 6; v < 6 + MOT_GAP + 2; v++)
        motion_step(&M, 0, 2, v, v + 1, 16);
    eng = knob = 90;
    motion_req_rec(&M, k2);
    motion_step(&M, 0, 2, 13, 14, 16);                 /* a new gesture: no line back to step 5 */
    CHECK(L[k2].val[12] == MOT_NONE && L[k2].val[13] == 90 && L[k2].val[6] == MOT_NONE,
          "a lone touch writes its step only (%d %d %d)", L[k2].val[12], L[k2].val[13], L[k2].val[6]);

    for (v = 0; v < NLANE; v++)                        /* the pool fills, and says so */
        if (motion_alloc(L, 3, 1, 0, v & 7, v >> 3) < 0)
            break;
    CHECK(v == NLANE - 2 && motion_alloc(L, 3, 1, 1, 0, 0) == -1, "pool of %d: full after %d more", NLANE, v);
    CHECK(motion_find(L, 3, 1, 0, 2, 1) >= 0 && motion_find(L, 4, 1, 0, 2, 1) < 0, "find keys on the pattern");
    test_pattern_ownership();
    test_clear_and_reuse();
    test_ramp_parity();
    printf("motion runtime: lane=%zu bytes, all lanes=%zu bytes, motion=%zu bytes\n",
           sizeof(lane_rt_t), sizeof M.rt, sizeof M);
    printf("motion: %s\n", fails ? "FAIL" : "ok");
    return fails != 0;
}
