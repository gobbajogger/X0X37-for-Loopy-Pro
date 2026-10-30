/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual MIDI/keyboard command -> engine recorder -> production sequencer regression.
 * Adapted timestamped lifecycle proof from 6496003d onto current source.
 * Disconnected SMC is a platform boundary, not a substituted recorder. */
#ifdef __APPLE__
#define _DARWIN_C_SOURCE 1 /* expose the simulator's libproc instrumentation declarations */
#endif
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main
#if BOARD_SMK37
static uint8_t test_clip_mode, test_clip_busy;
uint8_t smk_clip_mode_on(void) { return test_clip_mode; }
uint8_t smk_padcal_on(void) { return 0; }
uint8_t smk_padcal_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void smk_padcal_enter(void) {}
uint8_t smk_clip_lcd_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void smk_clip_lcd_force(void) {}
uint8_t smk_clip_wait(void) { return 0; }
uint8_t smk_ck_ext(void) { return 0; }
uint8_t smk_ck_flash_busy(void) { return test_clip_busy; }
uint8_t smk_ck_knobs_owned(void) { return 0; }
uint8_t smk_ck_panel(uint32_t *last, uint32_t force) { (void)last; (void)force; return 0; }
uint8_t smk_clip_history_button(uint8_t button, uint8_t pressed, uint8_t patch_held)
{ (void)button; (void)pressed; (void)patch_held; return 0; }
uint8_t smk_clip_transport_button(uint8_t button, uint8_t pressed)
{ (void)button; (void)pressed; return 0; }
void smk_play_note_button(uint8_t pressed, uint8_t home_held) { (void)pressed; (void)home_held; }
void smk_apc_transport_led_set(uint8_t note, uint8_t velocity) { (void)note; (void)velocity; }
uint8_t smk_apc_transport_led_state(uint8_t note) { (void)note; return 0; }
void smk_apc_clock_tick(void) {}
void smk_apc_clock_start(void) {}
void smk_apc_clock_position(uint32_t sixteenths) { (void)sixteenths; }
void smk_ck_midi_stop(void) {}
int smc_connected(void) { return 0; }
/* This fixture models disconnected SMC; project_smc_runtime_test.c covers
 * the production SMC project-active/reset implementation. */
/* This link does not include smc_mixer.c. ui.c still asks whether an SMC solo holds the project. */
int smc_project_active(void) { return 0; }
void smc_project_reset(void) {}
void smc_frame(void) {}
void smc_poll(void) {}
int smc_midi_host(uint32_t pkt) { (void)pkt; return 0; }
/* Presentation lives in smc_mixer.c; this fixture is disconnected SMC / live-rec only. */
void smc_ui_cancel(void) {}
int smc_ui_active(void) { return 0; }
void smc_ui_draw(void) {}
void smc_ui_clip_show(int n, const char *name, const char *value)
{ (void)n; (void)name; (void)value; }
#endif

