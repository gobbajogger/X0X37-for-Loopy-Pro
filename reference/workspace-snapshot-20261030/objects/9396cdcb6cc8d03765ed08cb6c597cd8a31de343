/* SPDX-License-Identifier: GPL-3.0-only */
/* Same actual audio caller and UI frame path on baseline/candidate. */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static unsigned nonzero;
static uint32_t audio_hash=2166136261u;
static void render(unsigned n) {
    int32_t out[512];
    while(n) {unsigned k=n>256?256:n;engine_render(out,k);
        for(unsigned j=0;j<k*2;j++) {audio_hash=(audio_hash^(uint32_t)out[j])*16777619u;nonzero+=out[j]!=0;}
        n-=k;
    }
}
static uint32_t glyph_hash(void) {
    uint32_t h=2166136261u;
    for(unsigned y=KNOB_Y+27;y<KNOB_Y+49;y++)for(unsigned x=17;x<44;x++)h=(h^fb[y*240+x])*16777619u;
    return h;
}
int main(void) {
    setup(V_HOME,PART_303A);
    for(int i=0;i<16;i++) {sm_set(proj.pat[0].drum[0].hit[0],i);
        proj.pat[0].bass[0].step[i].note=36+i;proj.pat[0].bass[0].step[i].flags=G_NOTE;}
    seq.bpm=120;seq.send_clock=0;seq_start(&seq);render(1);
    for(unsigned i=0;i<8;i++) {ui_frame();printf("GLYPH %u %08x\n",i,glyph_hash());render((i+1)*FS/16-i*FS/16);}
    seq.bpm=173;render(FS);seq_stop(&seq);render(256);seq_continue(&seq);render(FS);
    seq.send_clock=1;render(FS);seq_midi_rt(&seq,0xFA);
    for(unsigned i=0;i<96;i++){seq_midi_rt(&seq,0xF8);render(FS/48);}
    seq_midi_rt(&seq,0xFC);render(FS);
    CHECK(nonzero>1000);printf("AUDIO %08x nonzero=%u\n",audio_hash,nonzero);return 0;
}
