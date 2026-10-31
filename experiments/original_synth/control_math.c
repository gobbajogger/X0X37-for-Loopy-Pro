/* SPDX-License-Identifier: GPL-3.0-only */
#include "control_math.h"
#include "binary32_classify.h"
#include <limits.h>
#include <math.h>
#include "binary32_math.h"
#include <stddef.h>
#ifdef ORIGINAL_USE_SOFT_FMA
#include "fused_reference.h"
#define ref_fma original_rne_fmaf
#else
#define ref_fma fmaf
#endif
static float clamp01(float x){return x<0?0:x>1?1:x;}
static float fourth(float x){float q=x*x;return q*q;}
int original_control_tick(uint32_t *counter,uint32_t fs,uint32_t *duration) {
 if(!counter||!duration||fs<8000||fs>INT32_MAX-690u||*counter>fs)return -1;
 uint32_t next=*counter+690u;
 if(next<fs){*counter=next;return 0;}
 next-=fs;
 float d=original_binary32_ceil_finite((float)(fs-next)/690.0f);
 *counter=next;*duration=(uint32_t)d;return 1;
}
/* Fast decay can undershoot zero for one tick. Preserve that original value;
 * state 1 restores sustain on the next tick, or note-off retires it in state 3.
 * With Fs >= 8000 and D <= ceil(Fs/690), D/(Fs*.001) <= 1.575;
 * thus a decay from [0,1] remains above -1. This is an input-state bound,
 * not a clamp of the original first-tick arithmetic. */
int original_vca_tick(original_vca *s,float fs,uint32_t duration,float a,float d,float sustain,float r,unsigned *action) {
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
int original_patch_accumulate(const float *src,unsigned ns,const original_patch routes[8],float *dst,unsigned nd) {
 if(!src||!routes||!dst||!ns||!nd)return 0;
 for(unsigned k=0;k<8;k++) {
  const original_patch *r=&routes[k];
  if(r->source>=ns||r->destination>=nd||r->has_second>1||!original_binary32_isfinite(r->intensity)||r->intensity< -1||r->intensity>1||
     !original_binary32_isfinite(src[r->source])||src[r->source]< -1||src[r->source]>1)return 0;
  if(r->has_second&&(r->second>=ns||!original_binary32_isfinite(src[r->second])||src[r->second]< -1||src[r->second]>1))return 0;
 }
 for(unsigned j=0;j<nd;j++)dst[j]=0;
 for(unsigned k=0;k<8;k++) {
  const original_patch *r=&routes[k];float value=src[r->source];
  if(r->has_second)value=value*src[r->second];
  value=value*r->intensity;value=dst[r->destination]+value;
  dst[r->destination]=value< -1?-1:value>1?1:value;
 }
 return 1;
}
