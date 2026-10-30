/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN unused_smc_ui_main
#include "smc_ui_test.c"
static void tick(void){int32_t a[512];engine_render(a,256);audio_due_ms=now_ms;ui_frame();}
static void ready(void){setup(V_FX,PART_909);audio_due_ms=now_ms;any_button=1;*page_of(V_FX)=1;build_pages();pref_set(PR(R_TAPE,0,0,0),64);pref_set(PR(R_ENG,T_FX,0,FX_DL_FDBK),80);pref_set(PR(R_ENG,T_FX,0,FX_DL_LEVEL),100);pref_set(PR(R_ENG,T_MIX,PART_909,MX_DLY),100);tick();engine_drum(0,0,1.0f);run_ms(30);ui_frame();CHECK(engine_tape_active());CHECK(!seq.playing);}
int main(void){
 ready();uint8_t phase=tape_glyph_phase;uint32_t at=tape_glyph_at;now_ms=at+79;tick();CHECK(tape_glyph_phase==phase);now_ms=at+80;tick();CHECK(tape_glyph_phase==(phase+1)%16);phase=tape_glyph_phase;
 engine_fx_mute(2);tick();CHECK(!engine_tape_active());now_ms+=5000;ui_frame();CHECK(tape_glyph_phase==phase);engine_fx_mute(0);tick();CHECK(engine_tape_active());CHECK(tape_glyph_phase==phase);now_ms+=79;tick();CHECK(tape_glyph_phase==phase);now_ms++;tick();CHECK(tape_glyph_phase==(phase+1)%16);
 pref_set(PR(R_ENG,T_FX,0,FX_DL_LEVEL),0);tick();CHECK(!engine_tape_active());phase=tape_glyph_phase;now_ms+=500;ui_frame();CHECK(tape_glyph_phase==phase);pref_set(PR(R_ENG,T_FX,0,FX_DL_LEVEL),100);tick();CHECK(engine_tape_active());CHECK(tape_glyph_phase==phase);
 pref_set(PR(R_TAPE,0,0,0),15);tick();CHECK(!engine_tape_active());phase=tape_glyph_phase;now_ms+=1000;ui_frame();CHECK(tape_glyph_phase==phase);pref_set(PR(R_TAPE,0,0,0),64);tick();CHECK(engine_tape_active());CHECK(tape_glyph_phase==phase);
 engine_project_hold(1);tick();CHECK(!engine_tape_active());phase=tape_glyph_phase;now_ms+=1000;ui_frame();CHECK(tape_glyph_phase==phase);engine_project_release();tick();CHECK(engine_tape_active());CHECK(tape_glyph_phase==phase);
 setup(V_FX,PART_909);CHECK(!engine_tape_active());
 ready();uint16_t first[240*KNOB_H];memcpy(first,fb+240*KNOB_Y,sizeof first);uint8_t first_phase=tape_glyph_phase;uint32_t first_sig=knobs_sig();
 for(int i=1;i<=16;i++){now_ms=tape_glyph_at+80;engine_drum(0,0,1.0f);tick();CHECK(engine_tape_active());CHECK(tape_glyph_phase==(first_phase+i)%16);}
 CHECK(knobs_sig()==first_sig);CHECK(!memcmp(first,fb+240*KNOB_Y,sizeof first));
 pref_set(PR(R_TAPE,0,0,0),15);tick();uint32_t sig=knobs_sig();memcpy(first,fb+240*KNOB_Y,sizeof first);
 for(int i=0;i<16;i++){now_ms+=80;tick();CHECK(knobs_sig()==sig);CHECK(!memcmp(first,fb+240*KNOB_Y,sizeof first));}
 ready();*page_of(V_FX)=0;build_pages();ui_frame();sig=knobs_sig();memcpy(first,fb+240*KNOB_Y,sizeof first);
 for(int i=0;i<16;i++){now_ms+=80;engine_drum(0,0,1.0f);tick();CHECK(knobs_sig()==sig);CHECK(!memcmp(first,fb+240*KNOB_Y,sizeof first));}
 for(int phase=0;phase<16;phase++)for(int v=16;v<=127;v++){
  tape_glyph_phase=phase;cv_begin(240,KNOB_H,C_BLACK);CHECK(control_glyph(PR(R_TAPE,0,0,0),v,30,C_HI));
  for(int y=0;y<KNOB_H;y++)for(int x=0;x<240;x++)if(x<12||x>48||y<30||y>49)CHECK(cv_px[y*240+x]==swap16(C_BLACK));
 }
 printf("TAPE activity gates, init/hold, 79/80ms timing, freeze/resume without idle catchup PASS %u checks\n",checks);return 0;
}
