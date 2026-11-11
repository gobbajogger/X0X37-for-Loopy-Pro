/* SPDX-License-Identifier: GPL-3.0-only */
#include "oscillator_motion.h"
#include "binary32_classify.h"
#include "miami_fx_conversion.h"
#include <math.h>
#include "binary32_math.h"
#include <string.h>
#include <limits.h>
#ifdef ORIGINAL_USE_SOFT_FMA
#include "fused_reference.h"
#define ref_fma original_rne_fmaf
#else
#define ref_fma fmaf
#endif
static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
static float fl(uint32_t u){return original_math_float(u);}
static uint32_t bits(float f){return original_math_bits(f);}
/* Nonnegative finite binary32 values have the same integer ordering. Accept
 * negative zero explicitly, as the original floating comparisons do. */
static int oscillator_gain_valid(float gain){uint32_t u=bits(gain);return u<=bits(ORIGINAL_OSCILLATOR_GAIN_MAX)||u==UINT32_C(0x80000000);}
static int32_t signed32(uint32_t u){return u<=INT32_MAX?(int32_t)u:(int32_t)((int64_t)u-4294967296LL);}
static float phase_float(uint32_t u){return (float)signed32(u)*0x1p-31f;}
/* Prepared controls and finite waveforms bound this to [-2^31, 2^31].
 * The positive endpoint must saturate before the C cast; every other value
 * is representable after truncation, including the negative endpoint. */
