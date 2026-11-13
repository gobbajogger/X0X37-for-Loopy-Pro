/* SPDX-License-Identifier: GPL-3.0-only */
/* Production engine_render -> sequencer -> UI; platform substitutions are those
 * of smc_ui_test, not a fictional IRQ implementation. No device proof. */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static void audio(unsigned n)
{
    int32_t out[512];
    while(n) { unsigned k=n>256?256:n; engine_render(out,k); n-=k; }
}
static void clock_in(void) { seq_midi_rt(&seq,0xF8); audio(0); /* render(0) still drains */
    int32_t out[2];engine_render(out,0); }
int main(void)
{
    unsigned phase[8]; char name[80];
    setup(V_HOME,PART_303A);
    seq.bpm=120;seq.send_clock=0;seq.clk_src=SEQ_CLK_INT;
    audio(256);CHECK(seq.metro_view==0);shot("metro-stopped");
    seq_start(&seq);audio(1);CHECK(seq.metro_view==1);
    for(unsigned i=0;i<8;i++) {
        phase[i]=seq.metro_view;printf("FRAME %u phase=%u raw=%u period=%u bpm=%f ext=%u\n",i,phase[i],seq.metro_phase,seq.metro_period,seq.bpm,seq.ext);ui_frame();
        snprintf(name,sizeof name,"metro-clockout-off-%02u",i);shot(name);
        audio((i+1)*FS/16-i*FS/16);
    }
    CHECK(phase[0]==1);CHECK(phase[2]==9);CHECK(phase[4]==17);CHECK(phase[6]>=24 && phase[6]<=25);
    CHECK(seq.metro_view==1); /* one quarter, sample-derived even with no MIDI clock */
    seq_stop(&seq);audio(1);CHECK(seq.metro_view==0);ui_frame();shot("metro-stopped-after-play");
    audio(FS);CHECK(seq.metro_view==0);
    seq_continue(&seq);audio(1);CHECK(seq.metro_view==1);audio(FS/8);
    unsigned before=seq.metro_view;seq.bpm=240;audio(0);
    {int32_t out[2];engine_render(out,0);}CHECK(seq.metro_view==before);
    audio(FS/16+1);CHECK(seq.metro_view==17);
    /* Track rate, swing and direction do not own this publication. */
    proj.pat[0].bass[0].rate=RATE_32;proj.pat[0].bass[0].dir=DIR_RANDOM;
    audio(FS/8+1);CHECK(seq.metro_view==1);
    audio(FS/16);before=seq.metro_view;
    seq_stop(&seq);audio(1);CHECK(seq.metro_view==0);audio(FS);
    seq_continue(&seq);audio(1);CHECK(seq.metro_view==before);
    seq_stop(&seq);audio(1);seq.clk_src=SEQ_CLK_EXT;seq_start(&seq);audio(1);
    CHECK(seq.metro_view==0);clock_in();CHECK(seq.metro_view==1);
    for(unsigned i=1;i<24;i++) { audio(FS/48);clock_in();CHECK(seq.metro_view==1+i*32/24); }
    audio(FS/48);clock_in();CHECK(seq.metro_view==1);
    audio(FS);CHECK(seq.metro_view==0); /* external loss never free-runs visually */
    seq_stop(&seq);audio(1);clock_in();CHECK(seq.metro_view==0);
    seq_start(&seq);audio(1);CHECK(seq.metro_view==0);clock_in();CHECK(seq.metro_view==1);
    seq_init(&seq,proj.pat);CHECK(seq.metro_view==0);
    printf("PASS clocked metronome actual engine + UI: %u checks; sizeof(seq_t)=%zu\n",checks,sizeof(seq_t));
    return 0;
}
