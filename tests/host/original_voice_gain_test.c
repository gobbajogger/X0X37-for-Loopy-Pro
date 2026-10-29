/* SPDX-License-Identifier: GPL-3.0-only */
#include "voice_gain.h"
#include "miami_program.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
static uint32_t bits(float x){uint32_t u;memcpy(&u,&x,4);return u;}
static float fl(uint32_t u){float f;memcpy(&f,&u,4);return f;}
static float advance_oracle(original_ramp *r){
 float next=r->current+r->delta;
 float out=((r->delta>0)^(r->target>next))?r->target:next;
 r->current=out;return out;
}
static original_osc_controls controls(void){
 original_osc_controls c={{12000000,19000000,24000000,38000000},{0,0,0,0},{.5f,.4f,.3f,.2f},{0,1,2,3},.25f};return c;
}
static void target_witness(void){
 original_voice_gain s;original_voice_gain_init(&s);
 original_voice_gain_controls c={{.906f,.772f,1,0},0,{0,0,0,0},1,0,1,1,0,0,.27f,0,2,1,1};
 int32_t active[4]={-1,-1,-1,-1};assert(original_voice_gain_prepare(&s,&c,active,64));
 const uint32_t level[]={0x3f8c15fa,0x3f4b6c94,0x3faaa993,0};
 for(unsigned i=0;i<4;i++)assert(bits(s.oscillator[i].target)==level[i]);
 assert(bits(s.stereo[0].target)==0x3ea28f5c&&bits(s.stereo[1].target)==0x3e3ae148);
 c.unison_flag=0;assert(original_voice_gain_prepare(&s,&c,active,64));
 assert(bits(s.stereo[1].target)==0x3ea28f5c&&bits(s.stereo[0].target)==0x3e3ae148);
 /* Chicago KeyboardTrack route2: note60, velocity127, currentEG1. */
 c.ms20=0;c.unison_flag=255;c.mod_gain=.038750000298023224f;c.filter_gain=4;
 assert(original_voice_gain_prepare(&s,&c,active,64));
 assert(bits(s.stereo[0].target)==0x3f8a1cba&&bits(s.stereo[1].target)==0x3f8a1cba);
 original_voice_gain saved=s;original_voice_gain_miami_full_note(&s);
 assert(!memcmp(s.oscillator,saved.oscillator,sizeof s.oscillator));
 for(unsigned i=0;i<2;i++)assert(s.stereo[i].current==0&&s.stereo[i].target==0&&s.stereo[i].delta==0);
 c.level[0]=NAN;saved=s;assert(!original_voice_gain_prepare(&s,&c,active,64)&&!memcmp(&s,&saved,sizeof s));
 puts("PASS original coefficient witnesses, complementary unison, Chicago KeyboardTrack, full-note oscillator-current preservation");
}
static unsigned random_word(uint32_t *seed){*seed=*seed*1664525u+1013904223u;return *seed;}
static void exact_frames(void){
 original_voice_gain s;original_voice_gain_init(&s);original_osc_controls c=controls();
 original_prepared_oscillators prepared;assert(original_oscillator_prepare(&prepared,&c));
 original_oscillators phase={{0x12345678,0xabcdef01,0x80000001,0x7fffffff}},oracle=phase;
 original_voice_gain_controls g={{.2f,.4f,.7f,1},0,{0,0,0,0},1,0,1,1,0,0,.27f,0,2,1,1};
 int32_t active[4]={-1,-1,-1,-1};uint32_t seed=785431;unsigned count=0;
 for(unsigned block=0;block<4096;block++) {
  unsigned D=1+random_word(&seed)%64;
  for(unsigned i=0;i<4;i++)g.level[i]=(float)(random_word(&seed)%1001)/1000;
  g.curve=(uint8_t)(block%3);g.envelope=(float)(random_word(&seed)%1001)/1000;
  g.pan=(float)(random_word(&seed)%2001)/1000-1;g.unison_flag=(uint8_t)((block&1)?2:0);
  assert(original_voice_gain_prepare(&s,&g,active,D));
  for(unsigned frame=0;frame<D;frame++) {
   original_voice_gain expected=s;
   for(unsigned i=0;i<4;i++)c.gain[i]=expected.oscillator[i].current;
   float expected_mono,mono,left,right;
   assert(original_oscillator_sample(&oracle,&c,block&1,&expected_mono));
   for(unsigned i=0;i<4;i++)(void)advance_oracle(&expected.oscillator[i]);
   float expected_left=expected_mono*advance_oracle(&expected.stereo[0]);
   float expected_right=expected_mono*advance_oracle(&expected.stereo[1]);
   assert(original_voice_gain_oscillator(&s,&phase,&prepared,block&1,&mono));
   assert(bits(mono)==bits(expected_mono)&&!memcmp(&phase,&oracle,sizeof phase));
   assert(original_voice_gain_stereo(&s,mono,&left,&right));
   assert(bits(left)==bits(expected_left)&&bits(right)==bits(expected_right));
   assert(!memcmp(&s,&expected,sizeof s));count++;
  }
 }
 printf("PASS %u variable-gain frames, full state bit-identical to repeated coefficient preparation + independent ramp order\n",count);
}
static volatile float sink;
static double benchmark(int repeat){
 original_osc_controls c=controls();for(unsigned i=0;i<4;i++)c.gain[i]=.5f;
 original_prepared_oscillators p;assert(original_oscillator_prepare(&p,&c));
 original_oscillators phase={{0,1,2,3}};clock_t begin=clock();
 for(unsigned i=0;i<2000000;i++) {
  c.gain[i&3]=(float)(i%1001)/1000;float out;
  int ok=repeat?original_oscillator_sample(&phase,&c,0,&out):original_oscillator_sample_gains(&phase,&p,c.gain,0,&out);
  assert(ok);sink=out;
 }
 return (double)(clock()-begin)/CLOCKS_PER_SEC;
}
int main(void){target_witness();exact_frames();
 for(unsigned i=0;i<5;i++) {
  double before,after;if(i&1){after=benchmark(0);before=benchmark(1);}else{before=benchmark(1);after=benchmark(0);}
  printf("HOST_ONLY gain preparation run%u repeated%.6f cached%.6f ratio%.6f\n",i,before,after,before/after);
 }
 assert(fl(0x3faaa993)==ORIGINAL_OSCILLATOR_GAIN_MAX);return 0;
}
