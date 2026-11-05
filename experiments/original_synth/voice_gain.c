/* SPDX-License-Identifier: GPL-3.0-only */
#include "voice_gain.h"
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
/* Integer control-span durations 1..64: exact old 1.0f/(float)D results.
 * 256 bytes in rodata; larger durations retain the original division. */
static const float control_span_inverse[64]={
 0x1.0000000000000p+0f, 0x1.0000000000000p-1f, 0x1.5555560000000p-2f, 0x1.0000000000000p-2f,
 0x1.99999a0000000p-3f, 0x1.5555560000000p-3f, 0x1.24924a0000000p-3f, 0x1.0000000000000p-3f,
 0x1.c71c720000000p-4f, 0x1.99999a0000000p-4f, 0x1.745d180000000p-4f, 0x1.5555560000000p-4f,
 0x1.3b13b20000000p-4f, 0x1.24924a0000000p-4f, 0x1.1111120000000p-4f, 0x1.0000000000000p-4f,
 0x1.e1e1e20000000p-5f, 0x1.c71c720000000p-5f, 0x1.af286c0000000p-5f, 0x1.99999a0000000p-5f,
 0x1.8618620000000p-5f, 0x1.745d180000000p-5f, 0x1.642c860000000p-5f, 0x1.5555560000000p-5f,
 0x1.47ae140000000p-5f, 0x1.3b13b20000000p-5f, 0x1.2f684c0000000p-5f, 0x1.24924a0000000p-5f,
 0x1.1a7b960000000p-5f, 0x1.1111120000000p-5f, 0x1.0842100000000p-5f, 0x1.0000000000000p-5f,
 0x1.f07c200000000p-6f, 0x1.e1e1e20000000p-6f, 0x1.d41d420000000p-6f, 0x1.c71c720000000p-6f,
 0x1.bacf920000000p-6f, 0x1.af286c0000000p-6f, 0x1.a41a420000000p-6f, 0x1.99999a0000000p-6f,
 0x1.8f9c180000000p-6f, 0x1.8618620000000p-6f, 0x1.7d05f40000000p-6f, 0x1.745d180000000p-6f,
 0x1.6c16c20000000p-6f, 0x1.642c860000000p-6f, 0x1.5c98820000000p-6f, 0x1.5555560000000p-6f,
 0x1.4e5e0a0000000p-6f, 0x1.47ae140000000p-6f, 0x1.4141420000000p-6f, 0x1.3b13b20000000p-6f,
 0x1.3521d00000000p-6f, 0x1.2f684c0000000p-6f, 0x1.29e4120000000p-6f, 0x1.24924a0000000p-6f,
 0x1.1f70480000000p-6f, 0x1.1a7b960000000p-6f, 0x1.15b1e60000000p-6f, 0x1.1111120000000p-6f,
 0x1.0c97140000000p-6f, 0x1.0842100000000p-6f, 0x1.0410420000000p-6f, 0x1.0000000000000p-6f,
};
static float duration_inverse(uint32_t D){
 return D>=1u&&D<=64u?control_span_inverse[D-1u]:1.0f/(float)D;
}
static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
static void target(original_ramp *r,float t,float inverse){r->target=t;r->delta=inverse*(t-r->current);}
static float advance(original_ramp *r){
 float n=r->current+r->delta;
 /* Original strict comparisons and XOR selection. In particular, a rounded
  * zero delta does not imply an unconditional assignment of the target. */
 if((r->delta>0)!=(r->target>n))n=r->target;
 r->current=n;return n;
}
void original_voice_gain_init(original_voice_gain *s){if(s)memset(s,0,sizeof *s);}
void original_voice_gain_miami_full_note(original_voice_gain *s){if(s)memset(s->stereo,0,sizeof s->stereo);}
int original_voice_gain_prepare(original_voice_gain *s,const original_voice_gain_controls *c,const int32_t active[4],uint32_t D){
 if(!s||!c||!active||!D||D>2147482880u||c->curve>2||c->ms20>1)return 0;
 const float values[]={c->mod_level,c->envelope,c->mod_gain,c->output,c->velocity,c->pan,c->mod_pan,c->spread,c->filter_gain};
 for(unsigned i=0;i<sizeof values/sizeof values[0];i++)if(!original_binary32_isfinite(values[i]))return 0;
 if(c->envelope< -1||c->envelope>1||c->output<0||c->output>1||c->velocity<0||c->velocity>1||c->spread<0||c->spread>1||c->filter_gain<1||c->filter_gain>4||(c->ms20&&c->filter_gain!=1))return 0;
 for(unsigned i=0;i<4;i++)if(!original_binary32_isfinite(c->level[i])||!original_binary32_isfinite(c->mod_specific[i])||!original_binary32_isfinite(s->oscillator[i].current)||s->oscillator[i].current<0||s->oscillator[i].current>ORIGINAL_OSCILLATOR_GAIN_MAX)return 0;
 for(unsigned i=0;i<2;i++)if(!original_binary32_isfinite(s->stereo[i].current)||s->stereo[i].current<0||s->stereo[i].current>4)return 0;
 original_voice_gain n=*s;float inverse=duration_inverse(D);
 for(unsigned i=0;i<4;i++) {
  if(!active[i]) {n.oscillator[i].target=0;n.oscillator[i].delta=(-n.oscillator[i].current)*inverse;continue;}
  float q=c->level[i]+c->mod_level;q=q+c->mod_specific[i];q=clamp(q,0,1);
  if(c->curve==0)q=q*q;
  else if(c->curve==2){float u=1-q;q=ref_fma(-u,u,1);}
  target(&n.oscillator[i],q*ORIGINAL_OSCILLATOR_GAIN_MAX,inverse);
 }
 float a=clamp(ref_fma(c->mod_gain,.5f,.5f),0,1),g=c->filter_gain*(a*a);
 g=(c->envelope*c->envelope)*g;g=c->output*g;g=g*(c->velocity*c->velocity);g=clamp(g,0,2);
 float p=clamp(ref_fma(c->mod_pan,2,c->pan),-1,1),u=p+1,v=2-u;
 float left=g*original_binary32_min_one_finite(v*v),right=g*original_binary32_min_one_finite(u*u);
 /* Miami unison split. Chicago selected mono uses flag1 and no split;
  * callers pass ms20=0, flag255 to retain its original full L/R weights. */
 if(c->unison_flag!=255) {
  float l=.5f,r=.5f;
  if(c->unison_flag==0){l=ref_fma(-c->spread,.5f,.5f);r=1-l;}
  else if(c->unison_flag==2){r=ref_fma(-c->spread,.5f,.5f);l=1-r;}
  left=l*left;right=r*right;
 }
 if(c->ms20){left=left+left;right=right+right;}
 target(&n.stereo[0],left,inverse);target(&n.stereo[1],right,inverse);*s=n;return 1;
}
int original_voice_gain_oscillator(original_voice_gain *s,original_oscillators *phase,const original_prepared_oscillators *p,unsigned sync,float *out){
 if(!s)return 0;
 float gain[4];for(unsigned i=0;i<4;i++)gain[i]=s->oscillator[i].current;
 if(!original_oscillator_sample_gains(phase,p,gain,sync,out))return 0;
 for(unsigned i=0;i<4;i++)(void)advance(&s->oscillator[i]);
 return 1;
}
int original_voice_gain_stereo(original_voice_gain *s,float filtered,float *left,float *right){
 if(!s||!left||!right||left==right||!original_binary32_isfinite(filtered))return 0;
 float l=advance(&s->stereo[0]),r=advance(&s->stereo[1]);*left=filtered*l;*right=filtered*r;return 1;
}
