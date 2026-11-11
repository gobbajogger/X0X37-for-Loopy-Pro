/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_mp4.h"
#include "binary32_classify.h"
#include <math.h>
#include "binary32_math.h"
#include <string.h>
#ifdef ORIGINAL_USE_SOFT_FMA
#include "fused_reference.h"
#define ref_fma original_rne_fmaf
#else
#define ref_fma fmaf
#endif
static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
static void stage(original_ramp *r,float target,float reciprocal){r->target=target;r->delta=(target-r->current)*reciprocal;}
static float advance(original_ramp *r){float q=r->current+r->delta;if(r->delta>0){if(q>=r->target)q=r->target;}else if(r->delta<0){if(q<=r->target)q=r->target;}else q=r->target;r->current=q;return q;}
int original_chicago_feedback(float input,float *out){
 if(!out||!original_binary32_isfinite(input))return 0;
 float u=original_binary32_abs(input)>=1?0:1-original_binary32_abs(input);float u2=u*u,u4=u2*u2,u5=u*u4;float v=1-u5;*out=input>=0?v:-v;return 1;
}
int original_chicago_mp4_init(original_chicago_mp4 *s,float a0,float e0){
 if(!s||!original_binary32_isfinite(a0)||!original_binary32_isfinite(e0)||a0<0||a0>1||e0< -1||e0>1)return 0;
 memset(s,0,sizeof *s);s->a.current=s->a.target=a0;s->e.current=s->e.target=e0;return 1;
}
int original_chicago_mp4_prepare_reader(original_chicago_mp4 *s,original_mp4_reader read,const void *context,
 float cutoff,float depth,float env,float md,float mc,int note,float track,float resonance,float mr,uint32_t duration,unsigned kick,float *postgain){
 if(!s||!read||!postgain||!duration||duration>2147482880u||kick>1||note<0||note>127)return 0;
 float controls[]={cutoff,depth,env,md,mc,track,resonance,mr};for(unsigned i=0;i<8;i++)if(!original_binary32_isfinite(controls[i]))return 0;
 float sum=depth+md;if(!original_binary32_isfinite(sum)||!original_binary32_isfinite(resonance+mr))return 0;
 float d=clamp(sum,-1,1),z=ref_fma(env,d,cutoff),q=z*16384.0f,key=(float)(note-48)*128.0f;
 q=ref_fma(key,track,q);q=ref_fma(mc,16384.0f,q);if(!original_binary32_isfinite(q))return 0;
 q=clamp(q,0,16384);unsigned j=(unsigned)q;float a,e;if(!read(context,j,&a,&e))return 0;
 if(!original_binary32_isfinite(a)||a<0||a>1||!original_binary32_isfinite(e)||e< -1||e>1)return 0;
 float R=clamp(resonance+mr,0,1),reciprocal=1.0f/(float)duration;
 original_chicago_mp4 next=*s;stage(&next.a,a,reciprocal);stage(&next.e,e,reciprocal);stage(&next.resonance,R,reciprocal);
 if(kick){float r2=R*R,r4=r2*r2,r8=r4*r4;next.history[0]=R>.2f?r8:0;}
 next.prepared=1;*s=next;*postgain=clamp(ref_fma(resonance+mr,4,1),1,4);return 1;
}
static int raw_table_read(const void *context,uint32_t index,float *a,float *e){
 const original_mp4_tables *t=context;if(!t||!t->a||!t->e||index>16384)return 0;*a=t->a[index];*e=t->e[index];return 1;
}
int original_chicago_mp4_prepare(original_chicago_mp4 *s,const original_mp4_tables *tables,
 float cutoff,float depth,float env,float md,float mc,int note,float track,float resonance,float mr,uint32_t duration,unsigned kick,float *postgain){
 return original_chicago_mp4_prepare_reader(s,raw_table_read,tables,cutoff,depth,env,md,mc,note,track,resonance,mr,duration,kick,postgain);
}
int original_chicago_mp4_sample(original_chicago_mp4 *s,float input,float *out){
 if(!s||!out||!s->prepared||!original_binary32_isfinite(input))return 0;
 original_chicago_mp4 n=*s;float A=advance(&n.a),E=advance(&n.e),R=advance(&n.resonance);
 float tmp=n.history[3]+E*n.history[4],d=n.history[4]-E*tmp,nonlinear;
 if(!original_chicago_feedback(d,&nonlinear))return 0;
 float v=input-R*nonlinear,z1=v+A*(n.history[0]-v),z2=z1+A*(n.history[1]-z1),z3=z2+A*(n.history[2]-z2),z4=z3+A*(n.history[3]-z3);
 if(!original_binary32_isfinite(z1)||!original_binary32_isfinite(z2)||!original_binary32_isfinite(z3)||!original_binary32_isfinite(z4)||!original_binary32_isfinite(tmp))return 0;
 n.history[0]=z1;n.history[1]=z2;n.history[2]=z3;n.history[3]=z4;n.history[4]=tmp;*s=n;*out=z4;return 1;
}

