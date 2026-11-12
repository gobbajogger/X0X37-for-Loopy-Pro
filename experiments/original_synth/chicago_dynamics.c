/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_dynamics.h"
#include "chicago_dynamics_private.h"
#include <limits.h>
#include <math.h>
#include <string.h>
#ifdef ORIGINAL_USE_SOFT_FMA
#include "fused_reference.h"
#define r_fma original_rne_fmaf
#else
#define r_fma fmaf
#endif
static int32_t s32(uint32_t u){return u<=INT32_MAX?(int32_t)u:-1-(int32_t)(UINT32_MAX-u);}
static int64_t s64(uint64_t u){return u<=INT64_MAX?(int64_t)u:-1-(int64_t)(UINT64_MAX-u);}
static int32_t sar32(int32_t x,unsigned n){uint32_t u=(uint32_t)x;return s32((u>>n)|(x<0?~(UINT32_MAX>>n):0));}
static int64_t sar64(uint64_t u,unsigned n){return s64((u>>n)|((u>>63)?~(UINT64_MAX>>n):0));}
static int32_t mul32(int32_t a,int32_t b){return s32((uint32_t)a*(uint32_t)b);}
static int32_t smooth(int32_t a,int32_t target,int32_t k){return s32((uint32_t)a+(uint32_t)(target-sar32(a,15))*(uint32_t)k);}
#ifdef ORIGINAL_CHICAGO_MASTER_TABLE_BASELINE
#include "master_baseline_private.h"
static uint32_t curve(unsigned p){return (uint32_t)master_curve[p];}
static uint32_t gain_table(unsigned p){return (uint32_t)master_gain[p];}
#else
static uint32_t curve(unsigned p){return 32767u*p*p/16129u;}
static uint32_t gain_table(unsigned p){return p<=100?32767u*p/100u:32767u+32767u*(p-100u)/27u;}
#endif
static int bind(original_chicago_dyn_queue *q,const original_chicago_drive_event *p,uint32_t n){
 if(n&&!p)return 0;
 for(uint32_t i=1;i<n;i++)if(p[i].offset<p[i-1].offset)return 0;
 *q=(original_chicago_dyn_queue){p,n,0};return 1;
}
int original_chicago_mastercomp_init(original_chicago_mastercomp *s){
 if(!s)return 0;
 memset(s,0,sizeof *s);
 s->parameter[0]=50;s->parameter[1]=0;s->parameter[2]=100;
 s->target_sensitivity=212;s->sensitivity=6946816;s->attack=8;s->target_gain=32767;s->gain=1073709056;return 1;
}
void original_chicago_mastercomp_reset(original_chicago_mastercomp *s){
 if(!s)return;
 /* Native29a30 clears inputs/detector, but RETAINS both highpass outputs. */
 memset(s->previous_input,0,sizeof s->previous_input);
 s->peak=s->envelope=s->control=0;
 s->sensitivity=s32((uint32_t)s->target_sensitivity<<15);s->gain=s32((uint32_t)s->target_gain<<15);
}
int original_chicago_mastercomp_set(original_chicago_mastercomp *s,unsigned p,int32_t value){
 if(!s||p>2)return 0;
 unsigned v=(uint32_t)value&127u;s->parameter[p]=(uint8_t)v;
 if(p==0)s->target_sensitivity=(int32_t)(64u+((959u*curve(v))>>15));
 else if(p==1)s->attack=(int32_t)(8u+((3268u*curve(v))>>15));
 else s->target_gain=(int32_t)gain_table(v);
 return 1;
}
static int32_t highpass(int16_t input,int32_t old_input,int32_t old_output){
 uint64_t v=((uint64_t)(int64_t)old_output<<32)-((uint64_t)(int64_t)old_output<<25);
 int64_t y=sar64(v,32);const int64_t k=INT64_C(-4294705152);
 y=sar64((uint64_t)y*(uint64_t)k,32);y=sar64((uint64_t)y*(uint64_t)k,32);
 y+=(int64_t)input*32768-old_input;
 if(y>1073741823)y=1073741823;
 if(y< -1073741823)y=-1073741823;
 return (int32_t)y;
}
static int16_t q9_clip(int32_t x){return x< -16776704?-32767:x>=16777216?32767:(int16_t)sar32(x,9);}
static int16_t post_clip(int32_t x){return x< -1073709056?-32767:x>=1073741824?32767:(int16_t)sar32(x,15);}
static void master_frame(original_chicago_mastercomp *s,int16_t l,int16_t r,int16_t *ol,int16_t *orr){
 int32_t k=s->activity?32767:102;
 s->sensitivity=smooth(s->sensitivity,s->target_sensitivity,k);s->gain=smooth(s->gain,s->target_gain,k);
 int32_t hl=highpass(l,s->previous_input[0],s->highpass[0]),hr=highpass(r,s->previous_input[1],s->highpass[1]);
 uint32_t al=hl<0?(uint32_t)(-hl):(uint32_t)hl,ar=hr<0?(uint32_t)(-hr):(uint32_t)hr;
 int32_t peak=(int32_t)(al>ar?al:ar),decay=mul32(sar32(s->peak,15),32674);
 if(peak<decay)peak=decay;
 int32_t env=s32((uint32_t)s->envelope-(uint32_t)mul32(sar32(s->envelope,15),s->attack)+(uint32_t)mul32((int32_t)((uint32_t)peak>>15),s->attack));
 int32_t sens=sar32(s->sensitivity,15),div=sens?32768/sens:0;
 uint32_t inverted=((uint32_t)env^UINT32_C(0x7fff0000))>>16;
 int32_t control=s32((uint32_t)sar32(mul32((int32_t)inverted,s->control),15)+((uint32_t)div<<4));
 if(control>32767)control=32767;
 if(control< -32767)control=-32767;
 int32_t prod=mul32(control,sens);int64_t effective=sar32(prod,13); /* SBFX bit13,width19. */
 int32_t raw_l=s32((uint32_t)(((uint64_t)(int64_t)hl*(uint64_t)effective)>>15));
 int32_t raw_r=s32((uint32_t)(((uint64_t)(int64_t)hr*(uint64_t)effective)>>15));
 int32_t gain=sar32(s->gain,15);
 *ol=post_clip(mul32(q9_clip(raw_l),gain));*orr=post_clip(mul32(q9_clip(raw_r),gain));
 s->previous_input[0]=(int32_t)l*32768;s->previous_input[1]=(int32_t)r*32768;
 s->highpass[0]=hl;s->highpass[1]=hr;s->peak=peak;s->envelope=env;s->control=control;
}
static float value(uint32_t u){float f;memcpy(&f,&u,4);return f;}
static int32_t coefficient(float x){float scaled=x*16777216.0f;int32_t i=(int32_t)scaled;return (int32_t)(float)i;}
static float q_value(unsigned p){
 if(p>63)return r_fma((float)p,value(UINT32_C(0x3d820821)),1)+value(UINT32_C(0xc0820821));
 float q=(float)p*value(UINT32_C(0x3f666666));q=q*value(UINT32_C(0x3c800000));return q+value(UINT32_C(0x3dcccccd));
}
static void eq_coefficients(original_chicago_peaking *s,unsigned refresh_alpha){
 unsigned f=s->parameter[0],g=s->parameter[1];float a=original_chicago_eq_amplitude[g];
 if(refresh_alpha){float q=q_value(s->parameter[2]);float twice=q+q;s->alpha=original_chicago_eq_sin[f]/twice;}
 float boost=s->alpha*a,cut=s->alpha/a,den=cut+1;
 s->coefficient[0]=coefficient((boost+1)/den);
 s->coefficient[1]=coefficient((original_chicago_eq_cos[f]*-2)/den);
 s->coefficient[2]=coefficient((1-boost)/den);
 s->coefficient[3]=s->coefficient[1];s->coefficient[4]=coefficient((1-cut)/den);
}
int original_chicago_peaking_init(original_chicago_peaking *s){
 if(!s)return 0;
 memset(s,0,sizeof *s);s->parameter[0]=61;s->parameter[1]=70;s->parameter[2]=64;eq_coefficients(s,1);return 1;
}
void original_chicago_peaking_reset(original_chicago_peaking *s){if(s)memset(s->history,0,sizeof s->history);}
int original_chicago_peaking_set(original_chicago_peaking *s,unsigned p,unsigned v){
 if(!s||p>2||v>127)return 0;
 s->parameter[p]=(uint8_t)v;eq_coefficients(s,p!=1);return 1;
}
void original_chicago_mastercomp_activity(original_chicago_mastercomp *s,uint8_t a){if(s)s->activity=a;}
void original_chicago_peaking_activity(original_chicago_peaking *s,uint8_t a){if(s)s->activity=a;}
int original_chicago_mastercomp_bind_events(original_chicago_mastercomp *s,const original_chicago_drive_event *p,uint32_t n){return s?bind(&s->queue,p,n):0;}
int original_chicago_peaking_bind_events(original_chicago_peaking *s,const original_chicago_drive_event *p,uint32_t n){return s?bind(&s->queue,p,n):0;}
void original_chicago_mastercomp_direct_event(original_chicago_mastercomp *s,unsigned type){if(type==5)original_chicago_mastercomp_reset(s);}
void original_chicago_peaking_direct_event(original_chicago_peaking *s,unsigned type){if(type==5)original_chicago_peaking_reset(s);}
int original_chicago_mastercomp_process(original_chicago_mastercomp *s,int16_t *l,int16_t *r,uint32_t n){
 if(!s||(n&&(!l||!r||l==r)))return 0;
 for(uint32_t i=0;i<n;i++){
  while(s->queue.cursor<s->queue.count&&s->queue.events[s->queue.cursor].offset<=i){if(s->queue.events[s->queue.cursor].type==4)original_chicago_mastercomp_reset(s);s->queue.cursor++;}
  int16_t a=l[i],b=r[i];master_frame(s,a,b,&l[i],&r[i]);
 }return 1;
}
int original_chicago_peaking_process(original_chicago_peaking *s,int16_t *l,int16_t *r,uint32_t n){
 if(!s||(n&&(!l||!r||l==r)))return 0;
 for(uint32_t i=0;i<n;i++){
  while(s->queue.cursor<s->queue.count&&s->queue.events[s->queue.cursor].offset<=i){if(s->queue.events[s->queue.cursor].type==4)original_chicago_peaking_reset(s);s->queue.cursor++;}
  for(unsigned ch=0;ch<2;ch++){
   int16_t in=ch?r[i]:l[i];int32_t *h=s->history[ch];int32_t x=s32((uint32_t)(int32_t)in<<9);
   uint64_t sum=(uint64_t)(int64_t)s->coefficient[0]*(uint64_t)(int64_t)x;
   sum+=(uint64_t)(int64_t)s->coefficient[1]*(uint64_t)(int64_t)h[2];sum+=(uint64_t)(int64_t)s->coefficient[2]*(uint64_t)(int64_t)h[3];
   sum-=(uint64_t)(int64_t)s->coefficient[4]*(uint64_t)(int64_t)h[1];sum-=(uint64_t)(int64_t)s->coefficient[3]*(uint64_t)(int64_t)h[0];
   int32_t y=s32((uint32_t)(sum>>24));h[3]=h[2];h[2]=x;h[1]=h[0];h[0]=y;
   if(ch)r[i]=q9_clip(y);else l[i]=q9_clip(y);
  }
 }return 1;
}
int original_chicago_mastercomp_advance_zero(original_chicago_mastercomp *s,uint32_t n){
 (void)n;
 if(!s||s->queue.cursor<s->queue.count||s->previous_input[0]||s->previous_input[1]||
    s->highpass[0]||s->highpass[1]||sar32(s->sensitivity,15)!=s->target_sensitivity||
    sar32(s->gain,15)!=s->target_gain)return 0;
 int32_t decay=mul32(sar32(s->peak,15),32674),peak=decay>0?decay:0;
 int32_t env=s32((uint32_t)s->envelope-(uint32_t)mul32(sar32(s->envelope,15),s->attack)+
     (uint32_t)mul32((int32_t)((uint32_t)peak>>15),s->attack));
 int32_t sens=sar32(s->sensitivity,15),div=sens?32768/sens:0;
 uint32_t inverted=((uint32_t)env^UINT32_C(0x7fff0000))>>16;
 int32_t control=s32((uint32_t)sar32(mul32((int32_t)inverted,s->control),15)+((uint32_t)div<<4));
 if(control>32767)control=32767;
 if(control< -32767)control=-32767;
 return peak==s->peak&&env==s->envelope&&control==s->control;
}
int original_chicago_peaking_advance_zero(const original_chicago_peaking *s,uint32_t n,int16_t constant[2]){
 (void)n;if(!s||!constant||s->queue.cursor<s->queue.count)return 0;
 int16_t value[2];
 for(unsigned c=0;c<2;c++){
  const int32_t *h=s->history[c];
  if(h[2]||h[3]||h[0]!=h[1])return 0;
  /* The same unsigned Q24 recurrence as process(), with zero x histories.
   * Equality proves every future state/sample; negative residues survive. */
  uint64_t sum=0;
  sum-=(uint64_t)(int64_t)s->coefficient[4]*(uint64_t)(int64_t)h[1];
  sum-=(uint64_t)(int64_t)s->coefficient[3]*(uint64_t)(int64_t)h[0];
  int32_t y=s32((uint32_t)(sum>>24));
  if(y!=h[0])return 0;
  value[c]=q9_clip(y);
 }
 constant[0]=value[0];constant[1]=value[1];
 return 1;
}
