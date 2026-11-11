/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static void activity_frame(const char *prefix,int i){run_ms(80);ui.msg_until=0;ui_frame();char s[80];snprintf(s,sizeof s,"%s-%03d-active%d-phase%02d",prefix,i,engine_tape_active(),tape_glyph_phase);shot(s);printf("%s t=%u playing=%d active=%d phase=%d\n",s,now_ms,seq.playing,engine_tape_active(),tape_glyph_phase);}
int main(void){
 setup(V_FX,PART_909);audio_due_ms=now_ms;any_button=1;*page_of(V_FX)=1;build_pages();pref_set(PR(R_TAPE,0,0,0),64);pref_set(PR(R_ENG,T_FX,0,FX_DL_FDBK),48);pref_set(PR(R_ENG,T_FX,0,FX_DL_TIME),4);pref_set(PR(R_ENG,T_FX,0,FX_DL_LEVEL),100);pref_set(PR(R_ENG,T_MIX,PART_909,MX_DLY),100);run_ms(100);ui_frame();CHECK(!seq.playing);CHECK(!engine_tape_active());
 uint8_t phase=tape_glyph_phase;for(int i=0;i<8;i++){activity_frame("idle",i);CHECK(tape_glyph_phase==phase);}
 engine_drum(0,0,1.0f);run_ms(20);CHECK(engine_tape_active());CHECK(!seq.playing);
 for(int i=0;i<12;i++){if(i%4==0)engine_drum(0,0,1.0f);activity_frame("live-stopped-transport",i);CHECK(engine_tape_active());}
 pref_set(PR(R_ENG,T_MIX,PART_909,MX_DLY),0);run_ms(100);CHECK(engine_tape_active());int tail=0,tail_active=0;while(tail<70){activity_frame("tail",tail++);tail_active+=engine_tape_active();}CHECK(tail_active>2);CHECK(!engine_tape_active());phase=tape_glyph_phase;
 for(int i=0;i<8;i++){activity_frame("settled",i);CHECK(!engine_tape_active());CHECK(tape_glyph_phase==phase);}
 /* Running transport with all sends silent must not restart the reels. */
 for(int p=0;p<NPARTS;p++)pref_set(PR(R_ENG,T_MIX,p,MX_DLY),0);seq_start(&seq);
 for(int i=0;i<8;i++){activity_frame("running-silent",i);CHECK(!engine_tape_active());CHECK(tape_glyph_phase==phase);}seq_stop(&seq);
 printf("TAPE actual-path mock PASS %u checks; tailframes=%d\n",checks,tail);return 0;
}