/* Compile the real native wheel function, substituting only ADC readings. */
static struct { uint32_t frames; int32_t wheel[2]; } fm1_in;
static int clamp(int value,int lo,int hi) { return value < lo ? lo : value > hi ? hi : value; }
#include "break_smk_wheels.inc"
#include "adaptive_native_brk_service.h"
static unsigned checks, fails;
#define CHECK(c) do {checks++;if(!(c)){fails++;fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);}}while(0)
static uint8_t synthetic[4097];
static void audio(void) {int32_t out[512];engine_render(out,256);}
static void note(int src,int n,int on) { min_q[mi_w++ % MQ]=9u|((unsigned)src<<4)|((0x90u|seq.ch[TRK_BRK])<<8)|((unsigned)n<<16)|((unsigned)(on?100:0)<<24);audio(); }
static void count_set(int n,int bank) {unsigned code=n==4?1:n==8?0:n==16?2:3,shift=2*bank; brkpart_t *p=&proj.pat[seq.ppat[TRK_BRK]].brk;p->slice_mode=(p->slice_mode&~(3u<<shift))|(code<<shift);engine_brk_loops();audio();native_break_service();}
static int black(int index){for(int k=0;k<NKEYS;k++)if(KEY_BLACK[k]==index)return k;return -1;}
static void page(unsigned mask) {ui_brk_page_keys(0,1);ui_brk_page_keys(mask,1);ui_brk_page_keys(0,1);}
static void reset(void) {
    project_defaults();boot();seq.req_start=seq.req_stop=seq.req_cont=0;seq.playing=0;
    ui.view=V_SEQ;ui.part=PART_BRK;ui.overlay=O_NONE;ui.help=ui.btn=0;ui.brk_page=ui.brk_count_target=0;
    slot_nz[0]=1;slot_data[0][0]=synthetic;slot_n[0][0]=8193;slot_rate[0][0]=22050;
    engine_brk_loops();ui_brk_page_keys(0,1);ui_brk_page_keys(0,1);
}
static unsigned output_run(int stutter, int core2, int jungle)
{
    reset();plat_cpu2_set(core2);midi_pulse_crosscore=0;count_set(32,0);seq.mute=~(1u<<MUTE_BRK);seq.send_notes=1;
    proj.pat[0].brk.steps=65535;proj.pat[0].brk.set[BRK_COMPLEX]=0;proj.pat[0].brk.set[BRK_ALEN]=0;
    if(stutter) {engine_brk_live(10,1);audio();}
    if(jungle) {engine_brk_live(9,1);audio();}
    midi_log=tmpfile();CHECK(midi_log!=0);unsigned before=note_ons;seq.req_start=1;
    for(int i=0;i<200;i++){if(core2==2 && i==10)cpu2_fail_once=1;audio();}
    unsigned hits=note_ons-before;CHECK(hits>0);
    rewind(midi_log);char line[128];unsigned t,st,n,v,pending=0,previous=0,pairs=0;
    while(fgets(line,sizeof line,midi_log))if(sscanf(line,"%u ms: %x %x %x",&t,&st,&n,&v)==4) {
        if((st&240)==0x90&&v){CHECK(!pending);CHECK(n>=12&&n<44);pending=1;previous=n;}
        else if((st&240)==0x80){CHECK(pending&&previous==n);pending=0;pairs++;}
    }
    CHECK(!pending&&pairs==hits);CHECK(!midi_pulse_crosscore);fclose(midi_log);midi_log=0;
    seq.send_notes=0;before=note_ons;for(int i=0;i<24;i++)audio();CHECK(note_ons==before);
    seq.req_stop=1;audio();seq.send_notes=1;before=note_ons;note(0,12,1);for(int i=0;i<24;i++)audio();note(0,12,0);CHECK(note_ons==before);
    return hits;
}
/* Composition proof: actual framebuffer updates, not a guessed state signature. */
static unsigned main_pixels(void) {
    unsigned h=2166136261u;
    for(int i=MAIN_Y*240;i<(MAIN_Y+2*BAND_H)*240;i++) h=(h^fb[i])*16777619u;
    return h;
}
static void composed_geometry(void) {
    reset(); count_set(32,0); count_set(4,1); ui.msg_until=0;
    ui_frame(); unsigned first=main_pixels(); ui.brk_page=3;ui_frame();CHECK(main_pixels()!=first);
    first=main_pixels();ui.brk_count_target=1;ui_frame();CHECK(main_pixels()!=first);
    first=main_pixels();key_event(black(2),1);audio();ui_frame();CHECK(main_pixels()!=first);
    key_event(black(2),0);audio();ui_frame();CHECK(main_pixels()==first);
    for(int n=1;n<=32;n++){char t[4];put_i(t,n);CHECK(tw(&FONT_XS,t)<=12);}
    CHECK(8+tw(&FONT_XS,"A KEYS 25-32/32 PENDING")<144);
    CHECK(144+tw(&FONT_XS,"EDIT B 32")<=232);
    CHECK(8+tw(&FONT_T,"C/D COUNT C#/D# PAGE | LIVE A:32")<=232);
    const char *const *names=engine_brk_slot_names();
    for(int i=0;i<engine_brk_nslots();i++) {char t[32];put_s(put_s(t,"A  "),names[i]);CHECK(tw(&FONT_XS,t)<=108);}
    CHECK(8+15*14+12==230);
    printf("composed geometry: actual count/page/target/live repaint and 1-32 ink bounds checked\n");
}
int main(void)
{
    for(unsigned i=0;i<sizeof synthetic;i++)synthetic[i]=(uint8_t)(i*19+1);
    reset();
    const unsigned counts[]={4,8,16,32};
    for(unsigned ci=0;ci<4;ci++) {
        unsigned n=counts[ci];count_set(n,0);CHECK(engine_brk_count(0)==(int)n);
        for(int src=0;src<3;src++)for(int raw=0;raw<128;raw++) {
            unsigned sl=(raw+128-12)%n;
            note(src,raw,1);CHECK(engine_brk_held()==(1u<<sl));
            note((src+1)%3,raw,0);CHECK(engine_brk_held()==(1u<<sl));
            note(src,raw,0);CHECK(engine_brk_held()==0);
        }
        note(0,12,1);note(0,12+n,1);note(1,12,1);
        note(0,12,0);CHECK(engine_brk_held()==1);note(0,12+n,0);CHECK(engine_brk_held()==1);
        engine_brk_live(32,1);audio();note(1,12,0);CHECK(engine_brk_held()==1);
        count_set(counts[(ci+1)%4],0);CHECK(engine_brk_count(0)==(int)n);
        engine_brk_live(32,0);audio();native_break_service();CHECK(engine_brk_held()==0);CHECK(engine_brk_count(0)==(int)counts[(ci+1)%4]);
        count_set(n,0);note(0,0,1);note(1,0,1);min_q[mi_w++%MQ]=PLAT_MIDI_LINK_END(0);audio();CHECK(engine_brk_held()!=0);
        min_q[mi_w++%MQ]=PLAT_MIDI_LINK_END(1);audio();CHECK(engine_brk_held()==0);
    }
    count_set(32,0);page(2);CHECK(ui.brk_page==1);key_event(black(0),1);audio();CHECK(engine_brk_held()==(1u<<8));page(2);CHECK(ui.brk_page==1);
    ui.overlay=O_LIST;key_event(black(0),0);audio();CHECK(engine_brk_held()==0);ui.overlay=O_NONE;page(2);CHECK(ui.brk_page==2);
    key_event(black(7),1);audio();CHECK(engine_brk_held()==(1u<<23));ui.part=PART_303A;key_event(black(7),0);audio();CHECK(engine_brk_held()==0);ui.part=PART_BRK;
    count_set(4,0);key_event(black(4),1);audio();CHECK(engine_brk_held()==0);key_event(black(4),0);
    for(int i=8;i<11;i++){key_event(black(i),1);audio();CHECK(engine_project_notes());key_event(black(i),0);audio();}
    // A pin survives reduction and is resolved for display/playback, not rewritten.
    count_set(32,0);ui.brk_page=3;key_event(0,1);key_event(black(7),1);CHECK(proj.pat[seq.ppat[TRK_BRK]].brk.slice[0]==32);
    count_set(4,0);CHECK(engine_brk_count(0)==32);key_event(black(7),0);key_event(0,0);audio();native_break_service();CHECK(engine_brk_count(0)==4);CHECK(proj.pat[seq.ppat[TRK_BRK]].brk.slice[0]==32);
    // White outer C/D counts selected loop only. E is not in this input bitmap.
    ui.brk_count_target=1;count_set(8,1);page(8);CHECK(brk_slice_count(&proj.pat[0].brk,1)==16);CHECK(engine_brk_count(0)==4);page(4);CHECK(brk_slice_count(&proj.pat[0].brk,1)==8);
    // Held-at-entry, modal, busy, clip eligibility do not replay a press later.
    ui_brk_page_keys(8,0);ui_brk_page_keys(8,1);CHECK(brk_slice_count(&proj.pat[0].brk,1)==8);ui_brk_page_keys(0,1);
    ui.overlay=O_LIST;page(8);CHECK(brk_slice_count(&proj.pat[0].brk,1)==8);ui.overlay=O_NONE;ui_brk_page_keys(8,1);CHECK(brk_slice_count(&proj.pat[0].brk,1)==8);ui_brk_page_keys(0,1);
    project_busy=1;page(8);CHECK(brk_slice_count(&proj.pat[0].brk,1)==8);project_busy=0;ui_brk_page_keys(0,1);
    // Channel edits retire external owners but not local holds; old-channel offs are stale.
    note(0,12,1);engine_brk_live(33,1);audio();unsigned oldch=seq.ch[TRK_BRK];seq.ch[TRK_BRK]=(oldch+1)&15;audio();CHECK(engine_brk_held()==2);
    min_q[mi_w++%MQ]=8u|((0x80u|oldch)<<8)|(12u<<16);audio();CHECK(engine_brk_held()==2);engine_brk_live(33,0);audio();CHECK(!engine_brk_held());
    // Actual render, then owner preview still a separate gate.
    for(unsigned ci=0;ci<4;ci++){count_set(counts[ci],0);for(unsigned pg=0;pg<(counts[ci]+7)/8;pg++){ui.brk_page=pg;ui.msg_until=0;ui_frame();char path[128];snprintf(path,sizeof path,"build/host/adaptive-A%u-page%u.png",counts[ci],pg+1);png_write(path);}}
    count_set(8,0);count_set(32,1);ui.brk_count_target=1;ui.brk_page=0;ui.msg_until=0;ui_frame();png_write("build/host/adaptive-A8-B32-editB.png");
    count_set(32,0);count_set(4,1);ui.brk_count_target=1;ui.brk_page=3;ui.msg_until=0;ui_frame();png_write("build/host/adaptive-A32-B4-editB.png");
    key_event(black(2),1);audio();int sl,bk,dv,running;engine_brk_state(&sl,&bk,&dv,&running);CHECK(!running&&sl==26&&bk==0);ui.msg_until=0;ui_frame();png_write("build/host/adaptive-stopped-live-A27.png");key_event(black(2),0);audio();CHECK(!engine_brk_held());
    unsigned plain=output_run(0,0,0),remote=output_run(0,1,0),stutter=output_run(1,1,0);CHECK(plain==remote&&plain==stutter);CHECK(plain==output_run(0,1,1));unsigned fallback=output_run(0,2,0);CHECK(fallback>0&&fallback<=plain);
    // AS-001 native caller: no unrelated edit/pattern change after release.
    reset();count_set(32,0);key_event(black(0),1);audio();page(4);CHECK(engine_brk_count(0)==32);
    key_event(black(0),0);audio();project_busy=1;
    for(int i=0;i<10;i++){native_break_service();audio();}CHECK(engine_brk_count(0)==32);
    project_busy=0;for(int i=0;i<100;i++){ui_input_only();ui_frame();audio();native_break_service();}
    CHECK(engine_brk_count(0)==16);CHECK(!engine_brk_held());
    composed_geometry();
    reset();
    any_button=1;const char *a,*b,*c;footer_text(&a,&b,&c);CHECK(strstr(b,"JUNGLE")!=0);
    CHECK(8+tw(&FONT_T,b)<236);ui_frame();png_write("build/host/break-jungle-hint.png");
    uint8_t saved_pitch=proj.sound.v[T_BRK][0][1];
    fm1_in.frames=400;fm1_in.wheel[0]=550;fm1_in.wheel[1]=615;smk_wheels();audio();
    fm1_in.wheel[0]=630;smk_wheels();audio();CHECK(engine_brk_bend_value()==8191);
    ui.part=PART_303A;smk_wheels();audio();CHECK(engine_brk_bend_value()==8191);
    fm1_in.wheel[0]=470;smk_wheels();audio();CHECK(engine_brk_bend_value()==-8192);
    // Parameter flood cannot drop the latest centre update.
    for(int i=0;i<1000;i++)engine_set(T_BRK,0,1,i%25);
    fm1_in.wheel[0]=550;smk_wheels();audio();CHECK(engine_brk_bend_value()==0);
    CHECK(proj.sound.v[T_BRK][0][1]==saved_pitch);CHECK(PROJ_SND_REV==13);
    // External pitch bend consumes only BREAK's channel. Link/channel lifecycle
    // releases that source; another source cannot clear a physical gesture.
    reset();
    for(int src=0;src<3;src++) {
        min_q[mi_w++%MQ]=14u|((unsigned)src<<4)|((0xe0u|seq.ch[TRK_BRK])<<8)|(127u<<16)|(127u<<24);audio();
        CHECK(engine_brk_bend_value()==8191);
        min_q[mi_w++%MQ]=PLAT_MIDI_LINK_END((src+1)%3);audio();CHECK(engine_brk_bend_value()==8191);
        min_q[mi_w++%MQ]=PLAT_MIDI_LINK_END(src);audio();CHECK(engine_brk_bend_value()==0);
    }
    min_q[mi_w++%MQ]=14u|((0xe0u|seq.ch[TRK_BRK])<<8);audio();CHECK(engine_brk_bend_value()==-8192);
    seq.ch[TRK_BRK]=(seq.ch[TRK_BRK]+1)&15;audio();CHECK(engine_brk_bend_value()==0);
    engine_brk_bend(4096);audio();
    min_q[mi_w++%MQ]=14u|((0xe0u|seq.ch[TRK_BRK])<<8);audio();CHECK(engine_brk_bend_value()==4096);
    min_q[mi_w++%MQ]=PLAT_MIDI_LINK_END(0);audio();CHECK(engine_brk_bend_value()==4096);
    engine_brk_bend(0);audio();CHECK(engine_brk_bend_value()==0);
    // Clip keyboard does not acquire a new BREAK wheel gesture.
    reset();ui.part=PART_BRK;test_clip_mode=1;fm1_in.wheel[0]=630;smk_wheels();audio();
    CHECK(engine_brk_bend_value()==0);fm1_in.wheel[0]=550;smk_wheels();audio();test_clip_mode=0;
    printf("adaptive controls: %u checks, %u failures\n",checks,fails);return fails?1:0;
}
