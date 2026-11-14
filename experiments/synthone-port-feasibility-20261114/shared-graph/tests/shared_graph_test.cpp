// Synthetic ownership/source-oracle differential test; no uploaded presets/audio.
#include "shared_graph.hpp"
#include "../../probe/wave_frequencies.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>
#include <initializer_list>
#include <limits>

static unsigned long long checks, frames, snapshots, source_voice_calls;
static unsigned cases;
static void fail(const char *what, int line) {
    std::fprintf(stderr, "FAIL line %d: %s\n", line, what); std::exit(1);
}
#define CHECK(x) do { ++checks; if (!(x)) fail(#x, __LINE__); } while (0)
struct AllocationCounts { size_t mallocs, callocs, reallocs, frees; };
static AllocationCounts allocations;
static bool deny_heap;
extern "C" {
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
void __real_free(void *);
void *__wrap_malloc(size_t n) {
    if (deny_heap) fail("malloc in static action", __LINE__);
    ++allocations.mallocs;
    void *p = __real_malloc(n);
    if (p) std::memset(p, 0, n); // Determinize unused/padding bytes, not compute code.
    return p;
}
void *__wrap_calloc(size_t n, size_t s) {
    if (deny_heap) fail("calloc in static action", __LINE__);
    ++allocations.callocs; return __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n) {
    if (deny_heap) fail("realloc in static action", __LINE__);
    ++allocations.reallocs; return __real_realloc(p, n);
}
void __wrap_free(void *p) {
    if (deny_heap) fail("free in static action", __LINE__);
    ++allocations.frees; __real_free(p);
}
}
struct NoHeap {
    AllocationCounts before;
    NoHeap() : before(allocations) { CHECK(!deny_heap); deny_heap = true; }
    ~NoHeap() { deny_heap = false; CHECK(std::memcmp(&before, &allocations, sizeof before) == 0); }
};

