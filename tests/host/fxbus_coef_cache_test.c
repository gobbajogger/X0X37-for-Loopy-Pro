/* SPDX-License-Identifier: GPL-3.0-only */
/* Catch stale caches, rounded coefficients, boot-default changes, or history
 * changes by comparing the pinned old implementation after every operation. */
#define X0X_DRUM_TABLES_DEFINE
#include "fxbus.h"
const char *const x0x_dist_names[8] = {"Off","Diode","Clip","SAT","BFZ","PDIST","Fold","Crush"};
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

size_t ref_size(void);
void ref_init(void *, int16_t *, int);
void ref_set(void *, int, int);
void ref_bpm(void *, float);
void ref_process(void *, const float *, const float *, const float *, float *, int);
void ref_st(void *, const float *, const float *, const float *, const float *, float *, float *, int);
void ref_snapshot(void *, void *);

static uint32_t rng;
static unsigned blocks, samples, states;
static uint32_t draw(void) { rng ^= rng<<13; rng ^= rng>>17; rng ^= rng<<5; return rng; }
static float noise(void) { return (float)((int)(draw()&65535u)-32768) * (1.0f/32768.0f); }
static void check(int ok, const char *what) {
    if (!ok) { fprintf(stderr,"FAIL %s: block=%u samples=%u rng=%u\n",what,blocks,samples,rng); exit(1); }
}
static void state(fxbus_t *s, void *old, int16_t *ring, int16_t *oldring, int len) {
    unsigned char a[sizeof *s], b[sizeof *s];
    ref_snapshot(old,b);
    memcpy(a,s,sizeof a);
    /* Pointer identity is runtime metadata. All DSP fields,
     * coefficients, delay/reverb histories, cursors and other padding must match. */
    memset(a+offsetof(fxbus_t,dbuf),0,sizeof s->dbuf);
    memset(b+offsetof(fxbus_t,dbuf),0,sizeof s->dbuf);
    check(!memcmp(a,b,sizeof a),"complete state (normalised cached scalar)");
    check(!memcmp(ring,oldring,(size_t)len*sizeof *ring),"delay ring");
    states++;
}
static void set(fxbus_t *s, void *old, int i, int v) { fxbus_set(s,i,v); ref_set(old,i,v); }

int main(void) {
    check(sizeof(fxbus_t)==ref_size(),"zero extra state RAM");
    for (unsigned seed=1;seed<=8;seed++) for (unsigned stereo=0;stereo<2;stereo++) {
        const int len=4096;
        fxbus_t s;
        void *old=calloc(1,ref_size());
        int16_t ring[4096], oldring[4096];
        check(old!=NULL,"allocation");
        rng=seed*0x9e3779b9u;
        fxbus_init(&s,ring,len); ref_init(old,oldring,len);
        state(&s,old,ring,oldring,len);
        /* First equal-default write must apply the pot mapping, not retain the
         * distinct exact boot default (RvTone=.45, pot=57). */
        set(&s,old,FX_RV_TONE,57);
        set(&s,old,FX_RV_TONE,57);
        state(&s,old,ring,oldring,len);
        for (unsigned block=0;block<768;block++) {
            float dry[256], right[256], rev[256], delay[256], a[256], b[256], ar[256], br[256];
            int n=block%17==0 ? 0 : block%19==0 ? 1 : (int)(1+draw()%256u);
            /* All 128 tone values, repeated values, and changes during tails. */
            if (block<256 || block%5==0) {
                int v=(int)((block/2)%128);
                set(&s,old,FX_RV_TONE,v);
                set(&s,old,FX_DL_TONE,127-v);
            }
            if (block%13==0) {
                int i=(int)(draw()%FX_NPARAMS);
                set(&s,old,i,(int)(draw()%180u)-20);
            }
            if (block%29==0) { float bpm=20.0f+(float)(draw()%2800u)*.1f; fxbus_set_bpm(&s,bpm); ref_bpm(old,bpm); }
            if (block%67==0) { s.muted=(uint8_t)(draw()%8u); /* mirror through raw snapshot offset */
                ((unsigned char *)old)[offsetof(fxbus_t,muted)]=s.muted; }
            state(&s,old,ring,oldring,len);
            for (int i=0;i<n;i++) {
                dry[i]=noise()*.2f; right[i]=noise()*.2f;
                rev[i]=block%96<48 ? noise()*.1f : 0;
                delay[i]=block%96<48 ? noise()*.1f : 0;
            }
            if (stereo) {
                fxbus_process_st(&s,dry,right,rev,delay,a,ar,n);
                ref_st(old,dry,right,rev,delay,b,br,n);
                check(!memcmp(ar,br,(size_t)n*sizeof *ar),"right samples");
            } else { fxbus_process(&s,dry,rev,delay,a,n); ref_process(old,dry,rev,delay,b,n); }
            check(!memcmp(a,b,(size_t)n*sizeof *a),"left/mono samples");
            state(&s,old,ring,oldring,len);
            blocks++; samples+=(unsigned)n*(stereo?2:1);
        }
        free(old);
    }
    printf("PASS fxbus coefficient cache: %u blocks, %u samples, %u states, sizeof=%zu (unchanged)\n",blocks,samples,states,sizeof(fxbus_t));
    return 0;
}
