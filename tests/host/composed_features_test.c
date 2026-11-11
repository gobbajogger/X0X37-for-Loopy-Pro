/* SPDX-License-Identifier: GPL-3.0-only */
#define main saved_host_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/bass_presets.h"
static unsigned checked,failed;
#define TEST(x) do { checked++;if(!(x)){failed++;fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);} } while(0)
static void render(void) { static int32_t b[512];engine_render(b,256); }
static const x0x_param_t *descriptor(int bank,int parameter) { return original_sound_param((unsigned)bank,(unsigned)parameter); }
static void masks(void)
{
    uint8_t h[PROJECT_FILE_HDR];settings_t saved;
    for(unsigned mask=0;mask<32;mask++) {
        part_off_live=(uint8_t)mask;project_apply_io();TEST(seq.part_off==mask);
        project_capture_stage();TEST(part_off_staged==mask);
        project_file_header(h);TEST(project_file_validate(h)==0);
        part_off_live=0;project_commit_stage();project_apply_io();TEST(seq.part_off==mask);
        TEST(save_settings(&proj.set,(uint8_t)mask)==0);TEST(load_settings(&saved)==0);
        TEST(io_buf[4+sizeof saved]==mask);TEST(!memcmp(&saved,&proj.set,sizeof saved));
        unsigned word=get32(h+28);TEST(PROJ_FILE_OFF_OK(word));
        if(mask)for(unsigned b=0;b<32;b++)TEST(!PROJ_FILE_OFF_OK(word^(1u<<b)));
    }
    part_off_live=0;project_apply_io();
    for(int v=0;v<NBASS;v++) {
        engine_bass_on(v,60,0,0);render();render();TEST(engine_bass_keys_held()&(1<<v));
        seq.part_off=(uint8_t)(1u<<(TRK_BASS0+v));engine_bass_off(v);render();TEST(!(engine_bass_keys_held()&(1<<v)));
        engine_bass_on(v,62,0,0);render();TEST(!(engine_bass_keys_held()&(1<<v)));
        seq.part_off=0;
    }
}
static void presets(void)
{
    uint8_t cell[NPARAMS_MAX];
    for(unsigned kind=0;kind<2;kind++)for(unsigned n=0;n<original_sound_factories(kind);n++)
        TEST(bass_preset_decode((int)kind,(int)n,descriptor,cell)==BASS_PRESET_OK);
}
static void reset_and_play(void)
{
    run_ms(80);ui.overlay=O_NONE;ui.view=V_PART;ui.part=PART_909;
    sound_t saved,factory;engine_sound_defaults(&factory);
    proj.sound.v[T_909][0][0]^=31;proj.sound.v[T_MIX][0][MX_PAN]=11;seq.mute=1;
    saved=proj.sound;undo_commit_stable();
    reset_sound(PART_909);TEST(!memcmp(proj.sound.v[T_909],factory.v[T_909],sizeof proj.sound.v[T_909]));
    TEST(!memcmp(proj.sound.v[T_MIX],saved.v[T_MIX],sizeof proj.sound.v[T_MIX]));TEST(seq.mute==1);
    do_undo(0);TEST(!memcmp(&proj.sound,&saved,sizeof saved));
    ui.btn=0;seq.playing=0;ui.overlay=O_ASK;ui.ask_overlay=O_NONE;
    button_tap(B_PLAY);render();TEST(ui.overlay!=O_ASK);TEST(seq.playing);
    seq_stop(&seq);render();ui.overlay=O_ASK;ui.btn=1u<<B_HOME;
    button_tap(B_PLAY);render();TEST(!seq.playing);TEST(ui.overlay==O_ASK);
    ui.btn=0;ui.overlay=O_NONE;
}
static void recording(void)
{
    part_off_live=0;project_apply_io();seq.mute=0;seq.rec_mask=1;seq_start(&seq);render();
    engine_bass_record(0,48,100,0);render();TEST(seq.rec_bass[0].active);
    engine_request_notes_off();render();TEST(!seq.rec_bass[0].active);TEST(!seq.rec_bass[0].own_ok);TEST(seq.playing);TEST(seq.rec_mask==1);
    engine_bass_record(0,60,100,0);render();TEST(seq.rec_bass[0].active);TEST(seq.rec_bass[0].own_note==60);
    engine_bass_off(0);render();TEST(!seq.rec_bass[0].active);
}
int main(void){boot();run_ms(80);presets();masks();reset_and_play();recording();printf("Composed features: %u checks, %u failures\n",checked,failed);return failed?1:0;}
