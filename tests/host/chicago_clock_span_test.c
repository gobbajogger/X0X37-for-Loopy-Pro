/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_clock_span.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line %d case %u\n",__LINE__,checks);exit(1);}}while(0)
static unsigned checks;
static uint32_t seed=0x6d70;
static uint32_t random32(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static void compare(float x,float bpm,float threshold,uint32_t frames){
 float ref=x,got=x;uint32_t n=0;
 for(;n<frames;n++){float next=ref+bpm;if(next>=threshold)break;ref=next;}
 uint32_t m=original_chicago_clock_span(&got,bpm,threshold,frames);
 if(n!=m||original_math_bits(ref)!=original_math_bits(got))fprintf(stderr,"x=%a bpm=%a threshold=%a frames=%u ref=%a/%u got=%a/%u\n",x,bpm,threshold,frames,ref,n,got,m);
 CHECK(n==m);CHECK(original_math_bits(ref)==original_math_bits(got));checks++;
}
int main(void){
 /* Exponent changes, ties of both parities, stagnation, and threshold edges.
  * Includes the full legal BPM range, fractional and integer tempos. */
 const float bpms[]={20,120,400,123.456f,99.75f,20.000002f,399.99997f};
 for(unsigned e=0;e<255;e++)for(unsigned j=0;j<7;j++)for(unsigned k=0;k<32;k++){
  uint32_t u=(e<<23)|((k&1)?0x7fffffu-k/2:k/2);
  float x=original_math_float(u),b=bpms[j];
  compare(x,b,original_math_float(0x7f7fffffu),256);
  float edge=x+b*(float)(k+1);if(edge>0&&original_math_bits(edge)<0x7f800000u)compare(x,b,edge,4096);
 }
 /* The fast path is admitted only when the entire span fits the same
  * exponent/threshold interval without a 32-bit multiplication overflow.
  * Cover both sides of the 4096-frame and 20-bit ULP-delta guards. */
 const uint32_t lengths[]={0,1,2,63,64,255,256,257,4096,4097,65535};
 const float starts[]={0,20,120,200,2000,65536,8388608,33554432,1073741824};
 for(unsigned j=0;j<sizeof bpms/sizeof bpms[0];j++)
  for(unsigned k=0;k<sizeof starts/sizeof starts[0];k++)
   for(unsigned h=0;h<sizeof lengths/sizeof lengths[0];h++){
    float x=starts[k],b=bpms[j];uint32_t frames=lengths[h];
    compare(x,b,original_math_float(0x7f7fffffu),frames);
    compare(x,b,x+b*64,frames);
    compare(x,b,x+b*4096,frames);
   }
 /* Some callers can expose a negative accumulator before reset. These
  * values must still take the literal-addition path, without changed bits. */
 for(unsigned i=0;i<sizeof lengths/sizeof lengths[0];i++)
  compare(-120,123.456f,240,lengths[i]);
 for(unsigned i=0;i<300000;i++){
  float bpm=20+(float)(random32()&0xffffffu)*(380.0f/16777216.0f);
  uint32_t u=random32()%0x55000000u;
  float x=original_math_float(u);
  float threshold=x+bpm*(float)(1+(random32()%4096));
  compare(x,bpm,threshold,random32()%4097);
 }
 printf("Chicago clock span: %u exact comparisons PASS\n",checks);return 0;
}
