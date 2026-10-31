/* GPL-3.0-only, scoped host-only microbenchmark; no target performance claims. */
#define _POSIX_C_SOURCE 200809L
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "firmware/src/dsp/breaks.h"
static uint32_t rng(uint32_t *x){*x^=*x<<13;*x^=*x>>17;*x^=*x<<5;return *x;}
static uint64_t ns(void){struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000ull+t.tv_nsec;}
int main(int argc,char **argv){
 if(argc!=3){fprintf(stderr,"usage: %s reverse|forward 4096|176400|705600|2116800\n",argv[0]);return 2;}
 int reverse=!strcmp(argv[1],"reverse");if(!reverse && strcmp(argv[1],"forward"))return 2;
 uint32_t n=(uint32_t)strtoul(argv[2],NULL,10);if(n!=4096&&n!=176400&&n!=705600&&n!=2116800)return 2;
 uint8_t *data=malloc((n+1u)/2u);breaks_t *b=calloc(1,sizeof(*b));if(!data||!b)return 3;
 uint32_t seed=0xa9563137u^n;for(uint32_t i=0;i<(n+1u)/2u;i++)data[i]=(uint8_t)rng(&seed);
 brk_loop_t loop={.adpcm=data,.nsamples=n,.rate=44100,.bars=1};breaks_init(b);breaks_set_loop(b,0,&loop);
 breaks_set(b,BRK_LEVEL,127);if(reverse)breaks_live(b,8,1);
 breaks_step(b,0,0,5512.5f,1);
 if(!b->v[b->cur].on){fprintf(stderr,"no voice\n");return 1;}
 if((int)b->v[b->cur].rev!=reverse){fprintf(stderr,"wrong voice direction\n");return 1;}
 float out[256]; uint64_t hash=1469598103934665603ull;uint64_t start=ns();
 const unsigned blocks=4096;
 for(unsigned k=0;k<blocks;k++){
   breaks_render(b,out,256);
   for(unsigned j=0;j<256;j++){uint32_t v;memcpy(&v,&out[j],sizeof(v));hash^=v;hash*=1099511628211ull;}
 }
 uint64_t end=ns();
 printf("direction=%s n=%u rch=%u sizeof_breaks=%zu frames=%u time_ns=%" PRIu64 " hash=%016" PRIx64 "\n",reverse?"reverse":"forward",n,BRK_RCH,sizeof(*b),blocks*256,end-start,hash);
 free(b);free(data);return 0;
}
