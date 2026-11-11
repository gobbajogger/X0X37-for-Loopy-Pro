/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual wheel scanner, UI, queued engine, motion, SMC and DSP. Compare nonzero
 * rendered audio against a no-navigation control and observe command dispatch,
 * stored/displayed base and DSP pots/Hz. Hardware ADC/radio/timing are substitutes. */
#define BOARD_SMK37 1
#define X0X_DEV_PADCAL 1
#define main simulator_main
#include X0X_HOST_SOURCE
#undef main
#include <assert.h>
#include "dsp/drum909.h"
#include "dsp/drum808.h"

static bass303_t *observed_bass[2];
static unsigned commands;
static int last_target, last_voice, last_param, last_value;
void real_engine_set(int,int,int,int);
void wheel_test_engine_set(int t,int v,int i,int val) {
    commands++; last_target=t; last_voice=v; last_param=i; last_value=val;
    real_engine_set(t,v,i,val);
}
void wheel_test_bass303_set(bass303_t *d,int i,int v) {
    if (!observed_bass[0] || observed_bass[0]==d) observed_bass[0]=d;
    else observed_bass[1]=d;
    bass303_set(d,i,v);
}
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

static uint8_t wire_led[128];
int plat_ble_smc_send(const uint8_t *msg, uint8_t len)
{
    assert(len == 3);
    if (msg[0] == 0x90) wire_led[msg[1]] = msg[2];
    return 1;
}
#ifndef SMC_SLOT_SEND_IMPLEMENTED
/* Existing one-peer fixtures record peer zero. Dual-peer tests override this. */
int plat_ble_smc_send_slot(unsigned slot, const uint8_t *m, uint8_t n)
{ return slot == 0 ? plat_ble_smc_send(m, n) : 1; }
#endif

#include SMC_MIXER_SOURCE

