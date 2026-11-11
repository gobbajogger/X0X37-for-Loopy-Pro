/* SPDX-License-Identifier: GPL-3.0-only */
/* Compile this unchanged against baseline and candidate; compare emitted audio.
 * No private captures, hardware access or target-CPU claim. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "wobble.h"

static float l[256], r[256];
static volatile float sink;
static void emit(wobble_t *s, unsigned n)
{
    wobble_render(s, l, r, n);
    for (unsigned i=0; i<n; ++i) assert(isfinite(l[i]) && isfinite(r[i]));
    assert(fwrite(l, sizeof(float), n, stdout)==n);
    assert(fwrite(r, sizeof(float), n, stdout)==n);
}
static void setup(wobble_t *s, float sr, unsigned mode)
{
    wobble_init(s, sr);
    wobble_set(s,W_CARRIER_WAVE,(float)(mode&1));
    wobble_set(s,W_WOBBLE_SHAPE,(float)((mode>>1)&1));
    wobble_set(s,W_WOBBLE_DEPTH,.8f);
    wobble_set(s,W_XMOD_LEVEL, mode&4 ? .7f : 0);
    wobble_set(s,W_XMOD_DEPTH, mode&4 ? .6f : 0);
    wobble_set(s,W_CRUSH, mode&8 ? .75f : 0);
    wobble_set(s,W_STEREO,.8f);
    wobble_note_on(s,48,101);
}
static double now(void)
{
    struct timespec t; assert(!clock_gettime(CLOCK_MONOTONIC,&t));
    return (double)t.tv_sec+1e-9*(double)t.tv_nsec;
}
int main(int argc,char **argv)
{
    wobble_t s;
    if (argc==2 && !strcmp(argv[1],"--bench")) {
        for(unsigned mode=0;mode<16;++mode) {
            setup(&s,44100,mode);
            double t=now();
            for(unsigned b=0;b<2048;++b) {
                if(!(b%32)) wobble_note_on_slide(&s,(uint8_t)(36+b%48),100,b&1);
                wobble_render(&s,l,r,256); sink=l[b%256]+r[(b+1)%256];
            }
            printf("%u %.9f\n",mode,now()-t);
        }
        return 0;
    }
    const float rates[]={8000,44100,48000,96000,192000};
    for(unsigned sr=0;sr<sizeof(rates)/sizeof(rates[0]);++sr) {
        for(unsigned mode=0;mode<16;++mode) {
            setup(&s,rates[sr],mode); emit(&s,256);
            /* Every 7-bit control, while running, including cached controls,
             * unrelated controls, toggles and complete preset-like loads. */
            for(unsigned p=0;p<W_PARAMETER_COUNT;++p) {
                for(unsigned v=0;v<128;++v) {
                    assert(wobble_set(&s,p,(float)v/127.0f));
                    emit(&s,1+(v%17));
                }
            }
            /* Nonfinite rejection, clamping and resets must retain parity. */
            for(unsigned p=0;p<W_PARAMETER_COUNT;++p) {
                assert(!wobble_set(&s,p,NAN));
                assert(!wobble_set(&s,p,INFINITY));
                assert(wobble_set(&s,p,-1)); emit(&s,3);
                assert(wobble_set(&s,p,2)); emit(&s,3);
                wobble_reset(&s); wobble_note_on(&s,60,127); emit(&s,17);
            }
            wobble_set(&s,W_DAMPER,0); wobble_all_notes_off(&s);
            for(unsigned b=0;b<256;++b) emit(&s,256);
            emit(&s,0); wobble_note_on(&s,127,127); emit(&s,256);
            /* Reinitialization on an already-used object refreshes SR caches. */
            wobble_init(&s,rates[(sr+1)%5]); wobble_note_on(&s,48,90); emit(&s,256);
        }
    }
    return 0;
}
