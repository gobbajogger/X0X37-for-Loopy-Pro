/* SPDX-License-Identifier: GPL-3.0-only */
#include "juno6.h"
#include <math.h>
#include "fastmath.h"
#include <string.h>
#define PI 3.14159265358979323846f
#define TWO_PI 6.28318530717958647692f
static float clampf(float x,float a,float b){return x<a?a:x>b?b:x;}
static float midi_hz(unsigned n){return 440.0f*fm_exp2f(((float)n-69.0f)*(1.0f/12.0f));}
static float blep(float t,float dt){if(t<dt){t/=dt;return t+t-t*t-1.0f;}if(t>1.0f-dt){t=(t-1.0f)/dt;return t*t+t+t+1.0f;}return 0.0f;}
void juno6_init(juno6_t*s,float sr){memset(s,0,sizeof *s);s->sample_rate=sr>8000?sr:44100;s->attack=.008f;s->decay=.28f;s->sustain=.72f;s->release=.24f;s->cutoff=.68f;s->resonance=.12f;s->pulse_width=.5f;s->sub_level=.22f;s->macro[0]=.35f;s->macro[4]=.35f;s->macro[10]=.12f;s->macro[15]=1.0f;s->macro[16]=.5f;}
int juno6_note_on(juno6_t*s,uint8_t note,uint8_t vel){unsigned i,chosen=0;uint32_t oldest;if(!vel){juno6_note_off(s,note);return 1;}for(i=0;i<6;i++)if(s->voice[i].gate&&s->voice[i].note==note){chosen=i;goto assign;}for(i=0;i<6;i++)if(!s->voice[i].gate&&s->voice[i].envelope<.0001f){chosen=i;goto assign;}for(i=0;i<6;i++)if(!s->voice[i].gate){chosen=i;goto assign;}oldest=s->voice[0].age;for(i=1;i<6;i++)if(s->voice[i].age<oldest){oldest=s->voice[i].age;chosen=i;}assign:{juno6_voice_t*v=&s->voice[chosen];v->note=note;v->gate=1;v->stage=1;v->age=++s->age;v->increment=midi_hz(note)/s->sample_rate*fm_exp2f(s->bend*(1.0f/12.0f));v->velocity=(float)vel/127.0f;if(v->envelope<.0001f)v->phase=0;}return 1;}
void juno6_note_off(juno6_t*s,uint8_t note){unsigned i;for(i=0;i<6;i++)if(s->voice[i].gate&&s->voice[i].note==note){s->voice[i].gate=0;if(!s->damper)s->voice[i].stage=4;}}
void juno6_all_off(juno6_t *s)
{
 memset(s->voice,0,sizeof s->voice);
 s->damper=0;s->macro[17]=0;s->hp_state=s->hp_input=0;
}
void juno6_render(juno6_t *s, float *out, unsigned n)
{
 const float sr=s->sample_rate;
 const float attack=1.0f/fm_maxf(1,s->attack*sr);
 const float decay=(1-s->sustain)/fm_maxf(1,s->decay*sr);
 const float release=1.0f/fm_maxf(1,s->release*sr);
 const float base=35+s->cutoff*s->cutoff*12000;
 const float env=7000*s->macro[4];
 const float hp=1.0f/(1.0f+TWO_PI*(15+2000*s->hpf*s->hpf)/sr);
 const float rate=(.1f+8*s->macro[10]*s->macro[10])/sr;
 const float level=s->macro[15]/1.85f; /* same divide as before, once per block */
 const float pw=s->pulse_width;
 const float res8=s->resonance*.8f;
 const float saw_g=1-s->macro[0], pul_g=s->macro[0], sub_g=s->sub_level;
 for(unsigned k=0;k<n;k++) {
  float mix=0, lfo=1-4*fm_fabsf(s->lfo_phase-.5f);
  s->lfo_phase+=rate; if(s->lfo_phase>=1)s->lfo_phase-=1;
  for(unsigned i=0;i<JUNO6_VOICES;i++) {
   juno6_voice_t *v=&s->voice[i];
   if(!v->stage)continue;
   float dt=clampf(v->increment,.000001f,.45f),t=v->phase;
   float saw=2*t-1-blep(t,dt),width=clampf(pw+lfo*s->macro[11]*.2f,.05f,.95f);
   float pulse=(t<width?1.0f:-1.0f)+blep(t,dt),q=t-width;
   if(q<0)q+=1;
   pulse-=blep(q,dt);
   float sub=v->subphase<.5f?1.0f:-1.0f;
   float x=(saw*saw_g+pulse*pul_g)*.7f+sub*sub_g;
   if(v->stage==1) {v->envelope+=attack;if(v->envelope>=1){v->envelope=1;v->stage=2;}}
   else if(v->stage==2){v->envelope-=decay;if(v->envelope<=s->sustain){v->envelope=s->sustain;v->stage=3;}}
   else if(v->stage==4){
    /* Values below the old 1e-6 floor retire at 1e-5 in either form. */
    v->envelope-=v->envelope*release;
    if(v->envelope<.00001f){v->envelope=0;v->stage=0;}
   }
   /* Bounded two-pole lowpass, no transcendental work per voice/sample. */
   float a=clampf(TWO_PI*(base+v->envelope*env)/sr,.001f,.75f);
   v->filter_state+=a*(x-v->filter_state-res8*v->filter);
   v->filter+=a*(v->filter_state-v->filter);
   mix+=v->velocity*v->envelope*v->filter;
   v->phase+=dt;if(v->phase>=1)v->phase-=1;
   v->subphase+=dt*.5f;if(v->subphase>=1)v->subphase-=1;
  }
  mix*=level;
  if(s->hpf>0){float y=hp*(s->hp_state+mix-s->hp_input);s->hp_input=mix;s->hp_state=y;mix=y;}
  out[k]=mix;
 }
}
void juno6_chorus_init(juno6_chorus_t*c,float sr){memset(c,0,sizeof *c);c->sample_rate=sr>8000?sr:44100;c->rate_hz=.8f;c->depth_ms=3.4f;c->mix=.22f;}
void juno6_chorus_set(juno6_chorus_t*c,float r,float d,float m){c->rate_hz=clampf(r,.1f,5);c->depth_ms=clampf(d,1.7f,5.4f);c->mix=clampf(m,0,.5f);}
void juno6_chorus_process(juno6_chorus_t*c,float*l,float*r,unsigned n){
 unsigned i;
 float maxd=fm_minf(255,c->sample_rate*.0054f);
 float mind=c->sample_rate*.0017f;
 float span=c->depth_ms*.001f-.0017f;
 float dry=1-c->mix, wetg=c->mix;
 uint32_t phinc=(uint32_t)(c->rate_hz*4294967296.0f/c->sample_rate);
 for(i=0;i<n;i++){
  float in=(l[i]+r[i])*.5f,ph=(float)c->phase/4294967296.0f*TWO_PI,d=clampf(c->sample_rate*(.0017f+span*(.5f+.5f*fm_sinf(ph))),mind,maxd);
  unsigned whole=(unsigned)d,a=(c->write+256-whole)&255,b=(a+255)&255;
  float frac=d-whole,del=((float)c->delay[a]*(1-frac)+(float)c->delay[b]*frac)/32768.0f,wet=del*wetg;
  c->delay[c->write]=(int16_t)clampf(in*32767,-32768,32767);c->write=(c->write+1)&255;
  l[i]=l[i]*dry+wet;r[i]=r[i]*dry-wet;c->phase+=phinc;
 }
}

