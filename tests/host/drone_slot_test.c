/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the drone through the production slot selector, renderer and UI. */
#define SMC_UI_MAIN retained_fixture_main
#include "smc_ui_test.c"
static void render_blocks(unsigned count)
{
    int32_t audio[512];
    while (count--) engine_render(audio,256);
}
int main(void)
{
    setup(V_PART,PART_303A);
    for (int voice=0;voice<2;voice++) {
        int other=engine_bass_bank(1-voice);
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

