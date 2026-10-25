/* SPDX-License-Identifier: GPL-3.0-only */
#include "wobble.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
enum { N = 48000 };
static float a[N], b[N], c[N], d[N];
static void note(wobble_t *s, float *l, float *r, unsigned block)
{
    wobble_note_on(s, 48, 80);
    for (unsigned p = 0; p < N;) {
        if (p == 24000) wobble_note_off(s, 48);
        unsigned end = p < 24000 ? 24000 : N;
        unsigned n = end - p < block ? end - p : block;
        wobble_render(s, l+p, r+p, n); p += n;
    }
}
static void finite_buffer(unsigned n)
{
    for (unsigned i=0; i<n; ++i) {
        assert(isfinite(a[i]) && isfinite(b[i]));
        assert(fabsf(a[i]) < 4 && fabsf(b[i]) < 4);
    }
}
static void deterministic_blocks(void)
{
    wobble_t s;
    wobble_init(&s,48000); note(&s,c,d,256);
    for (unsigned block=1; block<=256; ++block) {
        wobble_init(&s,48000); note(&s,a,b,block);
        assert(memcmp(a,c,sizeof a)==0 && memcmp(b,d,sizeof b)==0);
    }
    note(&s,a,b,127);
    assert(memcmp(a,c,sizeof a)!=0); /* retained active phase, explicit history */
    wobble_reset(&s); note(&s,a,b,127);
    assert(memcmp(a,c,sizeof a)==0 && memcmp(b,d,sizeof b)==0);
    puts("PASS deterministic fresh/reset, active history, all block sizes 1..256");
}
static void pitch_tail_and_ownership(void)
{
    wobble_t s;
    wobble_init(&s,48000); wobble_set(&s,W_CUTOFF,1); note(&s,a,b,256);
    finite_buffer(N);
    for(unsigned i=0;i<N;++i) assert(a[i]==b[i]);
    double first=0,last=0; unsigned crossings=0;
    for(unsigned i=4801;i<19200;++i) if(a[i-1]<=0 && a[i]>0) {
        double t=i-1 + (double)(-a[i-1])/(a[i]-a[i-1]);
        if(!crossings)first=t; last=t; ++crossings;
    }
    assert(crossings>20);
    double hz=(crossings-1)*48000.0/(last-first);
    double cents=1200*log2(hz/(440*pow(2,-21.0/12)));
    printf("pitch_hz=%.8f pitch_error_cents=%.6f\n",hz,cents);
    assert(fabs(cents)<1.0);
    assert(!s.active && s.filter_low==0 && s.filter_band==0);
    for(unsigned i=26000;i<N;++i)assert(a[i]==0 && b[i]==0);
    wobble_note_on(&s,48,80); wobble_note_on(&s,55,80);
    wobble_note_off(&s,48); assert(s.gate);
    wobble_note_off(&s,55); assert(!s.gate);
    wobble_note_on(&s,48,80); wobble_set(&s,W_DAMPER,1);
    wobble_note_off(&s,48); assert(s.gate && s.deferred_off);
    wobble_set(&s,W_DAMPER,0); assert(!s.gate && !s.deferred_off);
    wobble_note_on(&s,48,80); wobble_all_notes_off(&s); assert(!s.gate);
    puts("PASS pitch, exact tail silence, mono, last-note ownership, damper cleanup");
}
static void modulation_null_and_stereo(void)
{
    wobble_t s;
    wobble_init(&s,48000); note(&s,c,d,256);
    wobble_init(&s,48000); wobble_set(&s,W_XMOD_DEPTH,.15f);
    wobble_set(&s,W_XMOD_WAVE,1); note(&s,a,b,256);
    assert(!memcmp(a,c,sizeof a));
    wobble_init(&s,48000); wobble_set(&s,W_STEREO,1); note(&s,a,b,256);
    double side=0;
    for(unsigned i=0;i<N;++i) {
        assert(fabsf((a[i]+b[i])*.5f-c[i])<1e-7f);
        side+=fabs(a[i]-b[i]);
    }
    assert(side>1);
    wobble_init(&s,48000); wobble_set(&s,W_XMOD_LEVEL,.7f);
    wobble_set(&s,W_XMOD_DEPTH,.15f); note(&s,a,b,256);
    assert(memcmp(a,c,sizeof a)!=0);
    puts("PASS mod-level0 control null, positive modulation, stereo and mono downmix");
}
static void sweeps(void)
{
    static const float rates[]={8000,44100,48000,192000};
    wobble_t s;
    for(unsigned sr=0;sr<4;++sr)for(unsigned note_i=0;note_i<3;++note_i) {
        wobble_init(&s,rates[sr]); wobble_note_on(&s,(uint8_t)(note_i*63),127);
        for(unsigned p=0;p<W_PARAMETER_COUNT;++p)for(unsigned step=0;step<=32;++step) {
            assert(wobble_set(&s,p,step/32.0f));
            wobble_render(&s,a,b,256); finite_buffer(256);
        }
        for(unsigned j=0;j<W_PARAMETER_COUNT;++j)wobble_set(&s,j,1);
        wobble_render(&s,a,b,256); finite_buffer(256);
        wobble_all_notes_off(&s);
        for(unsigned j=0;j<32;++j)wobble_render(&s,a,b,256);
        assert(!s.active);
    }
    wobble_init(&s,NAN); assert(s.sample_rate==48000);
    assert(!wobble_set(&s,W_CUTOFF,NAN));
    assert(!wobble_set(&s,W_CUTOFF,INFINITY));
    assert(!wobble_set(&s,W_PARAMETER_COUNT,0));
    assert(wobble_set(&s,W_CUTOFF,-1) && s.p[W_CUTOFF]==0);
    assert(wobble_set(&s,W_CUTOFF,2) && s.p[W_CUTOFF]==1);
    puts("PASS finite bounded extreme/swept controls at 8/44.1/48/192kHz, input validation");
}
static void control_sensitivity_and_glide(void)
{
    wobble_t s;
    static const unsigned controls[]={W_CARRIER_WAVE,W_SUB_LEVEL,W_CARRIER_PITCH,
        W_XMOD_LEVEL,W_CUTOFF,W_CRUSH,W_WOBBLE_DEPTH,W_STEREO,W_PITCH_BEND};
    static const float values[]={1,.7f,.75f,.7f,.2f,.6f,.7f,.8f,.75f};
    wobble_init(&s,48000); note(&s,c,d,256);
    for(unsigned i=0;i<sizeof controls/sizeof controls[0];++i) {
        wobble_init(&s,48000); wobble_set(&s,controls[i],values[i]);
        note(&s,a,b,256);
        assert(memcmp(a,c,sizeof a)!=0 || memcmp(b,d,sizeof b)!=0);
    }
    wobble_init(&s,48000); wobble_set(&s,W_XMOD_LEVEL,.7f); note(&s,c,d,256);
    wobble_init(&s,48000); wobble_set(&s,W_XMOD_LEVEL,.7f);
    wobble_set(&s,W_XMOD_WAVE,1); note(&s,a,b,256); assert(memcmp(a,c,sizeof a));
    wobble_init(&s,48000); wobble_set(&s,W_XMOD_LEVEL,.7f);
    wobble_set(&s,W_XMOD_TUNE,.75f); note(&s,a,b,256); assert(memcmp(a,c,sizeof a));
    wobble_init(&s,48000); wobble_set(&s,W_WOBBLE_DEPTH,.7f); note(&s,c,d,256);
    wobble_init(&s,48000); wobble_set(&s,W_WOBBLE_DEPTH,.7f);
    wobble_set(&s,W_WOBBLE_SHAPE,1); note(&s,a,b,256); assert(memcmp(a,c,sizeof a));
    wobble_init(&s,48000); wobble_set(&s,W_WOBBLE_DEPTH,.7f);
    wobble_set(&s,W_WOBBLE_RATE,1); note(&s,a,b,256); assert(memcmp(a,c,sizeof a));
    wobble_init(&s,48000); wobble_set(&s,W_GLIDE,1);
    wobble_note_on(&s,48,80); wobble_render(&s,a,b,256); float previous=s.hz;
    wobble_note_on(&s,60,80); assert(s.hz==previous && s.hz<s.target_hz);
    wobble_render(&s,a,b,256); assert(s.hz>previous && s.hz<s.target_hz);
    previous=s.hz;
    wobble_set(&s,W_GLIDE,0); wobble_render(&s,a,b,1);
    assert(s.hz==s.target_hz && s.hz>previous);
    wobble_reset(&s); wobble_set(&s,W_OUTPUT,0); note(&s,a,b,256);
    for(unsigned i=0;i<N;++i)assert(a[i]==0 && b[i]==0);
    puts("PASS control sensitivity, zero output, monotonic glide and immediate no-glide");
}
static void amp_decay_extension(void)
{
    wobble_t s;
    double rms0, rms1;
    wobble_init(&s, 48000);
    wobble_set(&s, W_CUTOFF, 1);
    wobble_note_on(&s, 48, 100);
    for (unsigned p = 0; p < 12000; p += 256)
        wobble_render(&s, a, b, 256);
    rms0 = 0;
    for (unsigned i = 4800; i < 9600; ++i)
        rms0 += (double)(a[i] * a[i]);
    rms0 = sqrt(rms0 / 4800.0);
    wobble_init(&s, 48000);
    wobble_set(&s, W_CUTOFF, 1);
    wobble_set(&s, W_DECAY, 1);
    wobble_note_on(&s, 48, 100);
    for (unsigned p = 0; p < 12000; p += 256)
        wobble_render(&s, a, b, 256);
    rms1 = 0;
    for (unsigned i = 4800; i < 9600; ++i)
        rms1 += (double)(a[i] * a[i]);
    rms1 = sqrt(rms1 / 4800.0);
    assert(rms1 < rms0 * 0.85);
    puts("PASS W_DECAY default matches legacy level; high decay lowers held RMS");
}
static void benchmark(void)
{
    wobble_t s; const unsigned frames=480000;
    volatile double checksum=0;
    wobble_init(&s,48000); wobble_note_on(&s,48,127);
    wobble_set(&s,W_XMOD_LEVEL,.7f); wobble_set(&s,W_XMOD_DEPTH,.8f);
    wobble_set(&s,W_CRUSH,.6f); wobble_set(&s,W_WOBBLE_DEPTH,.9f);
    wobble_set(&s,W_STEREO,.8f);
    clock_t start=clock();
    for(unsigned p=0;p<frames;p+=256) {
        unsigned n=frames-p<256?frames-p:256;
        wobble_render(&s,a,b,n); checksum+=a[n-1]+b[n-1];
    }
    double elapsed=(double)(clock()-start)/CLOCKS_PER_SEC;
    printf("host_benchmark_frames=%u cpu_seconds=%.6f cpu_seconds_per_audio_second=%.6f checksum=%.9f\n",
           frames,elapsed,elapsed/10,checksum);
}
int main(void)
{
    deterministic_blocks(); pitch_tail_and_ownership();
    modulation_null_and_stereo(); sweeps(); control_sensitivity_and_glide();
    amp_decay_extension(); benchmark();
    printf("PASS all host tests; sizeof(wobble_t)=%zu bytes\n",sizeof(wobble_t));
    return 0;
}
