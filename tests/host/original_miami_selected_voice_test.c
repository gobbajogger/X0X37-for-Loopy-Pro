/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_selected_voice.h"
#include "motion_tables_private.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static unsigned bits(float f){unsigned u;memcpy(&u,&f,4);return u;}
static float clamp(float x){return x< -1?-1:x>1?1:x;}
static original_miami_voice_tables tables;
static original_miami_voice_table_storage table_storage;
static original_miami_voice_note note(unsigned key){
 original_miami_voice_note n;memset(&n,0,sizeof n);n.note_key=key;n.tuned_pitch=(float)key;n.velocity=100;n.last_note=60;n.unison_coordinate=.5f;n.unison_flag=2;n.mg[0].phase=n.mg[1].phase=-1;n.mg[0].noise=5;n.mg[1].noise=7;return n;
}
static void table_and_compile(void){
 original_ms20_tables t={original_miami_a44100,original_miami_b44100,original_miami_r,original_miami_cap};
 CHECK(original_miami_voice_tables_init(&tables,&table_storage,&t,original_pitch44100));
 CHECK(!memcmp(tables.filter.a,t.a,sizeof table_storage.a));CHECK(!memcmp(tables.pitch,original_pitch44100,sizeof table_storage.pitch));
 original_miami_voice_tables saved=tables;float bad[129];memcpy(bad,t.a,sizeof bad);bad[128]=NAN;t.a=bad;
 CHECK(!original_miami_voice_tables_init(&tables,&table_storage,&t,original_pitch44100));CHECK(!memcmp(&tables,&saved,sizeof tables));
 memcpy(bad,original_miami_a44100,sizeof bad);CHECK(original_miami_voice_tables_init(&tables,&table_storage,&t,original_pitch44100));bad[64]=NAN;CHECK(isfinite(tables.filter.a[64]));CHECK(!memcmp(&tables,&saved,sizeof tables));

#ifdef MIAMI_VOICE_BORROW_TABLES
 t.a=original_miami_a44100;CHECK(original_miami_voice_tables_bind_immutable(&tables,&t,original_pitch44100));
 CHECK(tables.filter.a==original_miami_a44100&&tables.pitch==original_pitch44100);
#endif
 original_miami_program p;original_miami_voice_controls c;
 for(unsigned factory=0;factory<30;factory++){
  CHECK(original_miami_program_init(&p,factory));CHECK(original_miami_voice_compile(&c,&p));
  CHECK(c.pitch.modulation_on&&c.gain.curve==0&&c.gain.ms20&&c.vco_gain==1);
  CHECK(c.pitch.bend_range==2&&c.env_depth==0&&c.pitch.semitone[1]== -12&&c.pitch.semitone[3]==0);
  CHECK(bits(c.pitch.fine_tune[2])==bits(fmaf(.5015000104904175f,2,-1)));
  CHECK(c.gain.output==1&&c.gain.pan==0&&c.mg_base[1]==4&&c.mg_times[1]==4);
  CHECK(c.shape[3]==3&&c.gain.level[3]==0&&c.noise==0&&!c.damper);
  float master,crushA,crushB;CHECK(original_miami_program_native(&p,96,&master));CHECK(original_miami_program_native(&p,110,&crushA));CHECK(original_miami_program_native(&p,111,&crushB));
  CHECK(c.master_volume==master*master&&c.decimator_normalized[0]==crushA&&c.decimator_normalized[1]==crushB);
  for(unsigned i=0;i<4;i++)CHECK(c.shape[i]<=3);
 }
 original_miami_voice_controls previous=c;p.target[0]=NAN;CHECK(!original_miami_voice_compile(&c,&p));CHECK(!memcmp(&c,&previous,sizeof c));
 CHECK(original_miami_program_init(&p,16));CHECK(original_miami_voice_compile(&c,&p));
 for(unsigned macro=0;macro<18;macro++){
  original_miami_program low=p,high=p;original_miami_voice_controls a,b;
  CHECK(original_miami_program_set(&low,macro,0));CHECK(original_miami_program_set(&high,macro,1));CHECK(original_miami_voice_compile(&a,&low));CHECK(original_miami_voice_compile(&b,&high));
  /* Each macro changes an actual typed endpoint, including the explicitly
   * outer-owned master/Decimator and event-owned damper endpoints. */
  switch(macro){
   case 0:CHECK(a.shape[0]!=b.shape[0]&&a.shape[2]!=b.shape[2]);break;
   case 1:CHECK(a.gain.level[0]!=b.gain.level[0]);break;
   case 2:CHECK(a.gain.level[2]!=b.gain.level[2]);break;
   case 3:CHECK(a.pitch.semitone[0]!=b.pitch.semitone[0]&&a.pitch.semitone[2]!=b.pitch.semitone[2]);break;
   case 4:CHECK(a.pitch.portamento!=b.pitch.portamento);break;
   case 5:CHECK(a.gain.level[1]!=b.gain.level[1]);break;
   case 6:CHECK(a.shape[1]!=b.shape[1]);break;
   case 7:CHECK(a.patch_slave!=b.patch_slave);break;
   case 8:CHECK(a.xmod!=b.xmod);break;
   case 9:CHECK(a.cutoff!=b.cutoff);break;
   case 10:CHECK(a.resonance!=b.resonance&&a.decimator_normalized[0]!=b.decimator_normalized[0]&&a.decimator_normalized[1]!=b.decimator_normalized[1]);break;
   case 11:CHECK(a.mg_shape[0]!=b.mg_shape[0]&&a.mg_shape[1]!=b.mg_shape[1]);break;
   case 12:CHECK(a.patch_cutoff!=b.patch_cutoff);break;
   case 13:CHECK(a.mg_base[0]!=b.mg_base[0]&&a.mg_times[0]!=b.mg_times[0]);break;
   case 14:CHECK(a.master_volume!=b.master_volume);break;
   case 15:CHECK(a.gain.spread!=b.gain.spread);break;
   case 16:CHECK(a.bend_normalized==0&&b.bend_normalized==1);break;
   case 17:CHECK(!a.damper&&b.damper);break;
  }
 }
}
static void sample_reference(original_miami_selected_voice *s,const original_miami_voice_controls *c,float bpm){
 /* Full renderer sample ordering witness assembled from frozen independent
  * checked dependencies. The control tick itself is tested separately. */
 original_miami_selected_voice expected=*s;unsigned action;float a,b;
 CHECK(original_miami_selected_voice_control(&expected,c,&tables,bpm,&action));
 if(expected.active){float mix,filtered;CHECK(original_voice_gain_oscillator(&expected.gain,&expected.pitch.oscillator,&expected.prepared,0,&mix));CHECK(original_miami_ms20_sample(&expected.filter,mix,c->vco_gain,&filtered));CHECK(original_voice_gain_stereo(&expected.gain,filtered,&a,&b));}else a=b=0;
 float L=99,R=99;unsigned got=99;CHECK(original_miami_selected_voice_sample(s,c,&tables,bpm,&L,&R,&got));CHECK(bits(L)==bits(a)&&bits(R)==bits(b)&&got==action&&!memcmp(s,&expected,sizeof *s));
}
static void lifecycle(void){
 original_miami_program p;original_miami_voice_controls c;original_miami_selected_voice s;
 CHECK(original_miami_program_init(&p,16));CHECK(original_miami_voice_compile(&c,&p));CHECK(original_miami_selected_voice_init(&s,&tables,0,60));
 for(unsigned i=0;i<2;i++)CHECK(s.mg[i].phase==-1&&s.mg[i].noise==0x6d70u&&s.mg[i].wrapped==1&&s.mg[i].unipolar==0&&s.mg[i].bipolar==0&&s.mg[i].hold==0);
 original_miami_voice_note n=note(60);CHECK(original_miami_selected_voice_full_note(&s,&c,&tables,&n,120));
 CHECK(s.counter==690&&s.last_duration==63&&s.control_updates==1&&s.vca.level==1&&s.vcf.level==1);
 CHECK(s.previous_vca==0&&s.mod_gain==0&&s.mod_slave==0);
 CHECK(s.previous_mg1==s.mg[0].bipolar&&s.mod_cutoff==clamp(s.previous_mg1*c.patch_cutoff+0.0f));
 CHECK(s.pitch.oscillator.phase[0]==0&&s.filter.noise==1&&s.gain.oscillator[0].current==0&&s.gain.stereo[0].current==0);
 original_miami_selected_voice saved=s;CHECK(!original_miami_selected_voice_full_note(&s,&c,&tables,&n,120));CHECK(!memcmp(&s,&saved,sizeof s));
 for(unsigned k=0;k<128;k++)sample_reference(&s,&c,120);
 /* Partial helper when not due: no audio phase/noise/gain history mutation,
  * but both MG outputs refresh from their advanced phase. */
 while(s.counter>=42720){unsigned action;CHECK(original_miami_selected_voice_control(&s,&c,&tables,120,&action));}
 saved=s;CHECK(original_miami_selected_voice_partial_note(&s,&c,&tables,72,90,72.25f,5,120));
 CHECK(s.counter==saved.counter+690&&s.control_updates==saved.control_updates);
 CHECK(s.pitch.target[0]==72.25f&&s.pitch.target[2]==72.25f&&s.pitch.target[1]==saved.pitch.target[1]&&s.pitch.target[3]==saved.pitch.target[3]);
 CHECK(!memcmp(&s.pitch.oscillator,&saved.pitch.oscillator,sizeof s.pitch.oscillator)&&!memcmp(&s.filter,&saved.filter,sizeof s.filter)&&!memcmp(&s.gain,&saved.gain,sizeof s.gain));
 CHECK(s.vca.level==saved.vca.level&&s.vca.state==saved.vca.state&&s.mg[0].phase==saved.mg[0].phase);
 float bi,uni;CHECK(original_miami_mg_shape(c.mg_shape[0],s.mg[0].phase,&bi,&uni));CHECK(bits(s.mg[0].bipolar)==bits(bi));
 for(unsigned k=0;k<512;k++)sample_reference(&s,&c,120);
 CHECK(original_miami_program_set(&p,17,1)&&original_miami_voice_compile(&c,&p));CHECK(original_miami_selected_voice_pedal(&s,&c));saved=s;
 CHECK(original_miami_selected_voice_note_off(&s,&c)&&s.damper_deferred&&!s.key_down&&s.vca.state==saved.vca.state&&s.vca.level==saved.vca.level);
 for(unsigned k=0;k<128;k++)sample_reference(&s,&c,120);
 CHECK(original_miami_program_set(&p,17,0)&&original_miami_voice_compile(&c,&p));saved=s;CHECK(original_miami_selected_voice_pedal(&s,&c));
 CHECK(s.vca.state==3&&s.vcf.state==3&&s.vca.level==saved.vca.level&&s.vcf.level==saved.vcf.level&&!s.damper_deferred);
 unsigned frames=0;while(s.active&&frames++<2048)sample_reference(&s,&c,120);
 CHECK(!s.active&&s.vca.state==6&&s.vcf.state==6&&s.vca.level==0&&s.vcf.level==0&&frames<1024);
 saved=s;for(unsigned k=0;k<32;k++)sample_reference(&s,&c,120);CHECK(!memcmp(&saved,&s,sizeof s));
 n=note(48);n.mg[0].phase=.25f;n.mg[1].phase= -.375f;CHECK(original_miami_selected_voice_full_note(&s,&c,&tables,&n,120));
 CHECK(!memcmp(&s.pitch.oscillator,&saved.pitch.oscillator,sizeof s.pitch.oscillator)&&s.filter.noise==saved.filter.noise&&s.filter.noise_gain.current==saved.filter.noise_gain.current);
 for(unsigned i=0;i<4;i++)CHECK(s.gain.oscillator[i].current==saved.gain.oscillator[i].current);
 for(unsigned i=0;i<5;i++)CHECK(s.filter.history[i]==0);
 CHECK(s.gain.stereo[0].current==0&&s.gain.stereo[1].current==0);
 /* A zero-depth/zero-level note still advances every phase, both MGs and
  * filter noise. No silent fast path is allowed to change those states. */
 CHECK(original_miami_program_set(&p,1,0)&&original_miami_program_set(&p,2,0)&&original_miami_program_set(&p,5,0)&&original_miami_program_set(&p,8,0)&&original_miami_program_set(&p,12,0));CHECK(original_miami_voice_compile(&c,&p));
 for(unsigned k=0;k<256;k++)sample_reference(&s,&c,120);
 saved=s;sample_reference(&s,&c,120);for(unsigned i=0;i<4;i++)CHECK(s.pitch.oscillator.phase[i]!=saved.pitch.oscillator.phase[i]);CHECK(s.filter.noise!=saved.filter.noise);
 original_miami_selected_voice invalid=s;float L=123,R=234;unsigned action=99;CHECK(!original_miami_selected_voice_sample(&s,&c,&tables,NAN,&L,&R,&action));CHECK(!memcmp(&s,&invalid,sizeof s)&&L==123&&R==234&&action==99);
}
static void vcf_release_witness(void){
 /* Separately derived binary32 R0/D64 witness: the VCF keeps exponential
  * release below .04, while VCA switches to halving and later retires both.
  * A mistaken shared VCA implementation fails at the fourth entry. */
 static const unsigned expected[]={0x3e8c7b10,0x3d9a2db0,0x3ca93640,0x3bb9b600,0x3acbd000,0x39dfb000,0x38f58000,0x38068000,0};
 original_miami_program p;original_miami_voice_controls c;original_miami_selected_voice s;original_miami_voice_note n=note(60);
 CHECK(original_miami_program_init(&p,16)&&original_miami_voice_compile(&c,&p)&&original_miami_selected_voice_init(&s,&tables,0,60));CHECK(original_miami_selected_voice_full_note(&s,&c,&tables,&n,120));CHECK(original_miami_selected_voice_note_off(&s,&c));
 for(unsigned i=0;i<9;i++){
  unsigned action=0,old=s.control_updates;
  for(unsigned k=0;s.control_updates==old&&k<65;k++)CHECK(original_miami_selected_voice_control(&s,&c,&tables,120,&action));
  CHECK(s.control_updates==old+1&&s.last_duration==64&&bits(s.vcf.level)==expected[i]);CHECK(s.vcf_bipolar==s.vcf.level-1.0f);
  if(i==3)CHECK(bits(s.vcf.level)!=bits(s.vca.level));
 }
 CHECK(!s.active&&s.vca.state==6&&s.vcf.state==6);
 CHECK(s.vcf_bipolar==-1&&s.vca_bipolar==-1&&s.control_updates==10);
 for(unsigned i=0;i<4;i++)CHECK(s.activation[i]==0&&s.gain.oscillator[i].target==0);
 CHECK(s.gain.stereo[0].target==0&&s.gain.stereo[1].target==0&&s.filter.prepared);
}
static void write_state(FILE *f,const original_miami_selected_voice *s){CHECK(fwrite(s,sizeof *s,1,f)==1);}
static void rendered_campaign(const char *path){
 FILE *f=fopen(path,"wb");CHECK(f);unsigned frames=0;
 for(unsigned factory=0;factory<30;factory++)for(unsigned side=0;side<2;side++){
  original_miami_program p;original_miami_voice_controls c;original_miami_selected_voice s;original_miami_voice_note n=note(36+factory);
  n.unison_flag=side?0:2;n.unison_coordinate=side?-.5f:.5f;n.mg[0].phase=side?-.998f:-1;
  CHECK(original_miami_program_init(&p,factory)&&original_miami_voice_compile(&c,&p)&&original_miami_selected_voice_init(&s,&tables,side,60));
  CHECK(original_miami_selected_voice_full_note(&s,&c,&tables,&n,137));write_state(f,&s);
  for(unsigned k=0;k<4096;k++){
   if(k==700)CHECK(original_miami_selected_voice_partial_note(&s,&c,&tables,72,110,72,15,137));
   if(k>=1024&&k<1024+18*64&&(k%64)==0){unsigned m=(k-1024)/64;CHECK(original_miami_program_set(&p,m,(m&1)?1:0)&&original_miami_voice_compile(&c,&p));CHECK(original_miami_selected_voice_pedal(&s,&c));}
   if(k==3000){CHECK(original_miami_program_set(&p,17,0)&&original_miami_voice_compile(&c,&p));CHECK(original_miami_selected_voice_pedal(&s,&c));CHECK(original_miami_selected_voice_note_off(&s,&c));}
   float L,R;unsigned action,old=s.control_updates;uint32_t counter=s.counter;float oldVCA=s.vca.level,oldMG=s.mg[0].bipolar;
   CHECK(original_miami_selected_voice_sample(&s,&c,&tables,137,&L,&R,&action));CHECK(isfinite(L)&&isfinite(R));
   if(s.control_updates!=old){CHECK(s.previous_vca==oldVCA&&s.previous_mg1==oldMG);CHECK(s.counter==counter+690-44100&&s.last_duration==(unsigned)ceilf((float)(44100-s.counter)/690.0f));write_state(f,&s);}
   CHECK(fwrite(&L,4,1,f)==1&&fwrite(&R,4,1,f)==1);frames++;
  }
  CHECK(!s.active);write_state(f,&s);
 }
 CHECK(fclose(f)==0);printf("Rendered %u stereo frames, all30 factories, both explicit unison sides, all18 macro edits, legato and release.\n",frames);
}
static void cost(unsigned count){
 original_miami_program p;original_miami_voice_controls c;original_miami_selected_voice s;original_miami_voice_note n=note(60);CHECK(original_miami_program_init(&p,16)&&original_miami_voice_compile(&c,&p)&&original_miami_selected_voice_init(&s,&tables,0,60));CHECK(original_miami_selected_voice_full_note(&s,&c,&tables,&n,120));
 double sum=0;clock_t start=clock();
 for(unsigned k=0;k<count;k++){float L,R;unsigned action;CHECK(original_miami_selected_voice_sample(&s,&c,&tables,120,&L,&R,&action));sum+=(double)L+(double)R;}
 double seconds=(double)(clock()-start)/CLOCKS_PER_SEC;
 printf("Host only: %u frames, %.6f CPU seconds, checksum %.17g, control updates %u, waveform preparations %u.\n",count,seconds,sum,s.control_updates,s.wave_preparations);
 printf("Bytes: voice=%zu controls=%zu table_descriptor=%zu program=%zu copy_storage=%zu. No heap. Host timing is not device CPU, XRUN or latency evidence.\n",sizeof s,sizeof c,sizeof tables,sizeof p,sizeof table_storage);
}
int main(int argc,char **argv){CHECK(argc==2||argc==3);table_and_compile();if(argc==3){CHECK(!strcmp(argv[1],"--cost"));unsigned count=(unsigned)strtoul(argv[2],NULL,10);CHECK(count>0);cost(count);return 0;}lifecycle();vcf_release_witness();rendered_campaign(argv[1]);cost(441000);printf("PASS %u assertions. Native original execution, allocator/Analog/outer lifecycle, firmware fit and hardware remain outside this component test.\n",checks);return 0;}
