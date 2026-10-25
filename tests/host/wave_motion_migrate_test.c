/* SPDX-License-Identifier: GPL-3.0-only */
/* Prove pre-PROJ_SND_REV-6 Wave motion migrate: stored square 1 → 127 on
 * used T_303 / BASS303_WAVE knob lanes (MOT_NONE and 0 kept) and in base sound. */
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
    lane_t *knob, *unused_knob;
    int s;

    memset(&pr, 0, sizeof pr);
    pr.sound.v[T_303][0][BASS303_WAVE] = 1;
    pr.sound.v[T_303][1][BASS303_WAVE] = 64;

    knob = &pr.arr.lane[0];
    fill_lane_base(knob);
    knob->t = T_303;
    knob->v = 0;
    knob->i = BASS303_WAVE;
    knob->val[0] = 1;
    knob->val[1] = 0;
    knob->val[2] = MOT_NONE;
    knob->val[3] = 64;
    knob->val[4] = 1;

    unused_knob = &pr.arr.lane[1];
    fill_lane_base(unused_knob);
    unused_knob->used = 0;
    unused_knob->t = T_303;
    unused_knob->i = BASS303_WAVE;
    unused_knob->val[0] = 1;
    unused_knob->val[1] = MOT_NONE;

    sound_rev6_wave_motion(&pr);

    CHECK(pr.sound.v[T_303][0][BASS303_WAVE] == 127);
    CHECK(pr.sound.v[T_303][1][BASS303_WAVE] == 64);

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

    if (failures) {
        fprintf(stderr, "wave_motion_migrate: %u/%u checks failed\n", failures, checks);
        return 1;
    }
    printf("wave_motion_migrate: ok (%u checks)\n", checks);
    return 0;
}
