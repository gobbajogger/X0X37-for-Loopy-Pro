/* SPDX-License-Identifier: GPL-3.0-only */
/* BF-023: a gesture queued for a lane must not replay after another pattern ran.
 * Real motion.c host test; no mocking of motion_step, request, or release code. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/motion.h"

static lane_t lanes[NLANE];
static motion_t motion;
static int knob, emitted, errors;
static int base(void *ctx, int t, int v, int i)
{
    (void)ctx; (void)t; (void)v; (void)i;
    return knob;
}
static void apply(void *ctx, int t, int v, int i, int value)
{
    (void)ctx; (void)t; (void)v; (void)i;
    emitted = value;
}
#define CHECK(cond, label) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", label); errors++; } \
} while (0)

static void reset(void)
{
    memset(lanes, 0, sizeof lanes);
    knob = emitted = 64;
    motion_init(&motion, lanes);
    motion.base = base;
    motion.apply = apply;
}

int main(void)
{
    int k;
    /* A REC request arrives just before the part advances to a different pattern.
     * Its target lane is inactive there, so it must NOT write on re-entry. */
    reset();
    k = motion_alloc(lanes, 2, 0, 2, 0, 0);
    CHECK(k == 0, "REC fixture allocated first lane");
    lanes[k].val[5] = 41;
    motion_step(&motion, 0, 2, 4, 5, 16);
    motion_req_rec(&motion, k);
    knob = emitted = 79;
    motion_step(&motion, 1, 7, 1, 2, 16); /* other part must not consume it */
    CHECK(motion.rt[k].rec_req == 1, "unrelated part preserves queue");
    motion_step(&motion, 0, 3, 0, 1, 16); /* different pattern consumes it */
    CHECK(motion.rt[k].rec_req == 0, "pattern switch discards obsolete REC intent");
    CHECK(lanes[k].val[5] == 41, "unrelated pattern leaves prior p-lock intact");
    motion_step(&motion, 0, 2, 5, 6, 16);
    CHECK(lanes[k].val[5] == 41, "re-entered pattern does not record stale REC");
    CHECK(emitted == 41, "re-entered pattern plays its existing p-lock");
    knob = emitted = 97;
    motion_req_rec(&motion, k);
    motion_step(&motion, 0, 2, 6, 7, 16);
    CHECK(lanes[k].val[6] == 97, "fresh post-reentry REC request still records");

    /* A queued nonrecording HOLD must likewise not suppress playback after
     * returning from a different pattern several steps later. */
    reset();
    k = motion_alloc(lanes, 2, 0, 2, 0, 0);
    CHECK(k == 0, "HOLD fixture allocated first lane");
    lanes[k].val[5] = 36;
    motion_step(&motion, 0, 2, 4, 5, 16);
    motion_req_hold(&motion, k);
    knob = emitted = 88;
    motion_step(&motion, 0, 3, 0, 1, 16);
    CHECK(motion.rt[k].hold_req == 0, "pattern switch discards obsolete HOLD intent");
    motion_step(&motion, 0, 3, 1, 2, 16);
    motion_step(&motion, 0, 2, 5, 6, 16);
    CHECK(emitted == 36, "re-entered pattern applies p-lock, not stale HOLD");
    CHECK(motion.rt[k].hold_left == 0, "stale HOLD does not inhibit next pass");
    knob = emitted = 90;
    motion_req_hold(&motion, k);
    motion_step(&motion, 0, 2, 6, 7, 16);
    CHECK(emitted == 90 && motion.rt[k].hold_left == 15,
          "fresh post-reentry HOLD request still wins one pass");
    printf("motion-pattern-pending: %s (%d failures)\n", errors ? "FAIL" : "PASS", errors);
    return errors != 0;
}
