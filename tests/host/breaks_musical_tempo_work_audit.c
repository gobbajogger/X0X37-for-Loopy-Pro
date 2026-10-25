/* SPDX-License-Identifier: GPL-3.0-only */
/* Real production BREAK renderer with tempo-derived playback rates.
 * Host-only: not a claim that these tempos or sample lengths are device-verified. */
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "firmware/src/dsp/breaks.h"

#define BLOCK 64
#define BLOCKS 64
volatile uint64_t x0x_host_ima_decode_calls;
static FILE *trace;
static uint32_t rand32(uint32_t *p) {uint32_t x=*p; x^=x<<13; x^=x>>17; x^=x<<5;return *p=x;}
static int cmp_u64(const void *a,const void *b) {
    uint64_t x=*(const uint64_t*)a, y=*(const uint64_t*)b;
    return (x>y)-(x<y);
}
static void one(const breaks_t *bank, uint32_t n, int bars, int bpm, int semis,
                int reverse, int trial) {
    breaks_t *b=malloc(sizeof(*b));
    if (!b) exit(3);
    memcpy(b,bank,sizeof(*b));
    breaks_set(b,BRK_LEVEL,127);
    breaks_set(b,BRK_PITCH,12+semis);
    if (reverse) breaks_live(b,8,1);
    /* 16th at 44.1 kHz: 44100 * 60 / (4 * bpm); no fake rate multiplier. */
    float sp16=661500.0f/(float)bpm;
    x0x_host_ima_decode_calls=0;
    breaks_step(b,0,0,sp16,1);
    uint64_t setup=x0x_host_ima_decode_calls;
    float out[BLOCK];
    uint64_t calls[BLOCKS],total=0,max=0,audible=0;
    uint64_t hash=UINT64_C(1469598103934665603);
    for(int j=0;j<BLOCKS;j++) {
        if (trial && reverse && j==BLOCKS/2) breaks_live(b,8,0);
        if (trial && !reverse && j==BLOCKS/4) breaks_live(b,3,1);
        if (trial && !reverse && j==BLOCKS/2) breaks_live(b,3,0);
        uint64_t prior=x0x_host_ima_decode_calls;
        breaks_render(b,out,BLOCK);
        uint64_t d=x0x_host_ima_decode_calls-prior;
        total+=d;
        calls[j]=d;
        if(d>max)max=d;
        if (fwrite(out,sizeof(float),BLOCK,trace)!=BLOCK) exit(3);
        for(int i=0;i<BLOCK;i++) {
            if(!isfinite(out[i]) || fabsf(out[i])>2.0f) {
                fprintf(stderr,"nonfinite/or oversized audio\n");exit(1);
            }
            uint32_t bits;memcpy(&bits,&out[i],4);
            for(int z=0;z<4;z++) {hash^=(uint8_t)(bits>>(z*8));hash*=UINT64_C(1099511628211);}
            audible+=(out[i]!=0.0f);
        }
    }
    qsort(calls,BLOCKS,sizeof(calls[0]),cmp_u64);
    if(reverse)breaks_live(b,8,0);
    breaks_stop(b);breaks_set_quiet(b,1);
    for(int j=0;j<8;j++)breaks_render(b,out,BLOCK);
    if(!breaks_silent(b)){fprintf(stderr,"stop not silent\n");exit(1);}
    for(int i=0;i<BLOCK;i++)if(out[i]!=0.0f) {fprintf(stderr,"nonzero after stop\n");exit(1);}
    if(!audible){fprintf(stderr,"no audible frames\n");exit(1);}
    printf("n=%" PRIu32 " bars=%d bpm=%d semis=%d dir=%c trial=%d setup=%" PRIu64
           " decodes=%" PRIu64 " max=%" PRIu64 " p95=%" PRIu64
           " p99=%" PRIu64 " pcm=%016" PRIx64 " audible=%" PRIu64 " stopped=1\n",
           n,bars,bpm,semis,reverse?'R':'F',trial,setup,total,max,
           calls[(BLOCKS-1)*95/100],calls[(BLOCKS-1)*99/100],hash,audible);
    free(b);
}
int main(int argc,char **argv) {
    if (argc!=3 || strcmp(argv[1],"--trace")) {
        fputs("usage: breaks_musical_tempo_work_audit --trace /path/to/output.pcm\n",stderr);
        return 2;
    }
    trace=fopen(argv[2],"wb"); if(!trace){perror("trace open");return 3;}
    const uint32_t lengths[]={176400,2116800};
    const int bars[]={1,2,8}, bpms[]={60,120,300}, semis[]={0,12};
    for(unsigned z=0;z<2;z++) {
        uint32_t n=lengths[z],seed=UINT32_C(0x80945371)^n;
        uint8_t*adpcm=malloc((n+1u)/2u);
        if(!adpcm){fputs("alloc failed\n",stderr);return 3;}
        for(uint32_t i=0;i<(n+1u)/2u;i++)adpcm[i]=(uint8_t)rand32(&seed);
        for(unsigned b=0;b<3;b++) {
            breaks_t *prepared=calloc(1,sizeof(*prepared));
            if(!prepared){fputs("alloc failed\n",stderr);return 3;}
            brk_loop_t loop={.adpcm=adpcm,.nsamples=n,.rate=22050,.bars=(uint8_t)bars[b]};
            breaks_init(prepared);breaks_set_loop(prepared,0,&loop);
            if(!prepared->bank[0].valid){fputs("bank invalid\n",stderr);return 1;}
            for(unsigned t=0;t<3;t++) for(unsigned p=0;p<2;p++)
                for(int rev=0;rev<2;rev++)for(int trial=0;trial<2;trial++)
                    one(prepared,n,bars[b],bpms[t],semis[p],rev,trial);
            free(prepared);
        }
        free(adpcm);
    }
    if(fclose(trace)){perror("trace close");return 3;}
    return 0;
}
