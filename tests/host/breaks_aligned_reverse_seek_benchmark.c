/* SPDX-License-Identifier: GPL-3.0-only */
/* Timed real BREAK reverse seek helper; host microbenchmark, no target timing claim. */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "firmware/src/dsp/breaks.c"
static uint32_t rand32(uint32_t *p) {uint32_t x=*p;x^=x<<13;x^=x>>17;x^=x<<5;return *p=x;}
int main(int argc,char**argv) {
 if(argc!=2) return 2;
 uint32_t n=(uint32_t)strtoul(argv[1],0,10);
 if(n!=65535u&&n!=176400u) return 2;
 uint8_t *a=malloc((n+1u)/2u);
 breaks_t *b=calloc(1,sizeof(*b));
 if(!a||!b) return 3;
 uint32_t seed=0x5fe9842fu;
 for(uint32_t k=0;k<(n+1u)/2u;++k)a[k]=(uint8_t)rand32(&seed);
 brk_loop_t l={.adpcm=a,.nsamples=n,.rate=22050,.bars=2};
 breaks_init(b);breaks_set_loop(b,0,&l);
 if(!b->bank[0].valid||b->bank[0].ck_shift>9) return 3;
 brk_voice_t *v=&b->v[0];
 uint32_t blocks=(n-1u)/BRK_RCH;
 uint64_t sum=0;
 seed=0x40a71d9bu;
 for(uint32_t i=0;i<25000000u;i++){
  uint32_t p=(rand32(&seed)%blocks)*BRK_RCH;
  int32_t pred,idx;
  bank_seek_reverse(v,&b->bank[0],p,&pred,&idx);
  sum+=(uint32_t)(pred+32768)*(uint32_t)(idx+1u);
 }
 printf("length=%"PRIu32" shift=%u checks=%"PRIu64"\n",n,b->bank[0].ck_shift,sum);
 free(a);free(b);
 return 0;
}
