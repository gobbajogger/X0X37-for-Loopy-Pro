/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_ms20.h"
#include "binary32_classify.h"
#include "miami_n513_private.h"
#include <math.h>
#include "binary32_math.h"
#include "miami_pow2_fma.h"
#include <string.h>
#ifdef ORIGINAL_USE_SOFT_FMA
#include "fused_reference.h"
#include "miami_interpolation.h"
#define ref_fma original_rne_fmaf
/* Exact optimized interpolation was previously qualified against the
 * independent reference. Preserve the full software FMA elsewhere. */
#define interpolate original_miami_interpolate_rne
#else
#define ref_fma fmaf
#define interpolate fmaf
#endif
static float fl(uint32_t u){return original_math_float(u);}
static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
static float lookup(const float *t,float q){if(q<=fl(0x3727c5ac))return t[0];if(q>=128)return t[128];unsigned j=(unsigned)q;return interpolate(t[j+1]-t[j],q-(float)j,t[j]);}
static void stage(original_ramp *r,float target,float reciprocal){r->target=target;r->delta=(target-r->current)*reciprocal;}
static float advance(const original_ramp *r){float q=r->current+r->delta;if(r->delta>0){if(q>=r->target)q=r->target;}else if(r->delta<0){if(q<=r->target)q=r->target;}else q=r->target;return q;}
int original_miami_nonlinear(float x,float *out){
 if(!out||!original_binary32_isfinite(x))return 0;
 float u=512.0f*original_binary32_abs(x),result;
 if(u<512){unsigned j=(unsigned)u;float a=fl(original_miami_n513_bits[j]),b=fl(original_miami_n513_bits[j+1]);float g=interpolate(b-a,u-(float)j,a);result=x*g;}
 else result=clamp(x,-1,1)*fl(original_miami_n513_bits[512]);
 *out=result;return 1;
}
int original_miami_mg_shape(unsigned ordinal,float phase,float *b,float *u){
 if(!b||!u||b==u||ordinal>3||!original_binary32_isfinite(phase)||phase< -1||phase>1)return 0;
 float bipolar,unipolar;
 if(ordinal==0){bipolar=original_miami_mg_twice_plus_one(phase<0?phase:-phase);unipolar=original_miami_mg_half_plus_half(bipolar);}
 else if(ordinal==1){bipolar=-phase;unipolar=original_miami_mg_half_plus_half(-phase);}
 else if(ordinal==2){bipolar=phase;unipolar=original_miami_mg_half_plus_half(phase);}
 else {bipolar=phase>=0?-1:1;unipolar=phase>=0?0:1;}
 *b=bipolar;*u=unipolar;return 1;
}
int original_miami_ms20_init(original_miami_ms20 *s,unsigned index,float a0,float b0,float cap0){
 if(!s||index>=128||!original_binary32_isfinite(a0)||!original_binary32_isfinite(b0)||!original_binary32_isfinite(cap0))return 0;
 memset(s,0,sizeof *s);s->noise=index+1;s->noise_gain.current=fl(0x3727c5ac);s->a.current=s->a.target=a0;s->b=b0;s->cap=cap0;return 1;
}
int original_miami_ms20_note_reset(original_miami_ms20 *s,float a0){
 if(!s||!original_binary32_isfinite(a0))return 0;
 memset(s->history,0,sizeof s->history);s->a.current=s->a.target=a0;s->a.delta=0;
 s->resonance=(original_ramp){0,0,0};s->prepared=0;return 1;
}
int original_miami_ms20_prepare(original_miami_ms20 *s,const original_ms20_tables *t,
 float cutoff,float resonance,float depth,float env,float md,float mc,float mr,float noise,float mn,uint32_t duration){
 if(!s||!t||!t->a||!t->b||!t->r||!t->cap||!duration||duration>2147482880u)return 0;
 float controls[]={cutoff,resonance,depth,env,md,mc,mr,noise,mn};for(unsigned i=0;i<9;i++)if(!original_binary32_isfinite(controls[i]))return 0;
 for(unsigned i=0;i<129;i++)if(!original_binary32_isfinite(t->a[i])||!original_binary32_isfinite(t->b[i])||!original_binary32_isfinite(t->r[i])||!original_binary32_isfinite(t->cap[i]))return 0;
 float r=resonance+mr;if(!original_binary32_isfinite(r)||r<0||r>1)return 0;
 float d=clamp(depth+md,-1,1),z=ref_fma(env,d,cutoff),q=ref_fma(mc,128,z*128.0f);
 if(!original_binary32_isfinite(q))return 0;
 float a=lookup(t->a,q),b=lookup(t->b,q),cap=lookup(t->cap,q),R;
 if(r==1)R=t->r[128];else{float rq=r*128.0f;unsigned j=(unsigned)rq;R=interpolate(t->r[j+1]-t->r[j],rq-(float)j,t->r[j]);}
 float target=R<=fl(0x3727c5ac)?fl(0x3727c5ac):(R<cap?R:cap);
 float ng=clamp(noise+mn,0,1);ng=ng*ng+fl(0x3727c5ac);float reciprocal=1.0f/(float)duration;
 stage(&s->a,a,reciprocal);stage(&s->resonance,target,reciprocal);stage(&s->noise_gain,ng,reciprocal);s->b=b;s->cap=cap;s->prepared=1;return 1;
}
int original_miami_ms20_sample(original_miami_ms20 *s,float osc,float gain,float *out){
 if(!s||!out||!s->prepared||!original_binary32_isfinite(osc)||!original_binary32_isfinite(gain))return 0;
 /* Calculate without publishing state: a failed sample must leave every field
  * unchanged. Only the ramp currents, noise and history change on success;
  * avoid copying the complete filter object twice on this sample path. */
 float g=advance(&s->a),r=advance(&s->resonance),ng=advance(&s->noise_gain);
 unsigned choose=(s->noise&0x40000000u)==0;uint32_t next_noise=(s->noise+s->noise)^(choose?8393u:0);
 float noise=choose?.5f:-.5f;float v=ng*noise+osc;v=gain*v;
 float x=v*fl(0x3fa47678);x=x*fl(0x3dcccccd);
 float a=x+g*(s->history[0]-x);float b=a+g*(s->history[1]-a),nonlinear;
 if(!original_miami_nonlinear(s->history[2]+b,&nonlinear))return 0;
 float feedback=r*nonlinear,difference=feedback-s->history[4];
 float d=difference+s->b*(s->history[3]+difference);float c=d+s->b*(s->history[2]-d);
 if(!original_binary32_isfinite(a)||!original_binary32_isfinite(b)||!original_binary32_isfinite(c)||!original_binary32_isfinite(d)||!original_binary32_isfinite(feedback))return 0;
 s->a.current=g;s->resonance.current=r;s->noise_gain.current=ng;s->noise=next_noise;
 s->history[0]=a;s->history[1]=b;s->history[2]=c;s->history[3]=d;s->history[4]=feedback;
 /* The output may alias a float field of s, as before: publish it last. */
 *out=nonlinear;return 1;
}
