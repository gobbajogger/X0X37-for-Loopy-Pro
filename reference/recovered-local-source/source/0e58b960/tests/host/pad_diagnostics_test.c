/* SPDX-License-Identifier: GPL-3.0-only */
/* Live recording on the whole app (the host simulator, host/x0x_host.c without its script main):
 *  - 303: ENV or LFO held with a key writes an accent or a slide, as step input does. The release of
 *    ENV or LFO does not open the FX or MIX view, and the next keys still record (a73).
 *  - 909: after a Start with the external clock already running, a hit before the first tick goes to
 *    step 1, not to the step of the last run (a73).
 *  - 909: a live hit records on the nearest step; just before the loop start: step 1 (a73).
 * Build: see tests/run_tests.sh (needs build/gen from host/build_host.sh). */
#ifdef __APPLE__
#define _DARWIN_C_SOURCE 1 /* expose host libproc instrumentation under POSIX feature selection */
#endif
#define X0X_PAD_DIAG_TEST 1
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
int smk_ck_smc_turn(int n,int d){(void)n;(void)d;return 0;}
int smk_ck_smc_nav(uint8_t n){(void)n;return 0;}
static unsigned feedback;
int plat_ble_smc_send(const uint8_t *m,uint8_t n){(void)m;(void)n;feedback++;return 1;}
int plat_ble_smc_send_slot(unsigned slot,const uint8_t *m,uint8_t n){(void)slot;return plat_ble_smc_send(m,n);}
#include "../../firmware/src/app/smc_mixer.c"

#endif


