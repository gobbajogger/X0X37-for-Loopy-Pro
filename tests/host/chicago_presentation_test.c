/* SPDX-License-Identifier: GPL-3.0-only */
/* Production pixels, physical release edges, engine publication interruptions,
 * and actual SMK scanner. Host ADC/radio substitutes are not hardware proof. */
#define BOARD_SMK37 1
#define X0X_DEV_PADCAL 1
#define engine_bass_preset_pending cancel_test_pending
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#undef engine_bass_preset_pending
#include <assert.h>
#include "../../experiments/wobble/wobble.h" /* Miami control indices (match original_sound.h) */
int engine_bass_preset_pending(int);
static unsigned checks;
#define CHECK(x) do { checks++; if(!(x)) {fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static uint8_t clip_grid_mode;
uint8_t smk_clip_mode_on(void) { return clip_grid_mode; }
int smk_ck_smc_turn(int n, int d) { (void)n; (void)d; return 0; }
int smk_ck_smc_nav(uint8_t n) { (void)n; return 0; }
uint8_t smk_clip_transport_button(uint8_t b, uint8_t p) { (void)b; (void)p; return 0; }
uint8_t smk_clip_history_button(uint8_t b, uint8_t p, uint8_t h) { (void)b; (void)p; (void)h; return 0; }
void smk_play_note_button(uint8_t p, uint8_t h) { (void)p; (void)h; }
uint8_t smk_ck_flash_busy(void) { return 0; }
uint8_t smk_ck_ext(void) { return 0; }
uint8_t smk_ck_knobs_owned(void) { return 0; }
uint8_t smk_ck_panel(uint32_t *p, uint32_t f) { (void)p; (void)f; return 0; }
uint8_t smk_clip_lcd_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void smk_clip_lcd_force(void) {}
uint8_t smk_clip_wait(void) { return 0; }
uint8_t smk_apc_transport_led_state(uint8_t n) { (void)n; return 0; }
uint8_t smk_padcal_on(void) { return 0; }
void smk_padcal_enter(void) {}
uint8_t smk_padcal_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
int plat_ble_smc_send(const uint8_t *msg, uint8_t len) { (void)msg; (void)len; return 1; }
#ifndef SMC_SLOT_SEND_IMPLEMENTED
/* Existing one-peer fixtures record peer zero. Dual-peer tests override this. */
int plat_ble_smc_send_slot(unsigned slot, const uint8_t *m, uint8_t n)
{ return slot == 0 ? plat_ble_smc_send(m, n) : 1; }
#endif

#include "../../firmware/src/app/smc_mixer.c"


