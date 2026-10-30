/* SPDX-License-Identifier: GPL-3.0-only */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/factory_banks.c"
#include "../../firmware/src/app/bass_presets.h"
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
static int sound_page(void)
{
    for(int p=0;p<pg.n;p++) if(pg.r[p][0].kind==R_BENGINE) return p;
    return -1;
}
static void picture(const char *dir,const char *name,int voice)
{
    char path[512]; ui.part=PART_303A+voice;ui.view=V_PART;ui.overlay=O_NONE;ui.held_step=-1;
    build_pages();*page_of(V_PART)=(uint8_t)sound_page();ui.msg_until=0;ui.touched=-1;
    memset(blit_hash,0,sizeof blit_hash);draw_header();draw_main();draw_knobs();
    snprintf(path,sizeof path,"%s/%s-%d.png",dir,name,X0X_DEV_WOBBLE);png_write(path);
}
static void audio_block(void){int32_t audio[512];engine_render(audio,256);}
static int publication_probe;static unsigned publication_stages;
void bass_publish_test_hook(int voice,int stage)
{
    if(!publication_probe)return;
    CHECK(voice==0);CHECK(stage>=1 && stage<=3);publication_stages++;
    audio_block();CHECK(engine_bass_preset_index(0)==0);CHECK(engine_bass_preset_pending(0)==6);
}
static void request_apply(int voice,int bank,int preset)
{
    CHECK(engine_bass_select_preset(voice,bank,preset,&proj.sound)==0);
    CHECK(engine_bass_preset_pending(voice));audio_block();CHECK(!engine_bass_preset_pending(voice));
    CHECK(engine_bass_bank(voice)==bank);CHECK(engine_bass_preset_index(voice)==preset);
}
static void physical_browse(int voice)
{
    ui.part=PART_303A+voice;ui.view=V_PART;ui.overlay=O_NONE;ui.held_step=-1;build_pages();*page_of(V_PART)=(uint8_t)sound_page();
    memset(ui.enc_t,0,sizeof ui.enc_t);memset(ui.enc_run,0,sizeof ui.enc_run);
    for(int i=1;i<=6;i++){now_ms++;enc_acc[EN_K2]=1;ui_input_only();CHECK(engine_bass_requested_preset(voice)==i);}
    CHECK(engine_bass_preset_index(voice)==0);audio_block();CHECK(engine_bass_preset_index(voice)==6);
    for(int i=5;i>=0;i--){now_ms++;enc_acc[EN_K2]=-1;ui_input_only();CHECK(engine_bass_requested_preset(voice)==i);}
    audio_block();CHECK(engine_bass_preset_index(voice)==0);
    for(int i=1;i<=4;i++){now_ms++;enc_acc[EN_PRESET]=1;ui_input_only();CHECK(engine_bass_requested_preset(voice)==i);}
    audio_block();CHECK(engine_bass_preset_index(voice)==4);
    for(int i=3;i>=0;i--){now_ms++;enc_acc[EN_PRESET]=-1;ui_input_only();CHECK(engine_bass_requested_preset(voice)==i);}
    audio_block();CHECK(engine_bass_preset_index(voice)==0);
}
static void edited_pixels(int voice)
{
    unsigned colored=0;
    ui.part=PART_303A+voice;ui.view=V_PART;ui.overlay=O_NONE;ui.held_step=-1;
    build_pages();*page_of(V_PART)=(uint8_t)sound_page();ui.touched=-1;memset(blit_hash,0,sizeof blit_hash);draw_main();draw_knobs();
    /* EDITED stays in SOUND's display-only right half after its expansion. */
    CHECK(engine_bass_preset_edited(voice));
    int top=24+(cur_page()%4)*50+29;
    for(int y=top;y<top+12;y++)
        for(int x=125;x<234;x++) colored+=fb[y*240+x]==swap16(C_BLACK);
    CHECK(colored>20);
}
static uint32_t note_packet(int src,int off,int pitch)
{return (off?8u:9u)|((uint32_t)src<<4)|((uint32_t)((off?0x80:0x90)|seq.ch[TRK_BASS0])<<8)|((uint32_t)pitch<<16)|(100u<<24);}
static void queued_playback(void)
{
    int32_t audio[512];uint8_t before[32];uint32_t read;
    seq.playing=0;request_apply(0,0,0);seq.clk_src=SEQ_CLK_AUTO;
    for(int src=0;src<3;src++) {
        memcpy(before,proj.sound.v[T_303][0],32);
        for(int k=0;k<64;k++)min_q[mi_w++%MQ]=15u|(0xF8u<<8);
        min_q[mi_w++%MQ]=note_packet(src,0,48);read=mi_r;
        CHECK(engine_bass_select_preset(0,0,1,&proj.sound)==0);CHECK(mi_r==read);
        CHECK(engine_bass_preset_pending(0)==5);engine_render(audio,256);
        CHECK(engine_bass_preset_index(0)==0);CHECK(engine_bass_preset_pending(0)==5);
        engine_render(audio,256);CHECK(engine_midi_held()&1);CHECK(engine_bass_preset_pending(0)==2);
        CHECK(engine_bass_preset_index(0)==0);CHECK(!memcmp(before,proj.sound.v[T_303][0],32));
        /* Accepted overlapping notes slide on the old sound, then old-pitch
         * NoteOff cannot release the later owner or admit the preset early. */
        min_q[mi_w++%MQ]=note_packet(src,0,52);audio_block();
        min_q[mi_w++%MQ]=note_packet(src,1,48);audio_block();CHECK(engine_midi_held()&1);CHECK(engine_bass_preset_index(0)==0);
        min_q[mi_w++%MQ]=note_packet(src,1,52);
        for(int k=0;k<500 && engine_bass_preset_pending(0);k++){
            min_q[mi_w++%MQ]=15u|(0xF8u<<8);
            min_q[mi_w++%MQ]=11u|((0xB0u|seq.ch[TRK_BASS0])<<8)|(1u<<16)|(64u<<24);
            engine_render(audio,256);
        }
        CHECK(!engine_bass_preset_pending(0));CHECK(engine_bass_preset_index(0)==1);
        CHECK(mi_r==mi_w);CHECK(seq.clk_lock==1);request_apply(0,0,0);
    }
    seq.playing=1;seq.clk_src=SEQ_CLK_AUTO;seq.clk_lock=0;
    min_q[mi_w++%MQ]=15u|(0xF8u<<8);
    min_q[mi_w++%MQ]=11u|((0xB0u|seq.ch[TRK_BASS0])<<8)|(1u<<16)|(64u<<24);
    uint32_t frames=eng_prof_frames;request_apply(0,0,1);
    CHECK(eng_prof_frames==frames+256);CHECK(seq.playing);CHECK(seq.clk_lock==1);CHECK(mi_r==mi_w);
    for(int k=0;k<24;k++){min_q[mi_w++%MQ]=15u|(0xF8u<<8);audio_block();CHECK(seq.clk_lock==1);}
    /* No success-drop latch: the very next accepted live NoteOn still plays. */
    min_q[mi_w++%MQ]=note_packet(0,0,48);audio_block();CHECK(engine_midi_held()&1);
    min_q[mi_w++%MQ]=note_packet(0,1,48);seq.playing=0;run_ms(3000);
}
static int32_t break_reference[16][512];
static uint64_t break_tail(int change)
{
    int32_t audio[512];uint64_t h=1469598103934665603ull;int audible=0;
    boot();run_ms(50);engine_brk_live(0,1);
    for(int k=0;k<16;k++){
        if(change && k==2)CHECK(engine_bass_select_preset(0,0,0,&proj.sound)==0);
        engine_render(audio,256);
        if(change)CHECK(!memcmp(break_reference[k],audio,sizeof audio));else memcpy(break_reference[k],audio,sizeof audio);
        for(int j=0;j<512;j++){if(audio[j])audible=1;h=(h^(uint32_t)audio[j])*1099511628211ull;}
    }
    CHECK(audible);return h;
}
int main(int argc,char **argv)
{
    uint8_t expected[32];unsigned writes;boot();run_ms(50);
    CHECK(X0X_BASS_PRESET_PREVIEW==1);request_apply(0,0,0);
    CHECK(bass_preset_decode(0,0,engine_bass_bank_param,expected)==0);
    CHECK(!memcmp(expected,proj.sound.v[T_303][0],32));CHECK(!engine_bass_preset_edited(0));
    CHECK(!strcmp(part_display(PART_303A),"CHICAGO A"));CHECK(!strcmp(part_display(PART_303B),"CHICAGO B"));
    CHECK(tw(&FONT_B,part_display(PART_303A))+12<120);
    CHECK(tw(&FONT_XS,part_s(PART_303A))<=24);CHECK(tw(&FONT_XS,part_s(PART_303B))<=24);
    CHECK(!strcmp(bass_display_name(1,1,0),"MIAMI A"));CHECK(!strcmp(bass_display_name(1,1,1),"MIAMI B"));
    physical_browse(0);if(argc>1)picture(argv[1],"chicago-acid-lab-1",0);
    writes=store_writes;CHECK(project_save()==-9);CHECK(store_writes==writes);
    engine_bass_wheel_owners(1);CHECK(engine_bass_select_preset(0,0,2,&proj.sound)==0);
    audio_block();CHECK(engine_bass_preset_pending(0)==3);CHECK(engine_bass_preset_index(0)==0);
    engine_bass_cancel_preset(0);audio_block();CHECK(!engine_bass_preset_pending(0));CHECK(engine_bass_preset_index(0)==0);engine_bass_wheel_owners(0);
    proj.arr.lane[0].used=1;proj.arr.lane[0].t=T_303;proj.arr.lane[0].v=0;
    CHECK(engine_bass_select_preset(0,0,1,&proj.sound)==-4);CHECK(!engine_bass_preset_pending(0));proj.arr.lane[0].used=0;
#if X0X_DEV_WOBBLE
    ui.part=PART_303B;ui.view=V_PART;build_pages();*page_of(V_PART)=(uint8_t)sound_page();
    pref_set(PR(R_BENGINE,0,1,0),1);CHECK(engine_bass_requested_bank(1)==1);CHECK(engine_bass_bank(1)==0);
    CHECK(!strcmp(part_display(PART_303B),"CHICAGO B")); /* pending != committed */
    CHECK(!engine_bass_bank_available(0,1));CHECK(engine_bass_select_preset(0,1,0,&proj.sound)==-2);
    audio_block();ui_input_only();CHECK(engine_bass_bank(1)==1);CHECK(engine_nparams(T_303,1)==23);
    CHECK(bass_preset_decode(1,0,engine_bass_bank_param,expected)==0);CHECK(!memcmp(expected,proj.sound.v[T_303][1],32));
    CHECK(!strcmp(part_display(PART_303A),"CHICAGO"));CHECK(!strcmp(part_display(PART_303B),"MIAMI"));
    CHECK(tw(&FONT_XS,part_s(PART_303A))<=24);CHECK(tw(&FONT_XS,part_s(PART_303B))<=24);
    CHECK(engine_bass_select_preset(1,0,0,&proj.sound)==0);CHECK(!strcmp(part_display(PART_303B),"MIAMI"));
    engine_bass_cancel_preset(1);audio_block();CHECK(!strcmp(part_display(PART_303B),"MIAMI"));
    physical_browse(1);if(argc>1)picture(argv[1],"miami-16th-cutz",1);
    if(argc>1)picture(argv[1],"miami-occupied",0);
    pref_set(PR(R_ENG,T_303,1,W_CARRIER_LEVEL),50);audio_block();CHECK(engine_bass_preset_edited(1));edited_pixels(1);
    if(argc>1)picture(argv[1],"miami-edited",1);
    request_apply(1,1,0);engine_bass_on_velocity(1,48,1,0,110);
    {int32_t audio[512];int audible=0;for(int k=0;k<10;k++){engine_render(audio,256);for(int j=0;j<512;j++)if(audio[j])audible=1;}CHECK(audible);}
    CHECK(engine_bass_select_preset(1,1,1,&proj.sound)==0);audio_block();CHECK(engine_bass_preset_pending(1)==2);CHECK(engine_bass_preset_index(1)==0);
    engine_bass_on_velocity(1,52,1,1,110);audio_block();CHECK(engine_bass_preset_index(1)==0);
    engine_bass_off(1);for(int k=0;k<1000 && engine_bass_preset_pending(1);k++)audio_block();
    CHECK(!engine_bass_preset_pending(1));CHECK(engine_bass_preset_index(1)==1);
#else
    CHECK(!engine_bass_bank_available(0,1));CHECK(engine_bass_select_preset(0,1,0,&proj.sound)==-2);
#endif
    request_apply(0,0,0);
    CHECK(engine_bass_select_preset(0,0,1,&proj.sound)==0);
    publication_probe=1;CHECK(engine_bass_select_preset(0,0,2,&proj.sound)==0);publication_probe=0;
    CHECK(publication_stages==3);CHECK(engine_bass_requested_preset(0)==2);
    audio_block();CHECK(engine_bass_preset_index(0)==2);
    queued_playback();
    engine_bass_on(0,48,100,0);audio_block();CHECK(engine_bass_select_preset(0,0,4,&proj.sound)==0);audio_block();
    CHECK(engine_bass_preset_pending(0)==2);if(argc>1)picture(argv[1],"chicago-pending-note",0);
    /* Physical cancel while held preserves actual sounding selection. */
    ui.part=PART_303A;build_pages();*page_of(V_PART)=(uint8_t)sound_page();held_btn=1u<<B_HOME;ui_input_only();held_btn=0;ui_input_only();
    CHECK(!engine_bass_preset_pending(0));engine_bass_off(0);run_ms(3000);
    uint64_t control=break_tail(0),requested=break_tail(1);CHECK(control==requested);
    printf("PASS %u voice-scoped live preset/input/ownership/UI checks WBL%d; BREAK tail hash=%016llx\n",checks,X0X_DEV_WOBBLE,(unsigned long long)control);
    return 0;
}
