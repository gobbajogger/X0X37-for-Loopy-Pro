/* SPDX-License-Identifier: GPL-3.0-only */
/* Source-scoped arithmetic regression for exact GitHub blobs from PR #712.
 * This does not exercise original_runtime / audio golden / the firmware link. */
#include "miami_idle_clock.h"
#include "miami_zero.h"
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t rng=0x6d70;
static uint32_t next_u(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static unsigned clock_cases,zero_cases;
static void clock_compare(float x,float bpm,float th,uint32_t frames){
 float ref=x,got=x;
 uint32_t used=0;
 while(used<frames){float next=ref+bpm;if(next>=th)break;ref=next;used++;}
 uint32_t actual=original_miami_idle_clock_below(&got,bpm,th,frames);
 if(actual!=used||original_math_bits(got)!=original_math_bits(ref)){
  fprintf(stderr,"MIAMI CLOCK FAIL x=%a bpm=%a th=%a frames=%u ref=%u %a got=%u %a\n",x,bpm,th,frames,used,ref,actual,got);exit(1);
 }
 clock_cases++;
}
static void check_zero(uint32_t n){
 enum{GUARD=17,MAX=8192};
 static uint32_t a[MAX+GUARD*2];
 static uint16_t b[MAX+GUARD*2];
 static float c[MAX+GUARD*2];
 for(uint32_t i=0;i<MAX+GUARD*2;i++){
  a[i]=0xabcddcbaU;b[i]=0xa5a5;c[i]=original_math_float(0xdeadbeefU);
 }
 original_miami_zero_words(a+GUARD,n);
 original_miami_zero_halves(b+GUARD,n);
 original_miami_zero_floats(c+GUARD,n);
 for(uint32_t i=0;i<MAX+GUARD*2;i++){
  uint32_t want=(i>=GUARD&&i<GUARD+n)?0U:0xabcddcbaU;
  if(a[i]!=want||b[i]!=((i>=GUARD&&i<GUARD+n)?0U:0xa5a5U)||
    original_math_bits(c[i])!=((i>=GUARD&&i<GUARD+n)?0U:0xdeadbeefU)){
   fprintf(stderr,"MIAMI ZERO FAIL n=%u i=%u\n",n,i);exit(1);
  }
 }
 zero_cases++;
}
int main(void){
 const float bpms[]={20.0f,120.0f,400.0f,217.12346f,99.75f,20.000002f,399.99997f};
 const uint32_t len[]={0,1,2,63,64,255,256,257,4096,4097,65535};
 for(size_t j=0;j<sizeof(bpms)/sizeof(bpms[0]);j++)for(size_t k=0;k<sizeof(len)/sizeof(len[0]);k++){
  float bpm=bpms[j];uint32_t n=len[k];
  clock_compare(0,bpm,999999.0f,n);
  clock_compare(2000000.0f,bpm,2000000.0f+100000.0f,n);
  clock_compare(original_math_float(0x4b7fffffu),bpm,original_math_float(0x4b800000u),n);
  clock_compare(original_math_float(0x7e7fffffu),bpm,original_math_float(0x7f7fffffu),n);
 }
 for(uint32_t i=0;i<300000;i++){
  float bpm=20.0f+(float)(next_u()&0xffffffu)*(380.0f/16777216.0f);
  float x=original_math_float(next_u()%0x55000000u);
  uint32_t n=next_u()%4097u;
  float threshold=x+bpm*(float)(1u+(next_u()%4097u));
  if(threshold==threshold&&original_math_bits(threshold)<0x7f800000u)clock_compare(x,bpm,threshold,n);
 }
 for(uint32_t n=0;n<=8192;n++)check_zero(n);
 printf("Miami isolated source helpers: %u exact clock cases, %u guarded zero-store cases PASS\n",clock_cases,zero_cases);
 return 0;
}
