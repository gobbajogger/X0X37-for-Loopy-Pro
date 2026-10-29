/* SPDX-License-Identifier: GPL-3.0-only */
#include "fixed_fx.h"
#include "miami_fx_chain.h"
#include "control_math.h"
#include "master_unmuted.h"
#include "../wobble/miami_fx_conversion.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned calls,resets;
static void bypass(void *p,int16_t *l,int16_t *r,uint32_t n){(void)p;(void)l;(void)r;(void)n;calls++;}
static void reset(void *p){(void)p;resets++;}
static uint32_t bits(float x){uint32_t u;memcpy(&u,&x,4);return u;}
static int64_t wrap32(int64_t x){uint64_t u=(uint64_t)x&0xffffffffu;return u>=0x80000000u?(int64_t)u-4294967296LL:(int64_t)u;}
static int32_t shr(int64_t x){return (int32_t)(x>=0?x/32768:-((-x+32767)/32768));}
static int32_t omix(int32_t dry,int16_t fx,int32_t gain){int32_t q=shr(wrap32((int64_t)dry*(32767-gain)))+shr(wrap32((int64_t)fx*gain));return q>32767?32767:q< -32767?-32767:q;}
static void wrapper(void){
 original_fixed_fx s;original_fixed_fx_init(&s);original_fixed_fx saved=s;
 int32_t l[257],r[257],ol[257],orr[257];int16_t sl[257],sr[257];
 assert(!original_fixed_fx_process(&s,NULL,NULL,NULL,NULL,NULL,NULL,2049,bypass,NULL));assert(!memcmp(&s,&saved,sizeof s));
 assert(original_fixed_fx_process(&s,NULL,NULL,NULL,NULL,NULL,NULL,0,bypass,NULL));assert(calls==1&&s.target==32767&&s.gain_state==0&&!s.dirty);
 const int32_t on[]={1065287937,1073643264,1073740797,1073740797};
 for(unsigned i=0;i<4;i++){l[0]=32767;r[0]=-32768;assert(original_fixed_fx_process(&s,l,r,sl,sr,ol,orr,1,bypass,NULL));assert(s.gain_state==on[i]);}
 original_fixed_fx_reset(&s,reset,NULL);assert(resets==1&&s.gain_state==1073709056);
 original_fixed_fx_internal_enable(&s,0);
 const int32_t off[]={8421119,98303,33281,770,770};
 for(unsigned i=0;i<5;i++){assert(original_fixed_fx_process(&s,l,r,sl,sr,ol,orr,1,bypass,NULL));assert(s.gain_state==off[i]);}
 unsigned before=calls;assert(original_fixed_fx_process(&s,l,r,sl,sr,ol,orr,1,bypass,NULL));assert(calls==before+1);
 original_fixed_fx_reset(&s,reset,NULL);assert(s.target==32767&&!s.active_enable&&!s.pending_enable&&!s.dirty);
 original_fixed_fx_parameter16(&s);assert(s.pending_enable==1&&s.dirty);
 uint32_t seed=62532;
 for(unsigned trial=0;trial<2000;trial++) {
  unsigned n=trial%258;
  if(trial%7==0)original_fixed_fx_internal_enable(&s,(uint8_t)(trial%3));
  if(trial%31==0)original_fixed_fx_reset(&s,reset,NULL);
  original_fixed_fx oracle=s;if(oracle.dirty){oracle.active_enable=oracle.pending_enable;oracle.target=oracle.active_enable?32767:0;oracle.dirty=0;}
  for(unsigned j=0;j<n;j++){seed=seed*1664525u+1013904223u;l[j]=(int32_t)wrap32(seed);seed=seed*1664525u+1013904223u;r[j]=(int32_t)wrap32(seed);}
  assert(original_fixed_fx_process(&s,l,r,sl,sr,ol,orr,n,bypass,NULL));
  for(unsigned j=0;j<n;j++) {
   int32_t g=shr(oracle.gain_state);if(g!=oracle.target)oracle.gain_state=(int32_t)wrap32((int64_t)oracle.gain_state+(oracle.target-g)*32511LL);g=shr(oracle.gain_state);
   assert(ol[j]==omix(l[j],miami_fx_low16(l[j]),g));assert(orr[j]==omix(r[j],miami_fx_low16(r[j]),g));
  }
  assert(s.gain_state==oracle.gain_state&&s.target==oracle.target);
 }
 puts("PASS fixed wrapper lifecycle, signed32 product wrap, independent gain/mix oracle2000 histories");
}
static void control(void){
 uint32_t counter=44100,D=0;assert(original_control_tick(&counter,44100,&D)==1&&counter==690&&D==63);
 unsigned ticks=0;counter=0;for(unsigned i=0;i<44100;i++){int rc=original_control_tick(&counter,44100,&D);assert(rc>=0);ticks+=(unsigned)rc;}assert(ticks==690&&counter==0);
 counter=48000;assert(original_control_tick(&counter,48000,&D)==1&&D==69);
 unsigned action;original_vca e={0,0};assert(original_vca_tick(&e,44100,63,0,1,1,0,&action)&&e.level==1&&e.state==1);
 const uint32_t release[]={0x3e8c7b10,0x3d9a2db0,0x3ca93640,0x3c293600,0x3ba93600,0x3b293600,0x3aa93800,0x3a293800,0};
 e.state=3;for(unsigned i=0;i<9;i++){assert(original_vca_tick(&e,44100,64,0,1,1,0,&action));assert(bits(e.level)==release[i]);}assert(e.state==6&&action==1);
 e.level=1;e.state=5;assert(original_vca_tick(&e,44100,64,0,1,1,0,&action)&&e.level==0&&e.state==5&&!action);assert(original_vca_tick(&e,44100,64,0,1,1,0,&action)&&e.state==6&&action==2);
 e.level=.5f;e.state=0;original_vca saved=e;action=123;assert(!original_vca_tick(&e,44100,400,0,1,1,0,&action)&&action==123&&!memcmp(&e,&saved,sizeof e));
 /* INT-R1-01: preserve the original negative first tick and accept its next
  * decay tick, note-off, pending restart, and retirement transitions. */
 e=(original_vca){1,1};
 assert(original_vca_tick(&e,44100,64,0,0,0,0,&action));
 assert(e.level==-0.45124709606170654f&&e.state==1&&action==0);
 original_vca negative=e;
 assert(original_vca_tick(&e,44100,64,0,0,0,0,&action)&&e.level==0&&e.state==1&&action==0);
 for(unsigned state=3;state<=5;state++){
  e=negative;e.state=(uint8_t)state;
  assert(original_vca_tick(&e,44100,64,0,0,0,0,&action));
  assert(e.state==6&&action==(state==5?2u:1u));
 }
 const unsigned rates[]={8000,11025,22050,44100,48000,96000,192000,2147482880u};
 for(unsigned ri=0;ri<sizeof rates/sizeof rates[0];ri++){
  float fs=(float)rates[ri];unsigned dt=(unsigned)ceilf(fs/690.0f);
  for(unsigned si=0;si<=16;si++){
   float sustain=(float)si/16.0f;e=(original_vca){1,1};
   for(unsigned tick=0;tick<32;tick++){
    assert(original_vca_tick(&e,fs,dt,0,0,sustain,0,&action));
    assert(e.level>=-1&&e.level<=1&&e.state==1&&action==0);
   }
   assert(e.level==sustain);
  }
 }
 e=(original_vca){-1.0001f,1};saved=e;action=123;
 assert(!original_vca_tick(&e,44100,64,0,0,0,0,&action)&&action==123&&!memcmp(&e,&saved,sizeof e));
 puts("PASS INT-R1-01 original negative decay, next-tick sustain, note transitions and 4352 multi-rate ticks");
 float src[]={1,1,-1,0,.5f},dst[2];original_patch routes[8];
 for(unsigned k=0;k<8;k++){routes[k]=(original_patch){3,0,0,0,0};}
 routes[0]=(original_patch){0,0,0,0,1};routes[1]=(original_patch){1,0,0,0,1};routes[2]=(original_patch){2,0,0,0,1};
 assert(original_patch_accumulate(src,5,routes,dst,2)&&dst[0]==0); /* sum-first would1 */
 routes[7].source=99;dst[0]=123;assert(!original_patch_accumulate(src,5,routes,dst,2)&&dst[0]==123);
 puts("PASS explicit690 helper clock, exact original VCA witnesses, pending actions and ordered snapshot patch clamp");
}
static int32_t dryL[257],dryR[257];
static int16_t scratchL[257],scratchR[257];
static original_miami_fx_workspace work={dryL,dryR,scratchL,scratchR,257};
static void chain(void){
 original_miami_fx_chain s;original_miami_fx_init(&s);original_miami_fx_chain before=s;
 assert(original_miami_fx_process(&s,&work,NULL,NULL,NULL,NULL,0,0)&&!memcmp(&s,&before,sizeof s));
 uint32_t l[257],r[257],ol[257],orr[257];for(unsigned i=0;i<257;i++){l[i]=0x3f000000u+i;r[i]=l[i]^0x80000000u;}
 for(unsigned n=1;n<=257;n++)assert(original_miami_fx_process(&s,&work,l,r,ol,orr,n,0));
 original_fixed_fx_internal_enable(&s.fx1,0);uint16_t old=s.decimator.kernel.phase;
 assert(original_miami_fx_process(&s,&work,l,r,ol,orr,257,0));assert(s.decimator.kernel.phase!=old);
 original_miami_fx_reset(&s);assert(s.decimator.kernel.phase==0&&s.fx1.gain_state==1073709056);
 before=s;assert(!original_miami_fx_process(&s,&work,l,r,ol,orr,2049,0)&&!memcmp(&s,&before,sizeof s));
 printf("PASS integrated private fixed FX graph; chain state%zu workspace descriptor%zu bytes plus12*n sample storage, no active backend\n",sizeof s,sizeof work);
}
static void master(void){
 original_master_unmuted s;assert(original_master_unmuted_seed(&s,0));
 uint32_t in[]={0x3e800000,0x3e800000,0x3e800000,0x3e800000},out[4],right[4];
 assert(original_master_unmuted_process(&s,.25f,in,in,out,right,4));
 assert(out[0]==bits(.0625f)&&out[1]==bits(.125f)&&out[2]==bits(.1875f)&&out[3]==bits(.25f));assert(s.current_gain==1);
 assert(original_master_unmuted_process(&s,0,NULL,NULL,NULL,NULL,0)&&s.current_gain==1);
 assert(!original_master_unmuted_process(&s,NAN,in,in,out,right,4)&&s.current_gain==1);
 puts("PASS explicit-seed unmuted master increment-before-sample and native-empty semantics; mute/reset policy excluded");
}
int main(void){assert(miami_fx_rne_environment());wrapper();control();chain();master();return 0;}
