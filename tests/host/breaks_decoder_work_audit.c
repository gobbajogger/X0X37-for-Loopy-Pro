/* SPDX-License-Identifier: GPL-3.0-only */
/* Host-only deterministic BREAK per-callback decoder-work audit. */
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "firmware/src/dsp/breaks.h"

#define BLOCK 64
#define BLOCKS 96
volatile uint64_t x0x_host_ima_decode_calls = 0;
static FILE *x0x_trace;
static uint32_t rnd(uint32_t *p) { uint32_t x=*p; x^=x<<13; x^=x>>17; x^=x<<5;return *p=x; }
static int cmp_u64(const void *ap,const void *bp){uint64_t a=*(const uint64_t*)ap,b=*(const uint64_t*)bp;return (a>b)-(a<b);}
static void one(const breaks_t *prepared,uint32_t n,int reverse,int speed,int trial) {
    breaks_t *b=malloc(sizeof(*b));
    if(!b)exit(3);
    memcpy(b,prepared,sizeof(*b));
    breaks_set(b,BRK_LEVEL,127);
    breaks_set(b,BRK_PITCH,12);
    if(reverse)breaks_live(b,8,1);
    float sp16=((float)n/32.0f)/(float)speed;
    x0x_host_ima_decode_calls=0;
    breaks_step(b,0,0,sp16,1);
    uint64_t setup=x0x_host_ima_decode_calls;
    float out[BLOCK];
    uint64_t per_block[BLOCKS],sum=0,max=0,hash=UINT64_C(1469598103934665603),audible=0;
    for(int j=0;j<BLOCKS;++j){
        /* Deliberately exercise live-key NoteOff and reverse-direction release
         * inside rendering, not only after stopping. These events should not
         * alter the PCM or scheduling behavior of the optimized renderer. */
        if (trial == 1 && reverse && j == BLOCKS / 2)
            breaks_live(b, 8, 0);
        if (trial == 1 && !reverse && j == BLOCKS / 4)
            breaks_live(b, 3, 1);
        if (trial == 1 && !reverse && j == BLOCKS / 2)
            breaks_live(b, 3, 0);
        uint64_t prior=x0x_host_ima_decode_calls;
        breaks_render(b,out,BLOCK);
        uint64_t delta=x0x_host_ima_decode_calls-prior;
        if(delta>max)max=delta;
        sum+=delta;
        per_block[j]=delta;
        if (x0x_trace && fwrite(out, sizeof(out[0]), BLOCK, x0x_trace) != BLOCK) {
            fputs("trace write error\n", stderr); exit(3);
        }
        for(int i=0;i<BLOCK;++i){
            if(!isfinite(out[i])||fabsf(out[i])>2.0f) { fputs("bad sample\n",stderr);exit(1); }
            uint32_t x;memcpy(&x,&out[i],4);
            for(unsigned byte=0;byte<4;++byte){hash^=(uint8_t)(x>>(byte*8u));hash*=UINT64_C(1099511628211);}
            audible+=out[i]!=0.0f;
        }
    }
    qsort(per_block,BLOCKS,sizeof(per_block[0]),cmp_u64);
    uint64_t p99=per_block[(BLOCKS-1)*99/100],p95=per_block[(BLOCKS-1)*95/100];
    if(reverse)breaks_live(b,8,0);
    breaks_stop(b);breaks_set_quiet(b,1);
    for(int j=0;j<8;++j)breaks_render(b,out,BLOCK);
    if(!breaks_silent(b)) {fputs("not stopped\n",stderr);exit(1);}
    for(int i=0;i<BLOCK;++i)if(out[i]!=0.0f){fputs("not silent\n",stderr);exit(1);}
    if(!audible){fputs("no audible frames\n",stderr);exit(1);}
    printf("n=%" PRIu32 " direction=%c rate=%d trial=%d setup=%" PRIu64 " decodes=%" PRIu64 " max=%" PRIu64 " p95=%" PRIu64 " p99=%" PRIu64 " pcm=%016" PRIx64 " audible=%" PRIu64 " stopped=1\n",n,reverse?'R':'F',speed,trial,setup,sum,max,p95,p99,hash,audible);
    free(b);
}
int main(int argc,char**argv){
    if (argc != 4 || strcmp(argv[1], "--check") || strcmp(argv[2], "--trace")) {
        fputs("usage: decoder_work --check --trace /path/to/out.pcm\n",stderr);return 2;
    }
    x0x_trace = fopen(argv[3], "wb");
    if (!x0x_trace) {perror("trace open");return 3;}
    const uint32_t sizes[]={65535,176400,2116800};
    const int rates[]={1,4,16,64,256};
    for(unsigned si=0;si<sizeof(sizes)/sizeof(sizes[0]);++si){
        uint32_t n=sizes[si],seed=UINT32_C(0x80945371)^n;
        uint8_t*data=malloc((n+1u)/2u);
        breaks_t *prepared=calloc(1,sizeof(*prepared));
        if(!data||!prepared){fputs("allocation failed\n",stderr);return 3;}
        for(uint32_t k=0;k<(n+1u)/2u;++k)data[k]=(uint8_t)rnd(&seed);
        brk_loop_t loop={.adpcm=data,.nsamples=n,.rate=22050,.bars=2};
        breaks_init(prepared);breaks_set_loop(prepared,0,&loop);
        if(!prepared->bank[0].valid){fputs("invalid bank\n",stderr);return 1;}
        for(int reverse=0;reverse<2;++reverse)
          for(unsigned k=0;k<sizeof(rates)/sizeof(rates[0]);++k)
            for(int trial=0;trial<2;++trial)one(prepared,n,reverse,rates[k],trial);
        free(prepared);free(data);
    }
    if (fclose(x0x_trace)) {perror("trace close");return 3;}
    return 0;
}
