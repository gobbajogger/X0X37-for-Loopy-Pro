/* SPDX-License-Identifier: GPL-3.0-only */
/* Compare the actual composed raster against the retained 8be renderer. */
#define SMC_UI_MAIN retained_fixture_main
#include "smc_ui_test.c"
static uint16_t reference_pixels[240 * MAIN_ROWS];
static void reference_header_commit(void) { memcpy(reference_pixels,cv_px,240*HDR_H*2); }
#define cv_commit_header reference_header_commit
#include "reference/ui_perf_8be_reference.inc"
#undef cv_commit_header
static void same_header(void)
{
    ui_t prior=ui;
    draw_header_reference();ui=prior;
    draw_header();
    CHECK(!memcmp(reference_pixels,fb,240*HDR_H*2));
}
int main(void)
{
    for(int theme=0;theme<6;theme++)for(int part=0;part<NPARTS;part++) {
        setup(V_PART,part);palette_set(theme);
        for(int state=0;state<32;state++) {
            ui.rec=state&1;seq.playing=(state>>1)&1;seq.metro_view=(uint8_t)(state&3);
            ui.dirty=!!(state&4);ui.save_failed=!!(state&8);
            *page_of(V_PART)=(uint8_t)(state%pg.n);build_pages();
            now_ms+=100;ui.msg_until=ui.help=0;ui.overlay=O_NONE;seq.pcue[part]=255;
            same_header();same_header();
            ui.rec^=1;same_header();seq.metro_view^=1;same_header();
            ui.save_failed^=1;same_header();palette_set((theme+1)%6);same_header();
            palette_set(theme);same_header();ui.help=1;same_header();
            ui.help=0;ui.overlay=O_ASK;same_header();ui.overlay=O_NONE;
            say("RESET SOUND",0);same_header();now_ms=ui.msg_until;same_header();
            cv_begin(240,HDR_H,C_BLACK);cv_rect(0,0,240,HDR_H,C_AMB);cv_commit_header();same_header();
        }
    }
    for(int radius=7;radius<=9;radius++)for(int theme=0;theme<6;theme++)
    for(int bipolar=0;bipolar<2;bipolar++)for(int value=-1;value<=128;value++) {
        palette_set(theme);int x=value%3==0?0:value%3==1?120:239;
        cv_begin(240,69,C_LINE);arc_reference(x,35,radius,value/127.0f,C_GRAY,C_HI,bipolar);
        memcpy(reference_pixels,cv_px,240*69*2);
        cv_begin(240,69,C_LINE);arc(x,35,radius,value/127.0f,C_GRAY,C_HI,bipolar);
        CHECK(!memcmp(reference_pixels,cv_px,240*69*2));
    }
    setup(V_MIX,PART_909);
    for(int state=0;state<240;state++) {
        palette_set(state%6);ui.part=(uint8_t)(state%NPARTS);eng_scope_w=(uint32_t)(state*7)&(SCOPE_N-1u);
        for(unsigned i=0;i<SCOPE_N;i++)eng_scope[i]=state%3==0?0:state%3==1?(int16_t)(i*997+state):32000;
        now_ms+=state%2?100:99;
        cv_begin(240,MAIN_ROWS,C_BLACK);cv_band(0,BAND_H);draw_master_wave_reference();
        memcpy(reference_pixels,cv_px,sizeof reference_pixels);
        cv_begin(240,MAIN_ROWS,C_BLACK);cv_band(0,BAND_H);draw_master_wave();
        CHECK(!memcmp(reference_pixels,cv_px,sizeof reference_pixels));
    }
    printf("Composed UI identity: %u checks PASS\n",checks);return 0;
}
