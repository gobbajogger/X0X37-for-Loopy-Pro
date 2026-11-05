/* SPDX-License-Identifier: GPL-3.0-only */
/* Frozen reference: control_math.c at 9231c26285c09356b49b2d9afbcd225377aae5c5.
 * Compare every level bit, state, action and return across individual calls
 * and seeded streams with mid-stream parameter and gate changes. */
#include "control_math.h"
#include "binary32_classify.h"
#include "binary32_math.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef ORIGINAL_USE_SOFT_FMA
#include "fused_reference.h"
#define ref_fma original_rne_fmaf
#else
#define ref_fma fmaf
#endif
static float clamp01(float x){return x<0?0:x>1?1:x;}
static float fourth(float x){float q=x*x;return q*q;}
static int reference_vca_tick(original_vca *s,float fs,uint32_t duration,float a,float d,float sustain,float r,unsigned *action) {
 if(!s||!action||s->state>6||!original_binary32_isfinite(s->level)||s->level< -1||s->level>1||
    !original_binary32_isfinite(fs)||fs<8000||fs>2147482880.0f||duration==0||duration>2147482880u||
    !original_binary32_isfinite(a)||!original_binary32_isfinite(d)||!original_binary32_isfinite(sustain)||!original_binary32_isfinite(r))return 0;
 if(duration>(uint32_t)original_binary32_ceil_finite(fs/690.0f))return 0;
 a=clamp01(a);d=clamp01(d);sustain=clamp01(sustain);r=clamp01(r);
 float e=s->level,delta,den,q,D=(float)duration;unsigned result=0;
 switch(s->state) {
 case 0:
  den=fs*ref_fma(fourth(a),13.0f,0.0010000000474974513f);
  delta=D*(1.2999999523162842f-e);q=e+delta/den;
  if(q>=1){q=1;s->state=1;}e=q;break;
 case 1:
  if(e<sustain)e=sustain;
  else{den=fs*ref_fma(fourth(d),25.0f,0.0010000000474974513f);delta=D*(sustain-e);q=e+delta/den;e=(q+1.0f)-1.0f;}break;
 case 3:
  if(e>=0.03999999910593033f){den=fs*ref_fma(fourth(r),25.0f,0.0020000000949949026f);q=e*(1.0f-D/den);e=(q+1.0f)-1.0f;}
  else if(e>0.0010000000474974513f){q=e*0.5f;e=(q+1.0f)-1.0f;}
  else{e=0;s->state=6;result=1;}break;
 case 4:case 5:
  if(e>0)e=0;else{result=s->state==5?2:1;s->state=6;}break;
 default:break;
 }
 s->level=e;*action=result;return 1;
}
static unsigned long comparisons, equal_ticks;
static uint32_t rng;
static uint32_t next(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static float unit(void){return (float)(next()>>8)/16777216.0f;}
static void compare(original_vca *old, original_vca *new, float fs, uint32_t D,
                    float a,float d,float sustain,float r){
 unsigned oa=99,na=99;
 if(new->state==1 && new->level==clamp01(sustain))equal_ticks++;
 int ok=reference_vca_tick(old,fs,D,a,d,sustain,r,&oa);
 int nk=original_vca_tick(new,fs,D,a,d,sustain,r,&na);
 comparisons++;
 if(ok!=nk || oa!=na || old->state!=new->state ||
    original_math_bits(old->level)!=original_math_bits(new->level)){
  fprintf(stderr,"FAIL comparison %lu: old=%08x/%u/%u/%d new=%08x/%u/%u/%d\n",
   comparisons,original_math_bits(old->level),old->state,oa,ok,
   original_math_bits(new->level),new->state,na,nk);exit(1);
 }
}
int main(void){
 const float fs[]={8000,44100,48000,96000,2147482880.0f};
 const float edge[]={-1,-0.0f,0,0x1p-149f,0x1p-126f,0.001f,0.1f,0.5f,0x1.fffffep-1f,1,2};
 /* Equality, neighbours, clamping and signed zero, at all supported bounds. */
 for(unsigned f=0;f<5;f++)for(unsigned i=0;i<11;i++)for(unsigned j=0;j<11;j++)
 for(unsigned state=0;state<8;state++){
  original_vca a={edge[i],(uint8_t)state},b=a;
  compare(&a,&b,fs[f],(uint32_t)ceilf(fs[f]/690),edge[j],edge[j],edge[j],edge[j]);
 }
 /* Sweep every binary32 value in [0.5,1), plus a broad exponent sweep.
  * Each call enters the equality branch, including values changed by rounding. */
 for(uint32_t bits=0x3f000000;bits<=0x3f800000;bits++){
  float s=original_math_float(bits);original_vca a={s,1},b=a;
  compare(&a,&b,44100,64,0,s,s,0);
 }
 for(uint32_t bits=0;bits<0x3f800000;bits+=997){
  float s=original_math_float(bits);original_vca a={s,1},b=a;
  compare(&a,&b,8000,12,0,0.7f,s,0);
 }
 for(unsigned seed=1;seed<=32;seed++){
  rng=seed*0x9e3779b9u;
  original_vca a={0,0},b=a;float attack=0,decay=0,sustain=1,release=0;
  for(unsigned tick=0;tick<40000;tick++){
   if(tick%193==0){attack=unit();decay=unit();sustain=unit();release=unit();}
   if(tick%311==0)sustain=(float)(next()%2); /* exact plateaus */
   if(tick%401==0){a.state=b.state=0;}
   if(tick%401==300){a.state=b.state=3;}
   if(tick%997==0){a.state=b.state=1;a.level=b.level=sustain;}
   unsigned f=next()%5;
   uint32_t D=(next()&1)?1:(uint32_t)ceilf(fs[f]/690);
   compare(&a,&b,fs[f],D,attack,decay,sustain,release);
  }
 }
 printf("PASS VCA sustain: %lu comparisons, %lu equality entries; 32 seeds\n",comparisons,equal_ticks);
 return 0;
}
