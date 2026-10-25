/* SPDX-License-Identifier: GPL-3.0-only */
/* Combined stream5: migrate BOTH frozen incompatible stream4 layouts
 * (recorder endpoint flags vs Miami MOT_SNAPSHOT) into one revision.
 * Reuses real NOR/project fixtures from project_stage_test.c. */
#include "../../firmware/src/dsp/bass303.h"
#define main inherited_project_tests
#include "project_stage_test.c"
#undef main

static void test_stream5_snapshot_and_endpoint(void)
{
    uint8_t values[16], h[PROJECT_FILE_HDR];
    int i, k;
    fixture("stream5 roundtrip keeps snapshot + endpoint together");
    for (i = 0; i < 16; i++)
        values[i] = engine_param(T_303, 1, i)->def;
    REQUIRE(motion_snapshot_capture(proj.arr.lane, 31, 3, 63, 1, values) >= 0);
    REQUIRE(motion_snapshot_edit(proj.arr.lane, 31, 3, 63, 8, 127) >= 0);
    values[8] = 127;
    proj.pat[0].bass[0].step[0].flags = (uint8_t)(G_NOTE | (7u << BS_END_SHIFT));
    REQUIRE(project_save() == 0);
    project_capture_stage();
    REQUIRE(project_validate_stage() == 0);
    project_file_header(h);
    CHECK(get32(h + 20) == 5);
    CHECK(project_file_validate(h) == 0);
    put32(h + 20, 4);
    CHECK(project_file_validate(h) < 0); /* frozen-4 envelope cannot claim both features */
    put32(h + 20, 3);
    CHECK(project_file_validate(h) < 0); /* stream3 never carried either feature */
    memcpy(&saved_project, &proj, sizeof saved_project);
    memset(&proj, 0xA5, sizeof proj);
    REQUIRE(project_load_stage() == 0);
    CHECK(!memcmp(&project_stage, &saved_project, sizeof saved_project));
    CHECK(!memcmp(motion_snapshot_read(project_stage.arr.lane, 31, 3, 63), values, 16));
    CHECK(motion_snapshot_engine(project_stage.arr.lane, 31, 3, 63) == 1);
    CHECK((project_stage.pat[0].bass[0].step[0].flags >> BS_END_SHIFT) == 7);
    k = motion_snapshot_find(project_stage.arr.lane, 31, 3, 63);
    REQUIRE(k >= 0);
    project_stage.arr.lane[k].val[63] = 8;
    CHECK(project_validate_stage() < 0);
    project_stage = saved_project;
    k = motion_snapshot_find(project_stage.arr.lane, 31, 3, 63);
    project_stage.arr.lane[k].val[33] = 3;
    CHECK(project_validate_stage() < 0);
    {
        const int bad_param[] = {BASS303_WAVE, BASS303_DRVTYPE, BASS303_FENV, BASS303_DRIVE};
        unsigned n;
        for (n = 0; n < sizeof bad_param / sizeof bad_param[0]; n++) {
            unsigned bank;
            project_stage = saved_project;
            bank = project_stage.arr.lane[k].val[63];
            project_stage.arr.lane[k].val[bank + (unsigned)bad_param[n]] =
                (uint8_t)(engine_param(T_303, 1, bad_param[n])->max + 1u);
            CHECK(project_validate_stage() < 0);
        }
        project_stage = saved_project;
        for (i = 0; i < 16; i++)
            values[i] = 127;
        REQUIRE(motion_snapshot_capture(project_stage.arr.lane, 0, 2, 0, 2, values) >= 0);
        CHECK(project_validate_stage() == 0);
    }
    project_stage = saved_project;
    for (i = 0; i < NLANE; i++)
        if (!project_stage.arr.lane[i].used)
            break;
    REQUIRE(i < NLANE);
    project_stage.arr.lane[i] = project_stage.arr.lane[k];
    CHECK(project_validate_stage() < 0);
    check_protected_flash();
}

static void rewrite_stream_word(uint32_t stream)
{
    uint8_t data[PLAT_STORE_MAX];
    int k, n;
    for (k = 0; k < NSTREAM; k++) {
        n = st_load(STREAM_OBJ[k], data, sizeof data);
        if (n < 4)
            continue;
        put32(data, stream);
        REQUIRE(st_save(STREAM_OBJ[k], data, (uint32_t)n) == 0);
    }
    memset(stored_known, 0, sizeof stored_known);
}

