/* SPDX-License-Identifier: GPL-3.0-only */
/* BREAK reverse ADPCM checkpoint/refill bound and native-host throughput PROXY.
 * It models bank_seek, checkpoint-build and rev_fetch with an optional saved
 * adjacent reverse boundary state (not full renderer instrumentation).
 * from firmware/src/dsp/breaks.c at NIGHTLY 58b5b45a2dd90581de7a674c331ca5cbe8a67c7e.
 * It calls the actual shared production ima_decode() from ima_adpcm.h.
 * No firmware is compiled or flashed. Not a target CPU timing or audio XRUN test.
 *
 * gcc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -ffp-contract=off \
 *   -o /tmp/brk-rev tests/host/breaks_reverse_seek_perf_test.c
 * /tmp/brk-rev --check
 * /tmp/brk-rev --bench
 * Use -DBRK_PROFILE_NCK=480 -DBRK_SEEK_CACHE=0|1 for before/after.
 * The test exercises a faithful model of the unchanged seeks/refills, NOT
 * direct breaks.c instrumentation. Run the existing full BREAK regressions too.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../firmware/src/dsp/ima_adpcm.h"
#include "../../firmware/src/dsp/breaks.h"

#ifndef BRK_PROFILE_NCK
#define BRK_PROFILE_NCK BRK_NCK /* candidate header default, override for baseline */
#endif
#define BRK_PROFILE_RCH BRK_RCH
#ifndef BRK_SEEK_CACHE
#define BRK_SEEK_CACHE 1
#endif
#define TRIALS 9u
#define SPAN 65536u
#if BRK_PROFILE_NCK < 32 || BRK_PROFILE_NCK > 2048
#error "BRK_PROFILE_NCK must remain in the finite benchmark range 32..2048"
#endif
#if (BRK_PROFILE_RCH & (BRK_PROFILE_RCH - 1u)) != 0
#error "BRK_PROFILE_RCH must be a power of two"
#endif

typedef struct { int16_t pred; uint8_t idx, rsv; } ck_t;
typedef struct {
    const uint8_t *adpcm;
    uint32_t n;
    ck_t ck[BRK_PROFILE_NCK];
    uint8_t ck_shift;
} bank_t;
typedef struct {
    int16_t rbuf[BRK_PROFILE_RCH];
    uint32_t rbase;
    uint8_t rvalid;
#if BRK_SEEK_CACHE
    ck_t prev_seek_state;
    uint32_t prev_seek_pos;
#endif
} voice_t;
static uint64_t count_decode, max_refill;
static volatile uint64_t sink;
static uint32_t rnd(uint32_t *s) {uint32_t x=*s;x^=x<<13; x^=x>>17; x^=x<<5; return *s=x;}
static int16_t counted_decode(const uint8_t *d, uint32_t i, int32_t *p, int32_t *idx)
{
    ++count_decode;
    return (int16_t)ima_decode(d, i, p, idx);
}

