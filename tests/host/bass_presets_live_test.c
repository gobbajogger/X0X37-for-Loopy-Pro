/* SPDX-License-Identifier: GPL-3.0-only */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/factory_banks.c"
#include "../../firmware/src/app/bass_presets.h"
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
static void picture(const char *dir,const char *name,int voice)
{
    char path[512]; ui.part=PART_303A+voice;ui.view=V_PART;ui.overlay=O_NONE;ui.held_step=-1;
    build_pages();*page_of(V_PART)=0;ui.msg_until=0;ui.touched=-1;
    memset(blit_hash,0,sizeof blit_hash);draw_header();draw_main();draw_knobs();
    snprintf(path,sizeof path,"%s/%s-%d.png",dir,name,X0X_DEV_WOBBLE);png_write(path);
}
static void physical_browse(int voice)
{
    ui.part=PART_303A+voice;ui.view=V_PART;ui.overlay=O_NONE;ui.held_step=-1;
    build_pages();*page_of(V_PART)=0;
    memset(ui.enc_t,0,sizeof ui.enc_t);memset(ui.enc_run,0,sizeof ui.enc_run);
    for(int i=1;i<=6;i++){now_ms++;enc_acc[EN_K2]=1;ui_input_only();CHECK(engine_bass_preset_index(voice)==i);}
    for(int i=5;i>=0;i--){now_ms++;enc_acc[EN_K2]=-1;ui_input_only();CHECK(engine_bass_preset_index(voice)==i);}
    for(int i=1;i<=4;i++){now_ms++;enc_acc[EN_PRESET]=1;ui_input_only();CHECK(engine_bass_preset_index(voice)==i);}
    for(int i=3;i>=0;i--){now_ms++;enc_acc[EN_PRESET]=-1;ui_input_only();CHECK(engine_bass_preset_index(voice)==i);}
}
static void edited_pixels(int voice)
{
    uint16_t actual[16*80];unsigned colored=0;
    ui.part=PART_303A+voice;ui.view=V_PART;ui.overlay=O_NONE;ui.held_step=-1;
    build_pages();*page_of(V_PART)=0;ui.touched=-1;memset(blit_hash,0,sizeof blit_hash);draw_main();
    CHECK(FONT_S.h<=16);CHECK(12+FONT_S.h<=BAND_H);
    for(int y=0;y<16;y++)for(int x=0;x<80;x++)actual[y*80+x]=fb[(MAIN_Y+12+y)*240+148+x];
    cv_begin(240,BAND_H,C_BLACK);cv_text(148,12,&FONT_S,"EDITED",C_AMB);cv_commit(1,0,MAIN_Y);
    for(int y=0;y<16;y++)for(int x=0;x<80;x++) {uint16_t expected=fb[(MAIN_Y+12+y)*240+148+x];CHECK(actual[y*80+x]==expected);colored+=expected!=0;}
    CHECK(colored>20);
}
static uint32_t note_packet(int src,int off)
{ return (off?8u:9u)|((uint32_t)src<<4)|((uint32_t)((off?0x80:0x90)|seq.ch[TRK_BASS0])<<8)|(48u<<16)|(100u<<24); }
static void queued_midi_boundary(void)
{
    int32_t audio[512];uint8_t before[32];uint32_t read;
    CHECK(engine_bass_select_preset(0,0,0,&proj.sound)==0);run_ms(50);
    for(int src=0;src<3;src++) {
        memcpy(before,proj.sound.v[T_303][0],32);read=mi_r;
        uint32_t packet=note_packet(src,0);min_q[mi_w++%MQ]=packet;
        CHECK(plat_midi_pending());CHECK(mi_r==read);CHECK(min_q[read%MQ]==packet);
        CHECK(engine_bass_select_preset(0,0,1,&proj.sound)==-3);
        CHECK(mi_r==read && mi_w-read==1);CHECK(engine_bass_preset_index(0)==0);
        CHECK(!memcmp(before,proj.sound.v[T_303][0],32));CHECK(engine_bass_bank(0)==0);
        engine_render(audio,256);CHECK(engine_midi_held()&1);CHECK(!plat_midi_pending());
        int audible=0;for(int k=0;k<10;k++){engine_render(audio,256);for(int j=0;j<512;j++)if(audio[j])audible=1;}CHECK(audible);
        min_q[mi_w++%MQ]=note_packet(src,1);run_ms(2000);CHECK(!engine_project_notes());
    }
    /* Successful quiet admission has no preexisting packet. Transaction-era
     * input follows existing release policy; fresh input after empty survives. */
    CHECK(!plat_midi_pending());CHECK(engine_bass_select_preset(0,0,0,&proj.sound)==0);
    min_q[mi_w++%MQ]=note_packet(0,0);engine_render(audio,256);
    CHECK(!(engine_midi_held()&1));CHECK(!plat_midi_pending());
    min_q[mi_w++%MQ]=note_packet(0,0);engine_render(audio,256);CHECK(engine_midi_held()&1);
    min_q[mi_w++%MQ]=note_packet(0,1);run_ms(2000);
    seq.clk_src=SEQ_CLK_AUTO;read=mi_r;min_q[mi_w++%MQ]=15u|(0xFAu<<8);
    CHECK(engine_bass_select_preset(0,0,1,&proj.sound)==-3);CHECK(mi_r==read);
    CHECK(engine_bass_preset_index(0)==0);engine_render(audio,256);
    CHECK(seq.playing || seq.req_start);CHECK(!plat_midi_pending());
    min_q[mi_w++%MQ]=15u|(0xFCu<<8);run_ms(2000);
}
int main(int argc,char **argv)
{
    uint8_t expected[32], old[32];unsigned writes;int32_t audio[512];
    CHECK(X0X_BASS_PRESET_PREVIEW==1);boot();run_ms(50);
    CHECK(engine_bass_select_preset(0,0,0,&proj.sound)==0);
    CHECK(engine_bass_bank(0)==0);CHECK(engine_nparams(T_303,0)==32);
    CHECK(bass_preset_decode(0,0,engine_bass_bank_param,expected)==0);
    CHECK(!memcmp(expected,proj.sound.v[T_303][0],32));CHECK(!engine_bass_preset_edited(0));
    CHECK(engine_bass_preset_index(0)==0);CHECK(engine_bass_preview_changed());
    physical_browse(0);
    if(argc>1)picture(argv[1],"chicago-acid-lab-1",0);
    writes=store_writes;CHECK(project_save()==-9);CHECK(store_writes==writes);
    memcpy(old,proj.sound.v[T_303][0],32);engine_bass_wheel_owners(1);
    CHECK(engine_bass_select_preset(0,0,1,&proj.sound)==-3);CHECK(!memcmp(old,proj.sound.v[T_303][0],32));engine_bass_wheel_owners(0);
    engine_bass_on(0,48,100,0);CHECK(engine_bass_select_preset(0,0,1,&proj.sound)==-3);
    { int audible=0; for(int k=0;k<10;k++){engine_render(audio,256);for(int j=0;j<512;j++)if(audio[j])audible=1;} CHECK(audible); }
    engine_bass_off(0);run_ms(1500);
    CHECK(engine_bass_select_preset(0,0,1,&proj.sound)==0);CHECK(engine_bass_preset_index(0)==1);
    run_ms(20);
#if X0X_DEV_WOBBLE
    ui.part=PART_303B;ui.view=V_PART;build_pages();*page_of(V_PART)=0;
    pref_set(PR(R_BENGINE,0,1,0),1);
    CHECK(engine_bass_preset_index(1)==0);
    CHECK(engine_bass_bank(1)==1);CHECK(engine_nparams(T_303,1)==23);
    CHECK(bass_preset_decode(1,0,engine_bass_bank_param,expected)==0);
    CHECK(!memcmp(expected,proj.sound.v[T_303][1],32));CHECK(!engine_bass_preset_edited(1));
    physical_browse(1);
    if(argc>1)picture(argv[1],"miami-16th-cutz",1);
    engine_bass_on(1,48,100,0);
    { int audible=0;for(int k=0;k<10;k++){engine_render(audio,256);for(int j=0;j<512;j++)if(audio[j])audible=1;}CHECK(audible); }
    engine_bass_off(1);run_ms(2000);
    CHECK(!engine_bass_bank_available(0,1));CHECK(engine_bass_select_preset(0,1,0,&proj.sound)==-2);
    if(argc>1)picture(argv[1],"miami-occupied",0);
    proj.arr.lane[0].used=1;proj.arr.lane[0].t=T_303;proj.arr.lane[0].v=1;
    CHECK(engine_bass_select_preset(1,0,0,&proj.sound)==-4);CHECK(engine_bass_bank(1)==1);
    proj.arr.lane[0].used=0;
    pref_set(PR(R_ENG,T_303,1,W_CARRIER_LEVEL),50);run_ms(20);CHECK(engine_bass_preset_edited(1));
    edited_pixels(1);
    if(argc>1)picture(argv[1],"miami-edited",1);
#else
    CHECK(!engine_bass_bank_available(0,1));CHECK(engine_bass_select_preset(0,1,0,&proj.sound)==-2);
#endif
    queued_midi_boundary();
    printf("PASS %u live backend/preset/UI/save checks WBL%d\n",checks,X0X_DEV_WOBBLE);return 0;
}
