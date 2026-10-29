/* SPDX-License-Identifier: GPL-3.0-only */
#include "fused_reference.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
static float fl(uint32_t u){float f;memcpy(&f,&u,4);return f;}
static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
static uint32_t rng=6436;
static uint32_t rand32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static void check(uint32_t a,uint32_t b,uint32_t c){uint32_t out;assert(original_fma_finite_bits(a,b,c,&out));uint32_t oracle=bits(fmaf(fl(a),fl(b),fl(c)));if(out!=oracle){fprintf(stderr,"%08x %08x %08x =>%08x expected%08x\n",a,b,c,out,oracle);assert(0);}assert(bits(original_rne_fmaf(fl(a),fl(b),fl(c)))==oracle);}
int main(void){
 uint32_t edge[]={0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x3f000000,0xbf000000,0x3f800000,0xbf800000,0x3f800001,0xbf800001,0x7f7fffff,0xff7fffff,0x00800001};
 for(unsigned a=0;a<15;a++)for(unsigned b=0;b<15;b++)for(unsigned c=0;c<15;c++)check(edge[a],edge[b],edge[c]);
 for(unsigned i=0;i<1000000;i++){uint32_t a=rand32(),b=rand32(),c=rand32();if((a&0x7f800000)==0x7f800000)a^=0x00800000;if((b&0x7f800000)==0x7f800000)b^=0x00800000;if((c&0x7f800000)==0x7f800000)c^=0x00800000;check(a,b,c);}
 uint32_t out=123;assert(!original_fma_finite_bits(0x7f800000,0,0,&out)&&out==123);assert(!original_fma_finite_bits(0,0,0,NULL));
 clock_t t=clock();volatile uint32_t sink=0;for(unsigned i=0;i<100000;i++){assert(original_fma_finite_bits(0x3f000000+i,0x3f000001,0xbf000000,&out));sink^=out;}
 printf("PASS exact integer finite FMA:3375edge triples+1000000random, host100000calls %.6fs sink%u\n",(double)(clock()-t)/CLOCKS_PER_SEC,sink);return 0;
}
