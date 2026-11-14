/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual production UI, engine and framebuffer. The established SMC fixture
 * supplies simulated hardware/storage and clip-grid substitutes. These are
 * host renders, not firmware-emulator or device acceptance. */
#define SMC_UI_MAIN drone_unused_smc_main
#ifdef DRONE_UI_FIXTURE_FILE
#include DRONE_UI_FIXTURE_FILE
#else
#include "smc_ui_test.c"
#endif
#include "app/original_runtime.h"

static void du_audio(void) { int32_t out[512]; engine_render(out,256); }
static void du_wait(int voice)
{
    for (unsigned i=0;i<4096 && engine_bass_preset_pending(voice);i++) du_audio();
    CHECK(!engine_bass_preset_pending(voice));
}
static int du_sound_page(void)
{
    for (int p=0;p<pg.n;p++) if (pg.r[p][0].kind==R_BENGINE) return p;
    CHECK(0); return -1;
}
static void du_show_sound(void)
{
    build_pages(); *page_of(V_PART)=(uint8_t)du_sound_page(); build_pages();
    ui.touched=-1; ui.msg_until=0; ui_frame();
}
static void du_fixture(int voice,int bank,int factory,int edited)
{
    setup(V_PART,PART_303A+voice); engine_guard_enable(0);
    seq.playing=ui.rec=0; engine_bass_wheel_owners(0);
    CHECK(original_sound_init(proj.sound.v[T_303][voice],bank,factory));
    if (edited) {
        CHECK(original_sound_set(proj.sound.v[T_303][voice],0,127));
        CHECK(original_sound_set(proj.sound.v[T_303][voice],4,0));
        if (bank==ORIGINAL_SOUND_DRONE) {
            proj.sound.v[T_303][voice][6]=113;
            proj.sound.v[T_303][voice][7]=91;
            proj.sound.v[T_303][voice][22]|=0xc0u;
        }
    }
    engine_bass_sound_refresh(&proj.sound,voice);
    engine_project_hold(1);
    engine_init(proj.pat,&proj.arr.song,proj.arr.lane,&proj.sound);
    engine_apply_sound(&proj.sound); engine_project_release();
    for (int n=0;n<4;n++) du_audio();
    ui.part=(uint8_t)(PART_303A+voice); set_view(V_PART); du_show_sound();
    ui.dirty=save_wait=ui.save_failed=0; undo_start();
    CHECK(engine_bass_bank(voice)==bank);
    CHECK(engine_bass_preset_index(voice)==factory);
    CHECK(!engine_bass_preset_pending(voice));
}
typedef struct {
    project_t project;
    undo_t history;
    uint32_t serial,dirty;
    uint8_t saving;
} du_saved_t;
static du_saved_t du_saved;
static void du_remember(void)
{
    memcpy(&du_saved.project,&proj,sizeof proj);
    memcpy(&du_saved.history,&undo,sizeof undo);
    du_saved.serial=engine_bass_choice_serial(); du_saved.dirty=ui.dirty;
    du_saved.saving=save_wait;
}
static void du_unchanged(void)
{
    CHECK(!memcmp(&du_saved.project,&proj,sizeof proj));
    CHECK(!memcmp(&du_saved.history,&undo,sizeof undo));
    CHECK(du_saved.serial==engine_bass_choice_serial());
    CHECK(du_saved.dirty==ui.dirty && du_saved.saving==save_wait);
}
static void du_caption(const char *expected)
{ char text[32]; matrix_knob_caption(text,du_sound_page()); CHECK(!strcmp(text,expected)); }
static void du_value(pref_t ref,const char *expected)
{
    char number[64]={0},unit[32]={0};
    CHECK(original_value_text(ref,pref_get(ref),number,unit));
    CHECK(!strcmp(number,expected)); CHECK(!unit[0]);
}
static void du_capture(const char *name)
{
    const char *dir=getenv("SMC_UI_SHOTS");
    if (!dir) return;
    char path[1024]; ui.msg_until=0; ui.touched=-1;
    draw_header(); draw_main(); draw_knobs(); shot(name);
    snprintf(path,sizeof path,"%s/%s.rgb565",dir,name);
    FILE *file=fopen(path,"wb"); CHECK(file);
    CHECK(fwrite(fb,sizeof fb,1,file)==1); CHECK(fclose(file)==0);
    snprintf(path,sizeof path,"%s/%s.caption-y",dir,name);
    file=fopen(path,"w"); CHECK(file);
    CHECK(fprintf(file,"%d\n",MAIN_Y+4+(cur_page()%4)*(matrix_three_rows()?67:50))>0);
    CHECK(fclose(file)==0);
}
static void du_widget_bar(pref_t ref,int expected)
{
    const uint16_t background=RGB(3,7,11);
    int height=matrix_three_rows()?66:49;
    cv_begin(240,BAND_H,background);
    draw_param_widget(ref,63,0,59,1,matrix_col(),height,1);
    unsigned changed=0;
    for (int x=71;x<113;x++) changed+=cv_px[(height-4)*240+x]!=swap16(background);
    CHECK(changed==(expected?42u:0u));
}
static void du_restore_and_readonly(void)
{
    for (int voice=0;voice<2;voice++) for (int f=0;f<4;f++) for (int edited=0;edited<2;edited++) {
        du_fixture(voice,ORIGINAL_SOUND_DRONE,f,edited);
        CHECK(engine_bass_status(voice)==ORIGINAL_RUNTIME_UNAVAILABLE);
        CHECK(pg.n==1 && du_sound_page()==0);
        pref_t engine=PR(R_BENGINE,0,voice,0),preset=PR(R_BPRESET,0,voice,0);
        pref_t status=PR(R_BSTATUS,0,voice,0);
        CHECK(!pref_read_only(engine) && pref_read_only(preset) && pref_read_only(status));
        du_value(engine,"DRONE"); du_value(status,"UNAVAIL"); du_caption("KNOB 5");
        char num[16]; snprintf(num,sizeof num,edited?"%d*":"%d",f+1); du_value(preset,num);
        CHECK(tw(&FONT_B,"UNAVAIL")==54 && tw(&FONT_XS,"UNAVAIL")==43 && tw(&FONT_T,"UNAVAIL")==36);
        CHECK(original_value_font("UNAVAIL","",54)==&FONT_B);
        bstep_t *step=&proj.pat[0].bass[voice].step[7];
        step->note=61; step->velocity=97; step->flags=G_NOTE|BS_SLIDE;
        proj.pat[0].rsv[voice]=(uint8_t)(0xc1u+f);
        int lane=motion_alloc(proj.arr.lane,0,PART_303A+voice,T_MIX,PART_303A+voice,0);
        CHECK(lane>=0); proj.arr.lane[lane].val[5]=43;
        undo_start(); du_remember();
        /* Inserting a new current-pattern request is a lifecycle event. Let
         * the real dispatcher/boundary consume it before isolating rendering;
         * edited archived cells deliberately need a fresh silent recall. */
        pattern_sound_tick(); du_wait(voice);
        CHECK(!memcmp(&du_saved.project,&proj,sizeof proj));
        CHECK(!memcmp(&du_saved.history,&undo,sizeof undo));
        CHECK(du_saved.dirty==ui.dirty && du_saved.saving==save_wait);
        CHECK(engine_bass_choice_serial()==du_saved.serial+(edited?1u:0u));
        CHECK(engine_bass_bank(voice)==2 && engine_bass_status(voice)==-3);
        du_remember();
        for (int n=0;n<3;n++) { ui_frame(); draw_header(); draw_main(); draw_knobs(); }
        du_unchanged(); du_widget_bar(preset,0); du_widget_bar(status,0); du_unchanged();
        if (voice==1 && f==3 && edited) du_capture("terminal-drone");
        pref_set(preset,(f+1)%4); du_unchanged();
        pref_set_impl(preset,(f+1)%4,0); du_unchanged();
        turn_ref(preset,1,EN_K2); du_unchanged();
        pref_set(status,1); du_unchanged();
        seq.playing=ui.rec=1;
        for (unsigned id=0;id<original_sound_count(ORIGINAL_SOUND_DRONE);id++) {
            pref_t parameter=PR(R_ENG,T_303,voice,id);
            CHECK(pref_read_only(parameter) && !lockable(parameter));
            pref_set(parameter,17); du_unchanged();
            pref_set_impl(parameter,18,0); du_unchanged();
            turn_ref(parameter,1,EN_K1); du_unchanged();
            ui.held_step=7; lock_turn(parameter,1,EN_K1); du_unchanged(); ui.held_step=-1;
        }
        seq.playing=ui.rec=0;
        CHECK(engine_bass_bank(voice)==2 && engine_bass_status(voice)==-3);
    }
}
static void du_detent(int direction)
{ now_ms+=1000; enc_acc[EN_K1]=direction; ui_input_only(); }
static void du_selection(void)
{
    static const int banks[]={0,1,3};
    for (int voice=0;voice<2;voice++) for (unsigned i=0;i<3;i++) {
        du_fixture(voice,banks[i],0,0);
        du_remember(); pref_set_impl(PR(R_BENGINE,0,voice,0),2,0); du_unchanged();
        CHECK(!strcmp(ui.msg,"ENGINE UNAVAILABLE")); CHECK(engine_bass_bank(voice)==banks[i]);
        for (int d=-1;d<=1;d+=2) {
            du_fixture(voice,banks[i],0,0);
            int index=(int)i+d; if(index<0)index=0;if(index>2)index=2;
            uint32_t serial=engine_bass_choice_serial(); du_detent(d);
            CHECK(engine_bass_requested_bank(voice)==banks[index]);
            CHECK(engine_bass_choice_serial()==serial+(index!=(int)i));
            du_wait(voice); CHECK(engine_bass_bank(voice)==banks[index]);
        }
    }
    for(int voice=0;voice<2;voice++) for(int direction=-1;direction<=1;direction+=2) {
        du_fixture(voice,2,3,1); du_detent(direction);
        CHECK(engine_bass_requested_bank(voice)==(direction>0?3:1));
        CHECK(engine_bass_preset_pending(voice)==1); /* Real unavailable-slot escape, not a mocked queue. */
        du_wait(voice); CHECK(engine_bass_bank(voice)==(direction>0?3:1));
        CHECK(engine_bass_status(voice)==ORIGINAL_RUNTIME_OK);
    }
}
static void du_pending_and_cancel(void)
{
    for(int voice=0;voice<2;voice++) {
        du_fixture(voice,2,3,1); du_remember();
        engine_bass_wheel_owners(1u<<voice);
        pref_set(PR(R_BENGINE,0,voice,0),3);
        CHECK(engine_bass_bank(voice)==2 && engine_bass_status(voice)==-3);
        CHECK(engine_bass_requested_bank(voice)==3 && engine_bass_preset_pending(voice)==3);
        CHECK(!memcmp(&du_saved.project,&proj,sizeof proj));
        du_show_sound(); CHECK(pg.n==1); du_value(PR(R_BENGINE,0,voice,0),"JUNO");
        du_value(PR(R_BSTATUS,0,voice,0),"WHEEL"); du_caption("KNOBS 5-6");
        CHECK(!pref_read_only(PR(R_BPRESET,0,voice,0))); du_widget_bar(PR(R_BPRESET,0,voice,0),1);
        pref_set(PR(R_BPRESET,0,voice,0),1); CHECK(engine_bass_requested_preset(voice)==1);
        if(voice==1)du_capture("pending-juno-wheel");
        engine_bass_cancel_preset(voice); du_show_sound();
        CHECK(!engine_bass_preset_pending(voice)); du_value(PR(R_BSTATUS,0,voice,0),"UNAVAIL");
        du_value(PR(R_BENGINE,0,voice,0),"DRONE"); du_caption("KNOB 5");
        du_widget_bar(PR(R_BPRESET,0,voice,0),0);
        CHECK(!memcmp(&du_saved.project,&proj,sizeof proj));
        if(voice==1)du_capture("cancelled-drone");
        engine_bass_wheel_owners(0); for(int n=0;n<4;n++)du_audio();
        CHECK(engine_bass_bank(voice)==2 && engine_bass_status(voice)==-3);
    }
    /* Exercise real request admission reasons reachable from a DSP-less
     * placeholder. No fake HELD state is inserted: refused Drone onsets must
     * not acquire a voice or manufacture a release tail. */
    for(int voice=0;voice<2;voice++) for(int reason=0;reason<3;reason++) {
        du_fixture(voice,2,0,0);
        if(reason==1)engine_set(T_MIX,PART_303A+voice,MX_LEVEL,91);
        pref_set(PR(R_BENGINE,0,voice,0),3);
        if(reason==2) {
            int lane=motion_alloc(proj.arr.lane,0,PART_303A+voice,T_303,voice,0);
            CHECK(lane>=0);proj.arr.lane[lane].val[0]=41;
        }
        int pending=reason==0?1:reason==1?5:4;
        CHECK(engine_bass_preset_pending(voice)==pending);
        du_value(PR(R_BSTATUS,0,voice,0),reason==0?"WAIT":reason==1?"INPUT":"LOCKS");
        du_caption("KNOBS 5-6");engine_bass_cancel_preset(voice);
        CHECK(!engine_bass_preset_pending(voice));du_value(PR(R_BSTATUS,0,voice,0),"UNAVAIL");
    }
}
static void du_pattern_off(void)
{
    for(int voice=0;voice<2;voice++) for(int f=1;f<=4;f++) {
        du_fixture(voice,2,f-1,0);
        pref_t pattern=PR(R_BPATSND,0,voice,0);
        proj.pat[0].rsv[voice]=(uint8_t)(0xc0u+f); undo_start(); du_remember();
        CHECK(pref_get(pattern)==f);
        char number[32]={0},unit[32]={0}; pref_value_of(pattern,f,number,unit);
        CHECK(number[0]=='D' && number[1]=='0'+f);
        turn_ref(pattern,1,EN_K1); du_unchanged();
        for(int value=1;value<=4;value++) {pref_set_impl(pattern,value,0);du_unchanged();}
        turn_ref(pattern,-1,EN_K1);
        CHECK(proj.pat[0].rsv[voice]==0 && pref_get(pattern)==0 && ui.dirty);
        du_saved.project.pat[0].rsv[voice]=0;
        CHECK(!memcmp(&du_saved.project,&proj,sizeof proj));
        CHECK(engine_bass_choice_serial()==du_saved.serial);
        proj.pat[0].rsv[voice]=(uint8_t)(0xc0u+f);ui.dirty=0;
        pref_set_impl(pattern,0,0);CHECK(proj.pat[0].rsv[voice]==0 && ui.dirty);
    }
}
static void du_pending_archive(int voice,int factory,int phase)
{
    du_fixture(voice,ORIGINAL_SOUND_DRONE,factory-1,0);
    proj.pat[0].rsv[voice]=(uint8_t)(0xc0u+factory);
    pattern_sound_tick();du_wait(voice);
    CHECK(engine_bass_bank(voice)==ORIGINAL_SOUND_DRONE);
    CHECK(engine_bass_status(voice)==ORIGINAL_RUNTIME_UNAVAILABLE);
    if(phase==3)engine_bass_wheel_owners(1u<<voice);
    if(phase==5)engine_set(T_MIX,PART_303A+voice,MX_LEVEL,91);
    pref_set(PR(R_BENGINE,0,voice,0),ORIGINAL_SOUND_JUNO);
    if(phase==4) {
        int lane=motion_alloc(proj.arr.lane,0,PART_303A+voice,T_303,voice,0);
        CHECK(lane>=0);proj.arr.lane[lane].val[0]=41;
    }
    CHECK(engine_bass_requested_bank(voice)==ORIGINAL_SOUND_JUNO);
    CHECK(engine_bass_requested_preset(voice)==0);
    CHECK(engine_bass_preset_pending(voice)==phase);
    CHECK(proj.pat[0].rsv[voice]==(uint8_t)(0xc0u+factory));
    ui.dirty=save_wait=0;undo_start();du_remember();
}
static void du_pending_pattern_off(void)
{
    /* The stored byte owns a negative OFF gesture even while a different,
     * available engine is requested. Start with the reviewed C4/WHEEL failure;
     * all waits below come from actual engine admission, never fake state. */
    static const int factories[]={4,1,2,3},phases[]={3,1,4,5};
    for(int voice=0;voice<2;voice++) for(unsigned f=0;f<4;f++)
    for(unsigned p=0;p<4;p++) for(int gesture=0;gesture<3;gesture++) {
        int factory=factories[f],phase=phases[p];
        du_pending_archive(voice,factory,phase);
        pref_t pattern=PR(R_BPATSND,0,voice,0);
        CHECK(pref_get(pattern)==factory);
        char number[32]={0},unit[32]={0};pref_value_of(pattern,factory,number,unit);
        CHECK(number[0]=='D' && number[1]=='0'+factory && !number[2]);
        if(gesture==0) {
            turn_ref(pattern,-1,EN_K1);
            if(proj.pat[0].rsv[voice]!=0)
                fprintf(stderr,"archived negative: voice=%d raw-before=%02x raw-after=%02x requested=%d pending=%d dirty=%u\n",
                    voice,0xc0+factory,proj.pat[0].rsv[voice],engine_bass_requested_bank(voice),
                    engine_bass_preset_pending(voice),(unsigned)ui.dirty);
            CHECK(proj.pat[0].rsv[voice]==0 && pref_get(pattern)==0);
        } else if(gesture==1) {
            /* A positive gesture explicitly selects a future JUNO preset. */
            turn_ref(pattern,1,EN_K1);
            CHECK(proj.pat[0].rsv[voice]==bass_pattern_preset_encode(ORIGINAL_SOUND_JUNO,factory+1u));
        } else {
            pref_set_impl(pattern,2,0);
            CHECK(proj.pat[0].rsv[voice]==bass_pattern_preset_encode(ORIGINAL_SOUND_JUNO,2));
        }
        CHECK(ui.dirty==1);
        du_saved.project.pat[0].rsv[voice]=proj.pat[0].rsv[voice];du_saved.dirty=1;
        du_unchanged(); /* Only the explicit byte and dirty flag may change. */
        CHECK(engine_bass_bank(voice)==ORIGINAL_SOUND_DRONE && engine_bass_status(voice)==-3);
        CHECK(engine_bass_requested_bank(voice)==ORIGINAL_SOUND_JUNO);
        CHECK(engine_bass_requested_preset(voice)==0 && engine_bass_preset_pending(voice)==phase);
        if(gesture==0) {
            ui.dirty=0;du_remember();turn_ref(pattern,-1,EN_K1);du_unchanged();
            CHECK(!proj.pat[0].rsv[voice]); /* OFF stays an exact no-op. */
        }
        /* Repeated UI-frame adoption/cancellation belongs to the separate
         * pattern-transition fixture; this case isolates the control edge. */
    }
}
static void du_locks(void)
{
    for(int voice=0;voice<2;voice++) {
        du_fixture(voice,2,0,1);
        int lane=motion_alloc(proj.arr.lane,0,PART_303A+voice,T_303,voice,0);
        CHECK(lane>=0);proj.arr.lane[lane].val[0]=23;undo_start();du_remember();
        pref_set(PR(R_BENGINE,0,voice,0),3);du_unchanged();
        CHECK(!strcmp(ui.msg,"SOUND LOCKS OWN LANE"));
        CHECK(!engine_bass_preset_pending(voice));
        turn_ref(PR(R_BENGINE,0,voice,0),1,EN_K1);du_unchanged();
        CHECK(engine_bass_bank(voice)==2 && engine_bass_status(voice)==-3);
    }
}
static void du_available_and_captures(void)
{
    for(int voice=0;voice<2;voice++) for(int bank=0;bank<=3;bank++) {
        du_fixture(voice,bank,bank==2?3:0,bank==2);
        char name[64];snprintf(name,sizeof name,"sound-%s-%c",BENGINE_N[bank],voice?'B':'A');
        du_capture(name);
        if(bank==2)continue;
        int pages=bank==0?UI_ORIGINAL_CHICAGO_PAGES:bank==1?UI_ORIGINAL_MIAMI_PAGES:UI_ORIGINAL_JUNO_PAGES;
        CHECK(pg.n==pages+1);du_caption("KNOBS 5-6");
        pref_t preset=PR(R_BPRESET,0,voice,0),parameter=PR(R_ENG,T_303,voice,0);
        CHECK(!pref_read_only(preset) && !pref_read_only(parameter));du_widget_bar(preset,1);
        du_widget_bar(PR(R_BSTATUS,0,voice,0),0);
        du_remember();for(int n=0;n<3;n++)ui_frame();du_unchanged();
        *page_of(V_PART)=0;build_pages();int old=pref_get(parameter);
        pref_set(parameter,old==127?126:old+1);
        CHECK(pref_get(parameter)==(old==127?126:old+1) && ui.dirty);
    }
}
static void du_baseline_captures(void)
{
    for(int voice=0;voice<2;voice++) for(int bank=0;bank<=3;bank++) {
        du_fixture(voice,bank,bank==2?3:0,bank==2);
        char name[64];snprintf(name,sizeof name,"sound-%s-%c",BENGINE_N[bank],voice?'B':'A');
        du_capture(name);
    }
}
int main(int argc,char **argv)
{
    CHECK(!strcmp(SMK_MODEL,"SMK-37 Elite") || !strcmp(SMK_MODEL,"SMK-37 Pro"));
    CHECK(argc==2);
    if(!strcmp(argv[1],"baseline"))du_baseline_captures();
    else if(!strcmp(argv[1],"restore"))du_restore_and_readonly();
    else if(!strcmp(argv[1],"selection"))du_selection();
    else if(!strcmp(argv[1],"pending"))du_pending_and_cancel();
    else if(!strcmp(argv[1],"pattern"))du_pattern_off();
    else if(!strcmp(argv[1],"pending-pattern"))du_pending_pattern_off();
    else if(!strcmp(argv[1],"locks"))du_locks();
    else if(!strcmp(argv[1],"available"))du_available_and_captures();
    else CHECK(0);
    printf("drone_unavailable_ui model=%s scenario=%s PASS (%u checks); host substitutes, no emulator/device proof\n",SMK_MODEL,argv[1],checks);
    return 0;
}
