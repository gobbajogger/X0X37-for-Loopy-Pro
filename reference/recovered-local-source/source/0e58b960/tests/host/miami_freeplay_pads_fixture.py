# SPDX-License-Identifier: GPL-3.0-only
# Whole-app host fixture with exact extracted SMK digital pad dispatch.
# ADC, IRQ scheduling and physical LED wiring remain hardware substitutes.
from pathlib import Path
import os
r=Path(os.environ['X0X_PAD_TEST_OUT']);src=Path(__file__).resolve().parents[2]
s=(src/'tests/host/rec_live_test.c').read_text().split('int main(void)')[0].replace('#include "../../host/x0x_host.c"',f'#include "{src}/host/x0x_host.c"')
h=(src/'firmware/hal/smk_input.h').read_text(); plat=(src/'firmware/src/app/plat_fm1.c').read_text();main=(src/'firmware/src/app/main_fm1.c').read_text()
def fn(t,start):
 a=t.index(start);return t[a:t.index('\n}',a)+2]
s=s.replace('static uint8_t test_clip_mode, test_clip_busy;', 'static uint8_t test_clip_mode, test_clip_busy, test_padcal, test_smcui;').replace('uint8_t smk_padcal_on(void) { return 0; }', 'uint8_t smk_padcal_on(void) { return test_padcal; }').replace('int smc_ui_active(void) { return 0; }', 'int smc_ui_active(void) { return test_smcui; }')
s+='''
#include "app/boot_input.h"
static boot_input_t smk_boot_input; /* boot quarantine inactive in normal pad regression */
#define SMK_NPAD 16
#define SMK_KEY_F3 5
static struct { uint32_t notes,knotes,pnotes,notes_pressed; uint8_t pad_vel[16],key_vel[37]; } fm1_in;
static uint8_t clip_grid_mode;
'''+h[h.index('static const uint8_t SMK_WHITE_NOTE'):h.index('\n#define SMK_TRIM_FLASH')]+fn(h,'static void smk__padq(')+ '\n'+fn(main,'static void smk_grid(')+'''
static unsigned irq_depth;
static uint32_t irq_save(void){return irq_depth++;}
static void irq_restore(uint32_t flags){irq_depth=flags;}
'''+fn(plat,'int plat_division_pad(')+'''
static unsigned checks,cases;
#define ASSERT(c) do{checks++;CHECK(c,"%s case=%u",#c,cases);}while(0)
static void sync_keys(void){held_keys=fm1_in.notes;for(int i=0;i<NKEYS;i++)key_velocity[i]=fm1_in.key_vel[SMK_KEY_F3+i];}
static void hit(unsigned p,unsigned v){smk__padq(p,v);sync_keys();tick(15);}
static void keybed(unsigned key,unsigned v){if(v){fm1_in.knotes|=1u<<key;fm1_in.key_vel[SMK_KEY_F3+key]=v;}else fm1_in.knotes&=~(1u<<key);fm1_in.notes=fm1_in.knotes|fm1_in.pnotes;sync_keys();tick(15);}
static void fresh(int part,int view){
 seq_stop(&seq);tick(20);held_btn=0;ui.btn=0;ui.rec=0;ui.help=0;ui.overlay=O_NONE;test_clip_mode=test_padcal=test_smcui=0;
 ui.part=PART_303A+part;ui.freeplay[0]=ui.freeplay[1]=0;set_view(view);build_pages();tick(20);
 ASSERT(!engine_bass_select_preset(1-part,FACTORY_BANK_CHICAGO,0,&proj.sound));tick(800);
 ASSERT(!engine_bass_select_preset(part,FACTORY_BANK_MIAMI,0,&proj.sound));tick(50);
 ASSERT(engine_bass_bank(part)==FACTORY_BANK_MIAMI);pref_set(PR(R_FREEPLAY,part,0,0),1);tick(20);smk_grid();
 ASSERT(division_pad_owner()==part+1);ASSERT(smk_div_owner==part+1);
}
static void rate_matrix(void){
 for(int part=0;part<2;part++)for(int v=0;v<2;v++)for(int play=0;play<2;play++){
  fresh(part,v?V_PART:V_SEQ);if(play){seq_start(&seq);tick(100);}
  unsigned char before[sizeof proj.pat];memcpy(before,proj.pat,sizeof before);
  for(int pad=0;pad<16;pad++){
   cases++;int old=proj.sound.v[T_303][part][W_WOBBLE_RATE];
   keybed(SMK_WHITE_NOTE[pad],43);int note=kbd_held_note[part];
   hit(pad,120);int val=proj.sound.v[T_303][part][W_WOBBLE_RATE];
   ASSERT(val==(pad<8?(pad*127+3)/7:old));ASSERT(kbd_held_note[part]==note);
   ASSERT(!fm1_in.pnotes);ASSERT(fm1_in.key_vel[SMK_KEY_F3+SMK_WHITE_NOTE[pad]]==43);
   ASSERT(!memcmp(before,proj.pat,sizeof before));ASSERT(!irq_depth);
   if(play)ASSERT(seq.playing);
   hit(pad,0);ASSERT(kbd_held_note[part]==note);
   keybed(SMK_WHITE_NOTE[pad],0);ASSERT(!kbd_held_note[part]);ASSERT(!smk_div_held);
   leds();if(proj.set.keyled){ASSERT(lit_keys==(1u<<WHITE_KEY[division_pad_index()]));ASSERT(!(dim_keys & ~0x1fffu));}
  }
 }
}
static void ownership(void){
 for(int mode=0;mode<11;mode++){
  cases++;fresh(0,V_SEQ);int original=proj.sound.v[T_303][0][W_WOBBLE_RATE];
  smk__padq(7,100);sync_keys(); /* pending event, exit before UI consumes */
  if(mode==0)pref_set(PR(R_FREEPLAY,0,0,0),0);
  if(mode==1)ui.rec=1;
  if(mode==2)ui.part=PART_303B;
  if(mode==3)test_clip_mode=1;
  if(mode==4)ui.overlay=O_LIST;
  if(mode==5){ui.help=B_EDIT+1;held_btn=ui.btn=1u<<B_EDIT;ui.down_t[B_EDIT]=plat_ms();ui.turn_t=0;}
  if(mode==6){held_btn=1u<<B_HOME;ui.btn=held_btn;}
  if(mode==7)test_padcal=1;
  if(mode==8)test_smcui=1;
  if(mode==9)project_busy=1;
  if(mode==10){held_btn=ui.btn=1u<<B_SAVE;}
  tick(20);ASSERT(!smk_div_owner);ASSERT(proj.sound.v[T_303][0][W_WOBBLE_RATE]==original);
  hit(7,0);ASSERT(!kbd_held_note[0]);ASSERT(!fm1_in.pnotes);ASSERT(!smk_div_held);
  ui.help=0;ui.overlay=O_NONE;ui.rec=0;ui.part=PART_303A;test_clip_mode=test_padcal=test_smcui=0;project_busy=0;held_btn=ui.btn=0;
  pref_set(PR(R_FREEPLAY,0,0,0),1);tick(20);ASSERT(proj.sound.v[T_303][0][W_WOBBLE_RATE]==original);
  hit(7,100);ASSERT(proj.sound.v[T_303][0][W_WOBBLE_RATE]==127);hit(7,0);
 }
 /* A note-owning pad held at entry must release its original note. */
 fresh(0,V_PART);pref_set(PR(R_FREEPLAY,0,0,0),0);tick(20);
 hit(0,80);ASSERT(kbd_held_note[0]);ASSERT(fm1_in.pnotes);
 pref_set(PR(R_FREEPLAY,0,0,0),1);tick(20);int old=proj.sound.v[T_303][0][W_WOBBLE_RATE];
 hit(0,0);ASSERT(!kbd_held_note[0]);ASSERT(!fm1_in.pnotes);ASSERT(proj.sound.v[T_303][0][W_WOBBLE_RATE]==old);
 /* Two sub-frame hits: most recent division wins; repeated held press is inert. */
 smk__padq(1,90);smk__padq(1,0);smk__padq(6,90);smk__padq(6,0);tick(20);
 ASSERT(proj.sound.v[T_303][0][W_WOBBLE_RATE]==109);
 hit(2,90);int value=proj.sound.v[T_303][0][W_WOBBLE_RATE];hit(2,120);ASSERT(proj.sound.v[T_303][0][W_WOBBLE_RATE]==value);hit(2,0);
}
static void coexistence(void){
 fresh(0,V_SEQ);bpart_t *bp=&cur_pat()->bass[0];bp->len=16;
 for(int i=0;i<16;i++)bp->step[i]=(bstep_t){48,G_NOTE,100};
 unsigned char saved[sizeof proj.pat];memcpy(saved,proj.pat,sizeof saved);
 seq_start(&seq);tick(20);keybed(10,75);unsigned oldstep=eng_step[TRK_BASS0];
 for(int pad=0;pad<8;pad++){cases++;hit(pad,100);hit(pad,0);ASSERT(seq.playing);ASSERT(kbd_held_key[0]==11);ASSERT(!memcmp(saved,proj.pat,sizeof saved));}
 ASSERT(eng_step[TRK_BASS0]!=oldstep);keybed(10,0);ASSERT(!kbd_held_key[0]);
 hit(0,100);seq_stop(&seq);tick(20);ASSERT(!seq.playing);hit(0,0);ASSERT(!kbd_held_key[0]);ASSERT(!fm1_in.pnotes);
 seq_start(&seq);tick(20);ASSERT(seq.playing);seq_stop(&seq);tick(20);
 /* Ineligible Chicago retains ordinary note pads; pending Miami hit cannot cross bank. */
 fresh(0,V_PART);smk__padq(7,100);ASSERT(!engine_bass_select_preset(0,FACTORY_BANK_CHICAGO,0,&proj.sound));
 {int32_t audio[512];engine_render(audio,256);}tick(20);ASSERT(!division_pad_owner());ASSERT(!smk_div_owner);hit(7,0);
 hit(0,88);ASSERT(fm1_in.pnotes);ASSERT(kbd_held_key[0]==SMK_WHITE_NOTE[0]+1);hit(0,0);ASSERT(!kbd_held_key[0]);
}
static void pictures(void){
 any_button=1;fresh(0,V_SEQ);hit(7,100);hit(7,0);ui.msg_until=0;ui_frame();
 const char *a,*b,*c;footer_text(&a,&b,&c);ASSERT(tw(&FONT_T,a)+FOOT_GAP+tw(&FONT_T,b)<=232);
 char path[1024];snprintf(path,sizeof path,"%s/seq-rate-24.png",getenv("X0X_PAD_TEST_OUT"));png_write(path);uint32_t sig=knobs_sig();hit(0,100);hit(0,0);ui.msg_until=0;ASSERT(knobs_sig()!=sig);ui_frame();snprintf(path,sizeof path,"%s/seq-rate-2.png",getenv("X0X_PAD_TEST_OUT"));png_write(path);
 set_view(V_PART);build_pages();hit(7,100);hit(7,0);ui.msg_until=0;ui_frame();
 ASSERT(7+tw(&FONT_B,"MIAMI A")+8+tw(&FONT_T,"PADS 1/24")<175);snprintf(path,sizeof path,"%s/edit-rate-24.png",getenv("X0X_PAD_TEST_OUT"));png_write(path);
}
int main(void){boot();tick(300);engine_guard_enable(0);rate_matrix();ownership();coexistence();pictures();printf("PAD DIVISION %u cases %u assertions %d failures\\n",cases,checks,fails);return fails!=0;}
'''
(r/'probe.c').write_text(s)