#include <assert.h>
unsigned engine_live_key_probe(int voice,int field);
static unsigned accepted;
void engine_live_key_accepted(int v,int t,int p,int vel,int slide){(void)v;(void)t;(void)p;(void)vel;(void)slide;accepted++;}
static unsigned checks;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
uint64_t plat_physical_keys(void){return (uint64_t)plat_keys()<<5;}
int plat_outer_keys(plat_outer_event_t *e,uint64_t *keys,int *overflow,uint32_t epoch,int enabled,int octave,int publish)
{(void)e;(void)epoch;(void)enabled;(void)octave;(void)publish;*keys=plat_physical_keys();*overflow=0;return 0;}
static pad_diag_sample_t observed;
static uint32_t smk_diag_epoch, smk_boot_epoch; /* extracted wheel body: cold-boot baseline */
static struct {uint32_t frames,buttons;uint16_t wheel[2];uint8_t fader[4];int16_t enc_steps[16];} fm1_in;
static struct {uint8_t btn[NB],enc[NE];int8_t dir[NE];} panel;
static uint8_t smk_fbank;
#define SMK_BTN_FADER_BANK 1
#define SMK_BTN_LEFT 2
#define SMK_BTN_RIGHT 3
#define fm1_ms now_ms
#define clip_grid_mode test_clip_mode
static uint32_t fm1__lock(void){return 0;}
static void fm1__unlock(uint32_t f){(void)f;}
#include "../../firmware/src/app/smk_arrows.h"
static int32_t clamp(int32_t x,int32_t lo,int32_t hi){return x<lo?lo:x>hi?hi:x;}
#include "smk_wheels.inc"
#include "smk_faders.inc"
#include "smk_arrows.inc"
static int sample_result=1;
static unsigned raw_arrows;
uint32_t plat_pad_diag_session(int enabled){return enabled?++smk_diag_epoch:smk_diag_epoch;}
int plat_pad_diag_sample(unsigned selected,pad_diag_sample_t *out){(void)selected;*out=observed;return sample_result;}
unsigned plat_pad_diag_arrows(void){return raw_arrows;}
static void shot(const char *dir,const char *name){char p[512];pad_diag_draw();snprintf(p,sizeof p,"%s/%s.png",dir,name);png_write(p);}
int main(int argc,char **argv)
{
    const char *dir=argc>1?argv[1]:".";
    boot();ui.held_step=ui.hold_step=ui.brk_held=-1;
    memset(&pad_diag,0,sizeof pad_diag);pad_diag.epoch=1;smk_diag_epoch=1;pad_diag_on=1;
    observed.ack=1;observed.raw=4095;observed.base=2048;observed.peak=4095;
    observed.velocity=127;observed.pressure=127;observed.key=37;observed.on=24;observed.off=12;observed.peakn=3;
    for(int i=0;i<5;i++)observed.curve[i]=(uint16_t)(i*1000);
    shot(dir,"waiting");CHECK(pad_diag.status==PD_WAITING);
    now_ms=20;observed.frames=1;observed.ok=16;observed.states[0]=0x80;observed.states[2]=2;observed.seen=5;
    shot(dir,"live");CHECK(pad_diag.status==PD_LIVE);CHECK(pad_diag.last.seen==5);
    pad_diag.page=1;shot(dir,"signal");pad_diag.page=2;now_ms=1020;observed.frames+=400;observed.ok+=6400;shot(dir,"scan");CHECK(pad_diag.rates);CHECK(pad_diag.fps==393);
    sample_result=0;now_ms+=10;shot(dir,"busy");CHECK(pad_diag.status==PD_BUSY);
    now_ms+=501;shot(dir,"stale");CHECK(pad_diag.status==PD_STALE);
    memset(&pad_diag,0,sizeof pad_diag);sample_result=-1;shot(dir,"unavailable");CHECK(pad_diag.status==PD_UNAVAILABLE);

    /* Actual modal dispatch: all musical/parameter controls are parked, only
     * selector/pages and the explicit exit controls are consumed. */
    pad_diag_on=0;sample_result=1;held_keys=held_btn=0;ui.btn=ui.keys=0;
    ui.part=PART_303A;ui.view=V_GLO;ui.list_sel=3;ui.list_top=1;ui.rec=0;seq.playing=0;
    proj.set.bt=PROJ_BT_SMC_CENTRAL;smc_ble_link(1);smc_poll();
    uint8_t cc[3]={0xb0,16,1};smc_ble_midi(cc,3);
    pad_diag_enter();CHECK(ui_pad_diag_active());CHECK(smc_inputs[0].qr==smc_inputs[0].qw);
    fm1_in.frames=400;fm1_in.wheel[0]=550;fm1_in.wheel[1]=615;smk_wheels();
    fm1_in.wheel[0]=580;fm1_in.wheel[1]=550;smk_wheels();CHECK(!engine_bass_wheel_active());
    sound_t sound=proj.sound;unsigned page=pad_diag.page;
    held_keys=(1u<<10);held_btn=(1u<<B_PLAY)|(1u<<B_REC);enc_acc[EN_K1]=12;
    ui_input_only();CHECK(!seq.playing && !ui.rec);CHECK(!memcmp(&sound,&proj.sound,sizeof sound));
    CHECK(pad_diag.page==page);CHECK(ui.keys==held_keys);
    unsigned feedback_before=feedback;
    smc_ble_midi(cc,3);smc_poll();smc_frame();CHECK(feedback==feedback_before);
    CHECK(smc_inputs[0].qr==smc_inputs[0].qw && !smc_ui_active());
    CHECK(!memcmp(&sound,&proj.sound,sizeof sound));
    held_btn=0;enc_acc[EN_SELECT]=1;raw_arrows=2;ui_input_only();CHECK(pad_diag.selected==1 && pad_diag.page==1);
    smc_ble_midi(cc,3); /* backlog after the last diagnostic poll */
    raw_arrows=0;held_btn=1u<<B_HOME;ui_input_only();CHECK(!ui_pad_diag_active());CHECK(smc_inputs[0].qr==smc_inputs[0].qw);
    CHECK(ui.view==V_GLO && ui.list_sel==3);smk_wheels();CHECK(!engine_bass_wheel_active());CHECK(ui.keys==held_keys);smc_poll();CHECK(!memcmp(&sound,&proj.sound,sizeof sound));
    held_btn=0;ui_input_only();int32_t audio[512];engine_render(audio,256);CHECK(!engine_live_key_probe(0,0));CHECK(!accepted);
    held_keys=0;ui_input_only();
    /* An entry/exit between wheel polls still seeds the new session baseline. */
    plat_pad_diag_session(1);plat_pad_diag_session(0);fm1_in.wheel[0]=570;smk_wheels();CHECK(!engine_bass_wheel_active());
    seq.playing=1;pad_diag_enter();CHECK(!ui_pad_diag_active());seq.playing=0;
    test_clip_mode=1;pad_diag_enter();CHECK(!ui_pad_diag_active());test_clip_mode=0;
    kbd_held_key[0]=11;pad_diag_enter();CHECK(!ui_pad_diag_active());kbd_held_key[0]=0;
    /* Counter discontinuity must not manufacture fresh data/rates. */
    pad_diag_view_t v={0};pad_diag_sample_t q={0};v.epoch=q.ack=10;
    pad_diag_observe(&v,&q,1,0);q.frames=100;pad_diag_observe(&v,&q,1,1000);CHECK(v.rates&&v.progress);
    q.frames=2;pad_diag_observe(&v,&q,1,1001);CHECK(!v.rates&&!v.progress&&v.status==PD_WAITING);
    q.frames++;pad_diag_observe(&v,&q,1,1002);CHECK(v.status==PD_LIVE);
    v.rate_frames=v.last.frames=0xfffffff0u;v.rate_at=2000;q.frames=0x10;pad_diag_observe(&v,&q,1,3000);CHECK(v.rates&&v.fps==32);

    /* A frame consumer must seed across entry+exit with no frame, and also
     * across input arriving after its last active modal frame. */
    held_keys=held_btn=0;ui.btn=0;ui.view=V_PART;ui.part=PART_303A;ui.overlay=O_NONE;
    memset(panel.btn,255,sizeof panel.btn);panel.dir[EN_SELECT]=1;
    for(int mode=0;mode<2;mode++) {
        pref_t level=PR(R_ENG,T_MIX,part_at(0),MX_LEVEL);pref_set(level,64);
        fm1_in.fader[0]=0;smk_diag_epoch++;smk_faders();fm1_in.fader[0]=64;smk_faders();
        fm1_in.fader[0]=65;smk_faders();CHECK(pref_get(level)==65);
        fm1_in.buttons=0;smk_arrows();fm1_in.enc_steps[panel.enc[EN_SELECT]]=0;
        pad_diag_on=1;plat_pad_diag_session(1);
        if(mode){smk_faders();smk_arrows();smk_wheels();}
        fm1_in.fader[0]=100;fm1_in.buttons=1u<<SMK_BTN_RIGHT;
        fm1_in.wheel[0]=580+mode*5;fm1_in.wheel[1]=500+mode*10;
        pad_diag_on=0;plat_pad_diag_session(0);
        smk_faders();smk_arrows();smk_wheels();CHECK(!engine_bass_wheel_active());CHECK(pref_get(level)==65);CHECK(!fm1_in.enc_steps[panel.enc[EN_SELECT]]);
        now_ms+=1000;smk_faders();smk_arrows();smk_wheels();CHECK(!engine_bass_wheel_active());CHECK(pref_get(level)==65);CHECK(!fm1_in.enc_steps[panel.enc[EN_SELECT]]);
        fm1_in.buttons=0;smk_arrows();fm1_in.buttons=1u<<SMK_BTN_RIGHT;smk_arrows();CHECK(fm1_in.enc_steps[panel.enc[EN_SELECT]]==1);
        fm1_in.wheel[0]+=10;smk_wheels();CHECK(engine_bass_wheel_active());
        fm1_in.wheel[0]=550;fm1_in.wheel[1]=615;smk_wheels();CHECK(!engine_bass_wheel_active());
    }
    /* Boot's separate cutoff seeds the first post-animation wheel sample. */
    smk_boot_epoch++;fm1_in.wheel[0]=580;fm1_in.wheel[1]=520;smk_wheels();CHECK(!engine_bass_wheel_active());
    smk_wheels();CHECK(!engine_bass_wheel_active());
    fm1_in.wheel[0]+=10;smk_wheels();CHECK(engine_bass_wheel_active());
    fm1_in.wheel[0]=550;fm1_in.wheel[1]=615;smk_wheels();CHECK(!engine_bass_wheel_active());
    /* UNAVAILABLE must render exactly as no cached numeric samples, on all pages. */
    static uint16_t unavailable_fb[240*240];sample_result=-1;
    for(int page=0;page<3;page++) {
        pad_diag.page=page;pad_diag.valid=1;pad_diag.rates=1;pad_diag.last=observed;pad_diag.cached_selected=pad_diag.selected;
        pad_diag_draw();memcpy(unavailable_fb,fb,sizeof fb);
        pad_diag.valid=pad_diag.rates=0;memset(&pad_diag.last,0,sizeof pad_diag.last);pad_diag_draw();
        CHECK(!memcmp(unavailable_fb,fb,sizeof fb));
    }

    /* External transport is not filtered or stopped; diagnostic ownership ends. */
    pad_diag_on=0;seq.playing=seq.req_start=seq.req_cont=0;ui.held_step=ui.hold_step=ui.brk_held=-1;
    engine_bass_wheel_owners(0);kbd_held_key[0]=kbd_held_key[1]=0;
    held_btn=held_keys=ui.btn=ui.keys=0;
    seq.req_start=1;pad_diag_enter();CHECK(!pad_diag_on&&seq.req_start);seq.req_start=0;
    seq.req_cont=1;pad_diag_enter();CHECK(!pad_diag_on&&seq.req_cont);seq.req_cont=0;
    for(int mode=0;mode<3;mode++) {
        pad_diag_enter();CHECK(pad_diag_on);
        if(mode==0)seq.req_start=1;else if(mode==1)seq.req_cont=1;else seq.playing=1;
        ui_input_only();CHECK(!pad_diag_on);CHECK(seq.req_start||seq.req_cont||seq.playing);
        seq.req_start=seq.req_cont=seq.playing=0;
    }
    sample_result=1;memset(&pad_diag,0,sizeof pad_diag);pad_diag.epoch=observed.ack;observed.key=0;observed.state=0;observed.seen=0;memset(observed.states,0,sizeof observed.states);
    shot(dir,"input-none-idle");observed.frames++;observed.states[0]=observed.state=2;shot(dir,"input-peak");observed.frames++;observed.states[0]=observed.state=0x87;shot(dir,"input-unknown");
    CHECK(tw(&FONT_T,"KEY IDX NONE / STATE PEAK 3")<=228);

    memset(&pad_diag,0,sizeof pad_diag);pad_diag.epoch=observed.ack;
    sample_result=0;shot(dir,"initial-busy");CHECK(pad_diag.status==PD_BUSY&&!pad_diag.valid);
    sample_result=1;shot(dir,"initial-waiting");CHECK(pad_diag.status==PD_WAITING);
    now_ms++;observed.frames++;shot(dir,"recovered-live");CHECK(pad_diag.status==PD_LIVE);
    uint32_t advanced=pad_diag.last_advance;now_ms+=600;shot(dir,"coherent-stale");CHECK(pad_diag.status==PD_STALE&&pad_diag.last_advance==advanced);
    sample_result=0;pad_diag.selected=7;shot(dir,"selected-during-busy");CHECK(pad_diag.cached_selected!=pad_diag.selected);
    sample_result=1;observed.frames++;shot(dir,"selection-recovered");CHECK(pad_diag.cached_selected==7&&pad_diag.status==PD_LIVE);
    printf("PASS %u checks; state sizes sample=%zu view=%zu\n",checks,sizeof(pad_diag_sample_t),sizeof(pad_diag_view_t));return 0;
}
