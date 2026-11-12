/* SPDX-License-Identifier: GPL-3.0-only */
/* Bit identity: original_chicago_voice_span (held-ramp path and general path)
 * vs a private copy of the original per-sample ck_step. */
#include "chicago_mp4.h"
#include "voice_gain.h"
#include "oscillator_motion.h"
#include "binary32_math.h"
#include "binary32_classify.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int original_chicago_voice_span(original_voice_gain *vg,original_oscillators *ph,const original_prepared_oscillators *p,
 original_ramp *noise_gain,uint32_t *noise_state,float vco_gain,original_chicago_mp4 *f,uint32_t *left,uint32_t *right,uint32_t n);
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line %d case %u\n",__LINE__,checks);exit(1);}}while(0)
static unsigned checks;
static uint32_t seed=0x6d70u;
static uint32_t rng(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static float fl(uint32_t u){return original_math_float(u);}
static float frand(void){return fl((0x3f000000u+(rng()&0x7fffffu))&0x3fffffff);} /* (0.5,2) ish */
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
static float advance(original_ramp *r){float q=r->current+r->delta;if(r->delta>0){if(q>=r->target)q=r->target;}else if(r->delta<0){if(q<=r->target)q=r->target;}else q=r->target;r->current=q;return q;}
/* Exact copy of the original ck_step (general path). */
static int ref_step(const original_prepared_oscillators *p,float vco_gain,
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
static void set_ramp(original_ramp *r,float c,float t,float d){r->current=c;r->target=t;r->delta=d;}
static void held(original_ramp *r,float v){set_ramp(r,v,v,0.0f);}
static int eq_ramp(const original_ramp *a,const original_ramp *b){
 return original_math_bits(a->current)==original_math_bits(b->current)
  &&original_math_bits(a->target)==original_math_bits(b->target)
  &&original_math_bits(a->delta)==original_math_bits(b->delta);
}
static void fill_prep(original_prepared_oscillators *p,unsigned shape,uint32_t inc,float width,float xmod){
 memset(p,0,sizeof *p);
 for(unsigned i=0;i<4;i++){
  p->controls.increment[i]=inc+(i*65537u);
  p->controls.gain[i]=0.5f;
  p->controls.width[i]=width;
  p->controls.shape[i]=(uint8_t)((shape+i)&3);
  p->correction[i]=frand()*(1.0f/128.0f);
  p->triangle[i]=frand()*0.25f;
 }
 p->controls.xmod=xmod;
 p->ready=1;
}
static void compare(int held_ramps,uint32_t n,unsigned shape,int mid_fail){
 original_prepared_oscillators p;fill_prep(&p,shape,0x01000000u+(rng()&0xffffffu),(float)(rng()&0xff)/256.0f,(rng()&1)?0.0f:frand()*0.3f);
 original_voice_gain vg,vg_r;original_oscillators ph,ph_r;original_ramp ng,ng_r;
 original_chicago_mp4 f,f_r;uint32_t ns,ns_r;
 memset(&vg,0,sizeof vg);memset(&ph,0,sizeof ph);memset(&f,0,sizeof f);
 for(unsigned i=0;i<4;i++)ph.phase[i]=rng();
 ns=rng()|1u;
 f.prepared=1;
 if(held_ramps){
  for(unsigned i=0;i<4;i++)held(&vg.oscillator[i],(rng()&7)?frand()*0.8f:0.0f);
  held(&vg.stereo[0],frand());held(&vg.stereo[1],frand());
  held(&ng,frand()*0.2f);
  held(&f.a,frand()*0.8f);held(&f.e,frand()*0.6f-0.3f);held(&f.resonance,frand()*0.7f);
 }else{
  for(unsigned i=0;i<4;i++)set_ramp(&vg.oscillator[i],frand()*0.5f,frand()*0.8f,(frand()-1.0f)*0.01f);
  set_ramp(&vg.stereo[0],frand(),frand(),(frand()-1.0f)*0.01f);
  set_ramp(&vg.stereo[1],frand(),frand(),(frand()-1.0f)*0.01f);
  set_ramp(&ng,frand()*0.2f,frand()*0.2f,(frand()-1.0f)*0.001f);
  set_ramp(&f.a,frand()*0.8f,frand()*0.8f,(frand()-1.0f)*0.01f);
  set_ramp(&f.e,frand()*0.4f-0.2f,frand()*0.4f-0.2f,(frand()-1.0f)*0.01f);
  set_ramp(&f.resonance,frand()*0.7f,frand()*0.7f,(frand()-1.0f)*0.01f);
 }
 for(unsigned i=0;i<5;i++)f.history[i]=(mid_fail&&i==0)?fl(0x7f000000u):((float)(int32_t)rng())*0x1p-40f;
 vg_r=vg;ph_r=ph;ng_r=ng;f_r=f;ns_r=ns;
 uint32_t *L=malloc(n*4),*R=malloc(n*4),*Lr=malloc(n*4),*Rr=malloc(n*4);
 if(!L||!R||!Lr||!Rr){fprintf(stderr,"oom\n");exit(1);}
 float vco=(rng()&3)?1.0f+frand():0.0f;
 int ok=original_chicago_voice_span(&vg,&ph,&p,&ng,&ns,vco,&f,L,R,n);
 int okr=1;
 for(uint32_t k=0;k<n;k++){
  if(!ref_step(&p,vco,ph_r.phase,vg_r.oscillator,vg_r.stereo,&ng_r,&ns_r,f_r.history,&f_r.a,&f_r.e,&f_r.resonance,Lr+k,Rr+k)){
   okr=0;break;
  }
 }
 if(ok!=okr){fprintf(stderr,"ok %d vs %d n=%u held=%d\n",ok,okr,n,held_ramps);free(L);free(R);free(Lr);free(Rr);CHECK(0);}
 uint32_t upto=ok?n:0;
 if(!ok){
  /* both failed: find how far the product wrote. The product stops at the
   * first failure and still writes back committed state. Compare state only. */
 }else{
  for(uint32_t k=0;k<n;k++){
   if(L[k]!=Lr[k]||R[k]!=Rr[k]){fprintf(stderr,"pcm k=%u L %08x/%08x R %08x/%08x held=%d\n",k,L[k],Lr[k],R[k],Rr[k],held_ramps);CHECK(0);}
  }
 }
 CHECK(ph.phase[0]==ph_r.phase[0]&&ph.phase[1]==ph_r.phase[1]&&ph.phase[2]==ph_r.phase[2]&&ph.phase[3]==ph_r.phase[3]);
 CHECK(ns==ns_r);
 for(unsigned i=0;i<4;i++)CHECK(eq_ramp(&vg.oscillator[i],&vg_r.oscillator[i]));
 CHECK(eq_ramp(&vg.stereo[0],&vg_r.stereo[0])&&eq_ramp(&vg.stereo[1],&vg_r.stereo[1]));
 CHECK(eq_ramp(&ng,&ng_r)&&eq_ramp(&f.a,&f_r.a)&&eq_ramp(&f.e,&f_r.e)&&eq_ramp(&f.resonance,&f_r.resonance));
 for(unsigned i=0;i<5;i++)CHECK(original_math_bits(f.history[i])==original_math_bits(f_r.history[i]));
 (void)upto;
 free(L);free(R);free(Lr);free(Rr);
 checks++;
}
int main(void){
 const uint32_t lens[]={1,2,3,7,8,15,16,31,32,63,64,65,127,128,255,256,257,512};
 for(unsigned h=0;h<2;h++)
  for(unsigned s=0;s<4;s++)
   for(unsigned i=0;i<sizeof lens/sizeof lens[0];i++)
    compare((int)h,lens[i],s,0);
 for(unsigned i=0;i<4000;i++){
  uint32_t n=1+(rng()%512);
  compare((int)(rng()&1),n,rng()&3,(int)((rng()%50)==0));
 }
 /* n=1 kernel path (held and moving) */
 for(unsigned i=0;i<200;i++)compare((int)(i&1),1,i&3,0);
 printf("Chicago span hold identity: %u exact comparisons PASS\n",checks);
 return 0;
}
