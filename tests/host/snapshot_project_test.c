/* SPDX-License-Identifier: GPL-3.0-only */
/* Reuses real NOR/project fixtures; no project defaults or descriptors are mocked. */
#include "../../firmware/src/dsp/bass303.h"
#define main inherited_project_tests
#include "project_stage_test.c"
#undef main
static void test_snapshot_roundtrip(void)
{
    uint8_t values[16], h[PROJECT_FILE_HDR];
    int i,k;
    fixture("snapshot real flash and portable project roundtrip");
    for(i=0;i<16;i++) values[i]=engine_param(T_303,1,i)->def;
    REQUIRE(motion_snapshot_capture(proj.arr.lane,31,3,63,1,values)>=0);
    REQUIRE(motion_snapshot_edit(proj.arr.lane,31,3,63,8,127)>=0);
    values[8]=127;
    REQUIRE(project_save()==0);
    project_capture_stage();
    REQUIRE(project_validate_stage()==0);
    project_file_header(h);
    CHECK(get32(h+20)==4);
    CHECK(project_file_validate(h)==0);
    put32(h+20,3);
    CHECK(project_file_validate(h)<0); /* old-format envelope may not disguise snapshots */
    memcpy(&saved_project,&proj,sizeof proj);
    memset(&proj,0xA5,sizeof proj);
    REQUIRE(project_load_stage()==0);
    CHECK(!memcmp(&project_stage,&saved_project,sizeof saved_project));
    CHECK(!memcmp(motion_snapshot_read(project_stage.arr.lane,31,3,63),values,16));
    CHECK(motion_snapshot_engine(project_stage.arr.lane,31,3,63)==1);
    k=motion_snapshot_find(project_stage.arr.lane,31,3,63);
    REQUIRE(k>=0);
    project_stage.arr.lane[k].val[63]=8;
    CHECK(project_validate_stage()<0);
    project_stage=saved_project;
    k=motion_snapshot_find(project_stage.arr.lane,31,3,63);
    project_stage.arr.lane[k].val[33]=3;
    CHECK(project_validate_stage()<0);
    /* Imported native303 values must obey each descriptor, not only 0..127. */
    {
        const int bad_param[] = {BASS303_WAVE, BASS303_DRVTYPE, BASS303_FENV, BASS303_DRIVE};
        unsigned n;
        for(n=0;n<sizeof bad_param/sizeof bad_param[0];n++) {
            unsigned bank;
            project_stage=saved_project;
            bank=project_stage.arr.lane[k].val[63];
            project_stage.arr.lane[k].val[bank+(unsigned)bad_param[n]] =
                (uint8_t)(engine_param(T_303,1,bad_param[n])->max+1u);
            CHECK(project_validate_stage()<0);
        }
        project_stage=saved_project;
        for(i=0;i<16;i++) values[i]=127;
        REQUIRE(motion_snapshot_capture(project_stage.arr.lane,0,2,0,2,values)>=0);
        CHECK(project_validate_stage()==0); /* WBL1 retains its distinct full-byte parameter range. */
    }
    project_stage=saved_project;
    for(i=0;i<NLANE;i++) if(!project_stage.arr.lane[i].used) break;
    REQUIRE(i<NLANE);
    project_stage.arr.lane[i]=project_stage.arr.lane[k];
    CHECK(project_validate_stage()<0); /* duplicate ownership rejected */
    check_protected_flash();
}
static void test_snapshot_legacy3(void)
{
    uint8_t data[PLAT_STORE_MAX],h[PROJECT_FILE_HDR];
    int k,n;
    fixture("format3 project migration has no snapshots");
    musical_fixture();
    REQUIRE(project_save()==0);
    saved_project=proj;
    for(k=0;k<NSTREAM;k++) {
        n=st_load(STREAM_OBJ[k],data,sizeof data);
        if(n<4) continue;
        put32(data,3);
        REQUIRE(st_save(STREAM_OBJ[k],data,(uint32_t)n)==0);
    }
    memset(stored_known,0,sizeof stored_known);
    REQUIRE(project_load_stage()==0);
    CHECK(!memcmp(&project_stage,&saved_project,sizeof saved_project));
    for(k=0;k<NLANE;k++) CHECK(!project_stage.arr.lane[k].used || project_stage.arr.lane[k].t!=MOT_SNAPSHOT);
    project_file_header(h);put32(h+20,3);
    CHECK(project_file_validate(h)==0);
    check_protected_flash();
}
int main(void)
{
    test_snapshot_roundtrip();
    test_snapshot_legacy3();
    printf("snapshot projects: %u checks, %u failures; real flash/save/load/file validation/legacy3\n",checks,failures);
    return failures?1:0;
}
