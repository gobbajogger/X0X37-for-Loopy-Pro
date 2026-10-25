/* SPDX-License-Identifier: GPL-3.0-only */
/* Production SMC queue/dispatch and production sequencer bar-boundary checks. */
#define SMC_UI_MAIN smc_original_ui_main
#include "smc_ui_test.c"
static void tap_pattern(int note) { message(0x90,note,127); message(0x80,note,0); }
static int no_drum(void *x,int k,int v,float vel) { (void)x;(void)k;(void)v;(void)vel;return 1; }
static int no_on(void*x,int p,int n,int a,int sl,int v) { (void)x;(void)p;(void)n;(void)a;(void)sl;(void)v;return 1; }
static void no_off(void*x,int p) { (void)x;(void)p; }
static void no_break(void*x,int s,int bar,float spb,int en) { (void)x;(void)s;(void)bar;(void)spb;(void)en; }
static void no_stop(void*x) { (void)x; }
static void no_midi(void*x,uint8_t s,uint8_t a,uint8_t b) { (void)x;(void)s;(void)a;(void)b; }
static const seq_sink_t quiet_sink = {no_drum,no_on,no_off,no_break,no_stop,no_midi,0,0};
static void advance_pattern(uint32_t samples)
{
    seq_advance(&seq,0,&quiet_sink);
    while(samples) {
        uint32_t cap=samples<256?samples:256;
        uint32_t n=seq_until_event(&seq,cap);
        seq_advance(&seq,n,&quiet_sink); samples-=n;
    }
}
static void pattern_setup(int part, int pattern, int playing)
{
    setup(V_PART,part);
    for(int p=0;p<NPAT;p++) pattern_init(&proj.pat[p],0x100+p,0x200+p);
    seq_init(&seq,proj.pat); seq.bpm=120.0f; seq.send_clock=0;
    for(int t=0;t<NTRACKS;t++) seq.ppat[t]=pattern;
    if(playing) { seq_start(&seq); advance_pattern(0); CHECK(seq.playing); }
}
static void test_accumulation(void)
{
    pattern_setup(PART_303A,4,1);
    tap_pattern(0x5C); CHECK(seq_cue_of(&seq,PART_303A)==5);
    tap_pattern(0x5C); CHECK(seq_cue_of(&seq,PART_303A)==6);
    tap_pattern(0x5C); CHECK(seq_cue_of(&seq,PART_303A)==7);
    CHECK(seq.ppat[PART_303A]==4);
    tap_pattern(0x5B); CHECK(seq_cue_of(&seq,PART_303A)==6);
    tap_pattern(0x5B); CHECK(seq_cue_of(&seq,PART_303A)==5);
    tap_pattern(0x5B); CHECK(seq_cue_of(&seq,PART_303A)==-1);
    CHECK(seq.ppat[PART_303A]==4);
    for(int n=0;n<8;n++) { tap_pattern(0x5B); tap_pattern(0x5C); CHECK(seq_cue_of(&seq,PART_303A)==-1); }
    for(int t=0;t<NTRACKS;t++) if(t!=PART_303A) CHECK(seq_cue_of(&seq,t)==-1);
    puts("PASS rapid queued movement accumulates; reverse-to-current cancels only its part");
}
static void test_endpoints_and_stopped(void)
{
    pattern_setup(PART_909,NPAT-2,1);
    tap_pattern(0x5C); CHECK(seq_cue_of(&seq,PART_909)==NPAT-1);
    tap_pattern(0x5C); CHECK(seq_cue_of(&seq,PART_909)==NPAT-1); /* existing no-wrap */
    tap_pattern(0x5B); CHECK(seq_cue_of(&seq,PART_909)==-1);
    pattern_setup(PART_808,0,1);
    tap_pattern(0x5B); CHECK(seq_cue_of(&seq,PART_808)==-1);
    tap_pattern(0x5C); tap_pattern(0x5B); CHECK(seq_cue_of(&seq,PART_808)==-1);
    pattern_setup(PART_303B,4,0);
    tap_pattern(0x5C); tap_pattern(0x5C); CHECK(seq_cue_of(&seq,PART_303B)==6);
    advance_pattern(0); CHECK(seq.ppat[PART_303B]==6); CHECK(seq_cue_of(&seq,PART_303B)==-1);
    tap_pattern(0x5B); advance_pattern(0); CHECK(seq.ppat[PART_303B]==5);
    CHECK(!seq.playing);
    puts("PASS existing no-wrap endpoints preserved; stopped cues apply at next audio service");
}
static void test_edges(void)
{
    pattern_setup(PART_909,4,1);
    message(0x90,0x5C,127); message(0x90,0x5C,127); CHECK(seq_cue_of(&seq,PART_909)==5);
    message(0x90,0x5B,127); CHECK(seq_cue_of(&seq,PART_909)==-1);
    message(0x90,0x5B,127); CHECK(seq_cue_of(&seq,PART_909)==-1);
    message(0x80,0x5B,0); tap_pattern(0x5B); CHECK(seq_cue_of(&seq,PART_909)==3);
    message(0x80,0x5C,0); tap_pattern(0x5C); CHECK(seq_cue_of(&seq,PART_909)==-1);
    message(0x90,0x5C,127);
    const uint8_t noop[3]={0xB0,16,0}, release[3]={0x80,0x5C,0};
    for(unsigned i=0;i<SMC_Q;i++) smc_ble_midi(noop,3);
    smc_ble_midi(release,3); smc_poll();
    tap_pattern(0x5C); CHECK(seq_cue_of(&seq,PART_909)==6);
    message(0x90,0x5C,127); CHECK(smc.pattern_down);
    smc_ble_link(0); smc_poll(); smc_ble_link(1); smc_poll(); CHECK(!smc.pattern_down);
    tap_pattern(0x5B); CHECK(seq_cue_of(&seq,PART_909)==6);
    message(0x90,0x5B,127); CHECK(smc.pattern_down); smc_project_reset(); CHECK(!smc.pattern_down);
    puts("PASS held edge suppression, opposite held keys, lost release, reconnect and project reset");
}
static void host_pattern(int type, int note, int velocity)
{
    uint32_t packet = 0x09u | (uint32_t)(type | 15) << 8 | (uint32_t)note << 16 | (uint32_t)velocity << 24;
    CHECK(smc_midi_host(packet));
}
static void test_host_and_mmc(void)
{
    pattern_setup(PART_808,4,1); proj.set.bt=PROJ_BT_SMC_USER;
    host_pattern(0x90,0x5C,127); host_pattern(0x90,0x5C,127); smc_poll();
    CHECK(seq_cue_of(&seq,PART_808)==5);
    for(unsigned i=0;i<SMC_Q;i++) host_pattern(0xB0,16,0);
    host_pattern(0x80,0x5C,0); smc_poll(); CHECK(!smc.pattern_down);
    host_pattern(0x90,0x5C,127); host_pattern(0x80,0x5C,0); smc_poll();
    CHECK(seq_cue_of(&seq,PART_808)==6);
    pattern_setup(PART_808,4,1); proj.set.bt=PROJ_BT_SMC_USER;
    smc_mmc_host(37,0); smc_mmc_host(37,0); smc_poll();
    CHECK(seq_cue_of(&seq,PART_808)==6); CHECK(!smc.pattern_down);
    smc_mmc_host(36,0); smc_mmc_host(36,0); smc_poll();
    CHECK(seq_cue_of(&seq,PART_808)==-1);
    puts("PASS host lost-release recovery; independent release-less MMC presses accumulate and reverse");
}
static void test_quantization_and_ownership(void)
{
    pattern_setup(PART_303A,4,1);
    advance_pattern(5512*3);
    tap_pattern(0x5C); tap_pattern(0x5C); tap_pattern(0x5C);
    advance_pattern(88200-5512*3-1);
    CHECK(seq.ppat[PART_303A]==4); CHECK(seq_cue_of(&seq,PART_303A)==7);
    advance_pattern(2); CHECK(seq.ppat[PART_303A]==7); CHECK(seq_cue_of(&seq,PART_303A)==-1);
    for(int t=0;t<NTRACKS;t++) if(t!=PART_303A) CHECK(seq.ppat[t]==4);
    pattern_setup(PART_303A,4,1);
    tap_pattern(0x5C); tap_pattern(0x5B); advance_pattern(88201);
    CHECK(seq.ppat[PART_303A]==4 && seq_cue_of(&seq,PART_303A)==-1);
    pattern_setup(PART_303A,4,1); seq_chain(&seq,2,5);
    tap_pattern(0x5C); tap_pattern(0x5B); advance_pattern(88201);
    CHECK(seq.ppat[PART_303A]==5 && seq.chain_a==2 && seq.chain_b==5);
    pattern_setup(PART_303A,4,1); seq_cue(&seq,9); /* existing scene-style cue */
    tap_pattern(0x5C); CHECK(seq_cue_of(&seq,PART_303A)==10);
    for(int t=0;t<NTRACKS;t++) if(t!=PART_303A) CHECK(seq_cue_of(&seq,t)==9);
    tap_pattern(0x5B); CHECK(seq_cue_of(&seq,PART_303A)==9);
    static song_t arrangement;
    memset(&arrangement,0,sizeof arrangement); arrangement.len=1;
    for(int t=0;t<NTRACKS;t++) arrangement.bar[0].pat[t]=12;
    seq.song=&arrangement; seq.song_on=1; seq.song_rec=0;
    uint8_t chain_a=seq.chain_a, chain_b=seq.chain_b;
    tap_pattern(0x5C); CHECK(seq.song_on && !seq.song_rec && seq.song==&arrangement);
    CHECK(seq.chain_a==chain_a && seq.chain_b==chain_b);
    advance_pattern(88201); CHECK(seq.ppat[PART_303A]==12); CHECK(seq_cue_of(&seq,PART_303A)==-1);
    pattern_setup(PART_909,4,1); CHECK(smc_lock_toggle()); ui.part=PART_303B;
    tap_pattern(0x5C); tap_pattern(0x5C); CHECK(seq_cue_of(&seq,PART_909)==6);
    CHECK(seq_cue_of(&seq,PART_303B)==-1); CHECK(smc.bank==0);
    puts("PASS actual quantized bar boundary, other parts, scene/song ownership and locked part");
}
int main(void)
{
    test_accumulation(); test_endpoints_and_stopped(); test_edges(); test_host_and_mmc(); test_quantization_and_ownership();
    printf("smc_pattern_navigation: PASS (%u production-linked checks)\n",checks); return 0;
}
