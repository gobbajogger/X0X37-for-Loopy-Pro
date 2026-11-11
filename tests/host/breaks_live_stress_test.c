/* SPDX-License-Identifier: GPL-3.0-only */
/* Producer-only BREAK stress: live transport, tempo, reverse, retrigger, replacement. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <inttypes.h>
#include "firmware/src/dsp/breaks.h"

#define BLOCK 64
#define N_BLOCKS 320
static uint32_t rng(uint32_t *s) { uint32_t x = *s; x ^= x << 13; x ^= x >> 17; x ^= x << 5; return *s = x; }
static FILE *pcm;
static uint64_t fnhash = UINT64_C(1469598103934665603);
static void feed(const float *v, unsigned n, uint64_t *audible) {
    for (unsigned i = 0; i < n; ++i) {
        if (!isfinite(v[i]) || fabsf(v[i]) > 2.f) { fputs("non-finite/out-of-range PCM\n",stderr); exit(1); }
        if (v[i] != 0.f) ++*audible;
        uint32_t word; memcpy(&word, &v[i], 4);
        for (unsigned j = 0; j < 4; ++j) { fnhash ^= (uint8_t)(word >> (j * 8)); fnhash *= UINT64_C(1099511628211); }
    }
    if (fwrite(v,sizeof(*v),n,pcm)!=n) { perror("PCM trace fwrite"); exit(3); }
}
static void one(uint32_t n, unsigned rate_index) {
    static const float rates[] = {31.875f, 32.125f, 64.875f, 257.125f};
    uint32_t seed = 0x98137a5u ^ n ^ rate_index;
    uint8_t *raw = malloc((n+1u)/2u);
    breaks_t *b = calloc(1,sizeof(*b));
    if (!raw || !b) exit(3);
    for(uint32_t i=0; i<(n+1u)/2u; ++i) raw[i]=(uint8_t)rng(&seed);
    brk_loop_t loop = {.adpcm = raw, .nsamples=n, .rate=22050, .bars=2};
    breaks_init(b);
    breaks_set_loop(b,0,&loop);
    if (!b->bank[0].valid) exit(2);
    breaks_set(b,BRK_LEVEL,127);
    breaks_set(b,BRK_PITCH,12);
    float sp16 = ((float)n/32.f) / rates[rate_index];
    breaks_live(b,8,1);
    breaks_step(b,0,0,sp16,1);
    float out[BLOCK]; uint64_t audible=0;
    for (int blk=0; blk<N_BLOCKS; ++blk) {
        /* Deliberately cross 2-ms fades while sustaining/letting go of keys. */
        if (blk==15 || blk==158) breaks_live(b,6,1);
        if (blk==32 || blk==175) breaks_live(b,6,0);
        if (blk==41 || blk==89 || blk==145 || blk==205 || blk==259) breaks_live(b,8,0);
        if (blk==64 || blk==100 || blk==176 || blk==216 || blk==273) breaks_live(b,8,1);
        if (blk==70) breaks_live(b,9,1);
        if (blk==103) breaks_live(b,9,0);
        if (blk==116) breaks_set(b,BRK_PITCH,24);
        if (blk==139) breaks_set(b,BRK_PITCH,12);
        if (blk==155) breaks_set_loop(b,0,&loop); /* invalidate seek caches */
        if (blk==190) breaks_pin(b,4);
        if (blk==208) breaks_set(b,BRK_PITCH,0);
        if (blk==239) breaks_set(b,BRK_PITCH,12);
        if ((blk%16)==0 && blk>0) {
            float tempo = sp16 * ((blk/16)&1 ? 1.125f : 0.875f);
            breaks_step(b,(blk/16)&15,blk/256,tempo, (blk%64)!=0);
        }
        if (blk==N_BLOCKS-12) breaks_stop(b);
        if (blk==N_BLOCKS-10) breaks_set_quiet(b,1);
        breaks_render(b,out,BLOCK);
        feed(out,BLOCK,&audible);
    }
    if (!audible) { fputs("no audible samples\n",stderr); exit(1); }
    breaks_stop(b); breaks_set_quiet(b,1);
    for(int j=0;j<8;j++) breaks_render(b,out,BLOCK);
    for(int i=0;i<BLOCK;i++) if (out[i]!=0.f) {fputs("STOP is not silent\n",stderr); exit(1);}
    printf("n=%" PRIu32 " rate_ix=%u aud=%" PRIu64 " pcm_hash=%016" PRIx64 " silent=1\n",n,rate_index,audible,fnhash);
    free(raw); free(b);
}
int main(int argc,char **argv) {
    if(argc!=2){fputs("usage: <program> /path/to/pcm\n",stderr);return 2;}
    pcm=fopen(argv[1],"wb"); if(!pcm) {perror("pcm open");return 3;}
    const uint32_t sizes[] = {65u,4097u,176400u,2116800u};
    for(unsigned i=0;i<4;++i)for(unsigned r=0;r<4;++r)one(sizes[i],r);
    if(fclose(pcm)) {perror("pcm close");return 3;}
    return 0;
}
