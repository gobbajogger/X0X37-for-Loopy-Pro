/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_global_mg.h"
#include "fused_reference.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include "binary32_math.h"
static float next_random(original_chicago_global_mg_osc *s){
 uint32_t old=s->random&65535u,x=old<<1;
 x=(x&~1u)|((((old<<12)^(old<<4)^old^(old<<1))>>15)&1u);
 s->random=(uint16_t)x;
 int32_t value=(int32_t)((old>>7)&255u);if(value>=128)value-=256;
 float scaled=(float)value*.00390625f;return scaled+.5f;
}
static void notify(original_chicago_global_mg_osc *s,uint32_t waveform){
 s->wrapped=0;
 if(waveform==0||waveform==2)s->output=0;
 else if(waveform==1||waveform==3)s->output=1;
 else if(waveform==4){s->output=next_random(s);s->held=s->output;}
 s->phase=-1;
}
void original_chicago_global_mg1_tick(original_chicago_global_mg_osc *s,chicago_arp_clock *clock,uint32_t waveform,uint32_t sync,float bpm){
 if(clock->key_reset){clock->key_reset=0;clock->accumulator=0;}
 clock->accumulator=clock->accumulator+bpm;
 if(clock->accumulator>=clock->threshold){clock->accumulator=clock->accumulator-clock->threshold;if(sync==1)notify(s,waveform);}
}
void original_chicago_global_mg2_notify(original_chicago_global_mg_osc *s,uint32_t waveform){notify(s,waveform);}
static void waveform(original_chicago_global_mg_osc *s,uint32_t shape){
 if(shape==0){float v=s->phase>=0?-s->phase:s->phase;s->output=v+1;}
 else if(shape==1)s->output=original_rne_fmaf(-s->phase,.5f,.5f);
 else if(shape==2)s->output=original_rne_fmaf(s->phase,.5f,.5f);
 else if(shape==3)s->output=s->phase>=0?0:1;
 else if(shape==4){
  if(s->wrapped){s->output=next_random(s);s->held=s->output;}
  else s->output=original_rne_fmaf(s->held,.5f,.5f);
 }
 s->wrapped=0;
}
static unsigned phase(original_chicago_global_mg_osc *s,const chicago_arp_clock *clock,uint32_t sync,float rate,float frames,float sample_rate){
 if(sync){float q=clock->accumulator/clock->threshold;s->phase=original_rne_fmaf(q,2,-1);return 0;}
 float x=rate<0?0:(rate<=1?rate:1),square=x*x,cube=x*square;
 float hz=original_rne_fmaf(cube,199.899993896484375f,.009999999776482582f);
 float advance=frames*hz;advance=advance/sample_rate;s->phase=s->phase+advance;
 if(s->phase>=1){s->phase=s->phase+-2;s->wrapped=1;return 1;}return 0;
}
unsigned original_chicago_global_mg_after_voices(original_chicago_global_mg *s,const original_chicago_global_mg_input *in,const chicago_arp_clock *clock1,const chicago_arp_clock *clock2,void (*step)(void *),void *context){
 assert(in->sample_rate_integer>0&&step);
 uint32_t next=(uint32_t)s->counter+690u;
 int32_t count=next<=INT32_MAX?(int32_t)next:-1-(int32_t)(UINT32_MAX-next);
 s->counter=count;if(count<in->sample_rate_integer)return 0;
 s->counter=count-in->sample_rate_integer;
 float frames=(float)(in->sample_rate_integer-s->counter)/690.f;
 waveform(&s->mg1,in->waveform1);frames=original_binary32_ceil_finite(frames);
 (void)phase(&s->mg1,clock1,in->tempo_sync1,in->rate1,frames,in->sample_rate);
 waveform(&s->mg2,in->waveform2);
 if(phase(&s->mg2,clock2,in->tempo_sync2,in->rate2,frames,in->sample_rate))step(context);
 return 1;
}
uint32_t original_chicago_global_mg_quiet_frames(const original_chicago_global_mg *s,const original_chicago_global_mg_input *in){
 if(s->counter<0||s->counter>=in->sample_rate_integer)return 0;
 return (uint32_t)(in->sample_rate_integer-1-s->counter)/690u;
}