static void test_migrate_frozen_stream4_rec(void)
{
    uint8_t h[PROJECT_FILE_HDR];
    fixture("frozen recorder stream4 endpoints migrate to stream5");
    musical_fixture();
    proj.pat[2].bass[1].step[5].flags = (uint8_t)(G_NOTE | BS_ACCENT | (12u << BS_END_SHIFT));
    REQUIRE(project_save() == 0);
    saved_project = proj;
    rewrite_stream_word(4); /* pretend an older recorder-only stream4 save */
    memset(&proj, 0x5A, sizeof proj);
    REQUIRE(project_load_stage() == 0);
    CHECK((project_stage.pat[2].bass[1].step[5].flags >> BS_END_SHIFT) == 12);
    for (int k = 0; k < NLANE; k++)
        CHECK(!project_stage.arr.lane[k].used || project_stage.arr.lane[k].t != MOT_SNAPSHOT);
    project_file_header(h);
    put32(h + 20, 4);
    CHECK(project_file_validate(h) == 0); /* endpoint-only stream4 envelope still valid */
    REQUIRE(project_save() == 0);
    {
        uint8_t data[PLAT_STORE_MAX];
        int n = st_load(OBJ_PAT0, data, sizeof data);
        REQUIRE(n >= 4);
        CHECK(get32(data) == 5); /* rewrite as combined */
    }
    check_protected_flash();
}

static void test_migrate_frozen_stream4_miami(void)
{
    uint8_t values[16], h[PROJECT_FILE_HDR];
    int i, k;
    fixture("frozen Miami stream4 snapshots migrate to stream5");
    for (i = 0; i < 16; i++)
        values[i] = engine_param(T_303, 0, i)->def;
    REQUIRE(motion_snapshot_capture(proj.arr.lane, 4, 2, 9, 1, values) >= 0);
    REQUIRE(project_save() == 0);
    saved_project = proj;
    rewrite_stream_word(4); /* pretend an older Miami-only stream4 save */
    memset(&proj, 0x3C, sizeof proj);
    REQUIRE(project_load_stage() == 0);
    CHECK(!memcmp(motion_snapshot_read(project_stage.arr.lane, 4, 2, 9), values, 16));
    CHECK(motion_snapshot_engine(project_stage.arr.lane, 4, 2, 9) == 1);
    for (k = 0; k < NPAT; k++)
        for (i = 0; i < NBASS; i++)
            for (int s = 0; s < NSTEPS; s++)
                CHECK(!(project_stage.pat[k].bass[i].step[s].flags >> BS_END_SHIFT));
    project_file_header(h);
    put32(h + 20, 4);
    CHECK(project_file_validate(h) == 0);
    REQUIRE(project_save() == 0);
    {
        uint8_t data[PLAT_STORE_MAX];
        int n = st_load(OBJ_PAT0, data, sizeof data);
        REQUIRE(n >= 4);
        CHECK(get32(data) == 5);
    }
    check_protected_flash();
}

static void test_reject_mixed_frozen_stream4(void)
{
    uint8_t values[16], data[PLAT_STORE_MAX];
    int i, n;
    fixture("mixed frozen stream4 (endpoints+snapshots) is rejected");
    for (i = 0; i < 16; i++)
        values[i] = engine_param(T_303, 0, i)->def;
    REQUIRE(motion_snapshot_capture(proj.arr.lane, 1, 2, 2, 1, values) >= 0);
    proj.pat[1].bass[0].step[2].flags = (uint8_t)(G_NOTE | (3u << BS_END_SHIFT));
    REQUIRE(project_save() == 0);
    n = st_load(OBJ_PAT0, data, sizeof data);
    REQUIRE(n >= 4);
    put32(data, 4);
    REQUIRE(st_save(OBJ_PAT0, data, (uint32_t)n) == 0);
    memset(stored_known, 0, sizeof stored_known);
    CHECK(project_load_stage() < 0);
    check_protected_flash();
}

static void test_stream3_rejects_new_features(void)
{
    uint8_t values[16], h[PROJECT_FILE_HDR];
    int i, k;
    fixture("stream3 migration rejects snapshots and endpoints");
    musical_fixture();
    REQUIRE(project_save() == 0);
    saved_project = proj;
    rewrite_stream_word(3);
    REQUIRE(project_load_stage() == 0);
    CHECK(!memcmp(&project_stage, &saved_project, sizeof saved_project));
    for (k = 0; k < NLANE; k++)
        CHECK(!project_stage.arr.lane[k].used || project_stage.arr.lane[k].t != MOT_SNAPSHOT);
    project_file_header(h);
    put32(h + 20, 3);
    CHECK(project_file_validate(h) == 0);
    for (i = 0; i < 16; i++)
        values[i] = 40;
    REQUIRE(motion_snapshot_capture(project_stage.arr.lane, 0, 2, 0, 1, values) >= 0);
    CHECK(project_file_validate(h) < 0);
    project_stage = saved_project;
    project_stage.pat[0].bass[0].step[0].flags = (uint8_t)(G_NOTE | (1u << BS_END_SHIFT));
    CHECK(project_file_validate(h) < 0);
    check_protected_flash();
}

int main(void)
{
    test_stream5_snapshot_and_endpoint();
    test_migrate_frozen_stream4_rec();
    test_migrate_frozen_stream4_miami();
    test_reject_mixed_frozen_stream4();
    test_stream3_rejects_new_features();
    printf("stream5 combined: %u checks, %u failures; both frozen stream4 migrations + rejection\n",
           checks, failures);
    return failures ? 1 : 0;
}
