// New exact-alias boundary cases against the pinned shared graph.
// Field-wise comparators are adapted from #934 shared_graph_test.cpp.
// No uploaded preset banks or rendered audio are consumed or saved.
#include "shared_graph.hpp"
#include "wave_frequencies.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>
#include <limits>
#include <initializer_list>
#include <signal.h>

static unsigned long long checks, frames, snapshots, voice_calls;
static unsigned cases, protected_write_probes;
static void fail(const char *what, int line) {
    std::fprintf(stderr, "FAIL line %d: %s\n", line, what); std::exit(1);
}
#define CHECK(x) do { ++checks; if (!(x)) fail(#x, __LINE__); } while (0)
namespace synthone_shared {
struct TestAccess {
    static const S1DSPKernel &kernel(const Graph &g) { return g.kernel_; }
    static const S1NoteState &note(const Graph &g,unsigned i) { return g.notes_[i]; }
    static const sp_ftbl &table(const Graph &g,unsigned i) { return g.descriptors_[i]; }
    static void phase_and_band(Graph &g,unsigned voice,int band) {
        auto &v=g.notes_[voice];
        v.oscmorph1->lphs=SP_FT_MAXLEN-1;
        v.oscmorph2->lphs=SP_FT_MAXLEN-1;
        v.oscmorph1->bandlimitIndexOverride=band;
        v.oscmorph2->bandlimitIndexOverride=band;
        v.subOsc->lphs=SP_FT_MAXLEN-1;
        v.fmOsc->cphs=v.fmOsc->mphs=SP_FT_MAXLEN-1;
    }
};
}
using namespace synthone_shared;
struct Fixture {
    Tables tables{};
    float *payload=nullptr;
    size_t bytes=0;
    Fixture(const char *path,bool packed,const size_t *offsets) {
        const size_t morph_bytes=(packed?50:52)*table_samples*sizeof(float);
        const size_t page=static_cast<size_t>(sysconf(_SC_PAGESIZE));
        bytes=(morph_bytes+table_samples*sizeof(float)+page-1)/page*page;
        payload=static_cast<float *>(mmap(nullptr,bytes,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
        CHECK(payload!=MAP_FAILED);
        FILE *f=std::fopen(path,"rb");CHECK(f);
        if(packed)CHECK(std::fread(payload,1,morph_bytes,f)==morph_bytes);
        else for(size_t row=0;row<52;++row) {
            CHECK(std::fread(payload+row*table_samples,sizeof(float),table_size,f)==table_size);
            payload[row*table_samples+table_size]=0.0f;
        }
        CHECK(std::fgetc(f)==EOF);CHECK(std::fclose(f)==0);
        for(unsigned i=0;i<52;++i) {
            size_t offset=packed?offsets[i]:i*table_samples*sizeof(float);
            CHECK(offset% (table_samples*sizeof(float))==0);
            CHECK(offset<=morph_bytes-table_samples*sizeof(float));
            const float *row=reinterpret_cast<const float *>(reinterpret_cast<const char *>(payload)+offset);
            tables.morph[i]={row,table_samples,i/4,static_cast<Shape>(i%4)};
            CHECK(row[table_size]==0&&!std::signbit(row[table_size]));
        }
        for(unsigned band=0;band<13;++band)tables.frequency_band[band]=wave_frequencies[band];
        float *sine=reinterpret_cast<float *>(reinterpret_cast<char *>(payload)+morph_bytes);
        sp_data context{};context.sr=44100;
        sp_ftbl descriptor{};descriptor.tbl=sine;
        sp_ftbl_init(&context,&descriptor,table_size);sp_gen_sine(&context,&descriptor);
        sine[table_size]=0.0f;tables.sine=sine;tables.sine_count=table_samples;
        CHECK(mprotect(payload,bytes,PROT_READ)==0);
    }
    ~Fixture(){CHECK(munmap(payload,bytes)==0);}
};
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
static void compare(const Graph &ga,const Graph &gb,const Tables &ta,const Tables &tb) {
    ++snapshots;
    CHECK(ga.active_count()==gb.active_count());
    const auto &a=TestAccess::kernel(ga);const auto &b=TestAccess::kernel(gb);
    for(unsigned i=0;i<6;++i)compare_note(TestAccess::note(ga,i),TestAccess::note(gb,i),a,b);
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
        const auto &da=TestAccess::table(ga,i);const auto &db=TestAccess::table(gb,i);
        CHECK((i<52?a.ft_array[i]:a.sine)==&da);CHECK((i<52?b.ft_array[i]:b.sine)==&db);
        CHECK(da.tbl==(i<52?ta.morph[i].samples:ta.sine));
        CHECK(db.tbl==(i<52?tb.morph[i].samples:tb.sine));
        bits(da.size,db.size);bits(da.lobits,db.lobits);bits(da.lomask,db.lomask);
        bits(da.lodiv,db.lodiv);bits(da.sicvt,db.sicvt);CHECK(da.del==0&&db.del==0);
    }
    CHECK(std::memcmp(a.ft_frequencyBand,b.ft_frequencyBand,sizeof a.ft_frequencyBand)==0);
}
static void parameter(Graph &a,Graph &b,S1Parameter p,float value) {
    CHECK(a.set_parameter(p,value));CHECK(b.set_parameter(p,value));
}
static void render(Graph &a,Graph &b,const Tables &ta,const Tables &tb,unsigned length) {
    for(unsigned frame=0;frame<length;++frame) {
        float al=.123f,ar=-.217f,bl=al,br=ar;
        CHECK(a.process(&al,&ar));CHECK(b.process(&bl,&br));
        CHECK(std::isfinite(al)&&std::isfinite(ar)&&std::isfinite(bl)&&std::isfinite(br));
        bits(al,bl);bits(ar,br);++frames;voice_calls+=a.active_count();
        compare(a,b,ta,tb);
    }
}
static void configure(Graph &a,Graph &b,unsigned seed) {
    parameter(a,b,oscBandlimitEnable,1);
    parameter(a,b,morph1Volume,.73f);parameter(a,b,morph2Volume,.31f);
    parameter(a,b,morphBalance,.37f);parameter(a,b,fmVolume,.17f);
    parameter(a,b,fmAmount,1.7f);parameter(a,b,subVolume,.11f);
    parameter(a,b,noiseVolume,.021f);parameter(a,b,filterType,seed%3);
    parameter(a,b,cutoff,1700+seed*211);parameter(a,b,resonance,.31f);
    parameter(a,b,lfo1Rate,7.3f);parameter(a,b,lfo2Rate,3.1f);
    parameter(a,b,lfo1Amplitude,.71f);parameter(a,b,lfo2Amplitude,.29f);
    parameter(a,b,lfo1Index,seed%4);parameter(a,b,lfo2Index,(seed+1)%4);
    parameter(a,b,noiseLFO,1);parameter(a,b,cutoffLFO,2);parameter(a,b,fmLFO,3);
    parameter(a,b,attackDuration,.0005f);parameter(a,b,releaseDuration,.008f);
    parameter(a,b,filterAttackDuration,.001f);parameter(a,b,filterReleaseDuration,.009f);
}
static void exercise(Fixture &original,Fixture &packed,int rate) {
    ++cases;Graph a,b;CHECK(a.init(original.tables,rate));CHECK(b.init(packed.tables,rate));
    CHECK(a.set_active_count(6));CHECK(b.set_active_count(6));
    for(unsigned i=0;i<6;++i) {
        CHECK(a.note_on(i,60+i,19+i*19,400.1f+i*157.3f));
        CHECK(b.note_on(i,60+i,19+i*19,400.1f+i*157.3f));
    }
    unsigned step=0;
    for(int band:{10,11,12})for(float morph:{0.f,std::nextafter(1.f/3.f,0.f),1.f/3.f,
                                          std::nextafter(1.f/3.f,1.f),.5f,2.f/3.f,1.f}) {
        configure(a,b,step++);parameter(a,b,index1,morph);parameter(a,b,index2,1-morph);
        for(unsigned i=0;i<6;++i){TestAccess::phase_and_band(a,i,band);TestAccess::phase_and_band(b,i,band);}
        compare(a,b,original.tables,packed.tables);render(a,b,original.tables,packed.tables,65);
    }
    // Natural automatic band selection immediately around both duplicate bands.
    for(float frequency:{std::nextafter(11025.f,0.f),11025.f,std::nextafter(11025.f,INFINITY),
                         std::nextafter(22050.f,0.f),22050.f}) {
        if(frequency>rate*.5f)continue;
        for(unsigned i=0;i<6;++i) {
            CHECK(a.note_on(i,100-i,31+i*13,frequency));CHECK(b.note_on(i,100-i,31+i*13,frequency));
            TestAccess::phase_and_band(a,i,-1);TestAccess::phase_and_band(b,i,-1);
        }
        parameter(a,b,index1,.17f);parameter(a,b,index2,.49f);render(a,b,original.tables,packed.tables,33);
    }
    for(unsigned i=0;i<6;++i){CHECK(a.release(i));CHECK(b.release(i));}
    render(a,b,original.tables,packed.tables,129);
    CHECK(a.set_active_count(0));CHECK(b.set_active_count(0));render(a,b,original.tables,packed.tables,17);
    CHECK(a.reset());CHECK(b.reset());compare(a,b,original.tables,packed.tables);
    configure(a,b,4);CHECK(a.set_active_count(6));CHECK(b.set_active_count(6));
    for(unsigned i=0;i<6;++i) {
        CHECK(a.note_on(i,40+i,121-i*11,83.1f+i*173));CHECK(b.note_on(i,40+i,121-i*11,83.1f+i*173));
        TestAccess::phase_and_band(a,i,12);TestAccess::phase_and_band(b,i,12);
    }
    parameter(a,b,index1,.04f);parameter(a,b,index2,.31f);
    render(a,b,original.tables,packed.tables,67);
    CHECK(a.clear(2));CHECK(b.clear(2));render(a,b,original.tables,packed.tables,31);
    a.teardown();b.teardown();CHECK(a.init(original.tables,rate));CHECK(b.init(packed.tables,rate));
    compare(a,b,original.tables,packed.tables);
}
static void write_probe(const float *p) {
    const pid_t child=fork();CHECK(child>=0);
    if(child==0) {
        struct rlimit no_core={0,0};setrlimit(RLIMIT_CORE,&no_core);
        signal(SIGSEGV,SIG_DFL);signal(SIGBUS,SIG_DFL);
        *const_cast<volatile float *>(p)=1.f;_exit(0);
    }
    int status=0;CHECK(waitpid(child,&status,0)==child);
    CHECK(WIFSIGNALED(status)&&(WTERMSIG(status)==SIGSEGV||WTERMSIG(status)==SIGBUS));
    ++protected_write_probes;
}
static void wrong_alias_control(Fixture &original,Fixture &packed) {
    ++cases;Tables wrong=packed.tables;
    wrong.morph[48].samples=packed.tables.morph[49].samples; // Triangle becomes square.
    Graph a,b;CHECK(a.init(original.tables,44100));CHECK(b.init(wrong,44100));
    CHECK(a.set_active_count(1));CHECK(b.set_active_count(1));
    CHECK(a.note_on(0,60,127,440));CHECK(b.note_on(0,60,127,440));
    parameter(a,b,index1,0);parameter(a,b,index2,0);parameter(a,b,noiseVolume,0);
    parameter(a,b,fmVolume,0);parameter(a,b,subVolume,0);parameter(a,b,filterMix,0);
    parameter(a,b,oscBandlimitEnable,1);parameter(a,b,attackDuration,.0001f);
    TestAccess::phase_and_band(a,0,12);TestAccess::phase_and_band(b,0,12);
    bool mismatch=false;
    for(unsigned frame=0;frame<257;++frame) {
        float al=0,ar=0,bl=0,br=0;CHECK(a.process(&al,&ar));CHECK(b.process(&bl,&br));
        if(std::memcmp(&al,&bl,4)||std::memcmp(&ar,&br,4))mismatch=true;
    }
    CHECK(mismatch); // Functional oracle detects an in-domain nonduplicate alias.
}
int main(int argc,char **argv) {
    static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559,"IEEE754 binary32 required");
    uint32_t endian=1;CHECK(*reinterpret_cast<unsigned char *>(&endian)==1);
    if(argc!=55){std::fprintf(stderr,"usage: alias_graph_test RAW52x4096 PACKED50x4097 52_OFFSETS\n");return 2;}
    size_t offsets[52];
    for(unsigned i=0;i<52;++i) {
        char *end=nullptr;unsigned long long value=std::strtoull(argv[i+3],&end,10);
        CHECK(end&&*end=='\0'&&value<=819400-table_samples*sizeof(float));offsets[i]=value;
        const unsigned expected=i==48?44:(i==49?45:(i>=50?i-2:i));
        CHECK(offsets[i]==expected*table_samples*sizeof(float));
    }
    Fixture original(argv[1],false,offsets),packed(argv[2],true,offsets);
    CHECK(packed.tables.morph[44].samples==packed.tables.morph[48].samples);
    CHECK(packed.tables.morph[45].samples==packed.tables.morph[49].samples);
    CHECK(original.tables.morph[44].samples!=original.tables.morph[48].samples);
    CHECK(original.tables.morph[45].samples!=original.tables.morph[49].samples);
    for(unsigned i=0;i<52;++i)CHECK(std::memcmp(original.tables.morph[i].samples,packed.tables.morph[i].samples,
                                               table_samples*sizeof(float))==0);
    CHECK(std::memcmp(original.tables.sine,packed.tables.sine,table_samples*sizeof(float))==0);
    for(int rate:{44100,48000,96000})exercise(original,packed,rate);
    wrong_alias_control(original,packed);
    // Verify OS immutability separately for both alias pairs and their guards,
    // original distinct storage, and the separately generated pinned sine row.
    for(const float *p:{packed.tables.morph[48].samples,packed.tables.morph[49].samples,
                       packed.tables.morph[48].samples+table_size,packed.tables.morph[49].samples+table_size,
                       original.tables.morph[48].samples,packed.tables.sine,packed.tables.sine+table_size})write_probe(p);
    std::printf("{\"cases\":%u,\"frames_pcm_bitwise\":%llu,\"six_note_state_snapshots\":%llu,"
                "\"voice_calls_per_graph\":%llu,\"checks\":%llu,\"protected_write_probes\":%u,"
                "\"wrong_nonduplicate_alias_detected\":true,\"logical_rows\":52,\"unique_morph_rows\":50,"
                "\"guarded_morph_bytes\":819400,\"separate_sine_bytes\":16388,\"target_qualified\":false}\n",
                cases,frames,snapshots,voice_calls,checks,protected_write_probes);
}
