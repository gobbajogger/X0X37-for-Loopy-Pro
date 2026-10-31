/* SPDX-License-Identifier: GPL-3.0-only */
/* Original-control adapter in the actual released UI, without a donor renderer. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../experiments/original_synth/chicago_voice.h"
#include "../../experiments/original_synth/miami_selected_voice.h"

static unsigned checks, captures;
static const char *image_dir;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static void block(void) { int32_t audio[512]; engine_render(audio,256); }
static void settle(void) { for (int i=0;i<12;i++) block(); }
static void choose(unsigned voice,unsigned kind,unsigned factory)
{
    CHECK(engine_bass_select_preset((int)voice,(int)kind,(int)factory,&proj.sound)==0); settle();
    CHECK(!engine_bass_preset_pending((int)voice)); CHECK(engine_bass_status((int)voice)==1);
}
static void screen(int view,int part,int page)
{
    ui.view=(uint8_t)view; ui.part=(uint8_t)part; ui.overlay=O_NONE; ui.help=0;
    ui.touched=-1; ui.msg_until=ui.touch_until=0; ui.btn=ui.keys=0;
    ui.page[view][view==V_PART || view==V_SEQ || view==V_GEN?part:0]=(uint8_t)page;
    build_pages(); ui_frame();
}
static void shot(const char *name)
{
    if (!image_dir) return;
    char path[1024]; CHECK(snprintf(path,sizeof path,"%s/%s.png",image_dir,name)>0);
    png_write(path); captures++;
}
static void capture(const char *name,int view,int part,int page)
{ screen(view,part,page); shot(name); }
static void text_of(pref_t r,char *text)
{
    char unit[8]; pref_value(r,text,unit); put_s(text+strlen(text),unit);
}
static uint32_t bits(float x) { uint32_t u; memcpy(&u,&x,4); return u; }

/* Independent native compiler readback, not the UI getters/quantization. */
static void projection_one(const uint8_t cell[32],const uint8_t preview[22],uint32_t mask)
{
    ui_original_control_view v; uint8_t before[32]; memcpy(before,cell,32);
    CHECK(ui_original_controls_project(&v,cell,preview,mask)); CHECK(!memcmp(before,cell,32));
    if (!cell[29]) {
        original_chicago_program p; original_chicago_voice_controls c;
        CHECK(original_sound_restore_chicago(&p,cell));
        for (unsigned id=0;id<22;id++) if (mask&(1u<<id)) {
            float x; CHECK(original_sound_control_value(0,id,preview[id],&x)); CHECK(original_chicago_program_set(&p,id,x));
        }
        CHECK(original_chicago_voice_compile(&c,&p));
        CHECK(v.arp_on==c.routing.arp_enable && v.arp_mode==c.routing.arp_mode && v.arp_range==c.routing.arp_range);
        CHECK(v.amp_gate==c.routing.vca_eg_mode); CHECK(fabsf(v.env_percent/100.0f-c.env_depth)<=.00501f);
    } else {
        original_miami_program p; original_miami_voice_controls c;
        CHECK(original_sound_restore_miami(&p,cell));
        for (unsigned id=0;id<18;id++) if (mask&(1u<<id)) {
            float x=preview[id]/127.0f;
            if (id==7) x=preview[id]<64?preview[id]/128.0f:.5f+(preview[id]-64)/126.0f;
            CHECK(original_miami_program_set(&p,id,x));
        }
        CHECK(original_miami_voice_compile(&c,&p));
        CHECK(v.shape[0]==c.shape[0] && v.shape[1]==c.shape[1] && v.shape[2]==c.mg_shape[0]);
        CHECK(v.carrier_pitch==c.pitch.semitone[0]); CHECK(fabsf(v.xmod_tune/1000.0f-c.patch_slave)<=.000501f);
    }
}
static void projection_cases(void)
{
    uint8_t cell[32],preview[22]={0};
    static const unsigned ids[2][6]={{4,11,13,14,15,16},{0,3,6,7,11,13}};
    for (unsigned kind=0;kind<2;kind++) {
        for (unsigned f=0;f<original_sound_factories(kind);f++) {
            CHECK(original_sound_init(cell,kind,f)); projection_one(cell,preview,0);
            const char *name=original_sound_name(kind,f); CHECK(name && strlen(name)<24);
            /* Full names fit the existing readout/list using existing fonts. */
            char label[24]; snprintf(label,sizeof label,"PRESET %u",f+1);
            CHECK(tw(&FONT_T,caps_of(name)) <= 224-24-tw(&FONT_B,label)-10);
        }
        CHECK(original_sound_init(cell,kind,0));
        for (unsigned i=0;i<6;i++) for (unsigned q=0;q<128;q++) {
            memset(preview,0,sizeof preview); preview[ids[kind][i]]=(uint8_t)q;
            projection_one(cell,preview,1u<<ids[kind][i]);
        }
        uint32_t relevant=kind?UI_ORIGINAL_MIAMI_TEXT_MASK:UI_ORIGINAL_CHICAGO_TEXT_MASK;
        ui_original_control_view base,changed; memset(preview,0,sizeof preview);
        CHECK(ui_original_controls_project(&base,cell,preview,0));
        for (unsigned id=0;id<original_sound_count(kind)-2;id++) if (!(relevant&(1u<<id))) {
            unsigned max=original_sound_param(kind,id)->max;
            for (unsigned q=0;q<=max;q++) {
                preview[id]=(uint8_t)q; CHECK(ui_original_controls_project(&changed,cell,preview,1u<<id));
                CHECK(!memcmp(&base,&changed,sizeof base));
            }
            preview[id]=0;
        }
    }
    ui_original_control_cache cache={0}; memset(preview,0,sizeof preview); CHECK(original_sound_init(cell,0,0));
    CHECK(ui_original_controls_update(&cache,0,cell,preview,0));
    /* A sentinel proves a hit did not project/overwrite the cached native readout. */
    cache.view.env_percent=37;
    for (unsigned i=0;i<1024;i++) CHECK(ui_original_controls_update(&cache,0,cell,preview,0) && cache.view.env_percent==37);
    CHECK(ui_original_controls_update(&cache,1,cell,preview,0) && cache.view.env_percent!=37);
    preview[4]=cell[4]; CHECK(ui_original_controls_update(&cache,1,cell,preview,1u<<4)); CHECK(cache.preview_mask==(1u<<4));
    CHECK(!ui_original_controls_project(&cache.view,cell,preview,1u<<22));
    cell[28]=0; CHECK(!ui_original_controls_update(&cache,1,cell,preview,0) && !cache.valid);
    CHECK(sizeof(ui_original_control_cache)==164 && sizeof(ui_original_control_view)==104);
}
static void pages_and_edits(void)
{
    static const int8_t rows[2][7][4]={
        {{0,1,-1,-1},{2,3,4,-1},{5,6,-1,-1},{7,8,9,10},{11,12,-1,-1},{13,15,14,16},{17,18,19,-1}},
        {{0,1,2,3},{6,5,7,8},{9,10,12,-1},{11,13,-1,-1},{4,15,14,-1}}
    };
    for (unsigned kind=0;kind<2;kind++) for (unsigned voice=0;voice<2;voice++) {
        unsigned part=PART_303A+voice,n=kind?5:7; choose(voice,kind,0); screen(V_PART,part,0);
        CHECK(pg.n==(int)n+1); uint32_t seen=0;
        for (unsigned page=0;page<n;page++) for (unsigned col=0;col<4;col++) {
            pref_t r=pg.r[page][col]; int id=rows[kind][page][col];
            if (id<0) { CHECK(r.kind==R_NONE); continue; }
            CHECK(r.kind==R_ENG && r.a==T_303 && r.b==voice && r.c==id); CHECK(!(seen&(1u<<id))); seen|=1u<<id;
            CHECK(tw(&FONT_XS,caps_of(pref_name(r)))<=54);
            uint8_t saved[32]; memcpy(saved,proj.sound.v[T_303][voice],32);
            screen(V_PART,part,page); int value=pref_get(r),direction=value==pref_desc(r)->max?-1:1;
            now_ms+=500; enc_acc[EN_K1+col]=direction; ui_input_only(); block();
            CHECK(pref_get(r)!=value); CHECK(original_sound_edit_mask(proj.sound.v[T_303][voice])&(1u<<id));
            for (unsigned p=0;p<original_sound_count(kind);p++) if (p!=(unsigned)id) CHECK(proj.sound.v[T_303][voice][p]==saved[p]);
        }
        CHECK(seen==((1u<<(kind?16:20))-1)); CHECK(pg.r[n][0].kind==R_BENGINE && pg.r[n][1].kind==R_BPRESET);
        CHECK(pg.r[n][2].kind==R_BSTATUS && pg.r[n][3].kind==R_NONE);
        for (unsigned id=kind?16:20;id<original_sound_count(kind);id++) {
            pref_t r=PR(R_ENG,T_303,voice,id); uint8_t saved[32]; memcpy(saved,proj.sound.v[T_303][voice],32);
            CHECK(pref_read_only(r) && !lockable(r)); turn_ref(r,1,EN_K1); CHECK(!memcmp(saved,proj.sound.v[T_303][voice],32));
        }
    }
}
static void selector_cases(void)
{
    static const struct {unsigned kind,id,n;const char *text[10];} cases[]={
        {0,11,2,{"ENV","GATE"}},{0,13,2,{"OFF","ON"}},{0,14,3,{"DOWN","UP","UP/DN"}},
        {0,15,3,{"1 OCT","2 OCT","FULL"}},
        {0,16,10,{"1/2","1/4D","1/2T","1/4","1/8D","1/4T","1/8","1/8T","1/16","1/32"}},
        {1,0,3,{"TRI","SAW","SQR"}},{1,6,4,{"TRI","SAW","SQR","PUL"}},
        {1,11,4,{"TRI","DOWN","UP","SQR"}},{1,13,8,{"1/2","1/4","1/8","1/16","1/3","1/6","1/12","1/24"}},
        {1,3,25,{0}}
    };
    for (unsigned c=0;c<sizeof cases/sizeof cases[0];c++) {
        choose(0,cases[c].kind,0); screen(V_PART,PART_303A,0);
        pref_t r=PR(R_ENG,T_303,0,cases[c].id); pref_set(r,0); block();
        for (int direction=1;direction>=-1;direction-=2) for (unsigned i=0;i<cases[c].n;i++) {
            unsigned state=direction>0?i:cases[c].n-i-1; char actual[24],expected[24]; text_of(r,actual);
            if (cases[c].id==3 && cases[c].kind) snprintf(expected,sizeof expected,"%s%d",state>12?"+":"",(int)state-12);
            else snprintf(expected,sizeof expected,"%s",cases[c].text[state]);
            CHECK(!strcmp(actual,expected)); now_ms+=500; turn_ref(r,direction,EN_K1); block();
            if (i+1==cases[c].n) CHECK(pref_get(r)==(direction>0?127:0));
        }
    }
    for (unsigned kind=0;kind<2;kind++) {
        choose(0,kind,0); screen(V_PART,PART_303A,1); pref_t r=PR(R_ENG,T_303,0,kind?7:4);
        for (unsigned q=63;q<=65;q++) {
            pref_set(r,(int)q); block(); char text[24]; text_of(r,text);
            CHECK(!strcmp(text,kind?(q==63?"-0.08":q==64?"0.00":"+0.08"):(q==63?"-2%":q==64?"0%":"2%")));
        }
        pref_set(r,64); block(); float exact; CHECK(original_sound_value(proj.sound.v[T_303][0],r.c,&exact) && exact==.5f);
    }
}
static void locks_and_undo(void)
{
    choose(0,1,0); screen(V_PART,PART_303A,3);
    pref_t r=PR(R_ENG,T_303,0,13); pref_set(r,0); block(); uint8_t saved[32]; memcpy(saved,proj.sound.v[T_303][0],32);
    ui.held_step=0; ui.held_step_part=PART_303A; ui.held_step_pat=seq.ppat[PART_303A]; ui.rec=0;
    lock_turn(r,1,EN_K2); block(); char text[24]; text_of(r,text); CHECK(!strcmp(text,"1/4")); CHECK(!memcmp(saved,proj.sound.v[T_303][0],32));
    lock_release(); ui.held_step=-1; block(); text_of(r,text); CHECK(!strcmp(text,"1/2"));
    int lane=motion_find(proj.arr.lane,seq.ppat[PART_303A],PART_303A,T_303,0,13); CHECK(lane>=0); engine_motion_clear(lane); block();
    choose(0,1,0); screen(V_PART,PART_303A,1); r=PR(R_ENG,T_303,0,7); memcpy(saved,proj.sound.v[T_303][0],32);
    float exact; CHECK(original_sound_value(saved,7,&exact));
    ui.held_step=0; ui.held_step_part=PART_303A; ui.held_step_pat=seq.ppat[PART_303A];
    lock_turn(r,1,EN_K3); block(); CHECK(!memcmp(saved,proj.sound.v[T_303][0],32));
    lock_release(); ui.held_step=-1; block(); CHECK(engine_original_value_bits(0,7)==bits(exact));
    lane=motion_find(proj.arr.lane,seq.ppat[PART_303A],PART_303A,T_303,0,7); CHECK(lane>=0); engine_motion_clear(lane); block();
    /* Typed undo restores both engine identity and untouched factory bytes. */
    choose(0,0,54); undo_start(); memcpy(saved,proj.sound.v[T_303][0],32);
    choose(0,1,29); CHECK(undo_commit_stable()); do_undo(0); settle();
    CHECK(!undo_wait && engine_bass_bank(0)==0 && !memcmp(saved,proj.sound.v[T_303][0],32));
    do_undo(1); settle(); CHECK(!undo_wait && engine_bass_bank(0)==1 && proj.sound.v[T_303][0][30]==29);
}
/* Owner-approved restoration: real output fields, not a proposed screenshot. */
static void restoration_ui_cases(void)
{
    static const uint32_t input[] = {120u,255u,256u,300u,511u,65535u,65536u,UINT32_MAX};
    static const uint16_t output[] = {120u,255u,256u,300u,511u,65535u,65535u,65535u};
    for(unsigned i=0;i<sizeof input/sizeof input[0];i++)
        CHECK(ui_perf_percent(input[i])==output[i]);
    uint8_t cell[32],preview[22]={0};
    ui_original_control_view v;
    CHECK(original_sound_init(cell,0,0));
    CHECK(ui_original_controls_project(&v,cell,preview,0));
    CHECK(UI_ORIGINAL_WAVE_W==29 && UI_ORIGINAL_WAVE_H==19);
    for(int i=0;i<UI_ORIGINAL_WAVE_W;i++) CHECK(v.wave[0][i]<UI_ORIGINAL_WAVE_H);
    choose(1,0,0); screen(V_PART,PART_303B,3);
    for(unsigned stage=0;stage<4;stage++){
        ui.touched=(int8_t)stage; ui.touch_until=now_ms+1000u;
        pref_t touched=pg.r[cur_page()][stage];
        ui.touch_view=ui.view;ui.touch_part=ui.part;ui.touch_page=cur_page();
        ui.touch_kind=touched.kind;ui.touch_a=touched.a;ui.touch_b=touched.b;ui.touch_c=touched.c;
        CHECK(adsr_touch_active());
        draw_main(); draw_knobs();
        char name[48];snprintf(name,sizeof name,"chicago-touch-%u",stage);shot(name);
    }
    ui.touched=-1;
    choose(0,1,0);screen(V_PART,PART_303A,0);
    CHECK(ui_original_controls_project(&v,proj.sound.v[T_303][0],preview,0));
    for(int slot=0;slot<3;slot++)for(int i=0;i<UI_ORIGINAL_WAVE_W;i++)
        CHECK(v.wave[slot][i]<UI_ORIGINAL_WAVE_H);
    /* An injected PERF reading above 255 must survive the actual screen path. */
    perf.load=300; perf.peak=511;
    screen(V_PERF,PART_909,0);
    shot("perf-high-percent");
}

