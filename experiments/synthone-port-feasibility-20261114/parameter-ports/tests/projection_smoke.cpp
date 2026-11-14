// New narrow portability/binding smoke; the full representation oracle is separate.
#include "parameter_ports.hpp"
#include "reference_metadata.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <limits>
using synthone_ports::Ports;
static int checks;
#define CHECK(x) do {++checks;if(!(x)){std::fprintf(stderr,"smoke failed line %d\n",__LINE__);return 1;}}while(0)
static bool equal(float a,float b){return std::memcmp(&a,&b,sizeof a)==0;}
int main(){
    static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559,"source IEEE float32");
    float values[151]{},output[150]{},reference_output[150]{},targets[150]{};
    sp_port reference[150]{};sp_data context{};context.sr=44100;
    for(unsigned i=0;i<150;++i){const auto&m=source_metadata::parameters[i];values[i]=std::min(std::max(m.initial,m.minimum),m.maximum);targets[i]=reference_output[i]=values[i];}
    for(unsigned i=0;i<150;++i)if(source_metadata::parameters[i].selected){sp_port_init(&context,&reference[i],values[i]);reference[i].htime=values[144];}
    Ports ports;CHECK(ports.init(output,150,values,150,44100));
    CHECK(ports.port(0).state.prvhtim==-100.f);CHECK(ports.port(0).state.c1==0);
    for(unsigned frame=0;frame<6;++frame){
        if(frame==1){const float current=output[144];CHECK(ports.set_resolved_target(144,.8f));targets[144]=.8f;const float ht=std::min(std::max(current,.000001f),.99f);for(unsigned i=0;i<150;++i)if(source_metadata::parameters[i].selected)reference[i].htime=ht;CHECK(equal(ports.port(144).state.htime,ht));}
        if(frame==2){CHECK(ports.set_resolved_target(3,12));reference_output[3]=12;CHECK(output[3]==12);}
        CHECK(ports.tick());
        for(unsigned i=0;i<150;++i){if(source_metadata::parameters[i].selected)sp_port_compute(&context,&reference[i],&targets[i],&reference_output[i]);CHECK(equal(output[i],reference_output[i]));const auto view=ports.port(i);if(source_metadata::parameters[i].selected){CHECK(view.valid);CHECK(equal(view.state.yt1,reference[i].yt1));CHECK(equal(view.state.c1,reference[i].c1));CHECK(equal(view.state.c2,reference[i].c2));CHECK(equal(view.state.prvhtim,reference[i].prvhtim));CHECK(equal(view.state.sr,reference[i].sr));CHECK(equal(view.state.onedsr,reference[i].onedsr));}}
    }
    unsigned char before[sizeof ports];std::memcpy(before,&ports,sizeof ports);
    CHECK(!ports.update_half_time(0));CHECK(std::memcmp(before,&ports,sizeof ports)==0);
    CHECK(!ports.init(values+1,150,values,150,44100));CHECK(std::memcmp(before,&ports,sizeof ports)==0);
    std::printf("{\"projection_smoke_frames\":6,\"output_values\":900,\"checks\":%d,\"compact_host_bytes\":%zu}\n",checks,sizeof(Ports));
}
