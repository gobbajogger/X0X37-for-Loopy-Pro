/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_drive.h"
#include "chicago_drive_private.h"
#include <limits.h>
#include <string.h>
static int32_t si(uint32_t u){return u<=INT32_MAX?(int32_t)u:(int32_t)((int64_t)u-4294967296LL);}
static int32_t add(int32_t a,int32_t b){return si((uint32_t)a+(uint32_t)b);}
static int32_t sub(int32_t a,int32_t b){return si((uint32_t)a-(uint32_t)b);}
static int32_t mul(int32_t a,int32_t b){return si((uint32_t)a*(uint32_t)b);}
static int32_t shl(int32_t a,unsigned n){return si((uint32_t)a<<n);}
static int32_t asr(int32_t a,unsigned n){uint32_t u=(uint32_t)a;return si((u>>n)|(a<0?~(UINT32_MAX>>n):0));}
static int16_t low16(uint32_t u){uint32_t v=u&65535u;return v<=32767u?(int16_t)v:(int16_t)((int32_t)v-65536);}
static int32_t smooth(int32_t current,int32_t target,int32_t k){return add(sub(current,mul(asr(current,15),k)),mul(k,target));}
static int32_t feedback(int32_t y){int32_t a=sub(shl(y,1),shl(y,15)),b=shl(asr(a,15),1);return asr(sub(b,si((uint32_t)a&0xffff8000u)),15);}
void original_chicago_drive_init(original_chicago_drive *s){if(!s)return;memset(s,0,sizeof *s);s->target_drive=16254;s->target_gain=20801;s->edit1=63;s->edit2=100;}
void original_chicago_drive_reset(original_chicago_drive *s){if(!s)return;s->drive=shl(s->target_drive,15);s->gain=shl(s->target_gain,15);s->threshold=0;memset(s->previous_driven,0,sizeof s->previous_driven);memset(s->previous_nonlinear,0,sizeof s->previous_nonlinear);}
int original_chicago_drive_set(original_chicago_drive *s,unsigned parameter,unsigned value){
 if(!s||value>127||(parameter!=2&&parameter!=3))return 0;
 if(parameter==2){s->edit1=(uint8_t)value;s->target_drive=original_drive_table[value];}
 else{s->edit2=(uint8_t)value;s->target_gain=1279+asr(mul(original_drive_gain_table[value],31489),15);}
 return 1;
}
int original_chicago_drive_activity(original_chicago_drive *s,unsigned activity){if(!s||activity>255)return 0;s->activity=(uint8_t)activity;return 1;}
static int16_t channel(original_chicago_drive *s,unsigned channel,int16_t input,int32_t d,int32_t g,int32_t threshold,int32_t fb){
 int32_t v=add(add(s->previous_driven[channel],input),asr(mul(feedback(s->previous_nonlinear[channel]),fb),15));
 int32_t driven=asr(mul(d,input),13);if(v< -32767)v=-32767;if(v>32767)v=32767;
 int32_t magnitude=v<0?-v:v;int32_t z=sub(magnitude,threshold);if(z<0)z=0;
 int32_t z2=(int32_t)((uint32_t)mul(z,z)>>15);int32_t sign=v>0?26213:v<0?-26213:0;
 int32_t y=sub(v,asr(mul(z2,sign),15));s->previous_driven[channel]=driven;s->previous_nonlinear[channel]=y;
 return low16((uint32_t)mul(y,g)>>15);
}
int original_chicago_drive_process(original_chicago_drive *s,const int16_t *left,const int16_t *right,int16_t *ol,int16_t *orr,size_t count){
 if(!s)return 0;
 if(!count)return 1;
 if(!left||!right||!ol||!orr||ol==orr)return 0;
 int32_t k=s->activity?32767:102,threshold_target=s->target_drive<19660?19660-s->target_drive:0;
 size_t i=0;
 for(;i<count;i++){
  int32_t d0=s->drive,g0=s->gain,t0=s->threshold;
  s->drive=smooth(d0,s->target_drive,k);s->gain=smooth(g0,s->target_gain,k);s->threshold=smooth(t0,threshold_target,k);
  int32_t d=asr(s->drive,15),g=asr(s->gain,15),threshold=asr(s->threshold,15),fb=asr(mul(d,23919),15);
  int16_t l=left[i],r=right[i];ol[i]=channel(s,0,l,d,g,threshold,fb);orr[i]=channel(s,1,r,d,g,threshold,fb);
  /* Settled: smooth() returned its input for all three, so every later
   * sample of this block has the same d, g, threshold and fb. */
  if(s->drive==d0&&s->gain==g0&&s->threshold==t0){
   for(++i;i<count;i++){int16_t l2=left[i],r2=right[i];ol[i]=channel(s,0,l2,d,g,threshold,fb);orr[i]=channel(s,1,r2,d,g,threshold,fb);}
   break;
  }
 }
 return 1;
}
