/* SPDX-License-Identifier: GPL-3.0-only */
/* A driver's audio callback partition must not change BREAK's sample timeline.
 * Compare identical note/clock/tempo/bank events rendered in 1..256-frame chunks
 * against a single 256-frame callback. Compile with production breaks.c. */
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "firmware/src/dsp/breaks.h"

#define QUANTUM 256u
#define QUANTA 32u
#define NSAMPLES (QUANTUM * QUANTA)
static uint32_t rnd(uint32_t *p) { uint32_t s=*p; s^=s<<13; s^=s>>17; s^=s<<5; return *p=s; }
static void require(int ok, const char *reason)
{ if (!ok) { fprintf(stderr,"FAIL %s\n", reason); exit(1); } }
static uint64_t reached_live, reached_reverse, reached_bank, reached_stop;

/* Split each 256-sample event epoch differently. All inputs change ONLY at epoch boundaries. */
static const uint16_t partitions[5][6] = {
    {256,0,0,0,0,0}, {64,64,64,64,0,0}, {128,128,0,0,0,0},
    {1,7,13,31,64,140}, {1,1,1,1,1,1} /* special: repeated 1-frame chunks */
};

static void render_quantum(breaks_t *b, float *dst, unsigned partition)
{
    unsigned off=0, count=0;
    while (off<QUANTUM) {
        unsigned n;
        if (partition==4) n=1;
        else { n=partitions[partition][count++]; require(n!=0,"bad partition"); }
        require(n<=256 && off+n<=QUANTUM,"bad partition span");
        struct {uint32_t lo;float pcm[256];uint32_t hi;} guard;
        guard.lo=UINT32_C(0x1234bcde); guard.hi=UINT32_C(0xdebc4321);
        for (unsigned i=0;i<256;i++) guard.pcm[i]=NAN;
        breaks_render(b,guard.pcm,(int)n);
        require(guard.lo==UINT32_C(0x1234bcde)&&guard.hi==UINT32_C(0xdebc4321),"canary");
        for (unsigned i=n;i<256;i++) require(isnan(guard.pcm[i]),"wrote past requested chunk length");
        for (unsigned i=0;i<n;i++) require(isfinite(guard.pcm[i]) && fabsf(guard.pcm[i])<=2.f,"PCM bounds");
        /* IEEE negative zero is acoustically the same as zero. A dormant
         * voice can produce -0 until breaks_silent short-circuits to +0.
         * Canonicalize only zero signs; all nonzero PCM must match bitwise. */
        for (unsigned i=0;i<n;i++) dst[off+i]=(guard.pcm[i]==0.f)?0.f:guard.pcm[i];
        off+=n;
    }
}

static void playback(uint32_t n,unsigned seed,unsigned partition,float *dst)
{
    uint32_t r=UINT32_C(0x9937fcda)^n^(UINT32_C(0x9e3779b9)*seed);
    uint8_t *data_a=malloc((n+1u)/2u),*data_b=malloc((n+1u)/2u);
    breaks_t *b=calloc(1,sizeof(*b));
    require(data_a&&data_b&&b,"allocation");
    for (uint32_t i=0;i<(n+1u)/2u;i++) {data_a[i]=(uint8_t)rnd(&r);data_b[i]=(uint8_t)rnd(&r);}
    brk_loop_t a={.adpcm=data_a,.nsamples=n,.rate=22050,.bars=1};
    brk_loop_t c={.adpcm=data_b,.nsamples=n,.rate=22050,.bars=1};
    breaks_init(b);breaks_set_loop(b,0,&a);breaks_set_loop(b,1,&c);
    require(b->bank[0].valid&&b->bank[1].valid,"valid banks");
    breaks_set(b,BRK_LEVEL,127);breaks_set(b,BRK_COMPLEX,96);
    breaks_set(b,BRK_BCHANCE,100);breaks_set(b,BRK_R2,95);
    breaks_set(b,BRK_R4,100);breaks_set(b,BRK_ALEN,0);
    breaks_set(b,BRK_BLEN,0);
    float sp16=((float)n/16.f)/((seed&1u)?64.875f:256.125f);
    unsigned held=0;
    for (unsigned epoch=0;epoch<QUANTA;epoch++) {
        if (epoch==24) {
            for (int key=0;key<=10;key++) breaks_live(b,key,0);
            breaks_stop(b);breaks_set_quiet(b,1);
            reached_stop++;
        }
        if (epoch<24) {
            if (epoch%3u==0u) {unsigned down=(epoch/3u)&1u;breaks_live(b,8,(int)down);reached_reverse++;}
            if (epoch%5u==0u) {unsigned key=(epoch/5u)%8u;
                if (held<8u) breaks_live(b,(int)held,0);
                breaks_live(b,(int)key,1);held=key;reached_live++;
            }
            if (epoch%7u==0u) breaks_set(b,BRK_PITCH,(int)((epoch*9u+seed)%25u));
            if (epoch%11u==0u && epoch>0u) {
                breaks_set_loop(b,(int)((epoch/11u)&1u),((epoch/11u)&1u)?&c:&a);reached_bank++;
            }
            if (epoch%4u==0u) {
                if (epoch>0) sp16*=(epoch&4u)?1.125f:0.875f;
                breaks_step(b,(int)((epoch/4u)&15u),(int)(epoch/64u),sp16,1);
            }
        }
        render_quantum(b,dst+(size_t)epoch*QUANTUM,partition);
        if (epoch==QUANTA-1) require(breaks_silent(b),"STOP must finish silent");
    }
    free(b);free(data_a);free(data_b);
}

int main(int argc,char **argv)
{
    if (argc!=2) { fputs("usage: breaks_chunk_partition_property_test PCM_FILE\n",stderr);return 2; }
    FILE *fp=fopen(argv[1],"wb");
    if (!fp) {perror("PCM_FILE");return 3;}
    const uint32_t lengths[]={65,513,4097,176400,2116800};
    float *reference=calloc(NSAMPLES,sizeof(float)),*other=calloc(NSAMPLES,sizeof(float));
    require(reference&&other,"PCM allocation");
    for (unsigned i=0;i<sizeof(lengths)/sizeof(lengths[0]);i++) for(unsigned s=0;s<2;s++) {
        playback(lengths[i],s,0,reference);
        for (unsigned part=1;part<5;part++) {
            playback(lengths[i],s,part,other);
            if (memcmp(reference,other,NSAMPLES*sizeof(float))) {
                for(unsigned j=0;j<NSAMPLES;j++) if (memcmp(&reference[j],&other[j],sizeof(float))) {
                    fprintf(stderr,"FAIL chunk invariant n=%" PRIu32 " seed=%u mode=%u sample=%u %g vs %g\n",
                            lengths[i],s,part,j,(double)reference[j],(double)other[j]);
                    return 1;
                }
                return 1;
            }
        }
        require(fwrite(reference,sizeof(float),NSAMPLES,fp)==NSAMPLES,"write PCM");
        printf("n=%" PRIu32 " seed=%u partitions=5 exact=1 stopped=1\n",lengths[i],s);
    }
    require(reached_live>0&&reached_reverse>0&&reached_bank>0&&reached_stop>0,"coverage");
    printf("events: held=%" PRIu64 " reverse=%" PRIu64 " bank=%" PRIu64 " stop=%" PRIu64 "\n",
           reached_live,reached_reverse,reached_bank,reached_stop);
    free(reference);free(other);
    if (fclose(fp)) {perror("close PCM");return 3;}
    return 0;
}
