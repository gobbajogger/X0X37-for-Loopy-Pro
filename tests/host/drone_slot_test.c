/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the drone through the production slot selector, renderer and UI. */
#define SMC_UI_MAIN retained_fixture_main
#include "smc_ui_test.c"
#include "app/original_runtime.h"
static void render_blocks(unsigned count)
{
    int32_t audio[512];
    while (count--) engine_render(audio,256);
}
static project_t preserved;
static void unavailable_slot(int voice, int preset)
{
    setup(V_PART,PART_303A+voice);
    engine_guard_enable(0);
    CHECK(!original_runtime_kind_available(ORIGINAL_SOUND_DRONE));
    CHECK(!engine_bass_bank_available(voice,ORIGINAL_SOUND_DRONE));
    proj.pat[0].rsv[voice]=(uint8_t)(0xC1u+preset);
    int lane=motion_alloc(proj.arr.lane,0,PART_303A+voice,T_MIX,PART_303A+voice,0);
    CHECK(lane>=0); proj.arr.lane[lane].val[5]=43;
    memcpy(&preserved,&proj,sizeof proj);
    uint32_t serial=engine_bass_choice_serial();
    int bank=engine_bass_bank(voice), index=engine_bass_preset_index(voice);
    int status=engine_bass_status(voice);
    int other=engine_bass_bank(1-voice), other_index=engine_bass_preset_index(1-voice);
    int other_status=engine_bass_status(1-voice);
    CHECK(!engine_bass_preset_pending(voice));
    CHECK(engine_bass_select_preset(voice,ORIGINAL_SOUND_DRONE,preset,&proj.sound)==-2);
    render_blocks(2);
    CHECK(!memcmp(&preserved,&proj,sizeof proj)); /* cell, tag, pattern and lane */
    CHECK(engine_bass_choice_serial()==serial);
    CHECK(engine_bass_bank(voice)==bank && engine_bass_preset_index(voice)==index);
    CHECK(engine_bass_status(voice)==status && !engine_bass_preset_pending(voice));
    CHECK(engine_bass_requested_bank(voice)==bank && engine_bass_requested_preset(voice)==index);
    CHECK(engine_bass_bank(1-voice)==other && engine_bass_preset_index(1-voice)==other_index);
    CHECK(engine_bass_status(1-voice)==other_status);

    /* An archived cell remains valid and exact even without its live DSP. */
    CHECK(original_sound_init(proj.sound.v[T_303][voice],ORIGINAL_SOUND_DRONE,preset));
    engine_bass_sound_refresh(&proj.sound,voice);
    memcpy(&preserved,&proj,sizeof proj);
    engine_project_hold(1);
    engine_init(proj.pat,&proj.arr.song,proj.arr.lane,&proj.sound);
    engine_apply_sound(&proj.sound); engine_project_release();
    render_blocks(4);
    CHECK(engine_bass_sound_validate(&proj.sound)==0);
    CHECK(engine_bass_bank(voice)==ORIGINAL_SOUND_DRONE);
    CHECK(engine_bass_sound_bank(&proj.sound,voice)==ORIGINAL_SOUND_DRONE);
    CHECK(engine_bass_preset_index(voice)==preset);
    CHECK(engine_bass_status(voice)==ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(!engine_bass_preset_pending(voice));
    engine_bass_on_velocity(voice,60,0,0,100); render_blocks(32);
    float level=1,mod=1; unsigned state=0,down=1;
    CHECK(!engine_bass_voice_view(voice,&level,&mod,&state,&down));
    CHECK(level==0 && mod==0 && state==6 && !down);
    CHECK(!(engine_bass_keys_held()&(1u<<voice)));
    int32_t audio[512]; engine_render(audio,256);
    int32_t combined=0; for (unsigned i=0;i<512;i++) combined|=audio[i];
    CHECK(combined==0);
    ui.part=(uint8_t)(PART_303A+voice); set_view(V_PART); build_pages();
    CHECK(pg.n==1 && pg.r[0][0].kind==R_BENGINE);
    pref_t preset_ref=PR(R_BPRESET,0,voice,0), parameter=PR(R_ENG,T_303,voice,0);
    CHECK(!pref_read_only(PR(R_BENGINE,0,voice,0)));
    CHECK(pref_read_only(preset_ref) && pref_read_only(parameter));
    CHECK(pref_read_only(PR(R_BSTATUS,0,voice,0)));
    char text[32]={0},unit[8]={0};
    CHECK(original_value_text(PR(R_BSTATUS,0,voice,0),pref_get(PR(R_BSTATUS,0,voice,0)),text,unit));
    CHECK(!strcmp(text,"UNAVAIL") && !unit[0]);
    pref_set(preset_ref,(preset+1)%4); pref_set(parameter,17);
    CHECK(!memcmp(&preserved,&proj,sizeof proj));
    CHECK(engine_bass_bank(1-voice)==other && engine_bass_preset_index(1-voice)==other_index);
    CHECK(engine_bass_status(1-voice)==other_status);

    /* A deliberate available choice leaves the archived terminal state. */
    CHECK(engine_bass_select_preset(voice,ORIGINAL_SOUND_JUNO,0,&proj.sound)==0);
    for (int n=0;n<4096 && engine_bass_preset_pending(voice);n++) render_blocks(1);
    CHECK(!engine_bass_preset_pending(voice));
    CHECK(engine_bass_bank(voice)==ORIGINAL_SOUND_JUNO && engine_bass_status(voice)==ORIGINAL_RUNTIME_OK);
    CHECK(engine_bass_sound_bank(&proj.sound,voice)==ORIGINAL_SOUND_JUNO);
    CHECK(!memcmp(preserved.pat,proj.pat,sizeof proj.pat));
    CHECK(!memcmp(preserved.arr.lane,proj.arr.lane,sizeof proj.arr.lane));
    CHECK(!memcmp(preserved.sound.v[T_303][1-voice],proj.sound.v[T_303][1-voice],32));
    CHECK(!memcmp(&preserved.sound.bass[1-voice],&proj.sound.bass[1-voice],sizeof proj.sound.bass[0]));
    CHECK(engine_bass_bank(1-voice)==other && engine_bass_preset_index(1-voice)==other_index);
}
int main(void)
{
    if (!original_runtime_kind_available(ORIGINAL_SOUND_DRONE)) {
        for (int voice=0;voice<2;voice++) for (int preset=0;preset<4;preset++)
            unavailable_slot(voice,preset);
        printf("Drone slot integration: shipping refusal/archive/escape, %u checks PASS\n",checks);
        return 0;
    }
    setup(V_PART,PART_303A);
    for (int voice=0;voice<2;voice++) {
        int other=engine_bass_bank(1-voice);
        CHECK(engine_bass_bank_available(voice,ORIGINAL_SOUND_DRONE));
        for (int preset=0;preset<4;preset++) {
            engine_bass_off(voice);
            CHECK(engine_bass_select_preset(voice,ORIGINAL_SOUND_DRONE,preset,&proj.sound)==0);
            for (int n=0;n<1024 && engine_bass_preset_pending(voice);n++) render_blocks(1);
            CHECK(!engine_bass_preset_pending(voice));
            CHECK(engine_bass_bank(voice)==ORIGINAL_SOUND_DRONE);
            CHECK(engine_bass_bank(1-voice)==other);
            CHECK(engine_bass_preset_index(voice)==preset);
            CHECK(engine_bass_sound_bank(&proj.sound,voice)==ORIGINAL_SOUND_DRONE);
            ui.part=(uint8_t)(PART_303A+voice);build_pages();
            CHECK(pg.n==UI_ORIGINAL_DRONE_PAGES+1);
            engine_bass_on_velocity(voice,60,0,0,100);
            render_blocks(32);
            float level,mod;unsigned state,down;
            CHECK(engine_bass_voice_view(voice,&level,&mod,&state,&down));
            CHECK(level>0.0f && down);
            engine_bass_off(voice);
            render_blocks(2048);
            CHECK(engine_bass_voice_view(voice,&level,&mod,&state,&down));
            CHECK(!down);
            ui.help=0;ui.msg_until=0;draw_main();draw_knobs();draw_footer();draw_header();
            CHECK(!strcmp(part_header_snapshot.name,voice?"DRONE B":"DRONE A"));
            shot(voice?"drone-b":"drone-a");
            ui.help=1;draw_header();shot(voice?"drone-b-help":"drone-a-help");ui.help=0;
        }
        CHECK(engine_bass_select_preset(voice,ORIGINAL_SOUND_CHICAGO,0,&proj.sound)==0);
        for(int n=0;n<1024 && engine_bass_preset_pending(voice);n++) render_blocks(1);
        CHECK(!engine_bass_preset_pending(voice));
        CHECK(engine_bass_bank(voice)==ORIGINAL_SOUND_CHICAGO);
    }
    printf("Drone slot integration: %u checks PASS\n",checks);return 0;
}

