/* SPDX-License-Identifier: GPL-3.0-only */
/* Production UI/SMC output + actual sequencer clock/transport; no device I/O. */
#include <stdint.h>
#include <string.h>
static unsigned snapshot_mode, snapshot_calls;
static void snapshot_probe(unsigned attempt);
#define SMC_CLOCK_SNAPSHOT_TEST_HOOK snapshot_probe
/* Keep the observer available when replaying the older feedback source as a
 * baseline negative; that replay must compile before its old LED path fails. */
#include "../../firmware/src/app/smc_clock_feedback.h"
static unsigned sends, pause_sends;
static uint8_t pause_value;
int plat_ble_smc_send(const uint8_t *m, uint8_t n)
{
    sends++;
    if (n == 3 && m[0] == 0x90 && m[1] == 0x5D) { pause_sends++; pause_value = m[2]; }
    return 1;
}
#define SMC_UI_SEND_IMPLEMENTED 1
#define SMC_UI_MAIN smc_clock_prior_ui_main
#include "smc_ui_test.c"
static void snapshot_probe(unsigned attempt)
{
    if (!snapshot_mode) return;
    snapshot_calls++;
    if (snapshot_mode==2 || attempt==0) {
        seq.ext_q=(uint8_t)((seq.ext_q+12)%24);seq.ext_now++;
    }
}
static int drum(void *c,int k,int v,float a) { (void)c;(void)k;(void)v;(void)a;return 1; }
static int bass(void *c,int p,int n,int a,int s,int v) { (void)c;(void)p;(void)n;(void)a;(void)s;(void)v;return 1; }
static void off(void *c,int p) { (void)c;(void)p; }
static const seq_sink_t sink = {.drum=drum,.bass_on=bass,.bass_off=off};
static int pulse(void)
{
    seq_t before = seq;
    int p = smc_clock_feedback(&seq);
    CHECK(!memcmp(&before,&seq,sizeof seq)); /* observer never writes any music/transport state */
    return p;
}
static void reset_clock(int bpm)
{
    setup(V_HOME,PART_303A);
    seq_init(&seq,proj.pat);seq.bpm=(float)bpm;
    memset(&smc_beat,0,sizeof smc_beat);
    CHECK(pulse());
}
static void advance(uint32_t samples)
{
    while (samples) {
        uint32_t n=samples>64?64:samples;
        seq_advance(&seq,n,&sink);samples-=n;
    }
}
static void edges_and_stop(void)
{
    for (int bpm=20;bpm<=275;bpm+=5) {
        reset_clock(bpm);
        /* Integer-BPM oracle uses exact sample*BPM, independent of wall time. */
        for (unsigned n=0;n<3000;n++) {
            advance(37);
            if(n==1 || n==100 || n==299) seq_stop(&seq);
            uint64_t position=(uint64_t)seq.ext_now*(unsigned)bpm;
            int expected=position%(FS*60u)<FS*30u;
            CHECK(pulse()==expected);
            now_ms+=123457;CHECK(pulse()==expected); /* GUI/wall timers cannot change the beat */
        }
    }
    reset_clock(120);advance(FS/4-1);CHECK(pulse());
    seq_stop(&seq);advance(1);CHECK(!pulse());
    seq_stop(&seq);advance(FS/4);CHECK(pulse());
    /* Tempo edits preserve phase; stopping immediately after does too. */
    advance(FS/8);CHECK(pulse());uint32_t phase=smc_beat.phase;
    seq.bpm=60;CHECK(pulse());CHECK(smc_beat.phase==phase);
    seq_stop(&seq);advance(FS/8);CHECK(pulse());
    advance(FS/8+2);CHECK(!pulse());
    puts("PASS project tempos20..275, exact sample edges, Stop before/after edges, tempo-edit then Stop, wall-time independence");
}
static void transport(void)
{
    reset_clock(120);advance(FS/3);CHECK(!pulse());
    for(unsigned i=0;i<9;i++) {
        uint32_t before=smc_beat.phase;
        if(i%3==0)seq_start(&seq);else if(i%3==1)seq_stop(&seq);else seq_continue(&seq);
        advance(1);pulse();CHECK(smc_beat.phase==(before+120u*256u)%SMC_BEAT_UNITS);
        advance(137);pulse();
    }
    puts("PASS actual Start/Stop/Continue midbeat: shared visual phase never resets on transport state");
}
static void external_loss(void)
{
    for(unsigned stopped=0;stopped<2;stopped++) {
    reset_clock(125);seq.clk_src=SEQ_CLK_EXT;
    seq_midi_rt(&seq,0xFA);advance(1);
    for (unsigned n=0;n<240;n++) {
        if(stopped && n==120)seq_stop(&seq); /* clocks continue through Stop */
        seq_ext_clock(&seq);advance(1);pulse();
        CHECK(pulse()==(((seq.ext_q+23u)%24u)<12));
        advance(918);pulse();
    }
    uint32_t retained=smc_beat.bpm,phase=smc_beat.phase,sample=seq.ext_now;
    CHECK(retained!=125u*256u);if(stopped)seq_stop(&seq);advance(1);pulse();
    /* Force a complete external loss; seq intentionally rounds its own fallback
     * tempo. Visual state keeps its last measured tempo and phase independently. */
    advance(FS);pulse();CHECK(seq.clk_lost);CHECK(smc_beat.bpm==retained);
    CHECK(smc_beat.phase==(phase+(uint64_t)(seq.ext_now-sample)*retained)%SMC_BEAT_UNITS);
    advance(FS*2);pulse();CHECK(smc_beat.bpm==retained);
    for(unsigned n=0;n<60;n++) {
        seq_ext_clock(&seq);advance(1);pulse();
        CHECK(!seq.clk_lost);CHECK(pulse()==(((seq.ext_q+23u)%24u)<12));
        advance(734);pulse(); /* recovery clock changes tempo */
    }
    CHECK(smc_beat.bpm!=retained);
    }
    for(unsigned which=0;which<2;which++) {
        reset_clock(120);seq.ext=1;seq.clk_lost=0;seq.ext_period=1;seq.ext_q=1;
        seq.ext_bpm=which?300.25f:9.5f;pulse();
        CHECK(smc_beat.bpm==(which?76864u:2432u));
    }
    puts("PASS real external clock through Stop, running/stopped loss retains measured BPM+phase, recovery changed phase/tempo, external tempos outside project range");
}
static void locked_tempo_loss(void)
{
    reset_clock(120);smc.up=1;proj.set.bt|=PROJ_BT_SMC_CENTRAL;
    CHECK(smc_lock_toggle());
    seq.ext=1;seq.ext_period=1000u*256u;seq.ext_bpm=150.25f;seq.ext_q=1;
    seq.ext_since=0;seq.ext_now=1000;smc_frame();CHECK(smc_beat.bpm==38464u);
    seq.ext_bpm=173.5f;seq.ext_now=2000;smc_frame();CHECK(smc_beat.bpm==44416u);
    CHECK(smc_led(0x5D));
    smc_ble_link(0);smc_poll();seq.ext_bpm=181.25f;seq.ext_now=3000;smc_frame();
    CHECK(smc_beat.bpm==46400u); /* no UI draw and no connected-output requirement */
    uint32_t phase=smc_beat.phase, sample=seq.ext_now;
    seq.clk_lost=1;seq.ext_q=0;seq.bpm=181;seq.ext_since=FS;seq.ext_now+=FS;smc_frame();
    CHECK(smc_beat.bpm==46400u);
    CHECK(smc_beat.phase==(phase+(uint64_t)(seq.ext_now-sample)*46400u)%SMC_BEAT_UNITS);
    smc_ble_link(1);smc_poll();CHECK(smc.locked);CHECK(smc_lock_toggle());smc_frame();
    CHECK((pause_value!=0)==pulse());CHECK(smc_beat.bpm==46400u);
    puts("PASS locked/disconnected no-draw tempo changes, subsequent loss+unlock keeps last measured BPM and continuous phase");
}
static void snapshot_consistency(void)
{
    reset_clock(120);seq.ext=1;seq.ext_period=1000u*256u;seq.ext_bpm=180;
    seq.ext_q=1;seq.ext_since=0;seq.ext_now=1000;
    snapshot_mode=1;snapshot_calls=0;
    CHECK(!smc_clock_feedback(&seq));CHECK(snapshot_calls==2);CHECK(smc_beat.bpm==180u*256u);
    snapshot_mode=2;snapshot_calls=0;uint32_t phase=smc_beat.phase,bpm=smc_beat.bpm;
    seq.ext_now+=300;seq.ext_bpm=220;
    (void)smc_clock_feedback(&seq);CHECK(snapshot_calls==2);CHECK(smc_beat.bpm==bpm);
    CHECK(smc_beat.phase==(phase+(uint64_t)301*bpm)%SMC_BEAT_UNITS);
    snapshot_mode=0;pulse();CHECK(smc_beat.bpm==220u*256u);
    puts("PASS bounded clock-snapshot retry and carry-through after2 unstable reads; no cross-core racefree claim");
}
static void feedback(void)
{
    reset_clock(120);smc.up=1;proj.set.bt|=PROJ_BT_SMC_CENTRAL;smc.locked=0;
    memset(smc.known,0,sizeof smc.known);smc_frame();CHECK(pause_value==127);
    sends=pause_sends=0;
    for(unsigned n=0;n<4000;n++) {advance(44);smc_frame();}
    CHECK(pause_sends==15);CHECK(sends==15); /* only the quarter-note edges, no MIDI clock or feedback flood */
    uint32_t midbeat=smc_beat.phase;
    CHECK(smc_lock_toggle());smc_frame();CHECK(pause_value==127);CHECK(smc_beat.phase==midbeat);
    sends=pause_sends=0;for(unsigned n=0;n<2000;n++){advance(44);smc_frame();}
    CHECK(!sends);CHECK(smc_led(0x5D));
    CHECK(smc_lock_toggle());smc_frame();CHECK((pause_value!=0)==pulse());
    int prior=pulse();smc_ble_link(0);smc_poll();advance(73);smc_ble_link(1);smc_poll();
    smc_frame();CHECK((pause_value!=0)==pulse());CHECK(pulse()==prior);
    /* Physical-position pickup marker and its explicit UP/DOWN text pulse together. */
    reset_clock(120);pref_set(smc_target(0),90);smc.last[0]=20;smc.picked=0;
    smc_ui_begin(0,0);smc_ui_draw();
    unsigned y=MAIN_Y+111-20*45/127;
    CHECK(fb[y*240+1]==swap16(C_WHITE));
    advance(FS/4);smc_ui_draw();CHECK(fb[y*240+1]==swap16(C_DIM));
    CHECK(smc_target(0).kind!=R_NONE);CHECK(!(smc.picked&1));
    /* Skipping many UI frames produces one current state, never a replay burst. */
    smc.up=1;proj.set.bt|=PROJ_BT_SMC_CENTRAL;smc_frame();sends=pause_sends=0;
    advance(FS*10+FS/4);smc_frame();CHECK(pause_sends<=1);CHECK(sends<=1);
    uint32_t old=smc_beat.phase;seq.ext_now=UINT32_MAX-10;smc_beat.sample=seq.ext_now;
    seq.ext_now=20;pulse();CHECK(smc_beat.phase==(old+31u*smc_beat.bpm)%SMC_BEAT_UNITS);
    puts("PASS deduplicated Pause MIDI transitions, locked steady, unlock/reconnect phase, pickup marker pulse without parameter edits");
}
int main(void)
{
    edges_and_stop();transport();external_loss();locked_tempo_loss();snapshot_consistency();feedback();
    printf("smc_clock_feedback: PASS (%u checks)\n",checks);
    return 0;
}
