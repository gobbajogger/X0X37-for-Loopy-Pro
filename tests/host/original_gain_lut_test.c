/* SPDX-License-Identifier: GPL-3.0-only */
#include "../../experiments/original_synth/voice_gain.c"
#include <stdio.h>
static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
static int check(uint32_t d){
 float old=1.0f/(float)d,next=duration_inverse(d);
 if(bits(old)!=bits(next)){
  fprintf(stderr,"FAIL duration=%u old=%08x new=%08x\n",d,bits(old),bits(next));return 0;
 }
 return 1;
}
int main(void){
 for(uint32_t d=1;d<=64;d++)if(!check(d))return 1;
 const uint32_t edge[]={0,65,66,127,128,255,256,44100,16777215,16777216,16777217,2147482880u,UINT32_MAX};
 for(unsigned i=0;i<sizeof edge/sizeof edge[0];i++)if(!check(edge[i]))return 1;
 uint32_t rng=0x37abcdefu;
 for(unsigned i=0;i<10000000u;i++){
  rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
  if(!check(rng))return 1;
 }
 puts("PASS: all 64 table entries, 13 duration edges, 10000000 random integer durations; const=256 B");
 return 0;
}
