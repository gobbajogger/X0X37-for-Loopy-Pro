// Changed-input synthetic event/block differential; no preset bank or audio export.
#include "scheduler.hpp"
#include "oracle/oracle.hpp"
#include "wave_frequencies.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <limits>
#include <type_traits>
#include <sys/mman.h>
#include <unistd.h>

using namespace s1scheduler;
using namespace synthone_shared;
static unsigned long long checks=0, frames=0, snapshots=0, events=0;
static unsigned cases=0;
static const char *case_name="setup";
static void fail(const char *what,int line) {std::fprintf(stderr,"FAIL %s line %d: %s\n",case_name,line,what);std::exit(1);}
#define CHECK(x) do{++checks;if(!(x))fail(#x,__LINE__);}while(0)
static bool deny_heap=false;
extern "C" {
void *__real_malloc(size_t);void *__real_calloc(size_t,size_t);void *__real_realloc(void*,size_t);void __real_free(void*);
void *__wrap_malloc(size_t n){if(deny_heap)fail("heap malloc",__LINE__);void*p=__real_malloc(n);if(p)std::memset(p,0,n);return p;}
void *__wrap_calloc(size_t n,size_t m){if(deny_heap)fail("heap calloc",__LINE__);return __real_calloc(n,m);}
void *__wrap_realloc(void*p,size_t n){if(deny_heap)fail("heap realloc",__LINE__);return __real_realloc(p,n);}
void __wrap_free(void*p){if(deny_heap)fail("heap free",__LINE__);__real_free(p);}
}
struct NoHeap {NoHeap(){CHECK(!deny_heap);deny_heap=true;}~NoHeap(){deny_heap=false;}};
struct Fixture {
    Tables tables{};float *data=nullptr;size_t bytes=0;float *tuning=nullptr;
    explicit Fixture(const char *path) {
        size_t payload=(53*4097+128)*sizeof(float),page=sysconf(_SC_PAGESIZE);
        bytes=(payload+page-1)/page*page;
        data=static_cast<float*>(mmap(nullptr,bytes,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));CHECK(data!=MAP_FAILED);
        FILE*f=std::fopen(path,"rb");CHECK(f);CHECK(std::fread(data,sizeof(float),52*4097,f)==52*4097);CHECK(std::fgetc(f)==EOF);CHECK(!std::fclose(f));
        for(unsigned b=0;b<13;++b){tables.frequency_band[b]=wave_frequencies[b];for(unsigned s=0;s<4;++s)tables.morph[b*4+s]={data+(b*4+s)*4097,4097,b,static_cast<Shape>(s)};}
        tables.sine=data+52*4097;tables.sine_count=4097;
        sp_data context{};context.sr=44100;sp_ftbl d{};d.tbl=const_cast<float*>(tables.sine);
        sp_ftbl_init(&context,&d,4096);sp_gen_sine(&context,&d);d.tbl[4096]=0;
        tuning=data+53*4097;
        for(unsigned i=0;i<128;++i)tuning[i]=440.*exp2((static_cast<int>(i)-69)/12.);
        CHECK(!mprotect(data,bytes,PROT_READ));
    }
    ~Fixture(){CHECK(!munmap(data,bytes));}
};
// Canonical field serialization avoids pointer addresses, padding and hashes.
struct Snapshot {
    unsigned char bytes[16384]{};size_t size=0;
    template<class T>void put(const T &value){CHECK(size+sizeof value<=sizeof bytes);std::memcpy(bytes+size,&value,sizeof value);size+=sizeof value;}
};
#define F(x) z.put(a.x)
static void record(Snapshot&z,const sp_adsr&a){F(atk);F(dec);F(sus);F(rel);F(timer);F(atk_time);F(a);F(b);F(y);F(x);F(prev);F(mode);}
static void record(Snapshot&z,const sp_oscmorph2d&a){F(freq);F(amp);F(iphs);F(lphs);F(inc);F(wtpos);F(nft);F(nbl);F(enableBandlimit);F(bandlimitIndexOverride);}
static void record(Snapshot&z,const sp_crossfade&a){F(pos);}
static void record(Snapshot&z,const sp_osc&a){F(freq);F(amp);F(iphs);F(lphs);F(inc);}
static void record(Snapshot&z,const sp_fosc&a){F(amp);F(freq);F(car);F(mod);F(indx);F(iphs);F(mphs);F(cphs);}
static void record(Snapshot&z,const sp_noise&a){F(amp);}
static void record(Snapshot&z,const sp_moogladder&a){F(freq);F(res);F(istor);F(oldfreq);F(oldres);F(oldacr);F(oldtune);F(delay);F(tanhstg);}
static void record(Snapshot&z,const sp_butbp&a){F(sr);F(freq);F(bw);F(istor);F(lkf);F(lkb);F(pidsr);F(tpidsr);F(a);}
static void record(Snapshot&z,const sp_buthp&a){F(sr);F(freq);F(istor);F(lkf);F(pidsr);F(a);}
static void record(Snapshot&z,const sp_phasor&a){F(freq);F(phs);F(curphs);F(onedsr);}
static void record(Snapshot&z,const sp_port&a){F(htime);F(c1);F(c2);F(yt1);F(prvhtim);F(sr);F(onedsr);}
static void record(Snapshot&z,const S1NoteState&a,bool initialized){
    F(stage);F(internalGate);F(velocity);F(amp);F(filter);F(rootNoteNumber);F(transpose);
    if(!initialized)return;
    record(z,*a.adsr);record(z,*a.fadsr);record(z,*a.oscmorph1);record(z,*a.oscmorph2);
    record(z,*a.morphCrossFade);record(z,*a.filterCrossFade);record(z,*a.subOsc);record(z,*a.fmOsc);
    record(z,*a.noise);record(z,*a.loPass);record(z,*a.bandPass);record(z,*a.hiPass);
}
static Snapshot common_capture(const State&a){
    Snapshot z;
    F(parameters);F(playingNoteStatesIndex);F(monoFrequency);F(monoFrequencySmooth);F(previousProcessMonoPolyStatus);
    F(resetted);F(initializedNoteStates);F(processSampleCounter);
    z.put(a.heldNoteNumbers.count);z.put(a.heldNoteNumbersAE.count);
    for(int i=0;i<a.heldNoteNumbers.count;++i)z.put(a.heldNoteNumbers[i]);
    for(int i=0;i<a.heldNoteNumbersAE.count;++i)z.put(a.heldNoteNumbersAE[i]);
    z.put(a.sequencer.resets);z.put(a.sequencer.pending_empty);
    for(auto&v:a.poly)record(z,v,a.initializedNoteStates);record(z,a.mono,a.initializedNoteStates);
    record(z,*a.lfo1Phasor);record(z,*a.lfo2Phasor);record(z,*a.lfo1Port);record(z,*a.lfo2Port);record(z,*a.monoFrequencyPort);
    F(lfo1_0_1);F(lfo1_1_0);F(lfo2_0_1);F(lfo2_1_0);F(lfo3_0_1);F(lfo3_1_0);F(lfo1Smooth);F(lfo2Smooth);
    z.put(a.sp->sr);z.put(a.sp->rand);z.put(a.sp->pos);z.put(*a.sp->out);z.put(a.sp->nchan);z.put(a.sp->len);z.put(a.sp->filename);
    z.put(a.ft_frequencyBand);z.put(a.initialized());
    for(unsigned i=0;i<53;++i){const auto&d=i<52?*a.ft_array[i]:*a.sine;z.put(d.size);z.put(d.lobits);z.put(d.lomask);z.put(d.lodiv);z.put(d.sicvt);z.put(d.del);z.put(d.tbl);}
    return z;
}
static Snapshot capture(const RuntimeState&a){
    Snapshot z=common_capture(a);
    for(unsigned i=0;i<150;++i)if(synthone_ports::selected_slots[i]!=synthone_ports::unsmoothed_slot){
        z.put(a.compactPorts.target(i).value);record(z,a.compactPorts.port(i).state);
    }
    return z;
}
static Snapshot capture(const OracleState&a){
    Snapshot z=common_capture(a);
    for(unsigned i=0;i<150;++i)if(a.s1p[i].usePortamento){
        z.put(a.s1p[i].portamentoTarget);record(z,*a.s1p[i].portamento);
    }
    return z;
}
#undef F
static void equal(const Snapshot&a,const Snapshot&b){++snapshots;CHECK(a.size==b.size);if(std::memcmp(a.bytes,b.bytes,a.size)){for(size_t i=0;i<a.size;++i)if(a.bytes[i]!=b.bytes[i]){std::fprintf(stderr,"snapshot byte %zu of %zu: %u != %u\n",i,a.size,a.bytes[i],b.bytes[i]);break;}fail("exact state snapshot",__LINE__);}}
struct Trace {std::vector<Snapshot> values;};
template<class S>static void traced(const S&s,unsigned phase,unsigned frame,void*context){auto&t=*static_cast<Trace*>(context);CHECK(t.values.size()<t.values.capacity());auto z=capture(s);z.put(phase);z.put(frame);t.values.push_back(z);}
struct Pair {
    Scheduler s;Oracle o;Fixture&fixture;float values[150];
    Pair(Fixture&f,int rate,bool mono=false):fixture(f){std::memcpy(values,defaults,sizeof values);values[isMono]=mono?1:0;CHECK(s.init(f.tables,rate,values,150,f.tuning,128));CHECK(o.init(f.tables,rate,values,150,f.tuning,128));
        unsigned selected=0;
        for(unsigned i=0;i<150;++i){CHECK(o.s1p[i].usePortamento==(synthone_ports::selected_slots[i]!=synthone_ports::unsmoothed_slot));if(o.s1p[i].usePortamento){CHECK(synthone_ports::selected_indices[selected]==i);++selected;}}
        CHECK(selected==46);compare();}
    void compare(){equal(capture(s),capture(o));}
    void parameter(unsigned p,float v){NoHeap n;CHECK(s.set_target(p,v));o._setSynthParameter(p,v);++events;compare();}
    void start(int p,int v,float f=1){NoHeap n;CHECK(s.start(p,v,f));o.startNote(p,v,f);++events;compare();}
    void on(int p,int v,float f){NoHeap n;CHECK(s.on(p,v,f));o.turnOnKey(p,v,f);++events;compare();}
    void on_tuned(int p,int v){NoHeap n;CHECK(s.on(p,v));o.turnOnKey(p,v);++events;compare();}
    void off(int p){NoHeap n;CHECK(s.off(p));o.turnOffKey(p);++events;compare();}
    void stop(int p){NoHeap n;CHECK(s.stop(p));o.stopNote(p);++events;compare();}
    void reset(){NoHeap n;CHECK(s.reset());o.reset();++events;compare();}
    void reset_dsp(){NoHeap n;CHECK(s.reset_dsp());o.resetDSP();++events;compare();}
    void stop_all(){NoHeap n;CHECK(s.stop_all());o.stopAllNotes();++events;compare();}
    void restore(){NoHeap n;CHECK(s.restore(values,150));CHECK(o.restore(values,150));++events;compare();}
    void block(unsigned n,unsigned offset=0){
        CHECK(n<=512&&offset<=7);float sl[520],sr[520],ol[520],orr[520];
        for(unsigned i=0;i<520;++i)sl[i]=sr[i]=ol[i]=orr[i]=123.25f;
        Trace st,ot;st.values.reserve(n+1);ot.values.reserve(n+1);
        s.trace_fn=traced<RuntimeState>;o.trace_fn=traced<OracleState>;s.trace_context=&st;o.trace_context=&ot;
        {NoHeap guard;CHECK(s.process(n,offset,sl,sr));o.process(n,offset,ol,orr);}
        s.trace_fn=nullptr;o.trace_fn=nullptr;s.trace_context=o.trace_context=nullptr;
        CHECK(st.values.size()==n+1&&ot.values.size()==n+1);
        for(size_t i=0;i<st.values.size();++i)equal(st.values[i],ot.values[i]);
        CHECK(!std::memcmp(sl,ol,sizeof sl)&&!std::memcmp(sr,orr,sizeof sr));
        for(unsigned i=0;i<n;++i){CHECK(std::isfinite(sl[offset+i]));CHECK(std::isfinite(sr[offset+i]));}
        frames+=n;compare();
    }
};
// A constructed instance owns self-referential DSP/control pointers and cannot relocate.
static_assert(!std::is_copy_constructible<Scheduler>::value && !std::is_copy_assignable<Scheduler>::value &&
              !std::is_move_constructible<Scheduler>::value && !std::is_move_assignable<Scheduler>::value,"runtime address stability");