static void image_cases(void)
{
    capture("home",V_HOME,PART_909,0); capture("mix-parts",V_MIX,PART_909,0); capture("mix-master",V_MIX,PART_909,3);
    capture("drum-909",V_PART,PART_909,0);
    for (unsigned kind=0;kind<2;kind++) {
        unsigned voice=kind?0:1,part=PART_303A+voice,n=kind?6:8; choose(voice,kind,0);
        for (unsigned page=0;page<n;page++) { char name[64]; snprintf(name,sizeof name,"%s-page%u",kind?"miami":"chicago",page); capture(name,V_PART,part,page); }
        choose(voice,kind,original_sound_factories(kind)-1); screen(V_PART,part,n-1);
        ui.touched=1; ui.touch_until=now_ms+900; draw_main(); shot(kind?"miami-last-preset":"chicago-last-preset");
        open_list_of_pages(); ui.list_sel=ui.list_n-2; ui.list_top=ui.list_n>6?ui.list_n-6:0; draw_main(); shot(kind?"miami-preset-list":"chicago-preset-list");
        ui.overlay=O_NONE;
    }
    choose(1,0,0);
    static const felucca_font_t *const expected[]={&FONT_B,&FONT_B,&FONT_B,&FONT_XS,&FONT_T,&FONT_T,&FONT_B};
    for (unsigned fx=0;fx<7;fx++) {
        pref_set(PR(R_ENG,T_303,1,17),(int)fx); block(); char name[64],value[24],unit[8];
        screen(V_PART,PART_303B,6); pref_value(PR(R_ENG,T_303,1,17),value,unit);
        CHECK(!strcmp(value,original_sound_fx_name(fx)) && !unit[0]);
        const felucca_font_t *font=original_value_font(value,unit,54); CHECK(font==expected[fx] && tw(font,value)<=54);
        snprintf(name,sizeof name,"chicago-fx%u",fx); shot(name);
    }
    static const unsigned q[]={0,64,127};
    for (unsigned kind=0;kind<2;kind++) {
        unsigned voice=kind?0:1,part=PART_303A+voice; choose(voice,kind,0);
        for (unsigned i=0;i<3;i++) {
            pref_set(PR(R_ENG,T_303,voice,kind?7:4),(int)q[i]); block();
            char name[64]; snprintf(name,sizeof name,"%s-%s-%u",kind?"miami":"chicago",kind?"tune":"env",q[i]); capture(name,V_PART,part,1);
        }
    }
    choose(1,0,54); engine_bass_on_velocity(1,60,0,0,100); settle();
    CHECK(engine_bass_select_preset(1,1,0,&proj.sound)==0); block(); CHECK(engine_bass_preset_pending(1)==2);
    capture("sound-requested-miami-held-chicago",V_PART,PART_303B,7);
    CHECK(engine_bass_bank(1)==0 && engine_bass_requested_bank(1)==1);
    uint8_t saved[32]; memcpy(saved,proj.sound.v[T_303][1],32); button_tap(B_EDIT); block();
    CHECK(!engine_bass_preset_pending(1) && !memcmp(saved,proj.sound.v[T_303][1],32)); engine_bass_off(1);
}
#ifndef ORIGINAL_UI_MAIN
#define ORIGINAL_UI_MAIN main
#endif
int ORIGINAL_UI_MAIN(int argc,char **argv)
{
    image_dir=argc==2?argv[1]:0; projection_cases(); load_force=0; boot(); settle();
    pages_and_edits(); selector_cases(); locks_and_undo(); restoration_ui_cases(); image_cases();
    printf("PASS original_release_ui checks=%u captures=%u cache=%zu view=%zu ui=%zu pages=%zu freeplay=%zu\n",
           checks,captures,sizeof original_controls,sizeof original_controls.view,sizeof ui,sizeof pg,sizeof live_keys+sizeof live_outer);
    return 0;
}
