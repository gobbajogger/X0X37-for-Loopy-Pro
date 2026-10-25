/* SPDX-License-Identifier: GPL-3.0-only */
/* Prove pre-PROJ_SND_REV-6 Wave motion migrate: stored square 1 → 127 on
 * used T_303 / BASS303_WAVE knob lanes (MOT_NONE and 0 kept) and on used
 * MOT_SNAPSHOT banks with engine==1 at BASS303_WAVE (banks 0 and 16).
 * WBL1 engine==2 Wave slots must stay untouched. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/project.c"

static unsigned checks, failures;

static void expect(int ok, const char *what, int line)
{
    checks++;
    if (!ok) {
        fprintf(stderr, "%s:%d: %s\n", __FILE__, line, what);
        failures++;
    }
}
#define CHECK(x) expect(!!(x), #x, __LINE__)

static void fill_lane_base(lane_t *l)
{
    int s;
    memset(l, 0, sizeof *l);
    l->used = 1;
    l->pat = 0;
    l->part = PART_303A;
    for (s = 0; s < NSTEPS; s++)
        l->val[s] = MOT_NONE;
}

int main(void)
{
    project_t pr;
    lane_t *knob, *unused_knob, *snap303, *snap_wbl, *snap_both;
    int s;

    memset(&pr, 0, sizeof pr);

    /* Lane 0: used T_303 Wave knob motion — mix of 1, 0, MOT_NONE, other. */
    knob = &pr.arr.lane[0];
    fill_lane_base(knob);
    knob->t = T_303;
    knob->v = 0;
    knob->i = BASS303_WAVE;
    knob->val[0] = 1;            /* square → 127 */
    knob->val[1] = 0;            /* saw / zero kept */
    knob->val[2] = MOT_NONE;     /* base kept */
    knob->val[3] = 64;           /* already continuous kept */
    knob->val[4] = 1;            /* another square → 127 */

    /* Lane 1: unused T_303 Wave — must not remap even if val==1. */
    unused_knob = &pr.arr.lane[1];
    fill_lane_base(unused_knob);
    unused_knob->used = 0;
    unused_knob->t = T_303;
    unused_knob->i = BASS303_WAVE;
    unused_knob->val[0] = 1;
    unused_knob->val[1] = MOT_NONE;

    /* Lane 2: MOT_SNAPSHOT, engine 303 on both banks, Wave==1 at each bank. */
    snap303 = &pr.arr.lane[2];
    fill_lane_base(snap303);
    snap303->t = MOT_SNAPSHOT;
    snap303->v = 7;                              /* step */
    snap303->i = MOT_SNAPSHOT_SCHEMA;
    snap303->val[BASS303_WAVE] = 1;              /* bank 0 */
    snap303->val[MOT_SNAPSHOT_PARAMS + BASS303_WAVE] = 1; /* bank 16 */
    snap303->val[32] = 1;                        /* engine 303 */
    snap303->val[33] = 1;                        /* engine 303 */
    snap303->val[0] = 40;                        /* non-Wave param kept */
    snap303->val[MOT_SNAPSHOT_PARAMS] = 41;

    /* Lane 3: MOT_SNAPSHOT WBL1 only — Wave==1 must stay 1. */
    snap_wbl = &pr.arr.lane[3];
    fill_lane_base(snap_wbl);
    snap_wbl->t = MOT_SNAPSHOT;
    snap_wbl->v = 8;
    snap_wbl->i = MOT_SNAPSHOT_SCHEMA;
    snap_wbl->val[BASS303_WAVE] = 1;
    snap_wbl->val[MOT_SNAPSHOT_PARAMS + BASS303_WAVE] = 1;
    snap_wbl->val[32] = 2;                       /* WBL1 */
    snap_wbl->val[33] = 2;                       /* WBL1 */

    /* Lane 4: mixed engines — bank0=303 Wave1, bank16=WBL1 Wave1. */
    snap_both = &pr.arr.lane[4];
    fill_lane_base(snap_both);
    snap_both->t = MOT_SNAPSHOT;
    snap_both->v = 9;
    snap_both->i = MOT_SNAPSHOT_SCHEMA;
    snap_both->val[BASS303_WAVE] = 1;
    snap_both->val[MOT_SNAPSHOT_PARAMS + BASS303_WAVE] = 1;
    snap_both->val[32] = 1;                      /* 303 → remap */
    snap_both->val[33] = 2;                      /* WBL1 → keep */

    sound_rev6_wave_motion(&pr);

    CHECK(BASS303_WAVE == 5);
    CHECK(knob->val[0] == 127);
    CHECK(knob->val[1] == 0);
    CHECK(knob->val[2] == MOT_NONE);
    CHECK(knob->val[3] == 64);
    CHECK(knob->val[4] == 127);
    for (s = 5; s < NSTEPS; s++)
        CHECK(knob->val[s] == MOT_NONE);

    CHECK(unused_knob->used == 0);
    CHECK(unused_knob->val[0] == 1);
    CHECK(unused_knob->val[1] == MOT_NONE);

    CHECK(snap303->val[BASS303_WAVE] == 127);
    CHECK(snap303->val[MOT_SNAPSHOT_PARAMS + BASS303_WAVE] == 127);
    CHECK(snap303->val[0] == 40);
    CHECK(snap303->val[MOT_SNAPSHOT_PARAMS] == 41);
    CHECK(snap303->val[32] == 1);
    CHECK(snap303->val[33] == 1);

    CHECK(snap_wbl->val[BASS303_WAVE] == 1);
    CHECK(snap_wbl->val[MOT_SNAPSHOT_PARAMS + BASS303_WAVE] == 1);
    CHECK(snap_wbl->val[32] == 2);
    CHECK(snap_wbl->val[33] == 2);

    CHECK(snap_both->val[BASS303_WAVE] == 127);
    CHECK(snap_both->val[MOT_SNAPSHOT_PARAMS + BASS303_WAVE] == 1);
    CHECK(snap_both->val[32] == 1);
    CHECK(snap_both->val[33] == 2);

    if (failures) {
        fprintf(stderr, "wave_motion_migrate: %u/%u checks failed\n", failures, checks);
        return 1;
    }
    printf("wave_motion_migrate: ok (%u checks)\n", checks);
    return 0;
}
