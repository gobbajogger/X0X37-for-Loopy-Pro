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
        for (unsigned i=0;i<4;i++) CHECK(bits(v.vca[i])==bits(c.vca[i]));
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
    static const unsigned chicago_ids[]={4,7,8,9,10,11,13,14,15,16};
    static const unsigned miami_ids[]={0,3,6,7,11,13};
    for (unsigned kind=0;kind<2;kind++) {
        for (unsigned f=0;f<original_sound_factories(kind);f++) {
            CHECK(original_sound_init(cell,kind,f)); projection_one(cell,preview,0);
            const char *name=original_sound_name(kind,f); CHECK(name && strlen(name)<24);
            /* Full names fit the existing readout/list using existing fonts. */
            char label[24]; snprintf(label,sizeof label,"PRESET %u",f+1);
            CHECK(tw(&FONT_T,caps_of(name)) <= 224-24-tw(&FONT_B,label)-10);
        }
        CHECK(original_sound_init(cell,kind,0));
        const unsigned *ids=kind?miami_ids:chicago_ids;
        unsigned count=kind?sizeof miami_ids/sizeof miami_ids[0]:sizeof chicago_ids/sizeof chicago_ids[0];
        for (unsigned i=0;i<count;i++) for (unsigned q=0;q<128;q++) {
            memset(preview,0,sizeof preview); preview[ids[i]]=(uint8_t)q;
            projection_one(cell,preview,1u<<ids[i]);
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
    /* Native VCA envelope readout adds four floats and two alignment bytes. */
    CHECK(sizeof(ui_original_control_cache)==92 && sizeof(ui_original_control_view)==32);
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
            if (cases[c].id==3 && cases[c].kind) snprintf(expected,sizeof expected,"%s%dst",state>12?"+":"",(int)state-12);
            else snprintf(expected,sizeof expected,"%s",cases[c].text[state]);
            CHECK(!strcmp(actual,expected)); now_ms+=500; turn_ref(r,direction,EN_K1); block();
            if (i+1==cases[c].n) CHECK(pref_get(r)==(direction>0?127:0));
        }
    }
    {   /* Free Play shape pads 9-12 write these control values (ui.c division_pad_input). */
        static const int values[4]={0,42,85,127}; static const char *const names[4]={"TRI","DOWN","UP","SQR"};
        choose(0,1,0); screen(V_PART,PART_303A,0); pref_t r=PR(R_ENG,T_303,0,11);
        for (unsigned i=0;i<4;i++) { char t[24]; pref_set(r,values[i]); block(); text_of(r,t); CHECK(!strcmp(t,names[i])); }
    }
    for (unsigned kind=0;kind<2;kind++) {
        choose(0,kind,0); screen(V_PART,PART_303A,1); pref_t r=PR(R_ENG,T_303,0,kind?7:4);
        for (unsigned q=63;q<=65;q++) {
            pref_set(r,(int)q); block(); char text[24]; text_of(r,text);
            CHECK(!strcmp(text,kind?(q==63?"-0.08":q==64?"0.00":"+0.08"):(q==63?"-2%":q==64?"0%":"+2%")));
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
/* Semantic FX pictures keep identity, fit their existing cell and cannot edit
 * the sound. The bypass line reaches both edges; generic selector pips do not. */
static void fx_picture_cases(void)
{
    uint32_t pictures[ORIGINAL_CHICAGO_FX_CHOICES]; uint8_t saved[32];
    choose(1,ORIGINAL_SOUND_CHICAGO,0);
    memcpy(saved,proj.sound.v[T_303][1],sizeof saved);
    int dirty=ui.dirty;
    for(unsigned fx=0;fx<ORIGINAL_CHICAGO_FX_CHOICES;++fx) {
        cv_begin(240,KNOB_H,C_BLACK);
        orig_strip_glyph(PR(R_ENG,T_303,1,ORIGINAL_CHICAGO_FX_TYPE),(int)fx,10,10,C_HI);
        uint32_t hash=2166136261u; unsigned ink=0;
        for(int y=0;y<KNOB_H;++y) for(int x=0;x<240;++x) {
            uint16_t pixel=cv_px[y*240+x];
            if(pixel!=swap16(C_BLACK)) {CHECK(x>=10 && x<36 && y>=10 && y<=28);++ink;}
            hash=(hash^pixel)*16777619u;
        }
        CHECK(ink); pictures[fx]=hash;
        for(unsigned prior=0;prior<fx;++prior) CHECK(hash!=pictures[prior]);
        if(fx==0) CHECK(cv_px[19*240+35]==swap16(C_HI));
    }
    CHECK(!memcmp(saved,proj.sound.v[T_303][1],sizeof saved) && ui.dirty==dirty);
    cv_begin(240,KNOB_H,C_BLACK);
    int end=matrix_next_hint(10,8,C_HI); unsigned ink=0;
    CHECK(end>10 && end<=100);
    for(int y=0;y<KNOB_H;++y) for(int x=0;x<240;++x)
        if(cv_px[y*240+x]!=swap16(C_BLACK)) {CHECK(x>=10 && x<end && y>=8 && y<=18);++ink;}
    CHECK(ink);
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
    for (unsigned fx=0;fx<ORIGINAL_CHICAGO_FX_CHOICES;fx++) {
        pref_set(PR(R_ENG,T_303,1,17),(int)fx); block(); char name[64],value[24],unit[8];
        screen(V_PART,PART_303B,6); pref_value(PR(R_ENG,T_303,1,17),value,unit);
        CHECK(!strcmp(value,original_sound_fx_name(fx)) && !unit[0]);
        const felucca_font_t *font=original_value_font(value,unit,54);
        CHECK(tw(font,value)<=54);
        if(fx<7) CHECK(font==expected[fx]); /* retain all original font checks */
        snprintf(name,sizeof name,"chicago-fx%u",fx); shot(name);
    }
    for(unsigned f=55;f<original_sound_factories(ORIGINAL_SOUND_CHICAGO);++f) {
        char name[64]; choose(1,ORIGINAL_SOUND_CHICAGO,f);
        screen(V_PART,PART_303B,7);
        const char *preset=original_sound_name(ORIGINAL_SOUND_CHICAGO,f);
        CHECK(preset && !strcmp(matrix_preset_name(1),preset));
        snprintf(name,sizeof name,"chicago-restored-preset%u",f); shot(name);
        ui.touched=1; ui.touch_until=now_ms+900; draw_main();
        snprintf(name,sizeof name,"chicago-restored-readout%u",f); shot(name);
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
/* Waveform cases ported from 9e237cce; cache/preview checks prevent stale
 * factory, voice and held-step projections without editing the sound. */
static void restored_wave_cases(void)
{
    unsigned waves=0;
    for(unsigned kind=0;kind<2;kind++){
        unsigned voice=kind?0:1; choose(voice,kind,0);
        screen(V_PART,PART_303A+voice,0);
        uint8_t saved[ORIGINAL_SOUND_BYTES];
        memcpy(saved,proj.sound.v[T_303][voice],sizeof saved);
        for(int id=0;id<32;id++){
            int slot=original_wave_slot(kind,id),want=id==0?0:kind&&id==6?1:kind&&id==11?2:-1;
            CHECK(slot==want); waves+=slot>=0;
            if(slot>=0){
                pref_t r=PR(R_ENG,T_303,voice,id);
                const ui_original_wave_cache *v=original_wave_view(r,pref_get(r));
                CHECK(v && v->valid && v->voice==voice && v->kind==kind);
                for(int i=0;i<UI_ORIGINAL_WAVE_W;i++) CHECK(v->wave[slot][i]<UI_ORIGINAL_WAVE_H);
            }
        }
        CHECK(!memcmp(saved,proj.sound.v[T_303][voice],sizeof saved));
        pref_t r=PR(R_ENG,T_303,voice,kind?11:0);
        ui_original_wave_cache low=*original_wave_view(r,0);
        const ui_original_wave_cache *high=original_wave_view(r,127);
        CHECK(high && memcmp(low.wave[ kind?2:0 ],high->wave[ kind?2:0 ],UI_ORIGINAL_WAVE_W));
        CHECK(!memcmp(saved,proj.sound.v[T_303][voice],sizeof saved));
        /* A held step must supply the same explicit preview as its readout. */
        ui.held_step=0; ui.held_step_part=PART_303A+voice;
        ui.held_step_pat=seq.ppat[PART_303A+voice];
        lock_turn(r,1,EN_K1); block();
        int locked=lock_of(r,0); CHECK(locked>=0);
        const ui_original_wave_cache *locked_wave=original_wave_view(r,original_display_value(r));
        CHECK(locked_wave && (locked_wave->preview_mask&(1u<<r.c)) && locked_wave->preview[r.c]==locked);
        CHECK(!memcmp(saved,proj.sound.v[T_303][voice],sizeof saved));
        lock_release(); ui.held_step=-1;
        int lane=motion_find(proj.arr.lane,seq.ppat[PART_303A+voice],PART_303A+voice,T_303,voice,r.c);
        CHECK(lane>=0); engine_motion_clear(lane); block();
        const ui_original_wave_cache *base=original_wave_view(r,pref_get(r));
        CHECK(base && !(base->preview_mask&(1u<<r.c)));
    }
    CHECK(waves==4);
    /* Literal geometry from the original modulation laws: DOWN falls on the
     * display, UP rises, and TRI returns to its starting level. */
    ui_original_wave_cache v={0};
    ui_original_curve(&v,2,1,0,1);
    CHECK(v.wave[2][0]==1 && v.wave[2][14]==9 && v.wave[2][28]==17);
    ui_original_curve(&v,2,2,0,1);
    CHECK(v.wave[2][0]==17 && v.wave[2][14]==9 && v.wave[2][28]==1);
    ui_original_curve(&v,2,0,0,1);
    CHECK(v.wave[2][0]==17 && v.wave[2][14]==1 && v.wave[2][28]==17);
    uint8_t cell[ORIGINAL_SOUND_BYTES],preview[22]={0};
    CHECK(original_sound_init(cell,ORIGINAL_SOUND_MIAMI,0));
    CHECK(ui_original_wave_update(&v,0,cell,preview,0));
    CHECK(ui_original_wave_update(&v,1,cell,preview,0) && v.voice==1);
    CHECK(!ui_original_wave_update(&v,2,cell,preview,0) && !v.valid);
    CHECK(!ui_original_wave_project(&v,cell,preview,1u<<31));
    preview[0]=128; CHECK(!ui_original_wave_project(&v,cell,preview,1u)); preview[0]=0;
    cell[29]=255; CHECK(!ui_original_wave_update(&v,0,cell,preview,0) && !v.valid);
    choose(1,0,0); capture("restored-chicago-wave",V_PART,PART_303B,0);
    choose(0,1,0); capture("restored-miami-wave",V_PART,PART_303A,0);
    capture("restored-miami-xmod-wave",V_PART,PART_303A,1);
    capture("restored-miami-wobble-wave",V_PART,PART_303A,3);
}
static uint32_t picture_pixels(const uint16_t *p,int x,int y,int w,int h)
{
    uint32_t sum=2166136261u;
    for(int row=y;row<y+h;row++)for(int col=x;col<x+w;col++)sum=(sum^p[row*240+col])*16777619u;
    return sum;
}
static void picture_rule_cases(void)
{
    screen(V_HOME,PART_909,0); seq.playing=seq.song_on=seq.chain_a=seq.chain_b=0;
    for(int p=NPAT-2;p<NPAT;p++)pattern_init(&proj.pat[p],1,2);
    proj.set.accent_q7=127;
    sm_set(proj.pat[NPAT-1].drum[PART_909].hit[0],0);
    sm_set(proj.pat[NPAT-1].drum[PART_909].hit[0],8);
    sm_set(proj.pat[NPAT-2].drum[PART_909].hit[0],2);
    seq.ppat[PART_909]=NPAT-1; seq.pcue[PART_909]=NPAT-2;
    project_t *saved=malloc(sizeof proj); CHECK(saved); *saved=proj;
    seq_t before=seq;
    for(int theme=0;theme<=5;theme+=5){
        palette_set(theme); ui.msg_until=0; draw_header();
        /* Eight columns cover all saved steps: current {0,8} -> 0x11;
         * queued {2} -> 0x02. Header badges must show content, not assignment. */
        for(int i=0;i<8;i++)for(int y=8;y<=10;y++){
            uint16_t current=(0x11u&(1u<<i))?dim(PART_COL[PART_909],16):SOUND_FACE;
            uint16_t queued=(0x02u&(1u<<i))?dim(PART_COL[PART_909],9):SOUND_FACE;
            CHECK(fb[y*240+133+i]==swap16(current));
            CHECK(fb[y*240+165+i]==swap16(queued));
        }
        uint32_t idle=picture_pixels(fb,132,0,108,HDR_H);
        now_ms+=1000; ui.frame+=32; draw_header();
        CHECK(picture_pixels(fb,132,0,108,HDR_H)==idle);
        seq.pcue[PART_909]=0xff; draw_header(); uint32_t no_cue=picture_pixels(fb,156,0,30,HDR_H);
        seq.pcue[PART_909]=NPAT-1; draw_header();
        CHECK(picture_pixels(fb,156,0,30,HDR_H)==no_cue);
        seq.pcue[PART_909]=NPAT-2;
        seq.playing=1; eng_step[TRK_DRUM]=0; draw_header(); uint32_t step=picture_pixels(fb,156,6,6,7);
        eng_step[TRK_DRUM]=1; draw_header();
        CHECK(picture_pixels(fb,156,6,6,7)!=step);
        seq.playing=0;
    }
    CHECK(!memcmp(saved,&proj,sizeof proj)); CHECK(!memcmp(&before,&seq,sizeof seq)); free(saved);
    palette_set(proj.set.palette);
    /* A stop square and a play triangle are distinct, bounded status pictures. */
    cv_begin(240,HDR_H,C_BLACK); header_transport(10,0); uint32_t stopped=picture_pixels(cv_px,10,4,9,11);
    for(int y=0;y<HDR_H;y++)for(int x=0;x<240;x++)if(cv_px[y*240+x]!=swap16(C_BLACK))CHECK(x>=10&&x<19&&y>=4&&y<15);
    seq.playing=1; cv_begin(240,HDR_H,C_BLACK); header_transport(10,0);
    CHECK(picture_pixels(cv_px,10,4,9,11)!=stopped); seq.playing=0;
    for(int p=PART_303A;p<=PART_303B;p++){
        screen(V_PART,p,0); uint8_t page=cur_page(),view=ui.view,part=ui.part;
        cv_begin(240,KNOB_H,C_BLACK); int end=matrix_row_hint(7,50,C_HI);
        CHECK(end+8+tw(&FONT_T,UI_CAP_EDIT)+4+18<190);
        for(int y=0;y<KNOB_H;y++)for(int x=0;x<240;x++)if(cv_px[y*240+x]!=swap16(C_BLACK))CHECK(x>=7&&x<end&&y>=50&&y<62);
        CHECK(page==cur_page()&&view==ui.view&&part==ui.part);
        /* Exercise the composed footer, including the physical panel caption
         * when built with X0X_SMK_CAPTIONS=1, not just its row helper. */
        draw_knobs(); int next_x=end+8+tw(&FONT_T,UI_CAP_EDIT)+4;
        for(int x=next_x;x<next_x+8;x++)CHECK(fb[(KNOB_Y+70)*240+x]==swap16(SOUND_GRAY));
        ui.held_step=0;ui.held_step_part=(uint8_t)p;ui.held_step_pat=seq.ppat[p];draw_knobs();
        char held[48];snprintf(held,sizeof held,"STEP 1 %s: CHANCE %s",UI_CAP_ARROWS,UI_CAP_EDIT);
        int row_x=7+tw(&FONT_T,held)+4;
        for(int x=row_x;x<row_x+10;x++)CHECK(fb[(KNOB_Y+74)*240+x]==swap16(C_AMB));
        ui.held_step=-1;
        seq.ppat[p]=NPAT-1;seq.pcue[p]=NPAT-2;draw_header();
        CHECK(fb[9*240+137]==swap16(C_AMB));
        seq.pcue[p]=0xff;draw_header();CHECK(fb[9*240+137]==swap16(SOUND_FACE));
        seq.pcue[p]=NPAT-1;draw_header();CHECK(fb[9*240+137]==swap16(SOUND_FACE));
    }
    /* Preserve the exact direction text for lists/Help; the strip's existing
     * pictures distinguish the same four states without duplicate readouts. */
    screen(V_SEQ,PART_303B,0); int page=-1,slot=-1;
    for(int p=0;p<pg.n;p++)for(int k=0;k<4;k++)if(pg.r[p][k].kind==R_BDIR){page=p;slot=k;}
    CHECK(page>=0); *page_of(V_SEQ)=(uint8_t)page; build_pages();
    uint32_t direction[4];
    for(int d=0;d<4;d++){
        pref_t r=pg.r[page][slot]; pref_set(r,d); char value[16],unit[8]; pref_value(r,value,unit);
        CHECK(!strcmp(value,DIR_N[d])); ui.touched=-1; blit_hash[3]=0; draw_knobs();
        /* This area used to contain the duplicate FWD/REV/PING/RND value.
         * Reintroducing that text must fail even if the arrows are unchanged. */
        for(int y=53;y<64;y++)for(int x=slot*60+12;x<slot*60+48;x++)
            CHECK(fb[(KNOB_Y+y)*240+x]==swap16(SOUND_FACE));
        direction[d]=picture_pixels(fb,slot*60+10,KNOB_Y+26,40,24);
        for(int j=0;j<d;j++)CHECK(direction[d]!=direction[j]);
    }
}
#ifndef ORIGINAL_UI_MAIN
#define ORIGINAL_UI_MAIN main
#endif
int ORIGINAL_UI_MAIN(int argc,char **argv)
{
    image_dir=argc==2?argv[1]:0; projection_cases(); load_force=0; boot(); settle();
    pages_and_edits(); selector_cases(); locks_and_undo(); fx_picture_cases(); image_cases(); restored_wave_cases(); picture_rule_cases();
    printf("PASS original_release_ui checks=%u captures=%u cache=%zu view=%zu ui=%zu pages=%zu freeplay=%zu\n",
           checks,captures,sizeof original_controls,sizeof original_controls.view,sizeof ui,sizeof pg,sizeof live_keys+sizeof live_outer);
    return 0;
}
