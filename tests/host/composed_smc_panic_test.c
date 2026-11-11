/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN inherited_smc_fixture
#include "smc_ui_test.c"
static unsigned pc, pf;
#define PTEST(x) do {pc++;if(!(x)){pf++;fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);}}while(0)
static void audio(void) { static int32_t out[512];engine_render(out,256); }
static void begin(void) {setup(V_HOME,PART_303A);run_ms(80);smc.part=PART_303A;smc.view=V_HOME;smc_panic_reset();}
static void start_note(void){engine_bass_on(0,60,0,0);audio();audio();PTEST(engine_bass_keys_held()&1);}
static void chord(int first,int second)
{
 begin();start_note();unsigned part=smc.part;
 message(0x90,first,127);PTEST(smc.part==part);
 message(0x90,first,127);PTEST(smc.part==part);
 message(0x90,second,127);audio();PTEST(!(engine_bass_keys_held()&1));PTEST(smc.part==part);
 message(0x80,first,0);start_note();message(0x90,second,127);audio();PTEST(engine_bass_keys_held()&1);
 message(0x80,second,0);audio();PTEST(engine_bass_keys_held()&1);PTEST(smc.part==part);
 message(0x90,second,127);message(0x90,first,127);audio();PTEST(!(engine_bass_keys_held()&1));
 message(0x80,first,0);message(0x80,second,0);
}
static void singles(void)
{
 begin();unsigned part=smc.part;int previous=part_neighbor(part,-1);
 message(0x90,0x2e,127);PTEST(smc.part==part);message(0x90,0x2e,127);PTEST(smc.part==part);
 message(0x80,0x2e,0);PTEST(smc.part==previous);message(0x80,0x2e,0);PTEST(smc.part==previous);
}
static void blocked_release(void)
{
 begin();unsigned part=smc.part;message(0x90,0x2e,127);
 uint8_t up[3]={0x80,0x2e,0};PTEST(smc_message(up,3,0)==1);PTEST(smc.part==part);
 message(0x90,0x2f,127);audio();PTEST(!smc_panic.active);message(0x80,0x2f,0);
 begin();message(0x90,0x2e,127);smc_project_reset();PTEST(smc_panic_held==0);PTEST(!smc_panic.active);
}
static void question(void)
{
 begin();message(0x90,6,127);PTEST(smc.home_down);ui.overlay=O_ASK;ui.ask_overlay=O_NONE;smc_poll();PTEST(smc.home_down);
 message(0x90,0x5e,127);audio();PTEST(ui.overlay==O_ASK);PTEST(!seq.playing);
 message(0x80,0x5e,0);message(0x80,6,0);
 begin();ui.overlay=O_ASK;ui.ask_overlay=O_NONE;message(0x90,0x5e,127);audio();PTEST(ui.overlay!=O_ASK);PTEST(seq.playing);
 message(0x90,0x5e,127);audio();PTEST(seq.playing);message(0x80,0x5e,0);
}
int main(void){chord(0x2e,0x2f);chord(0x2f,0x2e);singles();blocked_release();question();printf("SMC real-path PANIC/question: %u checks, %u failures\n",pc,pf);return pf?1:0;}
