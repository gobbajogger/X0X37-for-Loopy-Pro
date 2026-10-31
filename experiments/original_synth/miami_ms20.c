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

/* Block kernel for one active Miami voice: oscillators (sync 0), MS-20 filter
 * and stereo gain, added into L/R. The same float operations in the same
 * order as original_voice_gain_oscillator + original_miami_ms20_sample +
 * original_voice_gain_stereo per sample, with the state kept in locals.
 * Finite checks run once at the end of the block (0: fault). */
#include "voice_gain.h"
#include "oscillator_motion.h"
static inline float k_saw(float p,float correction){return p-clamp((p*correction)*128.0f,-1,1);}
static inline float k_wave(unsigned shape,float p,float width,float correction,float triangle){
 if(shape==0){float t=2.0f*(.5f-original_binary32_abs(p));float t2=t*t,t3=t*t2;return t+triangle*t3;}
 float value=k_saw(p,correction);
 if(shape>=2){float q=p+(width+width);if(q>1)q=q-2;value=value-k_saw(q,correction);}
 return value;
}
static inline float k_phase(uint32_t u){return (float)(int32_t)u*0x1p-31f;}
static inline float k_gain_advance(original_ramp *r){
 float n=r->current+r->delta;if((r->delta>0)!=(r->target>n))n=r->target;r->current=n;return n;
}
int original_miami_voice_render_add(original_miami_ms20 *f,original_voice_gain *vg,original_oscillators *ph,
 const original_prepared_oscillators *p,float vco_gain,float *L,float *R,uint32_t n){
 if(!f->prepared||!p->ready||!original_binary32_isfinite(vco_gain))return 0;
 const original_osc_controls *c=&p->controls;
 original_ramp og[4],st[2];memcpy(og,vg->oscillator,sizeof og);memcpy(st,vg->stereo,sizeof st);
 uint32_t phase[4];memcpy(phase,ph->phase,sizeof phase);
 original_ramp ra=f->a,rr=f->resonance,rn=f->noise_gain;
 float h0=f->history[0],h1=f->history[1],h2=f->history[2],h3=f->history[3],h4=f->history[4],fb=f->b;
 uint32_t noise=f->noise;float xmod=c->xmod;
 float b1=(float)c->increment[1]*0x1p-31f,b3=(float)c->increment[3]*0x1p-31f;
 float st1=clamp(b1,-1,1)*0x1p31f,st3=clamp(b3,-1,1)*0x1p31f;
 int32_t pstep1=st1>=0x1p31f?INT32_MAX:(int32_t)st1,pstep3=st3>=0x1p31f?INT32_MAX:(int32_t)st3;
 for(uint32_t j=0;j<n;j++){
  float gain[4]={og[0].current,og[1].current,og[2].current,og[3].current},w[4];
  for(unsigned i=0;i<4;i+=2){
   float b=i?b3:b1;int32_t step_i;
   phase[i]-=c->increment[i];
   if(gain[i]!=0.0f||xmod!=0.0f){
    w[i]=k_wave(c->shape[i],k_phase(phase[i]),c->width[i],p->correction[i],p->triangle[i]);
    float a=w[i]*xmod;float step=clamp(a+b,-1,1)*0x1p31f;step_i=step>=0x1p31f?INT32_MAX:(int32_t)step;
   }else{w[i]=0.0f;step_i=i?pstep3:pstep1;}
   phase[i+1]-=(uint32_t)step_i;
   w[i+1]=gain[i+1]!=0.0f?k_wave(c->shape[i+1],k_phase(phase[i+1]),c->width[i+1],p->correction[i+1],p->triangle[i+1]):0.0f;
  }
  float mix=gain[0]!=0.0f?w[0]*(gain[0]*.5f):0.0f;for(unsigned i=1;i<4;i++)if(gain[i]!=0.0f)mix=mix+w[i]*(gain[i]*.5f);
  for(unsigned i=0;i<4;i++)(void)k_gain_advance(&og[i]);
  /* MS-20 */
  float g=advance(&ra),r=advance(&rr),ng=advance(&rn);ra.current=g;rr.current=r;rn.current=ng;
  unsigned choose=(noise&0x40000000u)==0;noise=(noise+noise)^(choose?8393u:0);
  float nz=choose?.5f:-.5f;float v=ng*nz+mix;v=vco_gain*v;
  float x=v*fl(0x3fa47678);x=x*fl(0x3dcccccd);
  float a=x+g*(h0-x);float b=a+g*(h1-a);
  float in=h2+b,u=512.0f*original_binary32_abs(in),nl;
  if(u<512){unsigned k=(unsigned)u;float ta=fl(original_miami_n513_bits[k]),tb=fl(original_miami_n513_bits[k+1]);float gg=interpolate(tb-ta,u-(float)k,ta);nl=in*gg;}
  else nl=clamp(in,-1,1)*fl(original_miami_n513_bits[512]);
  float feedback=r*nl,difference=feedback-h4;
  float d=difference+fb*(h3+difference);float cc=d+fb*(h2-d);
  h0=a;h1=b;h2=cc;h3=d;h4=feedback;
  float l=k_gain_advance(&st[0]),rg=k_gain_advance(&st[1]);
  L[j]=L[j]+nl*l;R[j]=R[j]+nl*rg;
 }
 if(!original_binary32_isfinite(h0)||!original_binary32_isfinite(h1)||!original_binary32_isfinite(h2)||
    !original_binary32_isfinite(h3)||!original_binary32_isfinite(h4))return 0;
 memcpy(vg->oscillator,og,sizeof og);memcpy(vg->stereo,st,sizeof st);memcpy(ph->phase,phase,sizeof phase);
 f->a=ra;f->resonance=rr;f->noise_gain=rn;f->noise=noise;
 f->history[0]=h0;f->history[1]=h1;f->history[2]=h2;f->history[3]=h3;f->history[4]=h4;
 return 1;
}
