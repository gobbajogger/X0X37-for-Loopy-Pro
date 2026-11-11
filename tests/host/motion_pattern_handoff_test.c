/* SPDX-License-Identifier: GPL-3.0-only */
/* BF-024: inactive lane teardown must precede new pattern's target writes. */
#include <stdio.h>
#include <string.h>
#include "motion.h"

typedef struct {
    int p[2];
    int writes[2];
    int base[2];
} synth_t;

static int channel(int t, int v, int i)
{
    return t == 7 && v == 3 && i == 2 ? 0 : 1;
}
static int read_base(void *ctx, int t, int v, int i)
{
    synth_t *s = ctx;
    return s->base[channel(t,v,i)];
}
static void write_value(void *ctx, int t, int v, int i, int val)
{
    synth_t *s = ctx;
    int c = channel(t,v,i);
    s->p[c] = val;
    s->writes[c]++;
}

#define CHECK(expr, what) do { \
    if (!(expr)) { fprintf(stderr, "FAIL: %s (line %d)\n", what, __LINE__); errors++; } \
    else passed++; \
} while (0)

static int check_order(int old_first, int use_tick)
{
    lane_t lanes[NLANE];
    static motion_t m;
    synth_t s;
    int errors = 0, passed = 0, old_lane, new_lane;
    memset(lanes,0,sizeof lanes);
    memset(&s,0,sizeof s);
    s.base[0] = 11;
    s.base[1] = 35;
    s.p[0] = s.base[0];
    s.p[1] = s.base[1];
    motion_init(&m, lanes);
    m.ctx = &s;
    m.base = read_base;
    m.apply = write_value;
    if (old_first) {
        old_lane = motion_alloc(lanes, 0, 0, 7,3,2);
        new_lane = motion_alloc(lanes, 1, 0, 7,3,2);
    } else {
        new_lane = motion_alloc(lanes, 1, 0, 7,3,2);
        old_lane = motion_alloc(lanes, 0, 0, 7,3,2);
    }
    CHECK(old_lane >= 0 && new_lane >= 0 && old_lane != new_lane, "both pattern lanes allocated");
    lanes[old_lane].val[0] = 40;
    lanes[new_lane].val[0] = 94;
    lanes[new_lane].val[1] = 96;
    int other = motion_alloc(lanes, 0, 1, 8,2,0);
    CHECK(other >= 0, "independent part lane allocated");
    lanes[other].val[0] = 71;
    motion_step(&m, 1,0,0,1,16);
    CHECK(s.p[1] == 71, "other part retains p-lock");
    motion_step(&m, 0,0,0,1,16);
    CHECK(s.p[0] == 40, "old pattern initially applies");
    if (use_tick) motion_tick(&m, 100);
    motion_step(&m, 0,1,0,1,16);
    CHECK(s.p[0] == 94, "switch applies new pattern regardless of slot order");
    CHECK(motion_value(&m,new_lane) == 94, "runtime new lane agrees with synth output");
    CHECK(motion_value(&m,old_lane) == -1, "old lane runtime is inactive");
    CHECK(s.p[1] == 71, "other part is not reset on target pattern change");
    motion_tick(&m,80);
    CHECK(s.p[0] >= 94 && s.p[0] <= 96, "new lane ramp continues after switch");
    motion_step(&m, 0,0,0,1,16);
    CHECK(s.p[0] == 40, "return to original pattern applies original p-lock");
    motion_step(&m, 0,1,0,1,16);
    CHECK(s.p[0] == 94, "subsequent switch preserves latest pattern p-lock");
    fprintf(stdout,"BF024 order=%s tick=%d: %d passed, %d failed\n", old_first?"old-first":"new-first",use_tick,passed,errors);
    return errors;
}
static int restores;
static void restore_value(void *ctx, int t, int v, int i)
{
    /* Separate callback exercises the engine's exact-base restore route. */
    restores++;
    write_value(ctx, t, v, i, read_base(ctx, t, v, i));
}

/* N6: a destination without a lane must sound the current knob base,
 * including with the exact-base callback used by the firmware engine. */
static int check_unautomated_pattern(int exact_restore)
{
    static lane_t lanes[NLANE];
    static motion_t m;
    synth_t s = {{11, 35}, {0, 0}, {11, 35}};
    int errors = 0, passed = 0;
    memset(lanes, 0, sizeof lanes);
    motion_init(&m, lanes);
    restores = 0;
    m.ctx = &s;
    m.base = read_base;
    m.apply = write_value;
    if (exact_restore) m.restore = restore_value;
    int old = motion_alloc(lanes, 0, 0, 7, 3, 2);
    int other = motion_alloc(lanes, 0, 1, 8, 2, 0);
    CHECK(old >= 0 && other >= 0, "N6 lanes allocated");
    if (old < 0 || other < 0) return errors;
    lanes[old].val[0] = 94;
    lanes[other].val[0] = 71;
    motion_step(&m, 0, 0, 0, 1, 16);
    motion_step(&m, 1, 0, 0, 1, 16);
    CHECK(s.p[0] == 94, "N6 outgoing pattern lock sounds");
    s.base[0] = 19; /* Restore the current base, not the value at lane onset. */
    motion_step(&m, 0, 2, 0, 1, 16);
    CHECK(s.p[0] == 19, "N6 no-lane destination restores current knob base");
    CHECK(restores == exact_restore, "N6 exact-base callback is used when supplied");
    CHECK(motion_value(&m, old) == -1, "N6 departing runtime retires");
    CHECK(s.p[1] == 71, "N6 other part keeps its automation");
    for (int step = 1; step < 16; step++) {
        motion_tick(&m, 80);
        motion_step(&m, 0, 2, step, (step + 1) % 16, 16);
        CHECK(s.p[0] == 19, "N6 old lock never returns in unautomated pattern");
    }
    motion_step(&m, 0, 0, 0, 1, 16);
    CHECK(s.p[0] == 94, "N6 original pattern lock plays on return");
    s.base[0] = 45;
    motion_step(&m, 0, 2, 0, 1, 16);
    CHECK(s.p[0] == 45, "N6 second departure restores latest knob base");
    CHECK(s.p[1] == 71, "N6 independent part remains unchanged");
    fprintf(stdout, "N6 exact_restore=%d: %d passed, %d failed\n",
            exact_restore, passed, errors);
    return errors;
}

int main(void)
{
    int failures=0;
    failures += check_order(0,0);
    failures += check_order(0,1);
    failures += check_order(1,0);
    failures += check_order(1,1);
    failures += check_unautomated_pattern(0);
    failures += check_unautomated_pattern(1);
    return failures ? 1 : 0;
}
