/* SPDX-License-Identifier: GPL-3.0-only */
/* BF-007: Bass Drum PAN motion keeps its parameter through save/load. In the
 * original-synth build an older sound revision is refused (-9), not migrated.
 * Mock NOR, real compression/storage. */
#define main project_stage_existing_main
#include "project_stage_test.c"
#undef main

static void legacy_case(uint8_t rev, uint8_t pan909, uint8_t pan808)
{
    lane_t *a, *b;
    fixture("legacy 909/808 BD PAN motion keeps parameter after import");
    proj.set.snd_rev = rev;
    proj.sound.v[T_909][0][pan909] = 73;
    proj.sound.v[T_808][0][pan808] = 81;
    a = &proj.arr.lane[0];
    b = &proj.arr.lane[1];
    memset(a, 0, sizeof *a);
    memset(b, 0, sizeof *b);
    a->used = b->used = 1;
    a->t = T_909;
    b->t = T_808;
    a->part = PART_909;
    b->part = PART_808;
    a->v = b->v = 0;
    a->i = pan909;
    b->i = pan808;
    memset(a->val, MOT_NONE, sizeof a->val);
    memset(b->val, MOT_NONE, sizeof b->val);
    a->val[0] = 73;
    b->val[0] = 81;
    REQUIRE(project_save() == 0);
    memset(stored_known, 0, sizeof stored_known);
    if (rev != PROJ_SND_REV) {
        /* The original-synth build loads only its own sound revision: an older
         * one is refused as unsupported (-9), never half-migrated. */
        CHECK(project_load_stage() == -9);
        return;
    }
    REQUIRE(project_load_stage() == 0);
    CHECK(project_stage.sound.v[T_909][0][10] == 73);
    CHECK(project_stage.sound.v[T_808][0][9] == 81);
    CHECK(project_stage.arr.lane[0].i == 10);
    CHECK(project_stage.arr.lane[1].i == 9);
    CHECK(project_stage.arr.lane[0].val[0] == 73);
    CHECK(project_stage.arr.lane[1].val[0] == 81);
}

int main(void)
{
    /* Before rev3 909 BD PAN=8; before rev4 808 BD PAN=7.
     * Rev3 already moved 909 to index 10; rev4 already moved both. */
    legacy_case(2, 8, 7);
    legacy_case(3, 10, 7);
    legacy_case(PROJ_SND_REV, 10, 9);
    printf("BF-007 BD PAN migration: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
