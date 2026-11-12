/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_modulation.h"
#include "chicago_chorus_span.h"
#ifndef ORIGINAL_MODULATION_COEFFICIENTS
#define ORIGINAL_MODULATION_COEFFICIENTS "modulation_private.h"
#endif
#include ORIGINAL_MODULATION_COEFFICIENTS
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <math.h>
#ifdef ORIGINAL_USE_SOFT_FMA
#include "fused_reference.h"
#define ref_fma original_rne_fmaf
#else
#define ref_fma fmaf
#endif
static int32_t si(uint32_t u){return u<=INT32_MAX?(int32_t)u:(int32_t)((int64_t)u-4294967296LL);}
static int32_t add(int32_t a,int32_t b){return si((uint32_t)a+(uint32_t)b);}
static int32_t sub(int32_t a,int32_t b){return si((uint32_t)a-(uint32_t)b);}
static int32_t mul(int32_t a,int32_t b){return si((uint32_t)a*(uint32_t)b);}
static int32_t shl(int32_t a,unsigned n){return si((uint32_t)a<<n);}
static int32_t asr(int32_t a,unsigned n){return si(((uint32_t)a>>n)|(a<0?~(UINT32_MAX>>n):0));}
static int16_t low16(int32_t a){uint32_t u=(uint32_t)a&65535u;return u<32768?(int16_t)u:(int16_t)((int32_t)u-65536);}
static int32_t clip(int32_t a,int32_t lo,int32_t hi){return a<lo?lo:a>hi?hi:a;}
static int32_t smooth(int32_t a,int32_t target,int32_t k){return add(sub(a,mul(asr(a,15),k)),mul(target,k));}
static uint32_t magnitude(int32_t a){return a<0?0u-(uint32_t)a:(uint32_t)a;}
int original_mod_events(original_mod_queue *q,const original_mod_event *e,uint32_t n){
 if(!q||(n&&!e))return 0;
 for(uint32_t i=1;i<n;i++)if(e[i-1].offset>e[i].offset)return 0;
 *q=(original_mod_queue){e,n,0};return 1;
}
void original_chicago_ring_init(original_chicago_ring *s){
 if(!s)return;
 memset(s,0,sizeof *s);s->edit1=s->edit2=64;s->dirty=1;
 s->pending_frequency=asr(mod_linear[64]*3,3);s->pending_wet=mod_linear[64];
}
void original_chicago_ring_reset(original_chicago_ring *s){if(s){s->frequency_acc=shl(s->frequency,15);s->wet_acc=shl(s->wet,15);s->phase=0;}}
int original_chicago_ring_set(original_chicago_ring *s,unsigned p,unsigned v){
 if(!s||v>127||(p!=2&&p!=3))return 0;
 if(p==2)s->edit1=(uint8_t)v;else s->edit2=(uint8_t)v;
 s->pending_frequency=asr(mod_linear[s->edit1]*3,3);s->pending_wet=mod_linear[s->edit2];s->dirty=1;return 1;
}
int original_chicago_ring_process(original_chicago_ring *s,int16_t *l,int16_t *r,uint32_t n){
 if(!s||(n&&(!l||!r||l==r)))return 0;
 if(!n)return 1;
 for(uint32_t i=0;i<n;i++){
  while(s->queue.cursor<s->queue.count&&s->queue.events[s->queue.cursor].offset<=i){if(s->queue.events[s->queue.cursor].type==4)original_chicago_ring_reset(s);s->queue.cursor++;}
  if(!(i&63u)&&s->dirty){s->frequency=s->pending_frequency;s->wet=s->pending_wet;s->dirty=0;}
  int32_t k=s->activity?32767:102;s->wet_acc=smooth(s->wet_acc,s->wet,k);s->frequency_acc=smooth(s->frequency_acc,s->frequency,k);
  int32_t wet=asr(s->wet_acc,15),dry=asr(sub(1073741823,s->wet_acc),15);s->dry_gain=dry;s->wet_gain=wet;
  s->phase=add(s->phase,s->frequency_acc);
  int32_t t=asr(s->phase,16),a=asr(s->phase,15);a=a<0?-a:a;t=add(t,asr(mul(32767-a,t),15));
  uint64_t f=s->frequency_acc< -4194303?(uint64_t)(int64_t)-1073741823:s->frequency_acc>=4194304?UINT64_C(1073741823):(uint64_t)(int64_t)s->frequency_acc<<8;
  uint64_t z=((f>>15)*(uint64_t)(int64_t)t)>>15;int32_t carrier=clip(si((uint32_t)(z+((UINT64_C(1073741823)-f)>>15))),-32767,32767);
  int16_t input[2]={l[i],r[i]};int16_t out[2];
  for(unsigned c=0;c<2;c++){int32_t x=asr(mul(carrier,input[c]),15);int32_t y=add(mul(input[c],dry),mul(x,wet));out[c]=low16(si((uint32_t)clip(y,-536870911,536870911)>>14));}
  l[i]=out[0];r[i]=out[1];
 }return 1;
}
int original_chicago_chorus_init(original_chicago_chorus *s,int16_t l[1024],int16_t r[1024]){
 if(!s||!l||!r||l==r)return 0;
 memset(s,0,sizeof *s);s->left=l;s->right=r;s->dirty=1;
 memset(l,0,1024*sizeof *l);memset(r,0,1024*sizeof *r);return 1;
}
void original_chicago_chorus_reset(original_chicago_chorus *s){
 if(!s)return;
 memset(s->left,0,1024*sizeof *s->left);memset(s->right,0,1024*sizeof *s->right);s->modulator[0]=s->modulator[1]=0;
 s->feedback_acc=shl(s->feedback,15);s->wet_acc=shl(s->wet,15);s->phase=0;/* original head is preserved */
}
int original_chicago_chorus_set(original_chicago_chorus *s,unsigned p,unsigned v){
 if(!s||v>127||(p!=2&&p!=3))return 0;
 if(p==2)s->edit1=(uint8_t)v;else s->edit2=(uint8_t)v;
 s->pending_speed=mod_speed[s->edit1];s->pending_feedback=mod_feedback[s->edit2];s->pending_wet=mod_wet[s->edit2];s->dirty=1;return 1;
}
static int32_t feedback_twice(int32_t y){int32_t a=sub(shl(y,1),shl(y,15));for(unsigned j=0;j<2;j++)a=sub(shl(asr(a,15),1),si((uint32_t)a&0xffff8000u));return asr(a,15);}
int original_chicago_chorus_process(original_chicago_chorus *s,int16_t *l,int16_t *r,uint32_t n){
 if(!s||(n&&(!l||!r||l==r||!s->left||!s->right)))return 0;
 if(!n)return 1;
 for(uint32_t i=0;i<n;i++){
  while(s->queue.cursor<s->queue.count&&s->queue.events[s->queue.cursor].offset<=i){if(s->queue.events[s->queue.cursor].type==4)original_chicago_chorus_reset(s);s->queue.cursor++;}
  if(!(i&63u)&&s->dirty){s->speed=s->pending_speed;s->feedback=s->pending_feedback;s->wet=s->pending_wet;s->dirty=0;}
  int32_t k=s->activity?32767:102;s->feedback_acc=smooth(s->feedback_acc,s->feedback,k);s->wet_acc=smooth(s->wet_acc,s->wet,k);s->phase=add(s->phase,s->speed);
  int16_t input[2]={l[i],r[i]},output[2];
  for(unsigned c=0;c<2;c++){
   uint32_t mag=magnitude(c?add(s->phase,208732160):s->phase);int32_t z=add(mul(s->modulator[c],8160),(int32_t)(mag>>11));
   s->modulator[c]=z< -268427264?-32767:z>=268435456?32767:(int32_t)low16(asr(z,13));
   int32_t coordinate=mul(s->modulator[c],667);uint32_t index=(s->head-(uint32_t)asr(add(coordinate,196608),16))&1023u,previous=(index-1)&1023u;
   int16_t *history=c?s->right:s->left;int32_t a=history[index],b=history[previous];
   int64_t product=(int64_t)(b-a)*(int32_t)((uint32_t)coordinate&65535u);int32_t delayed=add(a,si((uint32_t)((uint64_t)product>>16)));int32_t y=feedback_twice(delayed);
   history[s->head]=(int16_t)clip(add(input[c],asr(mul(y,asr(s->feedback_acc,15)),15)),-32767,32767);
   output[c]=(int16_t)clip(add(input[c],asr(mul(y,asr(s->wet_acc,15)),15)),-32767,32767);
  }
  l[i]=output[0];r[i]=output[1];s->head=(s->head+1)&1023u;
 }return 1;
}
void original_chicago_phaser_init(original_chicago_phaser *s){
 if(!s)return;
 memset(s,0,sizeof *s);s->edit1=30;s->edit2=63;s->speed=mod_speed[30];s->modulation=asr(mul(mod_linear[63],16102),15);
 int32_t x=asr(sub(shl(mod_linear[120],15),mod_linear[120]),15);s->depth=(int32_t)((uint32_t)mul(x,x)>>15);s->feedback=asr(mul(mod_ph_feedback[40],32440),15);s->wet=mod_linear[64];
}
void original_chicago_phaser_reset(original_chicago_phaser *s){
 if(!s)return;
 memset(s->history,0,sizeof s->history);s->depth_acc=shl(s->depth,15);s->modulation_acc=shl(s->modulation,15);s->feedback_acc=shl(s->feedback,15);s->wet_acc=shl(s->wet,15);s->phase=0;
}
int original_chicago_phaser_set(original_chicago_phaser *s,unsigned p,unsigned v){
 if(!s||v>127||(p!=2&&p!=3))return 0;
 if(p==2){s->edit1=(uint8_t)v;s->speed=mod_speed[v];}else{s->edit2=(uint8_t)v;s->modulation=asr(mul(mod_linear[v],16102),15);}return 1;
}
int original_chicago_phaser_process(original_chicago_phaser *s,int16_t *l,int16_t *r,uint32_t n){
 if(!s||(n&&(!l||!r||l==r)))return 0;
 if(!n)return 1;
 for(uint32_t i=0;i<n;i++){
  while(s->queue.cursor<s->queue.count&&s->queue.events[s->queue.cursor].offset<=i){if(s->queue.events[s->queue.cursor].type==4)original_chicago_phaser_reset(s);s->queue.cursor++;}
  int32_t k=s->activity?32767:102;s->depth_acc=smooth(s->depth_acc,s->depth,k);s->modulation_acc=smooth(s->modulation_acc,s->modulation,k);s->feedback_acc=smooth(s->feedback_acc,s->feedback,k);s->wet_acc=smooth(s->wet_acc,s->wet,k);s->phase=add(s->phase,s->speed);
  int32_t wet=asr(s->wet_acc,15),mono=asr((int32_t)l[i]+r[i],1),dry=asr(mul(32767-(int32_t)magnitude(wet),mono),15),feedback=asr(sub(0,s->feedback_acc),15);int16_t out[2];
  for(unsigned c=0;c<2;c++){
   int32_t p=c?add(s->phase,208732160):s->phase;int32_t triangle=clip(asr(si(magnitude(p)),16),0,32767);int32_t coefficient=sub(asr(s->depth_acc,15),asr(mul(triangle,asr(s->modulation_acc,15)),15));
   int32_t q[5];for(unsigned j=0;j<5;j++)q[j]=mul(asr(mul(s->history[j][c],-32766),15),-32766);
   int32_t y=clip(add(mono,asr(mul(s->history[2][c],feedback),15)),-32767,32767);s->history[0][c]=y;
   for(unsigned j=1;j<5;j++){y=clip(sub(add(asr(mul(asr(q[j],15),coefficient),15),asr(q[j-1],15)),asr(mul(y,coefficient),15)),-32767,32767);s->history[j][c]=y;}
   out[c]=(int16_t)clip(add(dry,asr(mul(y,wet),15)),-32767,32767);
  }l[i]=out[0];r[i]=out[1];
 }return 1;
}
int original_chicago_ring_advance_zero(original_chicago_ring *s,uint32_t n){
 if(!s||s->dirty||s->queue.cursor<s->queue.count||
    asr(s->wet_acc,15)!=s->wet||asr(s->frequency_acc,15)!=s->frequency)return 0;
 if(!n)return 1;
 s->dry_gain=asr(sub(1073741823,s->wet_acc),15);s->wet_gain=asr(s->wet_acc,15);
 s->phase=si((uint32_t)s->phase+(uint32_t)s->frequency_acc*n);return 1;
}
int original_chicago_chorus_advance_zero(original_chicago_chorus *s,uint32_t n){
 /* Do not pay a 4096-byte history scan for tiny event-split invocations. */
 if(n<64||!s||!s->left||!s->right||s->left==s->right||s->dirty||s->queue.cursor<s->queue.count||
    asr(s->feedback_acc,15)!=s->feedback||asr(s->wet_acc,15)!=s->wet||
    s->modulator[0]< -32767||s->modulator[0]>32767||s->modulator[1]< -32767||s->modulator[1]>32767)return 0;
 /* One bounded history scan per admitted invocation, never per sample. */
 for(unsigned i=0;i<1024;i+=8){
  /* memcpy keeps these word loads valid for int16-aligned borrowed arrays
   * and strict aliasing. Unroll the zero proof instead of 1024 branches. */
  uint32_t a[4],b[4];memcpy(a,s->left+i,sizeof a);memcpy(b,s->right+i,sizeof b);
  if(a[0]|a[1]|a[2]|a[3]|b[0]|b[1]|b[2]|b[3])return 0;
 }
 if(!n)return 1;
 for(unsigned c=0;c<2;c++)s->modulator[c]=original_chicago_chorus_span(
     c?add(s->phase,208732160):s->phase,s->speed,s->modulator[c],n);
 s->phase=si((uint32_t)s->phase+(uint32_t)s->speed*n);
 s->head=(s->head+n)&1023u;return 1;
}
int original_chicago_phaser_advance_zero(original_chicago_phaser *s,uint32_t n){
 if(!s||s->queue.cursor<s->queue.count||asr(s->depth_acc,15)!=s->depth||
    asr(s->modulation_acc,15)!=s->modulation||asr(s->feedback_acc,15)!=s->feedback||
    asr(s->wet_acc,15)!=s->wet)return 0;
 for(unsigned i=0;i<5;i++)for(unsigned c=0;c<2;c++)if(s->history[i][c])return 0;
 s->phase=si((uint32_t)s->phase+(uint32_t)s->speed*n);return 1;
}
static float fl(uint32_t u){float f;memcpy(&f,&u,4);return f;}
static void ensemble_phases(original_chicago_ensemble *s){
 const float initial[4]={0x1.555556p-1f,-0x1.555556p-1f,-2,0};
 for(unsigned g=0;g<2;g++)for(unsigned j=0;j<4;j++)s->phase[g][j]=s->right_phase[g][j]=initial[j];
}
int original_chicago_ensemble_init(original_chicago_ensemble *s,float *l,float *r,uint32_t slots){
 if(!s||!l||!r||l==r||(slots!=1024&&slots!=4096))return 0;
 memset(s,0,sizeof *s);s->left=l;s->right=r;s->slots=slots;s->spread=1;s->dirty=1;ensemble_phases(s);return 1;
}
void original_chicago_ensemble_reset(original_chicago_ensemble *s){
 if(!s)return;
 s->head=0;s->written=0;s->depth=44100.0f*fl(0x3ad1b717);s->spread=1;ensemble_phases(s);
}
int original_chicago_ensemble_set(original_chicago_ensemble *s,unsigned p,unsigned value){
 if(!s||value>127||(p!=2&&p!=3))return 0;
 if(p==2)s->edit1=(uint8_t)value;else s->edit2=(uint8_t)value;
 s->pending_depth=(float)s->edit1/127.0f;s->pending_spread=(float)s->edit2/127.0f;s->dirty=1;return 1;
}
static int ensemble_history(const original_chicago_ensemble *s,unsigned channel,uint32_t index,unsigned current,float *out){
 if(!s||!out||channel>1||index>4095)return 0;
 uint32_t age=(index-((s->head+(current?0:1))&4095u))&4095u;const float *h=channel?s->right:s->left;
 if(age>=s->written){*out=0;return 1;}
 if(s->slots==4096){*out=h[index];return 1;}
 if(age<512){*out=h[index&511u];return 1;}
 if(index<512){*out=h[512+index];return 1;}
 return 0;
}
int original_chicago_ensemble_history(const original_chicago_ensemble *s,unsigned channel,uint32_t index,float *out){return ensemble_history(s,channel,index,0,out);}
static void ensemble_write(original_chicago_ensemble *s,unsigned ch,float x){
 float *h=ch?s->right:s->left;
 if(s->slots==4096)h[s->head]=x;
 else {h[s->head&511u]=x;if(s->head<512)h[512+s->head]=x;}
}
static float ensemble_read(original_chicago_ensemble *s,unsigned ch,uint32_t index){
 float out;int ok=ensemble_history(s,ch,index&4095u,1,&out);assert(ok);(void)ok;return out;
}
static float ensemble_sine(float phase){
 float a=(phase<0?-phase:phase)-1.0f,a2=a*a,a4=a2*a2,a5=a*a4;
 float first=a*fl(0x3fc90042),second=a*(a2*fl(0x3f24619a));float p=(first-second)+(a5*fl(0x3d932c2b));
 return (p+fl(0x3f800237))*fl(0x3efffb8b);
}
static float ensemble_tap(original_chicago_ensemble *s,unsigned ch,float a,float b){
 float ratio=ensemble_sine(a)*fl(0x3daaaaa6);ratio=ratio+ensemble_sine(b);float coordinate=(float)s->head+ratio*s->depth;
 if(coordinate>=4096)coordinate=coordinate-4096;
 int32_t index=(int32_t)coordinate;float fraction=coordinate-(float)index;
 float x=ensemble_read(s,ch,(uint32_t)index),y=ensemble_read(s,ch,(uint32_t)index+1);
 return x+(y-x)*fraction;
}
static void ensemble_latch(original_chicago_ensemble *s){
 float x=s->pending_depth;
 if(x==1)s->unused_rate=7.0f/44100.0f;
 else {float v=x*128.0f;unsigned k=(unsigned)v;float lo=(.125f+(float)k*(6.875f/128.0f))/44100.0f,hi=(.125f+(float)(k+1)*(6.875f/128.0f))/44100.0f;s->unused_rate=ref_fma(hi-lo,v-(float)k,lo);}
 float a=ref_fma(x,.375f,.125f)*fl(0x3c51b717);a=a*44100.0f;
 s->delta_depth=a*fl(0x3b03126f);float b=s->pending_spread*s->pending_spread;b=b+1.0f;s->delta_spread=b*fl(0x3b03126f);s->dirty=0;
}
int original_chicago_ensemble_process(original_chicago_ensemble *s,int16_t *l,int16_t *r,uint32_t n){
 if(!s||(n&&(!l||!r||l==r||!s->left||!s->right)))return 0;
 if(!n)return 1;
 const float inc[2]={0x1.ee91040000000p-12f,0x1.9adee00000000p-16f};/* generated actual float32(20.8/Fs),float32(1.08/Fs), checked by producer */
 for(uint32_t i=0;i<n;i++){
  while(s->queue.cursor<s->queue.count&&s->queue.events[s->queue.cursor].offset<=i){if(s->queue.events[s->queue.cursor].type==4)original_chicago_ensemble_reset(s);s->queue.cursor++;}
  if(!(i&63u)&&s->dirty)ensemble_latch(s);
  s->depth=s->depth*fl(0x3f7f7cee)+s->delta_depth;s->spread=s->spread*fl(0x3f7f7cee)+s->delta_spread;
  float input[2]={(float)l[i]/32767.0f,(float)r[i]/32767.0f};input[0]=(input[0]+1.0f)-1.0f;input[1]=(input[1]+1.0f)-1.0f;
  ensemble_write(s,0,input[0]);ensemble_write(s,1,input[1]);if(s->written<4096)s->written++;
  for(unsigned g=0;g<2;g++)for(unsigned j=0;j<4;j++){
   float p=s->phase[g][j]+(j==3?0:inc[g]);if(p>2)p=p-4;s->phase[g][j]=p;
  }
  float taps[4];for(unsigned j=0;j<4;j++)taps[j]=ensemble_tap(s,0,s->phase[0][j],s->phase[1][j]);
  float y=((taps[0]+taps[1])+taps[2])+taps[3];y=y+0.0f;y=y*fl(0x3f2aaa3b);y=(y+1.0f)-1.0f;if(y>1)y=1;if(y< -1)y=-1;l[i]=low16((int32_t)(y*32767.0f));
  float shift=(s->spread-1.0f)*fl(0x3f2aaab0);
  for(unsigned g=0;g<2;g++)for(unsigned j=0;j<4;j++){float p=s->phase[g][j]+shift;if(p>2)p=p-4;s->right_phase[g][j]=p;}
  for(unsigned j=0;j<4;j++)taps[j]=ensemble_tap(s,1,s->right_phase[0][j],s->right_phase[1][j]);
  /* Preserve original NEON reduction: right lane3 is calculated but omitted. */
  y=(taps[2]+0.0f)+(taps[0]+taps[1]);y=y*fl(0x3f2aaa3b);y=(y+1.0f)-1.0f;if(y>1)y=1;if(y< -1)y=-1;r[i]=low16((int32_t)(y*32767.0f));
  s->head=(s->head-1)&4095u;
 }return 1;
}

void original_chicago_ring_direct_event(original_chicago_ring *s,unsigned type){if(type==5)original_chicago_ring_reset(s);}
void original_chicago_chorus_direct_event(original_chicago_chorus *s,unsigned type){if(type==5||type==2)original_chicago_chorus_reset(s);}
void original_chicago_phaser_direct_event(original_chicago_phaser *s,unsigned type){if(type==5)original_chicago_phaser_reset(s);}
void original_chicago_ensemble_direct_event(original_chicago_ensemble *s,unsigned type){if(type==5||type==2)original_chicago_ensemble_reset(s);}
