/* SPDX-License-Identifier: GPL-3.0-only */
/* Stock data and actual six-voice Juno DSP render; no JS synthesis or mock audio.
 * This is not a full device/master-mixer headroom or persistence qualification. */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/juno_stock_bank.h"
#include "../../firmware/src/dsp/juno6.h"

#define N (sizeof original_sound_juno_factories / sizeof original_sound_juno_factories[0])
#define SR 44100u
#define BLOCK 256u
#define HELD (SR * 3u / 4u)
#define TOTAL (SR * 2u)
static const uint8_t legacy[4][16] = {
    {42,28,70,18,45,8,40,92,40,0,15,8,64,25,45,100},
    {90,18,42,24,35,65,55,95,75,0,12,15,55,40,55,100},
    {15,50,58,36,80,0,35,30,20,5,20,0,64,25,18,110},
    {70,10,105,12,20,0,20,110,15,12,25,5,80,60,50,96}
};
static const char *const legacy_names[4] = {
    "JUNO POLY", "JUNO PAD", "JUNO PLUCK", "JUNO BRASS"
};
/* Distinct musical scenes: bass registers, keys/chords, lead range, slow strings. */
static const uint8_t notes[8][4] = {
    {48,55,60,0}, {48,55,60,0}, {60,64,67,0}, {60,64,67,0},
    {36,48,0,0}, {48,52,55,60}, {60,72,84,0}, {48,55,60,64}
};
static const uint8_t stress_notes[6] = {48,52,55,60,64,67};
static const uint8_t velocities[8] = {98,98,95,95,96,96,94,96};
static void setup(juno6_t *s, juno6_chorus_t *c, unsigned p)
{
    juno6_init(s, SR);
    juno6_chorus_init(c, SR);
    for (unsigned i=0;i<16;i++)
        assert(juno6_set(s, i, (float)original_sound_juno_factories[p][i]/127.0f));
    assert(s->macro[16] == .5f && s->macro[17] == 0.0f);
    juno6_chorus_set(c, .1f + 4.9f*s->macro[13], 5.4f, .5f*s->macro[14]);
}
typedef struct { double dry_rms, wet_rms; float dry_peak, wet_peak;
                 unsigned dry_clip, wet_clip, frames; uint64_t identity; } result_t;
static void hash_pcm(uint64_t *hash, float x)
{
    int32_t q = (int32_t)lrintf(x * 1048576.0f);
    *hash ^= (uint32_t)q;
    *hash *= UINT64_C(1099511628211);
}
static result_t render(unsigned p, int stress)
{
    juno6_t s; juno6_chorus_t c;
    float mono[BLOCK], l[BLOCK], r[BLOCK];
    result_t out = {0};
    unsigned total=0, nnotes=0;
    setup(&s,&c,p);
    if (stress) {
        for (unsigned i=0;i<6;i++)assert(juno6_note_on(&s,stress_notes[i],110));
        nnotes=6;
    } else {
        while (nnotes<4 && notes[p][nnotes]) {
            assert(juno6_note_on(&s,notes[p][nnotes],velocities[p])); nnotes++;
        }
    }
    out.identity=UINT64_C(14695981039346656037);
    while (total<TOTAL) {
        if (total==HELD) {
            for (unsigned i=0;i<nnotes;i++)juno6_note_off(&s,stress ? stress_notes[i] : notes[p][i]);
        }
        unsigned n=BLOCK;
        if (total<HELD && total+n>HELD)n=HELD-total;
        if (total+n>TOTAL)n=TOTAL-total;
        assert(n && n<=BLOCK);
        juno6_render(&s,mono,n);
        for (unsigned i=0;i<n;i++)l[i]=r[i]=mono[i];
        juno6_chorus_process(&c,l,r,n);
        for (unsigned i=0;i<n;i++) {
            float a=fabsf(mono[i]),b=fmaxf(fabsf(l[i]),fabsf(r[i]));
            assert(isfinite(a) && isfinite(b));
            if (a>out.dry_peak)out.dry_peak=a;
            if (b>out.wet_peak)out.wet_peak=b;
            if (a>=1.0f)out.dry_clip++;
            if (b>=1.0f)out.wet_clip++;
            out.dry_rms+=(double)mono[i]*mono[i];
            out.wet_rms+=((double)l[i]*l[i]+(double)r[i]*r[i])*.5;
            hash_pcm(&out.identity,mono[i]);
            out.frames++;
        }
        total+=n;
    }
    out.dry_rms=sqrt(out.dry_rms/out.frames);
    out.wet_rms=sqrt(out.wet_rms/out.frames);
    assert(out.frames==TOTAL);
    assert(out.dry_rms>1e-6 && out.wet_rms>1e-6);
    return out;
}
int main(void)
{
    assert(N==8u && (sizeof original_sound_juno_names/sizeof original_sound_juno_names[0])==N);
    for(unsigned i=0;i<4;i++) {
        assert(!memcmp(legacy[i],original_sound_juno_factories[i],16));
        assert(!strcmp(legacy_names[i],original_sound_juno_names[i]));
    }
    puts("preset,scene,notes,mono_rms,mono_peak,chorus_stereo_rms,chorus_stereo_peak,mono_ge1,chorus_ge1,pcm_hash");
    for(unsigned i=0;i<N;i++) {
        const char *name=original_sound_juno_names[i];
        assert(name && strlen(name)<=12);
        for(unsigned j=0;j<i;j++)assert(strcmp(name,original_sound_juno_names[j]));
        for(unsigned j=0;j<16;j++)assert(original_sound_juno_factories[i][j]<=127);
        for(unsigned scene=0;scene<2;scene++) {
            result_t r=render(i,(int)scene);
            assert(r.dry_clip==0 && r.wet_clip==0);
            printf("%s,%s,%u,%.8f,%.8f,%.8f,%.8f,%u,%u,%016llx\n",
                name,scene ? "six_voice" : "musical",
                scene ? 6u : i<4 ? 3u : i==4 ? 2u : i==5||i==7 ? 4u : 3u,
                r.dry_rms,r.dry_peak,r.wet_rms,r.wet_peak,r.dry_clip,r.wet_clip,
                (unsigned long long)r.identity);
        }
    }
    puts("PASS: legacy indices untouched; eight distinct stock sounds; sixteen actual Juno DSP dry/chorus scenes finite and below unity (host only)");
}