namespace synthone_shared {
struct TestAccess {
    static const S1DSPKernel &kernel(const Graph &g) { return g.kernel_; }
    static const S1NoteState &note(const Graph &g,unsigned i) { return g.notes_[i]; }
    static const sp_ftbl &table(const Graph &g,unsigned i) { return g.descriptors_[i]; }
};
}
using synthone_shared::Graph;
using synthone_shared::TestAccess;
using synthone_shared::Tables;
using synthone_shared::Shape;
using synthone_shared::table_size;
using synthone_shared::table_samples;
struct Fixture {
    Tables tables{};
    float *payload;
    size_t bytes;
    explicit Fixture(const char *path) {
        const size_t payload_bytes=53*table_samples*sizeof(float);
        const size_t page=static_cast<size_t>(sysconf(_SC_PAGESIZE));
        bytes=(payload_bytes+page-1)/page*page;
        payload=static_cast<float *>(mmap(nullptr,bytes,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
        CHECK(payload!=MAP_FAILED);
        FILE *input=std::fopen(path,"rb");CHECK(input);
        CHECK(std::fread(payload,sizeof(float),52*table_samples,input)==52*table_samples);
        CHECK(std::fgetc(input)==EOF);CHECK(std::fclose(input)==0);
        size_t overshoots=0;
        for(unsigned band=0;band<13;++band) {
            tables.frequency_band[band]=wave_frequencies[band];
            for(unsigned shape=0;shape<4;++shape) {
                float *p=payload+(band*4+shape)*table_samples;
                for(size_t i=0;i<table_size;++i) {
                    CHECK(std::isfinite(p[i])&&p[i]>=-2&&p[i]<=2);
                    if(p[i]<-1||p[i]>1)++overshoots;
                }
                bits_guard(p[table_size]);
                tables.morph[band*4+shape]={p,table_samples,band,static_cast<Shape>(shape)};
            }
        }
        CHECK(overshoots==58476); // Authentic input must survive, without clipping.
        float *sine=payload+52*table_samples;
        sp_data context{};context.sr=44100;
        sp_ftbl descriptor{};descriptor.tbl=sine;
        sp_ftbl_init(&context,&descriptor,table_size);sp_gen_sine(&context,&descriptor);
        sine[table_size]=0;
        tables.sine=sine;tables.sine_count=table_samples;
        CHECK(mprotect(payload,bytes,PROT_READ)==0);
    }
    static void bits_guard(float value) { CHECK(value==0&&!std::signbit(value)); }
    ~Fixture() { CHECK(munmap(payload,bytes)==0); }
};
struct Oracle {
    S1DSPKernel k{};
    S1NoteState v[6]{};
    unsigned count=0;
    Oracle(const Tables &tables,int rate) {
        sp_create(&k.sp);k.sp->sr=rate;k.sp->len=5ul*rate;
        std::memcpy(k.parameters,defaults,sizeof defaults);
        for(unsigned i=0;i<52;++i) {
            sp_ftbl_create(k.sp,&k.ft_array[i],table_size);
            std::memcpy(k.ft_array[i]->tbl,tables.morph[i].samples,table_samples*sizeof(float));
        }
        std::memcpy(k.ft_frequencyBand,tables.frequency_band,sizeof k.ft_frequencyBand);
        sp_ftbl_create(k.sp,&k.sine,table_size);
        std::memcpy(k.sine->tbl,tables.sine,table_samples*sizeof(float));
        sp_phasor_create(&k.lfo1Phasor);sp_phasor_init(k.sp,k.lfo1Phasor,0);
        sp_phasor_create(&k.lfo2Phasor);sp_phasor_init(k.sp,k.lfo2Phasor,0);
        sp_port_create(&k.lfo1Port);sp_port_init(k.sp,k.lfo1Port,.0053125f);
        sp_port_create(&k.lfo2Port);sp_port_init(k.sp,k.lfo2Port,.0053125f);
        for(auto &note:v){note.kernel=&k;note.init();note.clear();}
    }
    ~Oracle() {
        for(auto &note:v)note.destroy();
        sp_phasor_destroy(&k.lfo1Phasor);sp_phasor_destroy(&k.lfo2Phasor);
        sp_port_destroy(&k.lfo1Port);sp_port_destroy(&k.lfo2Port);
        for(auto &table:k.ft_array)sp_ftbl_destroy(&table);
        sp_ftbl_destroy(&k.sine);sp_destroy(&k.sp);
    }
};
// Compare fields, never pointer addresses/padding/uninitialized allocation bytes.
// These lists cover every declared field of all 12 voice DSP and four LFO objects.
template <typename T> static void bits(T a, T b) { CHECK(std::memcmp(&a, &b, sizeof a) == 0); }
#define FIELD(name) bits(a.name, b.name)
static void same(const sp_adsr &a, const sp_adsr &b) {
    FIELD(atk);FIELD(dec);FIELD(sus);FIELD(rel);FIELD(timer);FIELD(atk_time);
    FIELD(a);FIELD(b);FIELD(y);FIELD(x);FIELD(prev);FIELD(mode);
}
static void same(const sp_oscmorph2d &a, const sp_oscmorph2d &b) {
    FIELD(freq);FIELD(amp);FIELD(iphs);FIELD(lphs);FIELD(inc);FIELD(wtpos);
    FIELD(nft);FIELD(nbl);FIELD(enableBandlimit);FIELD(bandlimitIndexOverride);
    CHECK(a.tbl && b.tbl && a.fbl && b.fbl);
}
static void same(const sp_crossfade &a, const sp_crossfade &b) { FIELD(pos); }
static void same(const sp_osc &a, const sp_osc &b) {
    FIELD(freq);FIELD(amp);FIELD(iphs);FIELD(lphs);FIELD(inc); CHECK(a.tbl && b.tbl);
}
static void same(const sp_fosc &a, const sp_fosc &b) {
    FIELD(amp);FIELD(freq);FIELD(car);FIELD(mod);FIELD(indx);FIELD(iphs);FIELD(mphs);FIELD(cphs);
    CHECK(a.ft && b.ft);
}
static void same(const sp_noise &a, const sp_noise &b) { FIELD(amp); }
static void same(const sp_moogladder &a, const sp_moogladder &b) {
    FIELD(freq);FIELD(res);FIELD(istor);FIELD(oldfreq);FIELD(oldres);FIELD(oldacr);FIELD(oldtune);
    for (int i=0;i<6;++i) bits(a.delay[i],b.delay[i]);
    for (int i=0;i<3;++i) bits(a.tanhstg[i],b.tanhstg[i]);
}
static void same(const sp_butbp &a, const sp_butbp &b) {
    FIELD(sr);FIELD(freq);FIELD(bw);FIELD(istor);FIELD(lkf);FIELD(lkb);FIELD(pidsr);FIELD(tpidsr);
    for (int i=0;i<8;++i) bits(a.a[i],b.a[i]);
}
static void same(const sp_buthp &a, const sp_buthp &b) {
    FIELD(sr);FIELD(freq);FIELD(istor);FIELD(lkf);FIELD(pidsr);
    for (int i=0;i<8;++i) bits(a.a[i],b.a[i]);
}
static void same(const sp_phasor &a, const sp_phasor &b) {
    FIELD(freq);FIELD(phs);FIELD(curphs);FIELD(onedsr);
}
static void same(const sp_port &a, const sp_port &b) {
    FIELD(htime);FIELD(c1);FIELD(c2);FIELD(yt1);FIELD(prvhtim);FIELD(sr);FIELD(onedsr);
}
#undef FIELD

static void compare_note(const S1NoteState &a,const S1NoteState &b,const S1DSPKernel &ka,const S1DSPKernel &kb) {
    CHECK(a.kernel==&ka&&b.kernel==&kb);
    bits(a.stage,b.stage);bits(a.internalGate,b.internalGate);bits(a.velocity,b.velocity);
    bits(a.amp,b.amp);bits(a.filter,b.filter);bits(a.rootNoteNumber,b.rootNoteNumber);bits(a.transpose,b.transpose);
    same(*a.adsr,*b.adsr);same(*a.fadsr,*b.fadsr);
    same(*a.oscmorph1,*b.oscmorph1);same(*a.oscmorph2,*b.oscmorph2);
    same(*a.morphCrossFade,*b.morphCrossFade);same(*a.filterCrossFade,*b.filterCrossFade);
    same(*a.subOsc,*b.subOsc);same(*a.fmOsc,*b.fmOsc);same(*a.noise,*b.noise);
    same(*a.loPass,*b.loPass);same(*a.bandPass,*b.bandPass);same(*a.hiPass,*b.hiPass);
    CHECK(a.oscmorph1->tbl==ka.ft_array&&a.oscmorph2->tbl==ka.ft_array);
    CHECK(a.oscmorph1->fbl==ka.ft_frequencyBand&&a.oscmorph2->fbl==ka.ft_frequencyBand);
    CHECK(a.subOsc->tbl==ka.sine&&a.fmOsc->ft==ka.sine);
}
static void compare(const Graph &g,const Oracle &o,const Tables &tables) {
    ++snapshots;
    const auto &a=TestAccess::kernel(g);const auto &b=o.k;
    CHECK(g.active_count()==o.count);
    for(unsigned i=0;i<6;++i)compare_note(TestAccess::note(g,i),o.v[i],a,b);
    same(*a.lfo1Phasor,*b.lfo1Phasor);same(*a.lfo2Phasor,*b.lfo2Phasor);
    same(*a.lfo1Port,*b.lfo1Port);same(*a.lfo2Port,*b.lfo2Port);
    bits(a.lfo1_0_1,b.lfo1_0_1);bits(a.lfo1_1_0,b.lfo1_1_0);
    bits(a.lfo2_0_1,b.lfo2_0_1);bits(a.lfo2_1_0,b.lfo2_1_0);
    bits(a.lfo3_0_1,b.lfo3_0_1);bits(a.lfo3_1_0,b.lfo3_1_0);
    bits(a.lfo1Smooth,b.lfo1Smooth);bits(a.lfo2Smooth,b.lfo2Smooth);
    bits(a.monoFrequencySmooth,b.monoFrequencySmooth);
    CHECK(std::memcmp(a.parameters,b.parameters,sizeof a.parameters)==0);
    bits(a.sp->sr,b.sp->sr);bits(a.sp->nchan,b.sp->nchan);bits(a.sp->len,b.sp->len);
    bits(a.sp->pos,b.sp->pos);bits(a.sp->rand,b.sp->rand);bits(*a.sp->out,*b.sp->out);
    CHECK(std::memcmp(a.sp->filename,b.sp->filename,sizeof a.sp->filename)==0);
    for(unsigned i=0;i<53;++i) {
        const auto &da=TestAccess::table(g,i);const auto &db=i<52?*b.ft_array[i]:*b.sine;
        CHECK((i<52?a.ft_array[i]:a.sine)==&da);
        CHECK(da.tbl==(i<52?tables.morph[i].samples:tables.sine));
        bits(da.size,db.size);bits(da.lobits,db.lobits);bits(da.lomask,db.lomask);
        bits(da.lodiv,db.lodiv);bits(da.sicvt,db.sicvt);CHECK(da.del==0&&db.del==1);
    }
    CHECK(std::memcmp(a.ft_frequencyBand,b.ft_frequencyBand,sizeof a.ft_frequencyBand)==0);
    // All six notes share table metadata/context, but never their DSP objects.
    for(unsigned i=0;i<6;++i)for(unsigned j=i+1;j<6;++j) {
        const auto &x=TestAccess::note(g,i);const auto &y=TestAccess::note(g,j);
        CHECK(x.adsr!=y.adsr&&x.fadsr!=y.fadsr&&x.oscmorph1!=y.oscmorph1&&x.oscmorph2!=y.oscmorph2);
        CHECK(x.subOsc!=y.subOsc&&x.fmOsc!=y.fmOsc&&x.noise!=y.noise);
        CHECK(x.loPass!=y.loPass&&x.bandPass!=y.bandPass&&x.hiPass!=y.hiPass);
        CHECK(x.morphCrossFade!=y.morphCrossFade&&x.filterCrossFade!=y.filterCrossFade);
    }
}
static void parameter(Graph &g,Oracle &o,S1Parameter p,float value) {
    NoHeap n;CHECK(g.set_parameter(p,value));o.k.parameters[p]=o.k.clampedValue(p,value);
}
static void mono_frequency(Graph &g,Oracle &o,float frequency) {
    NoHeap n;CHECK(g.set_mono_frequency(frequency));o.k.monoFrequencySmooth=frequency;
}
static void count(Graph &g,Oracle &o,unsigned active) {
    NoHeap n;CHECK(g.set_active_count(active));o.count=active;
}
static void note(Graph &g,Oracle &o,unsigned i,int n,int velocity,float f) {
    NoHeap no_heap;CHECK(g.note_on(i,n,velocity,f));o.v[i].startNoteHelper(n,velocity,f);
}
static void release(Graph &g,Oracle &o,unsigned i) {
    NoHeap n;CHECK(g.release(i));o.v[i].internalGate=0;o.v[i].stage=S1NoteState::stageRelease;
}
static void clear(Graph &g,Oracle &o,unsigned i) {NoHeap n;CHECK(g.clear(i));o.v[i].clear();}
static uint32_t advance_rand(uint32_t state,unsigned count) {
    for(unsigned i=0;i<count;++i)state=(1103515245u*state+12345u)%SP_RANDMAX;
    return state;
}
static void render(Graph &g,Oracle &o,const Tables &tables,unsigned length) {
    for(unsigned frame=0;frame<length;++frame) {
        const auto previous_rand=TestAccess::kernel(g).sp->rand;
        float al=.125f,ar=-.25f,bl=al,br=ar;
        {NoHeap n;CHECK(g.process(&al,&ar));}
        o.k.processLfo();
        for(unsigned i=0;i<o.count;++i)o.v[i].run(0,&bl,&br);
        CHECK(std::isfinite(al)&&std::isfinite(ar)&&std::isfinite(bl)&&std::isfinite(br));
        bits(al,bl);bits(ar,br);++frames;source_voice_calls+=o.count;
        CHECK(TestAccess::kernel(g).sp->rand==advance_rand(previous_rand,o.count));
        if(o.count==0){bits(al,.125f);bits(ar,-.25f);}
        compare(g,o,tables);
    }
}
static uint32_t random_state=0xa53c91u;
static uint32_t random_word(){random_state^=random_state<<13;random_state^=random_state>>17;return random_state^=random_state<<5;}
static float unit(){return static_cast<float>(random_word()&65535)/65535.f;}
static void modulate(Graph &v, Oracle &o, unsigned step) {
    parameter(v,o,index1,unit());parameter(v,o,index2,unit());parameter(v,o,morphBalance,unit());
    parameter(v,o,morph1Volume,.2f+unit()*.6f);parameter(v,o,morph2Volume,.2f+unit()*.6f);
    parameter(v,o,morph1SemitoneOffset,static_cast<int>(step%25)-12);
    parameter(v,o,morph2SemitoneOffset,static_cast<int>((step*7)%25)-12);
    parameter(v,o,subVolume,.1f);parameter(v,o,subIsSquare,step%2);parameter(v,o,subOctaveDown,(step/2)%2);
    parameter(v,o,fmVolume,.15f);parameter(v,o,fmAmount,unit()*3.f);parameter(v,o,noiseVolume,.03f);
    parameter(v,o,filterType,step%3);parameter(v,o,cutoff,400+unit()*7000);
    parameter(v,o,resonance,.1f+unit()*.6f);parameter(v,o,filterADSRMix,unit());
    parameter(v,o,filterMix,.3f+unit()*.7f);parameter(v,o,morph2Detuning,-3+unit()*6);
    parameter(v,o,detuningMultiplier,.995f+unit()*.01f);parameter(v,o,adsrPitchTracking,unit());
    parameter(v,o,oscBandlimitEnable,step%2);parameter(v,o,isMono,step%2);
    parameter(v,o,pitchbend,unit()*16383);
    parameter(v,o,lfo1Index,step%4);parameter(v,o,lfo2Index,(step+1)%4);
    parameter(v,o,lfo1Amplitude,.1f+unit()*.8f);parameter(v,o,lfo2Amplitude,.1f+unit()*.8f);
    parameter(v,o,lfo1Rate,.1f+unit()*79);parameter(v,o,lfo2Rate,.1f+unit()*79);
    for(auto p:{cutoffLFO,resonanceLFO,oscMixLFO,decayLFO,noiseLFO,fmLFO,detuneLFO,filterEnvLFO,pitchLFO})
        parameter(v,o,p,(step+static_cast<unsigned>(p))%4);
    parameter(v,o,attackDuration,.0005f+unit()*.02f);parameter(v,o,decayDuration,.005f+unit()*.03f);
    parameter(v,o,sustainLevel,.2f+unit()*.8f);parameter(v,o,releaseDuration,.004f+unit()*.03f);
    parameter(v,o,filterAttackDuration,.0005f+unit()*.02f);parameter(v,o,filterDecayDuration,.005f+unit()*.03f);
    parameter(v,o,filterSustainLevel,unit());parameter(v,o,filterReleaseDuration,.004f+unit()*.03f);
}

static void exercise(Fixture &fixture,int rate,unsigned variant) {
    ++cases;Graph graph;
    {NoHeap n;CHECK(graph.init(fixture.tables,rate));}
    const auto before=allocations;
    {
        Oracle oracle(fixture.tables,rate);
        CHECK(allocations.mallocs-before.mallocs==184);
        compare(graph,oracle,fixture.tables);render(graph,oracle,fixture.tables,16);
        for(unsigned i=0;i<6;++i)note(graph,oracle,i,43+i*5,32+i*17,82.4f+i*137.1f);
        for(unsigned step=0;step<42;++step) {
            modulate(graph,oracle,step+variant*42);
            count(graph,oracle,(step+variant)%7);
            mono_frequency(graph,oracle,130.8f+step*31.2f);
            if(step%7==0)parameter(graph,oracle,isMono,.25f); // Source tests >0, not integer toggle.
            if(step%11==0)parameter(graph,oracle,lfo1Index,.5f); // Preserve noninteger selector behavior.
            if(step==4)release(graph,oracle,5); // Inactive suffix release, no rendered falling edge yet.
            if(step==5)note(graph,oracle,5,72,0,880); // Velocity0 does not mean release.
            if(step==11)note(graph,oracle,1,51,127,155.5f); // Already gated; no implicit DSP reset.
            if(step==17)clear(graph,oracle,2); // Metadata clear, not DSP reset.
            if(step==22)for(unsigned i=0;i<6;++i)release(graph,oracle,i);
            if(step==29)for(unsigned i=0;i<6;++i)note(graph,oracle,i,60+i,127-i*9,260+i*170);
            render(graph,oracle,fixture.tables,48);
        }
        count(graph,oracle,6);for(unsigned i=0;i<6;++i)release(graph,oracle,i);
        render(graph,oracle,fixture.tables,2048);
    }
    {NoHeap n;CHECK(graph.reset());}
    {Oracle fresh(fixture.tables,rate);compare(graph,fresh,fixture.tables);
     count(graph,fresh,6);note(graph,fresh,0,60,100,261.6255653006f);render(graph,fresh,fixture.tables,128);}
    {NoHeap n;graph.teardown();graph.teardown();CHECK(!graph.initialized());CHECK(!graph.reset());
     CHECK(!graph.set_active_count(1));CHECK(!graph.note_on(0,60,100,440));CHECK(!graph.release(0));CHECK(!graph.clear(0));
     CHECK(!graph.set_mono_frequency(440));CHECK(!graph.set_parameter(index1,.5f));
     float l=1,r=2;CHECK(!graph.process(&l,&r));bits(l,1.f);bits(r,2.f);}
    {NoHeap n;CHECK(graph.init(fixture.tables,rate==96000?44100:96000));}
    {Oracle fresh(fixture.tables,rate==96000?44100:96000);compare(graph,fresh,fixture.tables);}
}
static void boundaries(Fixture &fixture,int rate) {
    ++cases;Graph g;{NoHeap n;CHECK(g.init(fixture.tables,rate));}Oracle o(fixture.tables,rate);
    count(g,o,6);parameter(g,o,oscBandlimitEnable,1);parameter(g,o,fmAmount,15);parameter(g,o,fmVolume,.1f);
    parameter(g,o,noiseVolume,.03f);
    for(unsigned i=0;i<6;++i)note(g,o,i,60+i,127-i*10,220.f+i*79);
    for(unsigned band=0;band<13;++band)for(float f:{std::nextafter(wave_frequencies[band],0.f),wave_frequencies[band],std::nextafter(wave_frequencies[band],INFINITY)}) {
        if(f>rate*.5f)continue;
        note(g,o,band%6,60+band,127,f);
        for(float morph:{0.f,1.f/3.f,2.f/3.f,1.f}) {
            parameter(g,o,index1,morph);parameter(g,o,index2,1-morph);
            render(g,o,fixture.tables,8);
        }
    }
    for(unsigned i=0;i<6;++i)note(g,o,i,100,127,rate*.5f);
    render(g,o,fixture.tables,128); // Source row0 fallback and normalized sine FM15 phase domain.
}
static void invalid_inputs(Fixture &fixture) {
    ++cases;Graph g;CHECK(g.init(fixture.tables,44100));Oracle o(fixture.tables,44100);
    count(g,o,6);for(unsigned i=0;i<6;++i)note(g,o,i,60+i,100,220+i*157);
    render(g,o,fixture.tables,64);
    Tables bad=fixture.tables;
    unsigned char before[sizeof(Graph)];
    auto reject=[&](){
        std::memcpy(before,&g,sizeof g);NoHeap n;CHECK(!g.init(bad,44100));
        CHECK(std::memcmp(before,&g,sizeof g)==0);compare(g,o,fixture.tables);
    };
    bad.morph[0].samples=nullptr;reject();bad=fixture.tables;
    bad.morph[0].count=4096;reject();bad=fixture.tables;
    bad.morph[4].band=0;reject();bad=fixture.tables;
    bad.morph[1].shape=Shape::saw;reject();bad=fixture.tables;
    bad.frequency_band[3]=bad.frequency_band[2];reject();bad=fixture.tables;
    bad.frequency_band[1]=NAN;reject();bad=fixture.tables;
    bad.sine_count=4096;reject();bad=fixture.tables;
    bad.sine=nullptr;reject();bad=fixture.tables;
    float mutant[table_samples];std::memcpy(mutant,fixture.tables.morph[0].samples,sizeof mutant);
    bad.morph[0].samples=mutant;mutant[0]=NAN;reject();mutant[0]=std::nextafter(2.f,INFINITY);reject();
    mutant[0]=std::nextafter(-2.f,-INFINITY);reject();mutant[0]=0;mutant[table_size]=-0.f;reject();
    mutant[table_size]=1;reject();bad=fixture.tables;
    std::memcpy(mutant,fixture.tables.sine,sizeof mutant);bad.sine=mutant;
    mutant[0]=100.f;reject();mutant[0]=-100.f;reject();
    mutant[0]=std::nextafter(1.f,INFINITY);reject();mutant[0]=std::nextafter(-1.f,-INFINITY);reject();bad=fixture.tables;
    std::memcpy(before,&g,sizeof g);
    {NoHeap n;CHECK(!g.init(fixture.tables,0));CHECK(!g.init(fixture.tables,22050));
     CHECK(!g.set_active_count(7));CHECK(!g.note_on(6,60,100,440));CHECK(!g.release(6));CHECK(!g.clear(6));
     CHECK(!g.note_on(0,-1,100,440));CHECK(!g.note_on(0,128,100,440));CHECK(!g.note_on(0,60,-1,440));
     CHECK(!g.note_on(0,60,128,440));CHECK(!g.note_on(0,60,100,NAN));CHECK(!g.note_on(0,60,100,-1));
     CHECK(!g.note_on(0,60,100,22051));CHECK(!g.set_mono_frequency(NAN));CHECK(!g.set_mono_frequency(-1));CHECK(!g.set_mono_frequency(22051));
     CHECK(!g.set_parameter(static_cast<S1Parameter>(-1),0));CHECK(!g.set_parameter(S1ParameterCount,0));CHECK(!g.set_parameter(index1,NAN));
     float f=1;CHECK(!g.process(nullptr,&f));CHECK(!g.process(&f,nullptr));bits(f,1.f);
     CHECK(std::memcmp(before,&g,sizeof g)==0);}
    compare(g,o,fixture.tables);render(g,o,fixture.tables,64);
}
static void independent_graphs(Fixture &fixture) {
    ++cases;Graph a,b;{NoHeap n;CHECK(a.init(fixture.tables,44100));CHECK(b.init(fixture.tables,96000));}
    Oracle oa(fixture.tables,44100),ob(fixture.tables,96000);
    count(a,oa,6);count(b,ob,3);parameter(a,oa,noiseVolume,.2f);parameter(b,ob,noiseVolume,.1f);
    for(unsigned i=0;i<6;++i){note(a,oa,i,60+i,110-i*13,440+i*177);note(b,ob,i,65+i,45+i*15,731+i*79);}
    for(unsigned i=0;i<16;++i){render(a,oa,fixture.tables,16);render(b,ob,fixture.tables,32);}
}
static void ordering_negative_control(Fixture &fixture) {
    ++cases;Oracle ascending(fixture.tables,44100),descending(fixture.tables,44100);
    for(auto *o:{&ascending,&descending}) {
        o->k.parameters[morph1Volume]=o->k.parameters[morph2Volume]=0;
        o->k.parameters[noiseVolume]=.2f;
        for(unsigned i=0;i<6;++i)o->v[i].startNoteHelper(60+i,20+i*20,220+i*171);
    }
    bool mismatch=false;
    for(unsigned frame=0;frame<64;++frame) {
        ascending.k.processLfo();descending.k.processLfo();float a=0,b=0,c=0,d=0;
        for(unsigned i=0;i<6;++i)ascending.v[i].run(0,&a,&b);
        for(unsigned i=6;i-->0;)descending.v[i].run(0,&c,&d);
        if(std::memcmp(&a,&c,sizeof a))mismatch=true;
        CHECK(ascending.k.sp->rand==descending.k.sp->rand);
    }
    CHECK(mismatch); // Same PRNG end state alone would miss changed noise assignment/order.
}
int main(int argc,char **argv) {
    if(argc!=2){std::fprintf(stderr,"usage: shared_graph_test authenticated-public-morphs.f32\n");return 2;}
    static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559,"source IEEE754 float32 contract");Fixture fixture(argv[1]);
    for(int rate:{44100,48000,96000})for(unsigned variant=0;variant<3;++variant)exercise(fixture,rate,variant);
    for(int rate:{44100,48000,96000})boundaries(fixture,rate);
    invalid_inputs(fixture);independent_graphs(fixture);ordering_negative_control(fixture);
    std::printf("{\"cases\":%u,\"frames_pcm_bitwise\":%llu,\"source_voice_calls\":%llu,\"all_six_note_state_snapshots\":%llu,"
        "\"checks\":%llu,\"caller_graph_bytes_host\":%zu,\"payload_bytes_external\":%zu,\"static_action_heap_calls\":0,"
        "\"oracle_mallocs\":%zu,\"oracle_frees\":%zu,\"authentic_morph_unit_overshoots\":58476,\"pointer_bytes\":%zu,\"float_bytes\":%zu}\n",
        cases,frames,source_voice_calls,snapshots,checks,sizeof(Graph),53*table_samples*sizeof(float),allocations.mallocs,allocations.frees,sizeof(void*),sizeof(float));
}
