/* SPDX-License-Identifier: GPL-3.0-only */
/* Native-only adversarial whole-BREAK renderer: deterministic transport, macro
 * key-up (not MIDI routing), pitch, phase, bank replacement and voice retrigger.
 * Compare byte-exact trace to an independently compiled NIGHTLY baseline.
 * No physical device, ISR timing, host audio callback or target build claim. */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "firmware/src/dsp/breaks.h"

static uint32_t rng(uint32_t *p) { uint32_t x=*p; x^=x<<13; x^=x>>17; x^=x<<5; return *p=x; }
static uint64_t frames, audible, zeros_after_stop;
static int failures;

static void render_checked(breaks_t *b, int blocks, FILE *f, int after_stop)
{
    float v[256];
    for (int j=0;j<blocks;j++) {
        breaks_render(b,v,256);
        for (unsigned i=0;i<256;i++) {
            uint32_t bits;
            if (!isfinite(v[i]) || fabsf(v[i]) > 2.0f) {
                if (failures++ < 5) fprintf(stderr,"invalid PCM sample=%g frame=%llu\n",v[i],(unsigned long long)frames);
            }
            if (v[i]!=0.0f) audible++;
            if (after_stop && v[i]==0.0f) zeros_after_stop++;
            memcpy(&bits,&v[i],sizeof bits);
            if (fwrite(&bits,sizeof bits,1,f)!=1) {perror("trace");exit(3);}
            frames++;
        }
    }
}

static void scenario(uint32_t n, uint32_t salt, FILE *f)
{
    uint32_t n2=n >= 72u ? n-7u : n+7u;
    size_t a_bytes=(n+1u)/2u,b_bytes=(n2+1u)/2u;
    uint8_t *a=malloc(a_bytes),*alt=malloc(b_bytes);
    breaks_t *b=calloc(1,sizeof(*b));
    if (!a || !alt || !b) {fputs("allocation failed\n",stderr);exit(3);}
    uint32_t seed=salt ^ n;
    for(size_t i=0;i<a_bytes;i++)a[i]=(uint8_t)rng(&seed);
    for(size_t i=0;i<b_bytes;i++)alt[i]=(uint8_t)rng(&seed);
    brk_loop_t la={.adpcm=a,.nsamples=n,.rate=44100,.bars=1};
    brk_loop_t lb={.adpcm=alt,.nsamples=n2,.rate=22050,.bars=2};
    breaks_init(b);
    breaks_set_loop(b,0,&la);
    breaks_set_loop(b,1,&lb);
    if (!b->bank[0].valid || !b->bank[1].valid) {fputs("bad bank\n",stderr);exit(1);}
    breaks_set(b,BRK_LEVEL,119);
    breaks_set(b,BRK_BCHANCE,73);
    breaks_set(b,BRK_COMPLEX,43);
    breaks_set(b,BRK_ROLL,18);
    uint16_t held=0;
    for(int step=0;step<64;step++) {
        uint32_t x=rng(&seed);
        int key=(int)((x>>13)%11u);
        uint16_t bit=(uint16_t)(1u<<key);
        held^=bit;
        breaks_live(b,key,(held&bit)!=0);
        if ((step%3)==0) breaks_set(b,BRK_PITCH,6+(int)(x%13u));
        if ((step%5)==1) breaks_pin(b,(int)((x>>19)&7u));
        if ((step%7)==4) breaks_set(b,BRK_ROLL,(int)((x>>9)%101u));
        if ((step%11)==8) breaks_set_loop(b,step&1?0:1,step&1?&la:&lb);
        if ((step%13)==7) breaks_set_quiet(b,1);
        if ((step%13)==9) breaks_set_quiet(b,0);
        if (step==25 || step==49) breaks_stop(b);
        breaks_step(b,step%16,step/16,256.0f+(float)((x>>7)%8000u), (x&7u)!=0);
        render_checked(b,7,f,0);
    }
    /* Release every live key; turn off sound, stop and drain release/quiet tail. */
    for (int key=0;key<=10;key++)breaks_live(b,key,0);
    breaks_stop(b);
    breaks_set_quiet(b,1);
    render_checked(b,8,f,1);
    if (!breaks_silent(b)) {
        if (failures++ < 5) fprintf(stderr,"post-stop not silent n=%u\n",n);
    }
    free(b);free(alt);free(a);
}

int main(int argc, char **argv)
{
    if(argc!=3 || strcmp(argv[1],"--trace")){
        fputs("usage: breaks_renderer_adversarial_test --trace output.pcm\n",stderr);
        return 2;
    }
    FILE *f=fopen(argv[2],"wb");
    if(!f){perror(argv[2]);return 3;}
    static const uint32_t lens[]={64,65,511,512,513,1024,4096,176400,705600,2116800};
    for(unsigned i=0;i<sizeof lens/sizeof lens[0];i++){
        scenario(lens[i],0x35483247u+i*9187u,f);
        scenario(lens[i],0x92713441u+i*3371u,f);
    }
    if(fclose(f)){perror("close");return 3;}
    printf("ADVERSARIAL_TRACE scenarios=%zu frames=%llu audible=%llu zeros_after_stop=%llu failures=%d\n",
           sizeof lens/sizeof lens[0]*2,(unsigned long long)frames,(unsigned long long)audible,
           (unsigned long long)zeros_after_stop,failures);
    if(!audible || !zeros_after_stop || failures) return 1;
    return 0;
}
