/* SPDX-License-Identifier: GPL-3.0-only */
#include "binary32_widen.h"
#include "chicago_tuning.h"
#include "master_tune_rne.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static uint64_t checks;
static void check(uint32_t raw){
 float f;memcpy(&f,&raw,4);double old=(double)f,next=original_binary32_to_binary64_finite(f);uint64_t a,b;memcpy(&a,&old,8);memcpy(&b,&next,8);assert(a==b);checks++;
}
static uint64_t tune_checks,math_checks;
static void tune(uint32_t raw){
 float f;memcpy(&f,&raw,4);volatile double divided=(double)f/0x1.62e42fefa39efp-1;
 double product=divided*12.0;float expected=(float)product,actual=original_master_tune_semitones_rne(f);
 if(memcmp(&actual,&expected,4)){fprintf(stderr,"tune mismatch %08x: %08x != %08x\n",raw,original_math_bits(actual),original_math_bits(expected));assert(0);}tune_checks++;
 float a=original_binary32_abs(f),b=fabsf(f);assert(!memcmp(&a,&b,4));
 a=original_binary32_ceil_finite(f);b=ceilf(f);assert(!memcmp(&a,&b,4));
 a=original_binary32_min_one_finite(f);b=fminf(f,1.0f);assert(!memcmp(&a,&b,4));math_checks+=3;
}
int main(void){
 /* All signed subnormals/zeros, plus all significands at both extreme normal
  * exponents. Every exponent is also crossed with both mantissa boundaries. */
 for(uint32_t sign=0;sign<2;sign++){
  for(uint32_t m=0;m<0x800000u;m++){
   check((sign<<31)|m);check((sign<<31)|0x800000u|m);check((sign<<31)|0x7f000000u|m);
  }
  for(uint32_t e=1;e<255;e++)for(uint32_t j=0;j<24;j++){
   uint32_t m=1u<<j;if(m<0x800000u){check((sign<<31)|(e<<23)|m);check((sign<<31)|(e<<23)|(m-1u));}
  }
 }
 for(unsigned i=0;i<=10000;i++){
  float x=(float)i/10000.0f,shift=x+(-.5f),hz=fmaf(shift,40,440),v=logf(hz/440.0f),actual;
  double old=(double)v/0x1.62e42fefa39efp-1;old=old*12.0;float expected=(float)old;
  assert(original_chicago_master_tune_convert(x,logf,&actual));assert(!memcmp(&actual,&expected,4));
 }
 /* Every positive normal significand at exponent-1, every positive
  * subnormal, all signed exponent/mantissa boundaries, and overflow edges.
  * Binary64 divide/multiply rounding is independent of normal input exponent;
  * the final binary32 rounding is also checked at every exponent below. */
 for(uint32_t m=0;m<0x800000u;m++){tune(0x3f000000u|m);tune(m);}
 uint32_t seed=0x90378u;
 for(unsigned i=0;i<1000000;i++){seed=seed*1664525u+1013904223u;uint32_t raw=seed;if((raw&0x7f800000u)!=0x7f800000u)tune(raw);}
 for(uint32_t sign=0;sign<2;sign++)for(uint32_t e=0;e<255;e++)for(unsigned j=0;j<24;j++){
  uint32_t m=1u<<j;if(m<0x800000u){tune((sign<<31)|(e<<23)|m);tune((sign<<31)|(e<<23)|(m-1u));}
 }
 printf("PASS %llu finite widenings;%llu exact separately-rounded binary64 divide/multiply/final-cast witnesses;%llu finite abs/ceil/min1 comparisons;10001 actual MasterTune conversions; RNE result parity only, target dependencies/FP-status unqualified\n",(unsigned long long)checks,(unsigned long long)tune_checks,(unsigned long long)math_checks);return 0;
}