/* One Chicago voice sample: oscillators (sync 1), noise, MP4 filter and
 * stereo gain. The same float operations in the same order as
 * original_voice_gain_oscillator + the noise step + original_chicago_mp4_sample
 * + original_voice_gain_stereo, without the per-call state copies.
 * Returns 0 (state unchanged except as the original would fail) on a
 * nonfinite filter value. */
#include "voice_gain.h"
#include "oscillator_motion.h"
static inline float ck_clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
static inline float ck_saw(float p,float correction){return p-ck_clamp((p*correction)*128.0f,-1,1);}
static inline float ck_wave(unsigned shape,float p,float width,float correction,float triangle){
 if(shape==0){float t=2.0f*(.5f-original_binary32_abs(p));float t2=t*t,t3=t*t2;return t+triangle*t3;}
 float value=ck_saw(p,correction);
 if(shape>=2){float q=p+(width+width);if(q>1)q=q-2;value=value-ck_saw(q,correction);}
 return value;
}
static inline float ck_phase(uint32_t u){return (float)(int32_t)u*0x1p-31f;}
static inline float ck_xadvance(original_ramp *r){float n=r->current+r->delta;if((r->delta>0)!=(r->target>n))n=r->target;r->current=n;return n;}
/* One sample. Locals stand in for the voice state; the caller loads and
 * stores them once per span. Commit order matches the original: phases,
 * gain ramps and noise advance before the filter checks; the filter state
 * changes only when every filter value is finite. */