static int32_t phase_step_int(float step){return step>=0x1p31f?INT32_MAX:(int32_t)step;}
static float saw(float p,float correction){return p-clamp((p*correction)*128.0f,-1,1);}
static void coefficients(uint32_t D,float *correction,float *triangle){
 *correction=1;
 if(D>0x00ffffffu){uint64_t q=UINT64_C(0x7fffffff000000)/D;*correction=(float)signed32((uint32_t)q)*0x1p-31f;}
 float a=(float)D*0x1p-31f;a=a-.024f;float b=a>0?a*8:0;*triangle=b>1?fl(0xbeb020c5):b*fl(0xbeb020c5);
}
static float prepared_wave(unsigned shape,float p,float width,float correction,float triangle){
 if(shape==0){float t=2.0f*(.5f-original_binary32_abs(p));float t2=t*t,t3=t*t2;return t+triangle*t3;}
 float value=saw(p,correction);
 if(shape>=2){float q=p+(width+width);if(q>1)q=q-2;value=value-saw(q,correction);}
 return value;
}
int original_waveform(unsigned shape,float p,uint32_t D,float width,float *out){
 if(!out||shape>3||!original_binary32_isfinite(p)||p< -1||p>1||!D||D>INT32_MAX||!original_binary32_isfinite(width)||width<0||width>1)return 0;
 float correction,triangle;coefficients(D,&correction,&triangle);*out=prepared_wave(shape,p,width,correction,triangle);return 1;
}
int original_oscillator_prepare(original_prepared_oscillators *out,const original_osc_controls *c){
 if(!out||!c||!original_binary32_isfinite(c->xmod)||c->xmod<0||c->xmod>.8999999761581421f)return 0;
 for(unsigned i=0;i<4;i++)if(c->shape[i]>3||!c->increment[i]||c->increment[i]>INT32_MAX||!original_binary32_isfinite(c->gain[i])||c->gain[i]<0||c->gain[i]>ORIGINAL_OSCILLATOR_GAIN_MAX||!original_binary32_isfinite(c->width[i])||c->width[i]<0||c->width[i]>1)return 0;
 original_prepared_oscillators n;memset(&n,0,sizeof n);n.controls=*c;
 for(unsigned i=0;i<4;i++)coefficients(c->increment[i],&n.correction[i],&n.triangle[i]);
 n.ready=1;*out=n;return 1;
}
static int sample_gains(original_oscillators *s,const original_prepared_oscillators *prepared,const float gain[4],unsigned sync,float *output){
 if(!s||!prepared||!prepared->ready||!output||sync>1)return 0;
 const original_osc_controls *c=&prepared->controls;original_oscillators next=*s;float w[4];
 for(unsigned i=0;i<4;i+=2){
  uint32_t old=next.phase[i];next.phase[i]-=c->increment[i];
  /* Zero gain: the shaper output is not heard. The even one still drives XMOD
   * when xmod != 0. With xmod == 0, a+b == b exactly (b > 0), so it is skipped.
   * Phases always advance as before. */
  float b=(float)c->increment[i+1]*0x1p-31f,step;
  if(gain[i]!=0.0f||c->xmod!=0.0f){
   w[i]=prepared_wave(c->shape[i],phase_float(next.phase[i]),c->width[i],prepared->correction[i],prepared->triangle[i]);
   float a=w[i]*c->xmod;step=clamp(a+b,-1,1)*0x1p31f;
  }else{w[i]=0.0f;step=clamp(b,-1,1)*0x1p31f;}
  next.phase[i+1]-=(uint32_t)phase_step_int(step);
  if(sync&&signed32(next.phase[i])>signed32(old))next.phase[i+1]=0x7fffffffu;
  w[i+1]=gain[i+1]!=0.0f?prepared_wave(c->shape[i+1],phase_float(next.phase[i+1]),c->width[i+1],prepared->correction[i+1],prepared->triangle[i+1]):0.0f;
 }
 /* A zero-gain term adds a signed zero: it changes no nonzero sum. */
 float mix=gain[0]!=0.0f?w[0]*(gain[0]*.5f):0.0f;for(unsigned i=1;i<4;i++)if(gain[i]!=0.0f)mix=mix+w[i]*(gain[i]*.5f);
 *s=next;*output=mix;return 1;
}
int original_oscillator_sample_prepared(original_oscillators *s,const original_prepared_oscillators *prepared,unsigned sync,float *output){
 if(!prepared)return 0;
 return sample_gains(s,prepared,prepared->controls.gain,sync,output);
}
int original_oscillator_sample_gains(original_oscillators *s,const original_prepared_oscillators *prepared,const float gain[4],unsigned sync,float *output){
 if(!gain)return 0;
 for(unsigned i=0;i<4;i++)if(!oscillator_gain_valid(gain[i]))return 0;
 return sample_gains(s,prepared,gain,sync,output);
}
int original_oscillator_sample(original_oscillators *s,const original_osc_controls *c,unsigned sync,float *output){
 original_prepared_oscillators p;if(!original_oscillator_prepare(&p,c))return 0;return original_oscillator_sample_prepared(s,&p,sync,output);
}
int original_mg_tempo(float *p,unsigned rate,float bpm,float fs,uint32_t D,unsigned *wrapped){
 static const float base[8]={3,3,3,3,4,4,4,4},times[8]={16,8,4,2,8,4,2,1};
 if(!p||!wrapped||rate>7||!original_binary32_isfinite(*p)||*p< -1||*p>1||!original_binary32_isfinite(bpm)||bpm<20||bpm>400||!original_binary32_isfinite(fs)||fs<8000||fs>2147482880.0f||!D||D>(uint32_t)original_binary32_ceil_finite(fs/690.0f))return 0;
 float numerator=(bpm*fl(0x3f4ccccd))*(float)D;float denominator=(fs*base[rate])*times[rate];
 float delta=original_binary32_min_one_finite(numerator/denominator);float next=*p+delta;unsigned wrap=next>=1;if(wrap)next=next-2;
 *p=next;*wrapped=wrap;return 1;
}
int original_glide(float current[4],const float target[4],float port,float mod,float fs,uint32_t D){
 if(!current||!target||!original_binary32_isfinite(port)||!original_binary32_isfinite(mod)||!original_binary32_isfinite(fs)||fs<8000||fs>2147482880.0f||!D||D>(uint32_t)original_binary32_ceil_finite(fs/690.0f))return 0;
 for(unsigned i=0;i<4;i++)if(!original_binary32_isfinite(current[i])||current[i]<0||current[i]>127||!original_binary32_isfinite(target[i])||target[i]<0||target[i]>127)return 0;
 float sum=port+mod;if(!original_binary32_isfinite(sum))return 0;float t=clamp(sum,0,1),a=t*t;a=ref_fma(a,fl(0x49f42400),fl(0x3a83126f));a=fs*a;a=(float)D/a;a=a*fl(0x4a8ab75d);a=original_binary32_min_one_finite(a);
 float next[4];for(unsigned i=0;i<4;i++)next[i]=ref_fma(target[i]-current[i],a,current[i]);memcpy(current,next,sizeof next);return 1;
}
int original_pitch_increment(float pitch,const int32_t table[128],uint32_t *out){
 if(!table||!out||!original_binary32_isfinite(pitch))return 0;
 float p=clamp(pitch,0,120);unsigned j=(unsigned)p;
 if(table[j]<=0||table[j+1]<table[j])return 0;
 float f=p-(float)j;int32_t step=miami_fx_fcvtzs(bits((float)(table[j+1]-table[j])*f));
 int64_t result=(int64_t)table[j]+step;if(result>INT32_MAX)return 0;*out=(uint32_t)result;return 1;
}
int original_pw_width(unsigned shape,float intensity,float source,float pw,float mi,float mw,float *out){
 if(!out||shape>3||!original_binary32_isfinite(intensity)||!original_binary32_isfinite(source)||source< -1||source>1||!original_binary32_isfinite(pw)||!original_binary32_isfinite(mi)||!original_binary32_isfinite(mw)||!original_binary32_isfinite(intensity+mi)||!original_binary32_isfinite(pw+mw))return 0;
 float depth=clamp(intensity+mi,0,1),width=clamp(pw+mw,.01f,.99f);
 *out=shape<2?0:shape==2?(1-depth*source)*.5f:1-width;return 1;
}

int original_xmod_coefficient(float depth,float mod,float *out){
 if(!out||!original_binary32_isfinite(depth)||depth<0||depth>1||!original_binary32_isfinite(mod)||mod< -1||mod>1)return 0;
 *out=.8999999761581421f*clamp(ref_fma(depth,depth,mod),0,1);return 1;
}
