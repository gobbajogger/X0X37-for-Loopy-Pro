/* SPDX-License-Identifier: GPL-3.0-only */
/* Catch two regressions: coefficient work at equality, and lost binary32
 * rounding if equality returns the unrounded sustain value. */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../experiments/original_synth/fused_reference.h"

static unsigned coefficient_calls;
static float counted_fma(float x,float y,float z) {
 ++coefficient_calls;
 return original_rne_fmaf(x,y,z);
}
/* Instrument only the real coefficient call, with unchanged arithmetic. */
#define ORIGINAL_USE_SOFT_FMA
#define original_rne_fmaf counted_fma
#include "../../experiments/original_synth/control_math.c"
#undef original_rne_fmaf

static uint32_t bits(float x) { uint32_t u;memcpy(&u,&x,4);return u; }
static float from_bits(uint32_t u) { float x;memcpy(&x,&u,4);return x; }
/* Frozen pre-optimisation state-1 recurrence from a06e20189. */
static float reference(float e,float fs,uint32_t duration,float decay,float sustain) {
 if(decay<0)decay=0;else if(decay>1)decay=1;
 if(sustain<0)sustain=0;else if(sustain>1)sustain=1;
 if(e<sustain)return sustain;
 float d2=decay*decay;
 float den=fs*original_rne_fmaf(d2*d2,25.0f,0.0010000000474974513f);
 float delta=(float)duration*(sustain-e);
 float q=e+delta/den;
 return (q+1.0f)-1.0f;
}
static unsigned comparisons;
static void check(float e,float fs,uint32_t duration,float decay,float sustain) {
 float want=reference(e,fs,duration,decay,sustain);
 original_vca s={e,1};unsigned action=99;
 assert(original_vca_tick(&s,fs,duration,0,decay,sustain,0,&action));
 assert(bits(s.level)==bits(want));assert(s.state==1);assert(action==0);
 ++comparisons;
}
int main(void) {
 /* 0.1 is not fixed by (q+1)-1: simply returning at equality is wrong. */
 assert(bits(reference(0.1f,44100,64,0.5f,0.1f))!=bits(0.1f));
 check(0.1f,44100,64,0.5f,0.1f);
 coefficient_calls=0;
 check(0.5f,44100,64,0.5f,0.5f);
 assert(coefficient_calls==0 && "equal sustain must skip decay coefficient work");
 const float rates[]={8000,44100,48000,96000,2147482880.0f};
 const float decays[]={-1,0,0.01f,0.5f,1,2};
 const float levels[]={-0.5f,-0.0f,0,0x1p-149f,0.1f,0.5f,0.99999994f,1};
 for(unsigned r=0;r<sizeof rates/sizeof *rates;r++)
  for(unsigned d=0;d<sizeof decays/sizeof *decays;d++)
   for(unsigned i=0;i<sizeof levels/sizeof *levels;i++) {
    uint32_t max=(uint32_t)ceilf(rates[r]/690.0f);
    for(unsigned j=0;j<sizeof levels/sizeof *levels;j++) {
     check(levels[i],rates[r],1,decays[d],levels[j]);
     check(levels[i],rates[r],max,decays[d],levels[j]);
    }
   }
 uint32_t seed=0x37ca1234u;
 for(unsigned i=0;i<100000;i++) {
  seed=seed*1664525u+1013904223u;
  float sustain=(float)(seed&0xffffffu)/16777215.0f;
  float e=i%3==0?sustain:(i%3==1?nextafterf(sustain,0):nextafterf(sustain,1));
  check(e,44100,1+i%64,(float)(seed>>24)/255.0f,sustain);
  if(i%3==0) {
   float rounded=reference(e,44100,64,0.25f,sustain);
   check(rounded,44100,64,0.25f,sustain);
   check(rounded,44100,64,0.25f,1-sustain);
  }
 }
 /* All binary32 exponent ranges and signed zeros at equality. */
 for(uint32_t u=0;u<=0x3f800000u;u+=65537u) {
  float e=from_bits(u);check(e,44100,64,0.5f,e);
 }
 original_vca invalid={0.5f,1},before=invalid;unsigned action=77;
 assert(!original_vca_tick(&invalid,44100,0,0,0,0.5f,0,&action));
 assert(invalid.level==before.level&&invalid.state==before.state&&action==77);
 printf("PASS VCA sustain: %u bit-exact comparisons; equal coefficient calls=0\n",comparisons);
 return 0;
}
