/* SPDX-License-Identifier: GPL-3.0-only */
/* BF-018: deleting/reusing a motion lane must discard the old knob requests. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/motion.h"

static lane_t lanes[NLANE];
static motion_t motion;
static int knob, output, applications, failures;
#define CHECK(ok, msg) do { if (!(ok)) { fprintf(stderr, "FAIL: %s\n", msg); failures++; } } while (0)

/* The baseline compile uses exactly the original motion_clear(lanes, k) API.
 * The repaired API takes motion_t so it can cancel the slot's queued intents. */
#ifdef BF018_OLD_API
#define DELETE_LANE(m, k) motion_clear((m)->lane, (k))
#else
#define DELETE_LANE(m, k) motion_clear((m), (k))
#endif

static int base(void *ctx, int target, int voice, int index)
{
    (void)ctx; (void)target; (void)voice; (void)index;
    return knob;
}
static void apply(void *ctx, int target, int voice, int index, int value)
{
    (void)ctx; (void)target; (void)voice; (void)index;
    output = value;
    applications++;
}
static void fresh(void)
{
    memset(lanes, 0, sizeof lanes);
    motion_init(&motion, lanes);
    motion.base = base;
    motion.apply = apply;
    knob = output = 64;
    applications = 0;
}
static int prepare_old(void)
{
    int k = motion_alloc(lanes, 0, 0, 1, 0, 0);
    CHECK(k == 0, "first allocation uses lane zero");
    /* Synchronize the ISR-owned runtime to the old lane generation. */
    motion_step(&motion, 0, 0, 15, 0, 16);
    return k;
}
static int reuse(int old)
{
    int k;
    DELETE_LANE(&motion, old);
    k = motion_alloc(lanes, 0, 0, 2, 0, 0);
    CHECK(k == old, "lane reallocated at same slot for a different target");
    return k;
}
static void stale_record_is_discarded(void)
{
    int k;
    fresh();
    k = prepare_old();
    motion_req_rec(&motion, k);  /* previous target's event, not yet consumed */
    k = reuse(k);
    lanes[k].val[1] = 37;
    knob = 88;
    applications = 0;
    motion_step(&motion, 0, 0, 1, 2, 16);
    CHECK(lanes[k].val[1] == 37, "stale record must not overwrite a new target's step lock");
    CHECK(output == 37 && applications == 1, "new lane should play existing lock");
}
static void stale_hold_is_discarded(void)
{
    int k;
    fresh();
    k = prepare_old();
    motion_req_hold(&motion, k); /* previous target's event, not yet consumed */
    k = reuse(k);
    lanes[k].val[1] = 46;
    knob = 91;
    applications = 0;
    motion_step(&motion, 0, 0, 1, 2, 16);
    CHECK(output == 46 && applications == 1, "stale hold must not suppress a new target's lock");
}
static void new_events_after_reuse_are_kept(void)
{
    int k;
    fresh();
    k = prepare_old();
    motion_req_rec(&motion, k);
    k = reuse(k);
    knob = 73;
    motion_req_rec(&motion, k); /* legitimate fresh request before first new step */
    motion_step(&motion, 0, 0, 1, 2, 16);
    CHECK(lanes[k].val[1] == 73, "first new-lane recording survives generation reset");

    fresh();
    k = prepare_old();
    motion_req_hold(&motion, k);
    k = reuse(k);
    lanes[k].val[1] = 46;
    applications = 0;
    motion_req_hold(&motion, k); /* legitimate first-step hold on new target */
    motion_step(&motion, 0, 0, 1, 2, 16);
    CHECK(applications == 0, "first new-lane hold retains knob ownership");
    motion_step(&motion, 0, 0, 2, 3, 16);
    CHECK(applications == 0, "new hold continues for its duration");
}
static void original_lane_and_bounds_still_work(void)
{
    int k;
    fresh();
    k = prepare_old();
    knob = 95;
    motion_req_rec(&motion, k);
    DELETE_LANE(&motion, -1);
    DELETE_LANE(&motion, NLANE);
    motion_step(&motion, 0, 0, 1, 2, 16);
    CHECK(lanes[k].val[1] == 95, "out-of-range deletions do not cancel a valid request");
    CHECK(motion_count(lanes) == 1, "out-of-range deletions retain valid lane");

    fresh();
    k = prepare_old();
    motion_req_rec(&motion, k);
    DELETE_LANE(&motion, k);
    CHECK(motion_count(lanes) == 0, "cleared lane removed");
    CHECK(lanes[k].used == 0, "cleared lane stays unallocated");
}
int main(void)
{
    stale_record_is_discarded();
    stale_hold_is_discarded();
    new_events_after_reuse_are_kept();
    original_lane_and_bounds_still_work();
    printf("motion-clear-pending: %s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}