static inline __attribute__((always_inline)) int ck_step(const original_prepared_oscillators *p,float vco_gain,
 uint32_t phase[4],original_ramp og[4],original_ramp st[2],original_ramp *ngr,uint32_t *nsp,
 float H[5],original_ramp *ra,original_ramp *re,original_ramp *rr,uint32_t *lo,uint32_t *ro){
 const original_osc_controls *c=&p->controls;float w[4],gain[4];
 for(unsigned i=0;i<4;i++)gain[i]=og[i].current;
 for(unsigned i=0;i<4;i+=2){
  uint32_t old=phase[i];phase[i]-=c->increment[i];
  float b=(float)c->increment[i+1]*0x1p-31f,step;
  if(gain[i]!=0.0f||c->xmod!=0.0f){
   w[i]=ck_wave(c->shape[i],ck_phase(phase[i]),c->width[i],p->correction[i],p->triangle[i]);
   float a=w[i]*c->xmod;step=ck_clamp(a+b,-1,1)*0x1p31f;
  }else{w[i]=0.0f;step=ck_clamp(b,-1,1)*0x1p31f;}
  phase[i+1]-=(uint32_t)(step>=0x1p31f?INT32_MAX:(int32_t)step);
  if((int32_t)phase[i]>(int32_t)old)phase[i+1]=0x7fffffffu;
  w[i+1]=gain[i+1]!=0.0f?ck_wave(c->shape[i+1],ck_phase(phase[i+1]),c->width[i+1],p->correction[i+1],p->triangle[i+1]):0.0f;
 }
 float mix=gain[0]!=0.0f?w[0]*(gain[0]*.5f):0.0f;for(unsigned i=1;i<4;i++)if(gain[i]!=0.0f)mix=mix+w[i]*(gain[i]*.5f);
 for(unsigned i=0;i<4;i++)(void)ck_xadvance(&og[i]);
 float ng=ck_xadvance(ngr);uint32_t ns=*nsp;unsigned choose=(ns&0x40000000u)==0;*nsp=(ns+ns)^(choose?8393u:0);
 float input=ng*(choose?.5f:-.5f)+mix;input=vco_gain*input;
 if(!original_binary32_isfinite(input))return 0;
 original_ramp a2=*ra,e2=*re,r2=*rr;
 float A=advance(&a2),E=advance(&e2),R=advance(&r2);
 float tmp=H[3]+E*H[4],d=H[4]-E*tmp;
 if(!original_binary32_isfinite(d))return 0;
 float u=original_binary32_abs(d)>=1?0:1-original_binary32_abs(d);float u2=u*u,u4=u2*u2,u5=u*u4;float v5=1-u5;float nonlinear=d>=0?v5:-v5;
 float v=input-R*nonlinear,z1=v+A*(H[0]-v),z2=z1+A*(H[1]-z1),z3=z2+A*(H[2]-z2),z4=z3+A*(H[3]-z3);
 if(!original_binary32_isfinite(z1)||!original_binary32_isfinite(z2)||!original_binary32_isfinite(z3)||!original_binary32_isfinite(z4)||!original_binary32_isfinite(tmp))return 0;
 *ra=a2;*re=e2;*rr=r2;H[0]=z1;H[1]=z2;H[2]=z3;H[3]=z4;H[4]=tmp;
 float l=ck_xadvance(&st[0]),r=ck_xadvance(&st[1]);
 *lo=original_math_bits(z4*l);*ro=original_math_bits(z4*r);return 1;
}
/* n samples (n>=1) into bit-pattern outputs. Returns 0 at the first
 * nonfinite filter value, with state committed as the per-sample path. */
int original_chicago_voice_span(original_voice_gain *vg,original_oscillators *ph,const original_prepared_oscillators *p,
 original_ramp *noise_gain,uint32_t *noise_state,float vco_gain,original_chicago_mp4 *f,uint32_t *left,uint32_t *right,uint32_t n){
 if(!p->ready||!f->prepared)return 0;
 uint32_t phase[4]={ph->phase[0],ph->phase[1],ph->phase[2],ph->phase[3]};
 original_ramp og[4]={vg->oscillator[0],vg->oscillator[1],vg->oscillator[2],vg->oscillator[3]},st[2]={vg->stereo[0],vg->stereo[1]};
 original_ramp ngr=*noise_gain,ra=f->a,re=f->e,rr=f->resonance;uint32_t ns=*noise_state;
 float H[5]={f->history[0],f->history[1],f->history[2],f->history[3],f->history[4]};
 int ok=1;
 for(uint32_t k=0;k<n;k++)if(!ck_step(p,vco_gain,phase,og,st,&ngr,&ns,H,&ra,&re,&rr,left+k,right+k)){ok=0;break;}
 for(unsigned i=0;i<4;i++){ph->phase[i]=phase[i];vg->oscillator[i]=og[i];}
 vg->stereo[0]=st[0];vg->stereo[1]=st[1];*noise_gain=ngr;*noise_state=ns;
 f->a=ra;f->e=re;f->resonance=rr;for(unsigned i=0;i<5;i++)f->history[i]=H[i];
 return ok;
}
int original_chicago_voice_kernel(original_voice_gain *vg,original_oscillators *ph,const original_prepared_oscillators *p,
 original_ramp *noise_gain,uint32_t *noise_state,float vco_gain,original_chicago_mp4 *f,float *left,float *right){
 uint32_t l,r;if(!original_chicago_voice_span(vg,ph,p,noise_gain,noise_state,vco_gain,f,&l,&r,1))return 0;
 *left=original_math_float(l);*right=original_math_float(r);return 1;
}