/* Source-matched checkpoint construction: state is BEFORE nibble i. */
static void prepare_bank(bank_t *k, const uint8_t *d, uint32_t n)
{
    uint32_t sh;
    int32_t pred=0, idx=0;
    k->adpcm=d; k->n=n;
    for (sh=6; ((n+(1u<<sh)-1u)>>sh)>BRK_PROFILE_NCK; ++sh) {}
    k->ck_shift=(uint8_t)sh;
    for(uint32_t i=0;i<n;i++) {
        if(!(i & ((1u<<sh)-1u))) {
            k->ck[i>>sh].pred=(int16_t)pred;
            k->ck[i>>sh].idx=(uint8_t)idx;
        }
        ima_decode(d,i,&pred,&idx); /* setup costs are not render-time costs */
    }
}
/* These two helpers mirror breaks.c bank_seek and rev_fetch. */
static void bank_seek(const bank_t *k, voice_t *v, uint32_t p, int32_t *pred, int32_t *idx)
{
#if BRK_SEEK_CACHE
    if (v->prev_seek_pos == p) {
        *pred = v->prev_seek_state.pred;
        *idx = v->prev_seek_state.idx;
        v->prev_seek_pos = UINT32_MAX;
        return;
    }
#else
    (void)v;
#endif
    uint32_t c=p>>k->ck_shift, i=c<<k->ck_shift;
    *pred=k->ck[c].pred; *idx=k->ck[c].idx;
#if BRK_SEEK_CACHE
    if(p >= BRK_PROFILE_RCH && p-BRK_PROFILE_RCH >= i) {
        uint32_t target=p-BRK_PROFILE_RCH;
        while(i<target) counted_decode(k->adpcm,i++,pred,idx);
        v->prev_seek_state=(ck_t){(int16_t)*pred,(uint8_t)*idx,0};
        v->prev_seek_pos=target;
    } else v->prev_seek_pos=UINT32_MAX;
#endif
    while(i<p) counted_decode(k->adpcm,i++,pred,idx);
}
static int16_t rev_fetch(voice_t *v, const bank_t *k, uint32_t q)
{
    if (!v->rvalid || q < v->rbase || q >= v->rbase + BRK_PROFILE_RCH) {
        uint64_t before=count_decode;
        uint32_t base=q & ~(uint32_t)(BRK_PROFILE_RCH-1u), m=k->n-base, i;
        int32_t pred,idx;
        if(m>BRK_PROFILE_RCH) m=BRK_PROFILE_RCH;
        bank_seek(k,v,base,&pred,&idx);
        for(i=0;i<m;i++) v->rbuf[i]=counted_decode(k->adpcm,base+i,&pred,&idx);
        v->rbase=base; v->rvalid=1;
        if(count_decode-before>max_refill) max_refill=count_decode-before;
    }
    return v->rbuf[q-v->rbase];
}
static int prepare(uint32_t n, uint8_t **dp, int16_t **pcm, bank_t *k)
{
    uint8_t *d=malloc((n+1u)/2u); int16_t *out=malloc(n*sizeof(*out));
    if(!d || !out){free(d);free(out);return 1;}
    uint32_t seed=0x93d43fa5u;for(uint32_t i=0;i<(n+1u)/2u;i++) d[i]=(uint8_t)rnd(&seed);
    int32_t p=0,idx=0;for(uint32_t i=0;i<n;i++)out[i]=(int16_t)ima_decode(d,i,&p,&idx);
    prepare_bank(k,d,n);*dp=d;*pcm=out;return 0;
}
static int check_one(uint32_t n)
{
    bank_t *k=malloc(sizeof(*k)); uint8_t *d; int16_t *pcm;
    if(!k||prepare(n,&d,&pcm,k)){free(k);return 1;}
    voice_t v={0};
#if BRK_SEEK_CACHE
    v.prev_seek_pos = UINT32_MAX;
#endif
    count_decode=max_refill=0;
    /* Check every sample in a long descending scan. The cache path is exercised. */
    for (uint32_t q=n;q>0;) {--q; if(rev_fetch(&v,k,q)!=pcm[q]) {
        fprintf(stderr,"FAIL descending n=%u pos=%u shift=%u\n",n,q,k->ck_shift);
        free(k);free(d);free(pcm);return 1;
    }}
    /* Nonadjacent requests exercise both cache misses and odd tail lengths. */
    uint32_t seed=0x791de530u;
    for(unsigned j=0;j<2048;j++){uint32_t q=rnd(&seed)%n;
        if(rev_fetch(&v,k,q)!=pcm[q]){fprintf(stderr,"FAIL random n=%u pos=%u\n",n,q);free(k);free(d);free(pcm);return 1;}}
    /* A refill starts from a checkpoint, then decodes at most a full checkpoint interval. */
    uint32_t bound=(1u<<k->ck_shift) > BRK_PROFILE_RCH ? (1u<<k->ck_shift) : BRK_PROFILE_RCH;
    if(max_refill>bound){
        fprintf(stderr,"FAIL bound n=%u peak=%llu bound=%u\n",n,(unsigned long long)max_refill,bound);
        free(k);free(d);free(pcm);return 1;
    }
    printf("CHECK PASS n=%u shift=%u scans=%u random=%u peak_refill_decodes=%llu bound=%u\n",
           n,k->ck_shift,n,2048u,(unsigned long long)max_refill,bound);
    free(k);free(d);free(pcm);return 0;
}
static uint64_t ns(void){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return 0;return (uint64_t)t.tv_sec*1000000000ull+(uint64_t)t.tv_nsec;}
static int cmp(const void *a,const void *b){double x=*(const double*)a,y=*(const double*)b;return (x>y)-(x<y);}
static int benchmark_one(uint32_t n)
{
    bank_t *k=malloc(sizeof(*k));uint8_t *d;int16_t *pcm;
    if(!k||prepare(n,&d,&pcm,k)){free(k);return 1;}
    double samples[TRIALS];uint64_t peaks[TRIALS],decodes[TRIALS];
    const uint32_t len=n<SPAN?n:SPAN;
    for(unsigned t=0;t<TRIALS;t++){
        voice_t v={0};
#if BRK_SEEK_CACHE
        v.prev_seek_pos=UINT32_MAX;
#endif
        count_decode=max_refill=0;uint64_t start=ns(),sum=0;
        for(uint32_t q=n;q>n-len;) {--q;sum+=(uint16_t)rev_fetch(&v,k,q);}
        uint64_t end=ns();sink^=sum;if(!start||end<start){free(k);free(d);free(pcm);return 2;}
        samples[t]=(double)(end-start)/len;peaks[t]=max_refill;decodes[t]=count_decode;
    }
    double sorted[TRIALS],spread[TRIALS];memcpy(sorted,samples,sizeof sorted);qsort(sorted,TRIALS,sizeof sorted[0],cmp);
    for(unsigned i=0;i<TRIALS;i++){double z=samples[i]-sorted[TRIALS/2u];spread[i]=z<0?-z:z;}
    qsort(spread,TRIALS,sizeof spread[0],cmp);
    uint64_t peak=0;for(unsigned i=0;i<TRIALS;i++) if(peaks[i]>peak)peak=peaks[i];
    printf("BENCH n=%u checkpoints=%u shift=%u bytes_ck_per_bank=%zu trials=%u samples_per_trial=%u median_ns_per_sample=%.3f MAD_ns=%.3f min_ns=%.3f max_ns=%.3f total_refill_decodes=%llu peak_refill_decodes=%llu sink=%llu\n",
          n,(unsigned)BRK_PROFILE_NCK,k->ck_shift,sizeof(k->ck),TRIALS,len,
          sorted[TRIALS/2u],spread[TRIALS/2u],sorted[0],sorted[TRIALS-1],
          (unsigned long long)decodes[TRIALS/2u],(unsigned long long)peak,(unsigned long long)sink);
    free(k);free(d);free(pcm);return 0;
}
int main(int argc,char **argv)
{
    if(argc!=2 || (strcmp(argv[1],"--check") && strcmp(argv[1],"--bench"))) {
        fprintf(stderr,"usage: %s --check|--bench\n",argv[0]);return 2;
    }
    const uint32_t check_sizes[]={64,65,511,512,513,122880,122881,176400,705600,1411200,2116800};
    const uint32_t bench_sizes[]={513,176400,705600,1411200};
    const uint32_t *sizes = !strcmp(argv[1], "--check") ? check_sizes : bench_sizes;
    size_t n_sizes = !strcmp(argv[1], "--check") ? sizeof check_sizes/sizeof check_sizes[0] : sizeof bench_sizes/sizeof bench_sizes[0];
    for(size_t i=0;i<n_sizes;i++) {
        int rc=strcmp(argv[1],"--check")==0?check_one(sizes[i]):benchmark_one(sizes[i]);
        if(rc) return rc;
    }
    return 0;
}