int juno6_active(const juno6_t *s) {
 for(unsigned i=0;i<JUNO6_VOICES;i++) if(s->voice[i].stage) return 1;
 return 0;
}
int juno6_set(juno6_t *s,unsigned id,float x) {
 if(!s || id>=18 || !(x>=0 && x<=1)) return 0;
 s->macro[id]=x;
 switch(id) {
 case 0:s->waveform=(uint8_t)(x*2.99f);break;
 case 1:s->sub_level=x*.5f;break;
 case 2:s->cutoff=x;break;
 case 3:s->resonance=x;break;
 case 5:s->attack=.002f+x*x*3;break;
 case 6:s->decay=.01f+x*x*3;break;
 case 7:s->sustain=x;break;
 case 8:s->release=.01f+x*x*4;break;
 case 9:s->hpf=x;break;
 case 12:s->pulse_width=.1f+x*.8f;break;
 case 16:s->bend=(x-.5f)*4;for(unsigned i=0;i<6;i++) if(s->voice[i].stage) s->voice[i].increment=midi_hz(s->voice[i].note)/s->sample_rate*fm_exp2f(s->bend*(1.0f/12.0f));break;
 case 17:s->damper=x>=.5f;if(!s->damper)for(unsigned i=0;i<6;i++)if(!s->voice[i].gate&&s->voice[i].stage)s->voice[i].stage=4;break;
 default:break;
 }
 return 1;
}