static_assert(!std::is_copy_constructible<Oracle>::value && !std::is_copy_assignable<Oracle>::value &&
              !std::is_move_constructible<Oracle>::value && !std::is_move_assignable<Oracle>::value,"oracle address stability");
template<class S>static bool inside(const S&a,const void*p,size_t bytes){
    auto begin=reinterpret_cast<uintptr_t>(&a),x=reinterpret_cast<uintptr_t>(p);
    return x>=begin && x-begin<=sizeof a && bytes<=sizeof a-(x-begin);
}
template<class S>static void self_bound(const S&a){
    CHECK(inside(a,a.sp,sizeof *a.sp));CHECK(inside(a,a.sp->out,sizeof(float)));
    CHECK(inside(a,a.lfo1Phasor,sizeof *a.lfo1Phasor));CHECK(inside(a,a.lfo2Phasor,sizeof *a.lfo2Phasor));
    CHECK(inside(a,a.lfo1Port,sizeof *a.lfo1Port));CHECK(inside(a,a.lfo2Port,sizeof *a.lfo2Port));
    CHECK(inside(a,a.monoFrequencyPort,sizeof *a.monoFrequencyPort));
    for(unsigned i=0;i<53;++i)CHECK(inside(a,i<52?a.ft_array[i]:a.sine,sizeof(sp_ftbl)));
    if(a.initializedNoteStates)for(unsigned i=0;i<7;++i){const auto&v=i<6?a.poly[i]:a.mono;
        CHECK(v.kernel==static_cast<const State*>(&a));CHECK(inside(a,v.adsr,sizeof *v.adsr));CHECK(inside(a,v.fadsr,sizeof *v.fadsr));
        CHECK(inside(a,v.oscmorph1,sizeof *v.oscmorph1));CHECK(inside(a,v.oscmorph2,sizeof *v.oscmorph2));
        CHECK(inside(a,v.morphCrossFade,sizeof *v.morphCrossFade));CHECK(inside(a,v.filterCrossFade,sizeof *v.filterCrossFade));
        CHECK(inside(a,v.subOsc,sizeof *v.subOsc));CHECK(inside(a,v.fmOsc,sizeof *v.fmOsc));CHECK(inside(a,v.noise,sizeof *v.noise));
        CHECK(inside(a,v.loPass,sizeof *v.loPass));CHECK(inside(a,v.bandPass,sizeof *v.bandPass));CHECK(inside(a,v.hiPass,sizeof *v.hiPass));
    }
}
template<class S>static void rejected_aliases(S&a,void*control,Fixture&f,int rate){
    unsigned char before[sizeof(S)];std::memcpy(before,&a,sizeof a);
    float left[4]={1,2,3,4},right[4]={5,6,7,8};auto*internal=reinterpret_cast<float*>(control);
    Tables bad=f.tables;bad.morph[0].samples=internal;
    {NoHeap n;
        CHECK(!a.restore(internal,150));CHECK(!a.restore(a.parameters+1,150));
        CHECK(!a.init(f.tables,rate,internal,150,f.tuning,128));
        CHECK(!a.init(f.tables,rate,defaults,150,internal,128));
        CHECK(!a.init(bad,rate,defaults,150,f.tuning,128));
        CHECK(!a.valid_output(internal,right,1,0));CHECK(!a.valid_output(left,internal,1,0));
        CHECK(!a.valid_output(a.parameters,right,1,0));CHECK(!a.valid_output(left,left,1,0));
    }
    CHECK(!std::memcmp(before,&a,sizeof a));CHECK(left[0]==1&&right[0]==5);
}
static void run(Fixture&f,int rate){
    case_name="concrete whole-instance aliases reject atomically";++cases;
    {Pair p(f,rate);p.start(60,80);p.block(16);
        rejected_aliases(p.s,&p.s.compactPorts,f,rate);
        rejected_aliases(p.o,p.o.s1p,f,rate);
        rejected_aliases(p.o,p.o.s1p[149].portamento,f,rate);
        auto before=capture(p.s);float output=77;
        {NoHeap n;CHECK(!p.s.process(1,0,reinterpret_cast<float*>(&p.s.compactPorts),&output));}
        equal(before,capture(p.s));CHECK(output==77);p.compare();p.block(3);
    }
    case_name="exact output restore, independent instances and fresh reinit";++cases;
    {Pair p(f,rate);p.start(60,90);p.block(20);p.restore();
        {NoHeap n;CHECK(p.s.restore(p.s.parameters,150));CHECK(p.o.restore(p.o.parameters,150));}
        p.compare();p.start(67,100);p.block(20);self_bound(p.s);self_bound(p.o);
        for(unsigned i=0;i<150;++i)CHECK(inside(p.o,p.o.s1p[i].portamento,sizeof(sp_port)));
        auto before_s=capture(p.s),before_o=capture(p.o);
        {Pair other(f,rate);other.start(35,40);other.block(12);equal(before_s,capture(p.s));equal(before_o,capture(p.o));}
        {NoHeap n;CHECK(p.s.init(f.tables,rate,p.values,150,f.tuning,128));CHECK(p.o.init(f.tables,rate,p.values,150,f.tuning,128));}
        Pair fresh(f,rate);p.compare();equal(capture(p.s),capture(fresh.s));equal(capture(p.o),capture(fresh.o));
        p.start(40,90);p.block(20);self_bound(p.s);self_bound(p.o);
    }
    case_name="source-header fresh LFO inverses and 128 held pitches";++cases;
    {Pair p(f,rate);CHECK(p.s.lfo1_1_0==0&&p.s.lfo2_1_0==0&&p.s.lfo3_1_0==0);
        for(int i=0;i<128;++i)p.start(i,i%128);CHECK(p.s.heldNoteNumbers.count==128);p.start(63,117);CHECK(p.s.heldNoteNumbers.count==128&&p.s.heldNoteNumbers[0].noteNumber==63);p.block(32);p.stop_all();CHECK(p.s.heldNoteNumbers.count==0);p.block(10);}
    case_name="static fresh and lazy restore versus actual source NoteState init";++cases;
    {Pair p(f,rate);p.s.initializeNoteStates();p.o.initializeNoteStates();
        for(int pass=0;pass<2;++pass){
            if(pass){p.start(70,100);p.block(80);p.restore();p.s.poly[0].amp=p.o.poly[0].amp=.37f;p.s.poly[0].filter=p.o.poly[0].filter=.27f;}
            S1NoteState original{};original.kernel=&p.o;original.amp=p.s.poly[0].amp;original.filter=p.s.poly[0].filter;original.velocity=p.s.poly[0].velocity;
            original.init();original.stage=S1NoteState::stageOff;original.internalGate=0;original.rootNoteNumber=-1;original.transpose=0;
            {NoHeap n;p.s.initializeNoteStates();p.o.initializeNoteStates();}
            Snapshot a,b;record(a,p.s.poly[0],true);record(b,original,true);equal(a,b);original.destroy();p.compare();
        }
    }
    case_name="sparse slots and PRNG with zero noise volume";++cases;
    {Pair p(f,rate);p.start(60,100);p.start(64,72);p.start(67,110);p.start(71,50);p.block(67,3);p.stop(64);p.s.poly[1].amp=p.o.poly[1].amp=.009f;p.block(64);CHECK(p.s.poly[1].rootNoteNumber==-1);p.parameter(noiseVolume,0);auto before=p.s.sp->rand;p.block(128);CHECK(p.s.sp->rand!=before);p.stop(67);p.s.poly[2].amp=p.o.poly[2].amp=.009f;p.block(17);p.block(64);}
    case_name="cursor retrigger and stealing retain processor history";++cases;
    {Pair p(f,rate);for(int n=60;n<66;++n)p.start(n,80+n%30);p.block(80);p.start(62,97);CHECK(p.s.playingNoteStatesIndex==2);p.start(66,64);CHECK(p.s.playingNoteStatesIndex==3&&p.s.poly[3].rootNoteNumber==66);p.stop(63);p.block(32);p.start(62,40);p.block(65);}
    case_name="release threshold and extra poly ADSR step";++cases;
    {Pair p(f,rate);p.start(60,120);p.block(512);p.stop(60);p.s.poly[0].amp=p.o.poly[0].amp=.01f;p.block(0);CHECK(p.s.poly[0].rootNoteNumber==60);p.s.poly[0].amp=p.o.poly[0].amp=.0101f;p.s.poly[0].adsr->y=p.o.poly[0].adsr->y=.009f;p.block(128);CHECK(p.s.poly[0].amp<.01f&&p.s.poly[0].rootNoteNumber==60);p.block(0);CHECK(p.s.poly[0].rootNoteNumber==-1);}
    for(int legato=0;legato<2;++legato){case_name=legato?"mono nonzero legato fallback":"mono zero legato fallback";++cases;
        Pair p(f,rate,true);p.parameter(monoIsLegato,legato);p.parameter(glide,.17f);p.start(60,40,123);p.block(90);p.start(64,110,987);p.block(100);float noise=p.s.mono.noise->amp;p.stop(64);CHECK(p.s.mono.rootNoteNumber==60&&p.s.mono.noise->amp==noise);CHECK(p.s.mono.oscmorph1->amp!=noise);p.block(99);p.stop(12);p.block(43);p.start(67,90);p.stop(60);p.block(68);p.stop(67);p.block(100);}
    case_name="raw unequal positive mono reset and retained state";++cases;
    {Pair p(f,rate,true);p.start(60,100);p.block(75);auto rand=p.s.sp->rand;p.parameter(isMono,.5f);p.block(1);CHECK(p.s.mono.rootNoteNumber==-1&&p.s.resetted);CHECK(p.s.sp->rand==rand);CHECK(p.s.heldNoteNumbers.count==1);p.start(67,90);p.parameter(isMono,0);p.block(3);p.start(62,100);p.block(23);}
    case_name="stop versus reset versus fresh and restore";++cases;
    {Pair p(f,rate);p.start(62,100);p.start(67,70);p.block(512);p.stop_all();CHECK(p.s.heldNoteNumbers.count==0&&p.s.poly[0].stage==S1NoteState::stageRelease);p.block(20);p.start(70,90);int cursor=p.s.playingNoteStatesIndex;auto rand=p.s.sp->rand;p.reset();CHECK(p.s.playingNoteStatesIndex==cursor&&p.s.sp->rand==rand&&p.s.heldNoteNumbers.count==1);p.block(9);p.reset_dsp();CHECK(p.s.heldNoteNumbers.count==0&&p.s.playingNoteStatesIndex==cursor);p.block(10);p.start(50,30);p.block(55);p.restore();p.block(6);CHECK(p.s.playingNoteStatesIndex==cursor||p.s.playingNoteStatesIndex==0);}
    case_name="held pitch dedup, transpose and borrowed tuning clamp";++cases;
    {Pair p(f,rate,true);p.start(0,20);p.parameter(transpose,-24);p.start(0,100,999);CHECK(p.s.heldNoteNumbers.count==1);CHECK(p.s.mono.oscmorph1->freq==f.tuning[0]);p.block(50);p.parameter(transpose,24);p.start(127,120,123);CHECK(p.s.mono.oscmorph1->freq==f.tuning[127]);p.stop(127);p.block(12);p.on(12,45,222);CHECK(p.s.mono.oscmorph1->freq==222);p.block(11);p.stop(0);p.block(4);}
    case_name="target144 current-output half-time and glide phase";++cases;
    {Pair p(f,rate,true);p.parameter(portamentoHalfTime,.3f);p.parameter(glide,.2f);p.start(60,90);p.block(1);p.parameter(portamentoHalfTime,.04f);p.block(45);p.start(72,100);float prev=p.s.monoFrequencySmooth;p.block(1);CHECK(p.s.monoFrequencySmooth!=prev);p.parameter(lfo1Index,3);p.parameter(lfo2Index,2);p.parameter(lfo1Amplitude,.8f);p.parameter(lfo2Amplitude,.7f);p.parameter(pitchLFO,3);p.block(120);}
    case_name="target144 below-minimum current output is clamped";++cases;
    {Pair p(f,rate);p.values[portamentoHalfTime]=minimums[portamentoHalfTime];p.restore();p.block(1);CHECK(p.s.parameters[portamentoHalfTime]<minimums[portamentoHalfTime]);p.parameter(portamentoHalfTime,.9f);CHECK(p.s.compactPorts.port(index1).state.htime==minimums[portamentoHalfTime]);p.start(60,90);p.block(23);}
    case_name="mono release threshold selects only current bank";++cases;
    {Pair p(f,rate,true);p.start(60,100);p.block(80);p.stop(60);p.s.mono.amp=p.o.mono.amp=.01f;
        p.s.poly[2].stage=p.o.poly[2].stage=S1NoteState::stageRelease;p.s.poly[2].rootNoteNumber=p.o.poly[2].rootNoteNumber=65;p.s.poly[2].amp=p.o.poly[2].amp=.001f;
        p.block(0);CHECK(p.s.mono.rootNoteNumber==60&&p.s.poly[2].rootNoteNumber==65);p.s.mono.amp=p.o.mono.amp=.009f;p.block(0);CHECK(p.s.mono.rootNoteNumber==-1&&p.s.poly[2].rootNoteNumber==65);}
    case_name="direct tuned overload and current transpose";++cases;
    {Pair p(f,rate);p.parameter(transpose,12);p.on_tuned(60,100);CHECK(p.s.poly[0].oscmorph1->freq==f.tuning[72]);p.block(32);p.off(60);p.block(10);}
    case_name="empty block selected-bank release, counter, resetSequencer";++cases;
    {Pair p(f,rate);p.start(60,70);for(int i=0;i<5;++i)p.block(512);CHECK(p.s.processSampleCounter==0);{NoHeap n;CHECK(p.s.reset_sequencer());p.o.resetSequencer();p.compare();}p.block(0);}
    case_name="atomic invalid external admission";++cases;
    {Pair p(f,rate);p.start(60,70);p.block(20);auto before=capture(p.s);float x[4]={1,2,3,4},y[4]={5,6,7,8};
        {NoHeap n;CHECK(!p.s.start(-1,80));CHECK(!p.s.start(128,80));CHECK(!p.s.start(60,128));CHECK(!p.s.on(60,80,std::numeric_limits<float>::quiet_NaN()));CHECK(!p.s.set_target(150,1));CHECK(!p.s.set_target(arpIsOn,1));CHECK(!p.s.set_target(cutoff,std::numeric_limits<float>::infinity()));CHECK(!p.s.process(4,0,x,x));CHECK(!p.s.process(65537,0,x,y));CHECK(!p.s.process(1,0,p.s.parameters,y));CHECK(!p.s.process(1,0,const_cast<float*>(f.tables.sine),y));CHECK(!p.s.restore(nullptr,150));}
        equal(before,capture(p.s));CHECK(x[0]==1&&y[0]==5);}
}
int main(int argc,char**argv){CHECK(argc==2);case_name="runtime excludes original oracle control storage";CHECK(sizeof(Scheduler)<sizeof(Oracle));Fixture fixture(argv[1]);for(int rate:{44100,48000,96000})run(fixture,rate);
    std::printf("{\"cases\":%u,\"checks\":%llu,\"frames\":%llu,\"state_snapshots\":%llu,\"events\":%llu,\"heap_calls_during_actions\":0,\"failures\":0}\n",cases,checks,frames,snapshots,events);return 0;}
