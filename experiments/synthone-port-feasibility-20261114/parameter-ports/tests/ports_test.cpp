// New compact ownership/state proof against actual AudioKit port.c.
#include "parameter_ports.hpp"
#include "reference_metadata.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <initializer_list>
#include <sys/mman.h>
#include <unistd.h>
using synthone_ports::Ports;
static unsigned long long checks, output_comparisons, state_comparisons, ticks;
static unsigned cases;
static void fail(const char *expression,int line) { std::fprintf(stderr,"FAIL line %d: %s\n",line,expression);std::exit(1); }
#define CHECK(x) do { ++checks;if(!(x))fail(#x,__LINE__); } while(0)
struct Allocations { size_t mallocs,callocs,reallocs,frees,requested_bytes; };
static Allocations allocations{};
static bool deny_heap;
static unsigned long long compact_pow_calls, reference_pow_calls;
extern "C" {
void *__real_malloc(size_t);void *__real_calloc(size_t,size_t);void *__real_realloc(void*,size_t);void __real_free(void*);
double __real_pow(double,double);
double __wrap_pow(double a,double b){if(deny_heap)++compact_pow_calls;else ++reference_pow_calls;return __real_pow(a,b);}
void *__wrap_malloc(size_t bytes) { if(deny_heap)fail("malloc in compact action",__LINE__);++allocations.mallocs;allocations.requested_bytes+=bytes;void*p=__real_malloc(bytes);if(p)std::memset(p,0,bytes);return p; }
void *__wrap_calloc(size_t n,size_t bytes) { if(deny_heap)fail("calloc in compact action",__LINE__);++allocations.callocs;return __real_calloc(n,bytes); }
void *__wrap_realloc(void*p,size_t bytes) { if(deny_heap)fail("realloc in compact action",__LINE__);++allocations.reallocs;return __real_realloc(p,bytes); }
void __wrap_free(void*p) { if(deny_heap)fail("free in compact action",__LINE__);++allocations.frees;__real_free(p); }
}
struct NoHeap { Allocations before;NoHeap():before(allocations){CHECK(!deny_heap);deny_heap=true;}~NoHeap(){deny_heap=false;CHECK(std::memcmp(&before,&allocations,sizeof before)==0);} };
static void same(float a,float b){CHECK(std::memcmp(&a,&b,sizeof a)==0);}
static float physical(unsigned i,float value) {
    const auto&m=source_metadata::parameters[i];return std::min(std::max(value,m.minimum),m.maximum);
}
static float half_time(float value) {return physical(144,value);}
struct Reference {
    sp_data context{};
    sp_port *port[150]{};
    float output[150]{},target[150]{};
    Reference(const float *values,int rate) {
        context.sr=rate;
        for(unsigned i=0;i<150;++i)CHECK(sp_port_create(&port[i])==SP_OK);
        restore(values,rate);
    }
    ~Reference(){for(auto&p:port)CHECK(sp_port_destroy(&p)==SP_OK);}
    void update(float value) {
        const float h=half_time(value);
        for(unsigned i=0;i<150;++i)if(source_metadata::parameters[i].selected)port[i]->htime=h;
    }
    void restore(const float *values,int rate) {
        context.sr=rate;
        // Actual master setupParameterTree body and subsequent uniform update.
        for(unsigned i=0;i<150;++i) {
            if(source_metadata::parameters[i].selected) {
                target[i]=values[i];sp_port_init(&context,port[i],values[i]);port[i]->htime=.1f;
            }
            output[i]=values[i];
        }
        update(output[144]);
    }
    void set(unsigned i,float value) {
        if(source_metadata::parameters[i].selected)target[i]=value;else output[i]=value;
        // Actual getParameter returns output, unlike getSynthParameter's target.
        if(i==144)update(output[144]);
    }
    void tick() {
        // Original master150-metadata traversal, actual linked C port computation.
        for(unsigned i=0;i<150;++i)if(source_metadata::parameters[i].selected)
            sp_port_compute(&context,port[i],&target[i],&output[i]);
    }
};
static void values_for(float *values,unsigned profile) {
    for(unsigned i=0;i<150;++i) {
        const auto&m=source_metadata::parameters[i];
        values[i]=profile%3==0?physical(i,m.initial):profile%3==1?m.minimum:m.maximum;
    }
}
static void compare(const Ports&p,const Reference&r,const float*out) {
    NoHeap n;
    for(unsigned i=0;i<150;++i) {
        same(out[i],r.output[i]);++output_comparisons;
        const auto target=p.target(i);CHECK(target.valid);
        same(target.value,source_metadata::parameters[i].selected?r.target[i]:r.output[i]);
        const auto view=p.port(i);
        if(source_metadata::parameters[i].selected) {
            CHECK(view.valid);const auto&a=view.state;const auto&b=*r.port[i];
            same(a.htime,b.htime);same(a.c1,b.c1);same(a.c2,b.c2);same(a.yt1,b.yt1);
            same(a.prvhtim,b.prvhtim);same(a.sr,b.sr);same(a.onedsr,b.onedsr);++state_comparisons;
        } else CHECK(!view.valid);
    }
}
static void set(Ports&p,Reference&r,unsigned i,float value) {
    value=physical(i,value);{NoHeap n;CHECK(p.set_resolved_target(i,value));}r.set(i,value);
}
static void update(Ports&p,Reference&r,float h) {{NoHeap n;CHECK(p.update_half_time(h));}r.update(h);}
static void batch(Ports&p,Reference&r,float*out,unsigned count) {
    for(unsigned i=0;i<count;++i){{NoHeap n;CHECK(p.tick());}r.tick();++ticks;compare(p,r,out);}
}
static uint32_t rng=0x4db0327u;
static float unit(){rng=rng*1664525u+1013904223u;return (rng&65535u)/65535.f;}
static void source_lifecycle(int rate,unsigned profile) {
    ++cases;float values[150],output[150];values_for(values,profile);Ports p;
    {NoHeap n;CHECK(p.init(output,150,values,150,rate));}
    const auto before=allocations;
    Reference r(values,rate);CHECK(allocations.mallocs-before.mallocs==150);CHECK(allocations.requested_bytes-before.requested_bytes==150*sizeof(sp_port));
    compare(p,r,output); // Before first compute: dirty sentinel and untouched coefs.
    for(unsigned frame=0;frame<512;++frame) {
        if(frame%13==0) {
            for(unsigned i=0;i<150;++i) {
                const auto&m=source_metadata::parameters[i];
                set(p,r,i,m.minimum+unit()*(m.maximum-m.minimum));
            }
            compare(p,r,output); // Setter storage/output visibility before tick.
        }
        if(frame%37==0) {
            const float old_output=output[144];set(p,r,144,frame%74?.000001f:.99f);
            same(p.port(144).state.htime,half_time(old_output));compare(p,r,output);
        }
        if(frame%41==0) {
            const float half_times[]={.000001f,.0053125f,.1f,.99f};
            update(p,r,half_times[(frame/41)%4]);compare(p,r,output);
        }
        if(frame==129||frame==317) {
            values_for(values,profile+frame);const int new_rate=frame==129?96000:44100;
            {NoHeap n;CHECK(p.restore(values,150,new_rate));}r.restore(values,new_rate);
            compare(p,r,output); // Restore retains oldc1/c2 and changes cached rate.
        }
        if(frame%59==0) {
            set(p,r,0,-0.f);set(p,r,30,-std::numeric_limits<float>::denorm_min());
            set(p,r,1,std::numeric_limits<float>::denorm_min());compare(p,r,output);
        }
        batch(p,r,output,frame%5==0?0:1); // Empty batches do not prepare coefficients.
        if(frame%47==0)batch(p,r,output,7);
    }
    batch(p,r,output,128);
    // Explicit in-place restore is permitted; copy reference inputs first.
    std::memcpy(values,output,sizeof values);
    {NoHeap n;CHECK(p.restore(output,150,48000));}r.restore(values,48000);
    compare(p,r,output);batch(p,r,output,64);
    {NoHeap n;p.teardown();p.teardown();CHECK(!p.initialized());CHECK(!p.tick());
     CHECK(!p.set_resolved_target(0,1));CHECK(!p.update_half_time(.1f));CHECK(!p.restore(values,150,rate));
     CHECK(!p.target(0).valid);CHECK(!p.port(0).valid);}
}
template<typename Action> static void invalid_action(Ports &p,float *output,Action action) {
    unsigned char before[sizeof p];float old_output[151];
    std::memcpy(before,&p,sizeof p);std::memcpy(old_output,output,sizeof old_output);
    {NoHeap n;CHECK(!action());}
    CHECK(std::memcmp(before,&p,sizeof p)==0);CHECK(std::memcmp(old_output,output,sizeof old_output)==0);
}
static void rejection_atomicity() {
    ++cases;float values[151]{},output[151]{};values_for(values,0);Ports p;
    CHECK(p.init(output,150,values,150,44100));Reference r(values,44100);batch(p,r,output,17);
    invalid_action(p,output,[&](){return p.init(nullptr,150,values,150,44100);});
    invalid_action(p,output,[&](){return p.init(output,149,values,150,44100);});
    invalid_action(p,output,[&](){return p.init(output,151,values,150,44100);});
    invalid_action(p,output,[&](){return p.init(output,150,nullptr,150,44100);});
    invalid_action(p,output,[&](){return p.init(output,150,values,149,44100);});
    invalid_action(p,output,[&](){return p.init(output,150,values,150,0);});
    invalid_action(p,output,[&](){return p.init(output,150,values,150,22050);});
    invalid_action(p,output,[&](){return p.init(values+1,150,values,150,44100);});
    invalid_action(p,output,[&](){return p.init(output,150,output+1,150,44100);});
    invalid_action(p,output,[&](){return p.init(reinterpret_cast<float*>(&p),150,values,150,44100);});
    invalid_action(p,output,[&](){return p.init(output,150,reinterpret_cast<const float*>(&p),150,44100);});
    for(float bad:{NAN,INFINITY,-INFINITY,48001.f,-48001.f}) {
        values[37]=bad;invalid_action(p,output,[&](){return p.restore(values,150,44100);});values[37]=physical(37,1);
        invalid_action(p,output,[&](){return p.set_resolved_target(37,bad);});
    }
    for(float bad:{0.f,-0.f,-.1f,NAN,INFINITY,-INFINITY,std::nextafter(.000001f,0.f),std::nextafter(.99f,INFINITY)})
        invalid_action(p,output,[&](){return p.update_half_time(bad);});
    invalid_action(p,output,[&](){return p.set_resolved_target(150,1);});
    invalid_action(p,output,[&](){return p.restore(values,149,44100);});
    CHECK(!p.target(150).valid);CHECK(!p.port(150).valid);CHECK(!p.port(3).valid);
    compare(p,r,output);batch(p,r,output,32);
}
static void instances_and_lifetime() {
    ++cases;float values[150],a_out[150],b_out[150];values_for(values,0);Ports a,b;
    std::memcpy(a_out,values,sizeof values);
    {NoHeap n;CHECK(a.init(a_out,150,a_out,150,44100));}
    {Reference in_place(values,44100);compare(a,in_place,a_out);batch(a,in_place,a_out,8);}
    CHECK(a.init(a_out,150,values,150,44100));Reference ar(values,44100);
    values_for(values,2);CHECK(b.init(b_out,150,values,150,96000));Reference br(values,96000);
    for(unsigned i=0;i<73;++i){set(a,ar,i%150,unit()*48000);set(b,br,(i*7)%150,-unit()*60);batch(a,ar,a_out,1);batch(b,br,b_out,2);}
    // Rebind to new caller output while the previous allocation becomes unreadable.
    const size_t page=static_cast<size_t>(sysconf(_SC_PAGESIZE));
    auto *old=static_cast<float*>(mmap(nullptr,page,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));CHECK(old!=MAP_FAILED);
    values_for(values,1);{NoHeap n;CHECK(a.init(old,150,values,150,44100));}
    {Reference fresh(values,44100);batch(a,fresh,old,8);}
    values_for(values,0);{NoHeap n;CHECK(a.init(a_out,150,values,150,48000));}
    CHECK(mprotect(old,page,PROT_NONE)==0);
    {Reference fresh(values,48000);compare(a,fresh,a_out);batch(a,fresh,a_out,33);}
    {NoHeap n;a.teardown();}CHECK(munmap(old,page)==0);
    // The consumed initial-value buffer need not outlive initialization.
    auto *initial=static_cast<float*>(mmap(nullptr,page,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));CHECK(initial!=MAP_FAILED);
    values_for(initial,0);std::memcpy(values,initial,sizeof values);
    {NoHeap n;CHECK(a.init(a_out,150,initial,150,44100));}CHECK(mprotect(initial,page,PROT_NONE)==0);
    {Reference fresh(values,44100);batch(a,fresh,a_out,17);}
    {NoHeap n;a.teardown();}CHECK(munmap(initial,page)==0);
}
static void map_contract() {
    ++cases;unsigned selected=0;
    for(unsigned i=0;i<150;++i) {
        const auto &metadata=source_metadata::parameters[i];
        CHECK(std::isfinite(metadata.minimum)&&std::isfinite(metadata.maximum));
        CHECK(metadata.minimum>=-48000.f&&metadata.maximum<=48000.f&&metadata.minimum<=metadata.maximum);
        if(source_metadata::parameters[i].selected) {
            CHECK(synthone_ports::selected_slots[i]==selected);
            CHECK(synthone_ports::selected_indices[selected]==i);++selected;
        } else CHECK(synthone_ports::selected_slots[i]==synthone_ports::unsmoothed_slot);
    }
    CHECK(selected==46);
}
struct DenseStateOnly { sp_port port[150];float target[150]; };
struct LegacyOwnershipSubtotal { sp_port port[150];sp_port*pointer[150];float target[150];float output[150]; };
int main() {
    static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559,"source IEEE float32 contract");
    map_contract();
    for(int rate:{44100,48000,96000})for(unsigned profile=0;profile<3;++profile)source_lifecycle(rate,profile);
    rejection_atomicity();instances_and_lifetime();
    CHECK(reference_pow_calls==compact_pow_calls*46);
    std::printf("{\"cases\":%u,\"ticks\":%llu,\"all150_output_comparisons\":%llu,\"seven_field_selected_state_comparisons\":%llu,\"checks\":%llu,"
        "\"compact_state_bytes_host\":%zu,\"caller_output_bytes\":%zu,\"dense_ports_and_targets_bytes_host\":%zu,\"legacy_with_pointer_output_subtotal_bytes_host\":%zu,"
        "\"immutable_map_bytes\":196,\"compact_pow_calls\":%llu,\"reference_pow_calls\":%llu,\"static_action_heap_calls\":0,\"oracle_mallocs\":%zu,\"oracle_frees\":%zu,\"oracle_requested_bytes\":%zu,\"pointer_bytes\":%zu}\n",
        cases,ticks,output_comparisons,state_comparisons,checks,sizeof(Ports),150*sizeof(float),sizeof(DenseStateOnly),sizeof(LegacyOwnershipSubtotal),compact_pow_calls,reference_pow_calls,allocations.mallocs,allocations.frees,allocations.requested_bytes,sizeof(void*));
}
