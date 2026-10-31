/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual host framebuffer capture only; no device I/O. */
#define SMC_UI_MAIN ignored_main
#include "../../../../tests/host/smc_ui_test.c"
#undef SMC_UI_MAIN
static void prepare(void) {
 setup(V_SEQ,PART_909);seq.ppat[PART_909]=3;seq.pcue[PART_909]=0xff;
 /* Populate the real cued pattern so its HOME glyph is visible in the dump. */
 for(int step=0;step<16;step+=4) sm_set(proj.pat[4].drum[0].hit[0],step);
 sm_set(proj.pat[4].drum[1].hit[0],2);
 proj.pat[4].bass[0].step[3].flags=G_NOTE;
 proj.pat[4].bass[1].step[7].flags=G_NOTE;
 proj.pat[4].brk.steps=1u<<10;
 ui.part=PART_303B;set_view(V_FX);*page_of(V_FX)=1;build_pages();
 smc_ui_cancel();ui.msg_until=0;
}
int main(void) {
 prepare();ui_frame();shot("keyboard-underneath");
 message(0x90,0x5c,127);ui_frame();shot("pattern-from-keyboard");
 prepare();fader(0,0);ui_frame();shot("existing-popup");
 message(0x90,0x5c,127);ui_frame();shot("pattern-over-popup");
 message(0x80,0x5c,0);now_ms+=1001;ui_frame();shot("notice-dismissed");
 return 0;
}
