/* SPDX-License-Identifier: GPL-3.0-only */
#include "fused_reference.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned long compact,fallback,checks;
static uint32_t rng=0x821599ab;
static uint32_t random32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static float fl(uint32_t u){float f;memcpy(&f,&u,4);return f;}
static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
static void check(uint32_t a,uint32_t b,uint32_t c){uint32_t ref,out=123,whole,host=bits(fmaf(fl(a),fl(b),fl(c)));assert(original_fma_finite_bits_reference(a,b,c,&ref));int ok=original_fma_compact_bits(a,b,c,&out);if(ok){compact++;assert(out==ref);}else{fallback++;assert(out==123);}assert(original_fma_finite_bits(a,b,c,&whole)&&whole==ref&&host==ref);checks++;}
int main(void){
 const uint32_t edge[]={0,0x80000000,1,0x80000001,0x007fffff,0x807fffff,0x00800000,0x80800000,0x3effffff,0x3f000000,0x3f000001,0x3f7fffff,0x3f800000,0xbf800000,0x7f7fffff,0xff7fffff};
 for(unsigned a=0;a<16;a++)for(unsigned b=0;b<16;b++)for(unsigned c=0;c<16;c++)check(edge[a],edge[b],edge[c]);
 for(unsigned i=0;i<2000000;i++){uint32_t a=random32(),b=random32(),c=random32();if((a&0x7f800000)==0x7f800000)a^=0x00800000;if((b&0x7f800000)==0x7f800000)b^=0x00800000;if((c&0x7f800000)==0x7f800000)c^=0x00800000;check(a,b,c);}
 /* Every binary32 hot cleanup input in +/−[0.5,1], preserving exact nested operations. */
 for(uint32_t u=0x3f000000;u<=0x3f800000;u++)for(unsigned sign=0;sign<2;sign++){
  float z=fl(u|(sign<<31));uint32_t a;check(bits(z),sign?0xc0000000:0x40000000,0xbf800000);assert(original_fma_finite_bits(bits(z),sign?0xc0000000:0x40000000,0xbf800000,&a));float b=1-fl(a);check(bits(-b),bits(b),0x3f800000);uint32_t c;assert(original_fma_finite_bits(bits(-b),bits(b),0x3f800000,&c));check(bits(sign?-fl(c):fl(c)),0x3e800000,sign?0xbf000000:0x3f000000);
 }
 uint32_t out=123;assert(!original_fma_compact_bits(0x7f800000,0,0,&out)&&out==123);assert(!original_fma_finite_bits(0x7fc00000,0,0,&out)&&out==123);
 printf("PASS %lu compact %lu fallback %lu total exact reference+hostFMA comparisons; exhaustive signed cleanup domain\n",compact,fallback,checks);return 0;
}
