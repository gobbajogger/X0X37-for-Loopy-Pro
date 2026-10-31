/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual host framebuffer capture only; no device I/O. */
#define SMC_UI_MAIN ignored_main
#include "../../../../tests/host/smc_ui_test.c"
#undef SMC_UI_MAIN
static void prepare(void) {
 setup(V_SEQ,PART_909);seq.ppat[PART_909]=3;seq.pcue[PART_909]=0xff;
 ui.part=PART_303B;set_view(V_FX);*page_of(V_FX)=1;build_pages();
 smc_ui_cancel();ui.msg_until=0;
}
int main(void) {
 prepare();ui_frame();shot("keyboard-underneath");
 message(0x90,0x5c,127);ui_frame();shot("pattern-from-keyboard");
 prepare();fader(0,0);ui_frame();shot("existing-popup");
 message(0x90,0x5c,127);ui_frame();shot("pattern-over-popup");
 message(0x80,0x5c,0);now_ms+=1301;ui_frame();shot("notice-dismissed");
 return 0;
}