static struct { uint32_t frames; int32_t wheel[2]; } fm1_in;
static int clamp(int v,int lo,int hi) { return v<lo?lo:v>hi?hi:v; }
#include "smk_wheels.inc"
static int race_voice=-1, publication_voice=-1, race_count, publication_count, unstable;
static int old_bank,old_preset;
static void audio(void) {int32_t out[512];engine_render(out,256);}
int cancel_test_pending(int v)
{
    int p=engine_bass_preset_pending(v);
    if(unstable) return 6;
    if(race_voice==v) {
        race_voice=-1;race_count++;audio();
        CHECK(engine_bass_bank(v)==old_bank && engine_bass_preset_index(v)==old_preset);
    }
    return p;
}
void bass_publish_test_hook(int v,int stage)
{
    if(v!=publication_voice) return;
    CHECK(stage>=1 && stage<=3);publication_count++;audio();
    CHECK(engine_bass_preset_pending(v)==6);
    CHECK(engine_bass_bank(v)==old_bank && engine_bass_preset_index(v)==old_preset);
}
static int row(int kind,int param)
{for(int p=0;p<pg.n;p++)for(int k=0;k<4;k++)if(pg.r[p][k].kind==kind && (kind!=R_ENG || pg.r[p][k].c==param))return p;return -1;}
static void edges(uint32_t btn,uint32_t keys)
{held_btn=btn;held_keys=keys;now_ms+=10;ui_input_only();}
static void patch(void) {edges(1u<<B_HOME,held_keys);edges(0,held_keys);}
static void shot(const char *dir,const char *name)
{char path[512];ui.touched=-1;ui.msg_until=0;draw_header();draw_main();draw_knobs();snprintf(path,sizeof path,"%s/%s.png",dir,name);png_write(path);}
/* Independent expected primitive for the footer, not the production helper. */
static int expected_row_hint(int x,int y,uint16_t c)
{
    cv_text(x,y,&FONT_T,UI_CAP_ARROWS,c);x+=tw(&FONT_T,UI_CAP_ARROWS)+4;
    for(int i=0;i<2;i++) {
        int xx=x+i*6,tip=y+(i?9:1),base=tip+(i?-2:2);
        cv_line(xx+2,y+1,xx+2,y+9,c);
        cv_line(xx,base,xx+2,tip,c);cv_line(xx+4,base,xx+2,tip,c);
    }
    cv_text(x+14,y,&FONT_T,"ROW",c);return x+14+tw(&FONT_T,"ROW");
}
/* Independent expected palette/geometry for the accepted Chicago second screen. */
static int night_fixture(void)
{return !engine_bass_is_wobble(bidx()) && pg.n==7 && cur_page()>=4;}
static void expected_footer_bg(int night)
{
    cv_begin(240,12,night?RGB(16,26,31):C_BLACK);
    if(night) {
        cv_rect(0,0,1,11,RGB(28,44,49));cv_rect(239,0,1,12,RGB(7,14,17));
        cv_rect(1,11,239,1,RGB(7,14,17));
    }
}
static void fixture(int voice,int bank)
{
    publication_voice=race_voice=-1;unstable=0;clip_grid_mode=0;
    engine_bass_wheel_owners(0);held_btn=held_keys=0;ui.btn=ui.keys=ui.btn_used=0;
    ui.help=ui.rec=0;ui.held_step=-1;seq.playing=0;ui.overlay=O_NONE;project_busy=0;
    engine_bass_off(0);engine_bass_off(1);
    engine_bass_cancel_preset(0);engine_bass_cancel_preset(1);
    for(int i=0;i<600;i++)audio();
    memset(proj.arr.lane,0,sizeof proj.arr.lane);
    /* Only one Miami instance: put the other voice in Chicago first. */
    CHECK(!engine_bass_select_preset(voice^1,0,0,&proj.sound));audio();
    CHECK(!engine_bass_select_preset(voice,bank,0,&proj.sound));audio();
    CHECK(engine_bass_bank(voice)==bank && !engine_bass_preset_pending(voice));
    ui.part=PART_303A+voice;set_view(V_PART);build_pages();*page_of(V_PART)=row(R_BENGINE,0);
    ui.dirty=save_wait=ui.save_failed=0;ui.act_t=now_ms;ui_input_only();
}
static void request(int v,int bank)
{CHECK(!engine_bass_select_preset(v,bank,1,&proj.sound));CHECK(engine_bass_preset_pending(v));}
static void cancel_edges(const char *dir)
{
    /* This request is eligible NOW: only the short UI hold can prevent the
     * injected boundary between the authoritative check and cancel publication. */
    for(int v=0;v<2;v++)for(int bank=0;bank<2;bank++) {
        fixture(v,bank);old_bank=bank;old_preset=0;request(v,bank^1);
        CHECK(engine_bass_preset_pending(v)==1);
        sound_t before=proj.sound;int page=cur_page();
        edges(1u<<B_HOME,0);CHECK(engine_bass_preset_pending(v)==1);
        race_voice=v;publication_voice=v;race_count=publication_count=0;
        edges(0,0);publication_voice=-1;
        CHECK(race_count==1 && publication_count==3);
        CHECK(!engine_bass_preset_pending(v) && ui.view==V_PART && cur_page()==page);
        CHECK(!memcmp(&before,&proj.sound,sizeof before));
    }
    for(int v=0;v<2;v++)for(int bank=0;bank<2;bank++)for(int reason=0;reason<4;reason++) {
        fixture(v,bank);old_bank=bank;old_preset=0;
        if(reason==0) {engine_bass_on(v,48,100,0);audio();}
        if(reason==1) engine_bass_wheel_owners(1u<<v);
        if(reason==2) engine_bass_on(v,48,100,0); /* queued CQ, not drained */
        if(reason==3) min_q[mi_w++%MQ]=9u|((uint32_t)(0x90|seq.ch[TRK_BASS0+v])<<8)|(48u<<16)|(100u<<24);
        request(v,bank^1);
        int wait=engine_bass_preset_pending(v);CHECK(wait==(reason==0?2:reason==1?3:5));
        sound_t before=proj.sound;int page=cur_page(),dirty=ui.dirty,sw=save_wait,playing=seq.playing;
        uint32_t history=song_structure_revision;
        edges(1u<<B_HOME,0);CHECK(engine_bass_preset_pending(v));CHECK(ui.view==V_PART);
        race_voice=v;publication_voice=v;race_count=publication_count=0;
        edges(0,0);publication_voice=-1;
        CHECK(race_count==1 && publication_count==3);CHECK(!engine_bass_preset_pending(v));
        CHECK(ui.view==V_PART && cur_page()==page);CHECK(!memcmp(&before,&proj.sound,sizeof before));
        CHECK(ui.dirty==dirty && save_wait==sw && seq.playing==playing && history==song_structure_revision);
        if(v==0 && bank==0 && reason==1) shot(dir,"patch-cancelled-chicago");
        patch();CHECK(ui.view==V_HOME);CHECK(engine_bass_bank(v)==bank && engine_bass_preset_index(v)==0);
        if(reason==3) {min_q[mi_w++%MQ]=8u|((uint32_t)(0x80|seq.ch[TRK_BASS0+v])<<8)|(48u<<16);audio();}
    }
    fixture(0,0);request(0,1);audio();CHECK(engine_bass_bank(0)==1);patch();
    CHECK(ui.view==V_HOME && engine_bass_bank(0)==1 && engine_bass_preset_index(0)==1); /* not undo */
    fixture(0,0);request(0,1);unstable=1;patch();unstable=0;
    CHECK(engine_bass_preset_pending(0) && ui.view==V_PART); /* no fabricated cancellation */
    puts("PASS physical PATCH both voices/engine directions: note/wheel/CQ/MIDI, apply race and publication stages 1-3");
}
static void protected_contexts(void)
{
    for(int context=0;context<12;context++) {
        fixture(0,0);engine_bass_wheel_owners(1);request(0,1);
        switch(context) {
        case 0: ui.overlay=O_ASK;ui.ask_overlay=O_LIST;break;
        case 1: ui.overlay=O_LIST;break;
        case 2: set_view(V_GLO);break;
        case 3: ui.held_step=0;ui.held_step_part=ui.part;ui.held_step_pat=seq.ppat[ui.part];break;
        case 4: clip_grid_mode=1;break;
        case 5: project_busy=1;break;
        case 6: *page_of(V_PART)=0;break;
        case 7: set_view(V_SEQ);break;
        case 8: ui.help=B_HOME+1;ui.btn_used=1u<<B_HOME;break;
        case 9: edges(1u<<B_HOME,0);now_ms+=HELP_HOLD+1;ui_input_only();CHECK(ui.help);break;
        case 10: perf.test=0;perf.phase=1;break;
        case 11: save_wait=1;break;
        }
        if(context==8) {held_btn=ui.btn=1u<<B_HOME;edges(0,0);}
        else if(context==9) edges(0,0);
        else patch();
        if(context==11) CHECK(!engine_bass_preset_pending(0)); /* queued save is not a modifier */
        else {if(!engine_bass_preset_pending(0))fprintf(stderr,"context %d unexpectedly cancelled\n",context);CHECK(engine_bass_preset_pending(0));}
        if(context==0) CHECK(ui.overlay==O_LIST);
        if(context==1 || context==2) CHECK(ui.overlay==O_NONE);
        if(context==5) {project_busy=0;edges(0,0);CHECK(engine_bass_preset_pending(0));}
        perf.test=-1;perf.phase=0;
    }
    /* Previous-frame and simultaneous current-frame buttons both count. */
    for(int b=0;b<NB;b++)if(b!=B_HOME)for(int simultaneous=0;simultaneous<2;simultaneous++) {
        fixture(0,0);engine_bass_wheel_owners(1);request(0,1);
        edges(1u<<B_HOME,0);
        if(simultaneous) edges(1u<<b,0);
        else {edges((1u<<B_HOME)|(1u<<b),0);edges(0,0);}
        CHECK(engine_bass_preset_pending(0));edges(0,0);
    }
    fixture(0,0);engine_bass_wheel_owners(1);request(0,1);
    edges(1u<<B_HOME,0);enc_acc[EN_SELECT]=1;ui_input_only();edges(0,0);
    CHECK(engine_bass_preset_pending(0)); /* PATCH + physical K2 tempo */
    fixture(0,0);engine_bass_wheel_owners(1);request(0,1);
    edges(1u<<B_HOME,1u<<0);edges(0,0);CHECK(engine_bass_preset_pending(0));
    fixture(0,0);engine_bass_wheel_owners(1);request(0,1);
    edges(1u<<B_HOME,0);edges(0,1u<<0);CHECK(engine_bass_preset_pending(0));edges(0,0);
    fixture(0,0);engine_bass_wheel_owners(1);request(0,1);
    edges(1u<<B_SAVE,0);edges((1u<<B_SAVE)|(1u<<B_HOME),0);edges(1u<<B_SAVE,0);
    CHECK(engine_bass_preset_pending(0));edges(0,0);
    fixture(0,0);engine_bass_wheel_owners(1);request(0,1);
    engine_bass_edit_hold(1);patch();CHECK(!engine_bass_preset_pending(0));
    engine_bass_wheel_owners(0);request(0,1);audio();CHECK(engine_bass_bank(0)==0);
    engine_bass_edit_hold(0);audio();CHECK(engine_bass_bank(0)==1); /* hold nesting is balanced */
    fixture(0,0);engine_bass_wheel_owners(1);request(0,1);
    for(int d=-1;d<=1;d+=2) {enc_acc[EN_K3]=d;now_ms+=1000;ui_input_only();CHECK(engine_bass_preset_pending(0));}
    puts("PASS modal/help/busy/held-step/clip priorities, all button chords, K2/key edges, and inert K7");
}
static void wheel_and_live_keys(const char *dir)
{
    for(int v=0;v<2;v++)for(int bank=0;bank<2;bank++) {
        fixture(v,bank);
        int bend=bank?W_PITCH_BEND:CHI_PBEND;
        proj.sound.v[T_303][v][bend]=71;engine_set(T_303,v,bend,71);audio();
        fm1_in.frames=400;fm1_in.wheel[0]=550;fm1_in.wheel[1]=615;smk_wheels();
        sound_t before=proj.sound;
        fm1_in.wheel[0]=600;smk_wheels();audio();request(v,bank^1);audio();
        CHECK(engine_bass_preset_pending(v)==3);
        if(!v && !bank) shot(dir,"chicago-pending-wheel-patch");
        patch();CHECK(!engine_bass_preset_pending(v));
        CHECK(ui.view==V_PART && engine_bass_bank(v)==bank && !memcmp(&before,&proj.sound,sizeof before));
        /* Original-voice ownership survives page/part navigation and wheel down. */
        ui.part=PART_303A+(v^1);build_pages();fm1_in.wheel[0]=500;smk_wheels();audio();
        CHECK(!memcmp(&before,&proj.sound,sizeof before));
        fm1_in.wheel[0]=550;smk_wheels();audio();CHECK(!memcmp(&before,&proj.sound,sizeof before));
        ui.part=PART_303A+v;build_pages();*page_of(V_PART)=row(R_BENGINE,0);
        request(v,bank^1);audio();CHECK(!engine_bass_preset_pending(v) && engine_bass_bank(v)==(bank^1));
    }
    fixture(0,0);edges(0,1u<<0);audio();CHECK(engine_midi_held()&1);
    request(0,1);patch();CHECK(!engine_bass_preset_pending(0) && (engine_midi_held()&1));
    edges(0,0);for(int i=0;i<600;i++)audio();
    fixture(0,0);engine_bass_wheel_owners(1);request(0,1);seq.playing=1;
    patch();CHECK(seq.playing && !engine_bass_preset_pending(0));seq.playing=0;
    puts("PASS actual wheel scanner wait/cancel/release, both engines/voices, saved noncentral bends, live held keys and transport");
}
static void presentation(const char *dir)
{
    static const char *e1[]={"UNUSED","TILT","AMOUNT","AMOUNT","FREQ","MIX","MIX","MIX","MIX","MIX","MIX","MIX"};
    static const char *e2[]={"UNUSED","MIX","UNUSED","UNUSED","MIX","SHAPE","SHAPE","SHAPE","FEEDBACK","DIVISION","DECAY","SHAPE"};
    for(int v=0;v<2;v++) {
        fixture(v,0);int arp=row(R_ENG,CHI_ARP_SWITCH),fx=row(R_ENG,CHI_FX_TYPE),aux=row(R_ENG,CHI_DAMPER),snd=row(R_BENGINE,0);
        CHECK(pg.n==7 && arp==4 && fx==5 && aux==fx && snd==6);
        CHECK(pg.r[fx][0].c==CHI_FX_TYPE && pg.r[fx][1].c==CHI_FX_EDIT1 &&
              pg.r[fx][2].c==CHI_FX_EDIT2 && pg.r[fx][3].c==CHI_DAMPER);
        CHECK(pg.r[snd][2].kind==R_NONE && pg.r[snd][3].kind==R_NONE);
        for(int p=0;p<pg.n;p++)for(int k=0;k<4;k++)CHECK(pg.r[p][k].kind!=R_ENG || pg.r[p][k].c!=CHI_PBEND);
        for(int k=0;k<pg.nmore;k++)CHECK(pg.more[k].c!=CHI_PBEND);
        for(int on=0;on<2;on++)for(int mode=0;mode<3;mode++)for(int range=0;range<3;range++)for(int speed=0;speed<10;speed++) {
            *page_of(V_PART)=arp;
            int vals[]={on,mode,range,speed};
            for(int k=0;k<4;k++) pref_set(pg.r[arp][k],vals[k]);
            ui.touched=-1;ui.msg_until=0;draw_header();draw_main();draw_knobs();
            /* Owner retained the existing bars, including discrete enums. */
            for(int k=0;k<4;k++) {
                uint16_t ink=!on && k?RGB(128,146,148):RGB(224,235,214);
                CHECK(fb[(24+62)*240+12+k*59]==swap16(vals[k]?ink:RGB(21,39,27)));
                CHECK(fb[(24+62)*240+53+k*59]==swap16(vals[k]==pref_desc(pg.r[arp][k])->max?ink:RGB(21,39,27)));
            }
            CHECK(!strcmp(matrix_name(pg.r[arp][1]),"DIRECTION"));
            if(!v && range==2 && speed==9) {char n[64];snprintf(n,sizeof n,"arp-%s-direction-%d-three-octaves-fast",on?"on":"off",mode);shot(dir,n);}
        }
        /* OFF subordinate values remain editable, including fractional rates. */
        pref_set(pg.r[arp][0],0);pref_set(pg.r[arp][3],4);now_ms+=1000;enc_acc[EN_K4]=1;ui_input_only();
        CHECK(pref_get(pg.r[arp][3])==5 && !pref_get(pg.r[arp][0]));
        if(!v)shot(dir,"arp-off-retained-one-sixth");
        for(int ty=0;ty<CHI_FX_NTYPES;ty++) {
            *page_of(V_PART)=fx;pref_set(pg.r[fx][0],ty);
            CHECK(!strcmp(matrix_name(pg.r[fx][1]),e1[ty]));CHECK(!strcmp(matrix_name(pg.r[fx][2]),e2[ty]));
            CHECK(!strcmp(matrix_group(fx),"FX / DAMPER"));
            if(!v) {char n[40];snprintf(n,sizeof n,"fx-type-%02d",ty);shot(dir,n);}
        }
        *page_of(V_PART)=aux;
        pref_set(pg.r[aux][3],63);if(!v)shot(dir,"damper-63-off");
        pref_set(pg.r[aux][3],64);if(!v)shot(dir,"damper-64-on");
        ui.touched=-1;draw_header();draw_main();draw_knobs();
        CHECK(fb[(91+20)*240+181]==swap16(RGB(21,39,27))); /* K8 DAMPER has its own cell. */
        CHECK(fb[(158+20)*240+181]==swap16(RGB(16,26,31))); /* STATUS has no K8 divider. */
        *page_of(V_PART)=snd;request(v,0);if(!v)shot(dir,"chicago-pending-patch");
        CHECK(7+tw(&FONT_T,UI_CAP_HOME ": CANCEL REQUEST")+8+tw(&FONT_T,"KEYS/MIDI")<=234);
        const char *lines[HELP_LINES];int n=help_lines(B_HOME,lines);CHECK(!strcmp(lines[0],"TAP: CANCEL SOUND REQUEST"));
        for(int i=0;i<n;i++)CHECK(tw(&FONT_XS,lines[i])<=224);
        patch();CHECK(!engine_bass_preset_pending(v));
        fixture(v,0);char nme[40];*page_of(V_PART)=0;snprintf(nme,sizeof nme,"chicago-%c-page1",'a'+v);shot(dir,nme);
        *page_of(V_PART)=snd;snprintf(nme,sizeof nme,"chicago-%c-page2",'a'+v);shot(dir,nme);
        fixture(v,1);int p=row(R_ENG,W_GLIDE);CHECK(pg.r[p][0].c==W_GLIDE && pg.r[p][1].kind==R_NONE && pg.r[p][2].c==W_CRUSH);
        *page_of(V_PART)=row(R_BENGINE,0);snprintf(nme,sizeof nme,"miami-%c-page2",'a'+v);shot(dir,nme);
    }
    puts("PASS actual pixels: both voices/banks, 360 ARP states, 12 algorithm labels, raw pots, damper threshold, blank columns and hints");
}
/* The readout-only follow-on reuses this real-engine/SMK fixture. */
#ifndef CHICAGO_PRESENTATION_NO_MAIN
int main(int argc,char **argv)
{
    CHECK(argc==2);boot();run_ms(50);engine_guard_enable(0);
    presentation(argv[1]);cancel_edges(argv[1]);protected_contexts();wheel_and_live_keys(argv[1]);
    printf("PASS %u Chicago presentation/PATCH checks\n",checks);return 0;
}

#endif
