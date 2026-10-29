/* SPDX-License-Identifier: GPL-3.0-only */
#include "lossless_table.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint32_t rng=0x147b02e9u;
static uint32_t next(void){rng=rng*1664525u+1013904223u;return rng;}
static void put32(uint8_t*p,uint32_t x){for(unsigned k=0;k<4;k++)p[k]=(uint8_t)(x>>(8*k));}
int main(void){unsigned cases=0;for(unsigned width=0;width<=32;width++)for(unsigned count=1;count<=32;count++)for(unsigned trial=0;trial<64;trial++){
 uint8_t bytes[129]={0};uint32_t off=0,expect[32],delta=next();expect[0]=next();put32(bytes,expect[0]);put32(bytes+4,delta);bytes[8]=(uint8_t)width;size_t bit=0;
 if(count>1)expect[1]=expect[0]+delta;
 for(unsigned i=2;i<count;i++){uint32_t z=next();if(width<32)z&=width?((1u<<width)-1u):0;
  for(unsigned b=0;b<width;b++,bit++)bytes[9+bit/8]|=(uint8_t)(((z>>b)&1u)<<(bit%8));
  delta+=(z>>1)^(0u-(z&1u));expect[i]=expect[i-1]+delta;
 }
 size_t n=9+(bit+7)/8;original_lossless_table t={bytes,&off,n,count,1};
 for(unsigned i=0;i<count;i++){uint32_t out=123;assert(original_lossless_second_lookup(&t,i,&out));assert(out==expect[i]);cases++;}
 uint32_t out=123;assert(!original_lossless_second_lookup(&t,count,&out)&&out==123);t.bytes=n-1;assert(!original_lossless_second_lookup(&t,0,&out)&&out==123);t.bytes=n;bytes[8]=33;assert(!original_lossless_second_lookup(&t,0,&out)&&out==123);bytes[8]=(uint8_t)width;off=1;assert(!original_lossless_second_lookup(&t,0,&out)&&out==123);
 }
 /* A second block has its own value/delta, independent of first block history. */
 uint8_t b[18]={0};uint32_t offsets[2]={0,9},out=0;b[4]=255;put32(b+9,0x80000000);original_lossless_table t={b,offsets,18,33,2};assert(original_lossless_second_lookup(&t,32,&out)&&out==0x80000000);offsets[1]=10;out=123;assert(!original_lossless_second_lookup(&t,0,&out)&&out==123);
 printf("PASS %u arbitrary-bit second-order lookups, widths0..32/counts1..32/wrap/malformed/partial blocks\n",cases);return 0;}