static struct { uint32_t frames; int32_t wheel[2]; } fm1_in;
static int clamp(int v,int lo,int hi) { return v<lo?lo:v>hi?hi:v; }
static uint32_t smk_diag_epoch; /* scanner session token; no diagnostics in this fixture */
#include "smk_wheels.inc"
static int32_t peak_sample;
static uint64_t hash=1469598103934665603ULL;
static void render(void) {
    int32_t out[512]; engine_render(out,256);
    for (unsigned i=0;i<512;i++) { if(out[i]>peak_sample) peak_sample=out[i]; hash^=(uint32_t)out[i]; hash*=1099511628211ULL; }
}
static void select_part(int part,int smc_path) {
    if(smc_path) {
        /* Source-strip SELECT notes 24..28 are momentary FX (owner hold-FX
         * buses), not part navigation — same as smc_reachability. Use the
         * production SMC part selector that REC / bank-channel still call. */
        smc_select_part(part);
        smc_poll(); smc_frame();
        assert(ui.part==part);
    } else { bass_step_cancel(); drum_step_cancel(); ui.part=(uint8_t)part; build_pages(); }
}
int main(int argc,char **argv) {
    assert(argc==7);
    int nav=atoi(argv[1]), playing=atoi(argv[2]), locks=atoi(argv[3]), smc_path=atoi(argv[4]), part=atoi(argv[5]), gesture=atoi(argv[6]);
    boot(); engine_guard_enable(0); render();
    for(int p=0;p<2;p++) {
        proj.sound.v[T_303][p][BASS303_CUTOFF]=30+p*10;
        engine_set(T_303,p,BASS303_CUTOFF,30+p*10);
    }
    if(smc_path) { proj.set.bt=PROJ_BT_SMC_CENTRAL; smc_ble_link(1); smc_poll(); smc_frame(); }
    select_part(PART_303A+part,smc_path); set_view(V_PART);
    fm1_in.frames=400; fm1_in.wheel[0]=550; fm1_in.wheel[1]=615; smk_wheels(); render();
    /* An intentional mod gesture; saved/displayed base remains 30/40. */
    fm1_in.wheel[1]=(gesture&1)?550:615;
    fm1_in.wheel[0]=(gesture&2)?580:550; smk_wheels(); render();
    /* 550 is up from the down end at 615: the wheel replaces the knob (57), it does not add. */
    if(gesture&1) {
        assert(last_target==T_303 && last_param==BASS303_CUTOFF && last_value==57);
        assert(observed_bass[part]->pot[BASS303_CUTOFF]==57);
    }
    if(locks) {
        int k=motion_alloc(proj.arr.lane,0,PART_303A+part,T_303,part,BASS303_CUTOFF);
        assert(k>=0);
        for(int i=0;i<NSTEPS;i++) proj.arr.lane[k].val[i]=(locks==2 && (i&1))?95:77;
        engine_motion_reset();
    }
    seq.mute=seq.vmute=0;
    bpart_t *bp=&proj.pat[0].bass[part]; bp->len=4;
    for(int i=0;i<4;i++) bp->step[i]=(bstep_t){48,G_NOTE,100};
    if(playing) { seq_start(&seq); render(); }
    else { engine_bass_on(part,48,0,0); render(); }
    unsigned before=commands;
    int cutoff=observed_bass[part]->pot[BASS303_CUTOFF];
    for(int i=0;i<160;i++) {
        /* Both leaving a 303 and selecting the other 303 must be silent routing changes. */
        if(nav && i==5) select_part(PART_909,smc_path);
        if(nav && i==15) select_part(PART_303A+(1-part),smc_path);
        if(nav && i==25) select_part(PART_303A+part,smc_path);
        if(nav && i==35) set_view(V_SEQ);
        if(nav && i==45) set_view(V_PART);
        smk_wheels();
        if(commands!=before) fprintf(stderr,"navigation scan=%d queued target=%d voice=%d param=%d value=%d; stored=%d DSP=%d\n",i,last_target,last_voice,last_param,last_value,proj.sound.v[T_303][part][BASS303_CUTOFF],observed_bass[part]->pot[BASS303_CUTOFF]);
        if(commands!=before) {
            render();
            fprintf(stderr,"after queue drain: DSP cutoff=%d hz=%.9g stored/displayed=%d\n",
                observed_bass[part]->pot[BASS303_CUTOFF],observed_bass[part]->cutoff,
                pref_get(PR(R_ENG,T_303,part,BASS303_CUTOFF)));
        }
        assert(commands==before); /* no queue write even before the next render */
        render();
        if(locks!=2 || !playing) assert(observed_bass[part]->pot[BASS303_CUTOFF]==cutoff);
        hash^=observed_bass[part]->pot[BASS303_CUTOFF]; hash*=1099511628211ULL;
        assert(pref_get(PR(R_ENG,T_303,part,BASS303_CUTOFF))==30+part*10);
        assert(proj.sound.v[T_303][part][BASS303_CUTOFF]==30+part*10);
    }
    assert(peak_sample>0);
    printf("audio=%016llx cutoff=%d hz=%.9g base=%d commands=%u\n",
        (unsigned long long)hash,cutoff,observed_bass[part]->cutoff,
        proj.sound.v[T_303][part][BASS303_CUTOFF],commands-before);
    /* Release offscreen still goes to gesture owner, never current screen. */
    select_part(PART_909,smc_path);
    fm1_in.wheel[1]=615; fm1_in.wheel[0]=550; smk_wheels();
    assert(commands==before+((gesture&1)!=0)+((gesture&2)!=0)); render();
    /* Down closes. The saved knob stays. The filter is not left open. */
    if((gesture&1) && !playing) assert(observed_bass[part]->pot[BASS303_CUTOFF]==0);
    /* A knob edit is authoritative while a stationary wheel is still held. */
    seq_stop(&seq); render();
    select_part(PART_303A+part,smc_path);
    fm1_in.wheel[1]=550; smk_wheels(); render();
    proj.sound.v[T_303][part][BASS303_CUTOFF]=51;
    engine_set(T_303,part,BASS303_CUTOFF,51); render();
    before=commands;
    select_part(PART_909,smc_path); smk_wheels(); render();
    select_part(PART_303A+part,smc_path); smk_wheels(); render();
    assert(commands==before && observed_bass[part]->pot[BASS303_CUTOFF]==51);
    fm1_in.wheel[1]=615; smk_wheels(); render();
    /* Simultaneous gestures retain independent targets across part changes. */
    fm1_in.wheel[1]=550; smk_wheels(); render();
    select_part(PART_303A+(1-part),smc_path);
    fm1_in.wheel[0]=580; smk_wheels(); render();
    assert(last_param==CHI_PBEND && last_voice==1-part && last_value!=64);
    assert(observed_bass[1-part]->pot[BASS303_TUNE]==64);
    assert(observed_bass[1-part]->bend_pot==(uint8_t)last_value);
    before=commands;
    fm1_in.wheel[1]=615; smk_wheels();
    assert(commands==before+1 && last_voice==part && last_param==BASS303_CUTOFF && last_value==0);
    fm1_in.wheel[0]=550; smk_wheels();
    assert(commands==before+2 && last_voice==1-part && last_param==CHI_PBEND);
    assert(last_value==proj.sound.v[T_303][1-part][CHI_PBEND]);
    render();
    assert(observed_bass[1-part]->pot[BASS303_TUNE]==64);
    assert(observed_bass[1-part]->bend_pot==proj.sound.v[T_303][1-part][CHI_PBEND]);
    return 0;
}
