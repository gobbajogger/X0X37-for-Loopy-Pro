#include "scheduler.hpp"
#include "wave_frequencies.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <limits>

namespace s1scheduler {
void Held::remove(size_t i) {
    assert(i < static_cast<size_t>(count));
    for (size_t n=i+1;n<static_cast<size_t>(count);++n) values[n-1]=values[n];
    --count;
}
void Held::insert(NoteNumber value,size_t i) {
    assert(count<128 && i<=static_cast<size_t>(count));
    for(size_t n=count;n>i;--n) values[n]=values[n-1];
    values[i]=value; ++count;
}
void EmptySequencer::process(float *p,const Held &) const {
    assert(p[arpIsOn]==0.f && pending_empty);
}
namespace {
bool overlap(const void *a,size_t an,const void *b,size_t bn) {
    const auto x=reinterpret_cast<uintptr_t>(a), y=reinterpret_cast<uintptr_t>(b);
    if(x>std::numeric_limits<uintptr_t>::max()-an || y>std::numeric_limits<uintptr_t>::max()-bn) return true;
    return x<y+bn && y<x+an;
}
bool payload(const float *p,size_t n,float bound,const State *self) {
    if(!p || n!=4097 || overlap(p,n*sizeof(float),self,sizeof(State))) return false;
    for(size_t i=0;i<n;++i) if(!std::isfinite(p[i]) || p[i]<-bound || p[i]>bound) return false;
    return p[4096]==0.f && !std::signbit(p[4096]);
}
bool values_valid(const float *p,size_t n) {
    if(!p || n!=150 || p[arpIsOn]!=0) return false;
    for(unsigned i=0;i<150;++i)
        if(!std::isfinite(p[i]) || p[i]<minimums[i] || p[i]>maximums[i]) return false;
    return true;
}
}
bool State::init(const synthone_shared::Tables &tables,int rate,const float *values,
                 size_t count,const float *tuning,size_t tuning_count) {
    if((rate!=44100 && rate!=48000 && rate!=96000) || !values_valid(values,count) ||
       overlap(values,150*sizeof(float),this,sizeof *this) ||
       !tuning || tuning_count!=128 || overlap(tuning,128*sizeof(float),this,sizeof *this)) return false;
    for(unsigned i=0;i<128;++i)
        if(!std::isfinite(tuning[i]) || tuning[i]<=0 || tuning[i]>rate*.5f) return false;
    for(unsigned i=0;i<52;++i) {
        const auto &r=tables.morph[i];
        if(r.band!=i/4 || static_cast<unsigned>(r.shape)!=i%4 ||
           !payload(r.samples,r.count,2.f,this)) return false;
    }
    for(unsigned i=0;i<13;++i) if(tables.frequency_band[i]!=wave_frequencies[i]) return false;
    if(!payload(tables.sine,tables.sine_count,1.f,this)) return false;
    // Validate everything before replacing a live instance.
    compactPorts.teardown();
    context_={}; context_output_=0; context_.out=&context_output_;
    context_.sr=rate; context_.nchan=1; context_.len=5ul*static_cast<unsigned>(rate);
    std::memcpy(context_.filename,"test.wav",sizeof "test.wav"); sp=&context_;
    tuningTable=tuning;
    for(unsigned i=0;i<53;++i) {
        auto &d=descriptors_[i]; d={}; sp_ftbl_init(sp,&d,4096);
        d.tbl=const_cast<float *>(i<52?tables.morph[i].samples:tables.sine); d.del=0;
        if(i<52) ft_array[i]=&d; else sine=&d;
    }
    std::memcpy(ft_frequencyBand,tables.frequency_band,sizeof ft_frequencyBand);
    lfo1_0_1=lfo2_0_1=lfo3_0_1=0; lfo1_1_0=lfo2_1_0=lfo3_1_0=0;
    lfo1Smooth=lfo2Smooth=0; monoFrequencySmooth=261.6255653006f;
    monoFrequency=440.f*exp2((60.f-69.f)/12.f);
    phasor1_={}; phasor2_={}; port1_={}; port2_={}; mono_port_={};
    lfo1Phasor=&phasor1_; lfo2Phasor=&phasor2_; lfo1Port=&port1_; lfo2Port=&port2_;
    monoFrequencyPort=&mono_port_;
    sp_phasor_init(sp,lfo1Phasor,0); sp_phasor_init(sp,lfo2Phasor,0);
    sp_port_init(sp,lfo1Port,.0053125f); sp_port_init(sp,lfo2Port,.0053125f);
    sp_port_init(sp,monoFrequencyPort,.05f);
    poly={}; mono={}; noteStates=&poly; monoNote=&mono;
    playingNoteStatesIndex=0; heldNoteNumbers={}; heldNoteNumbersAE={};
    resetted=false; processSampleCounter=0; sequencer={};
    for(unsigned i=0;i<150;++i) {
        reference_ports_[i]={}; s1p[i]={};
        s1p[i].usePortamento=synthone_ports::selected_slots[i]!=synthone_ports::unsmoothed_slot;
        s1p[i].portamento=&reference_ports_[i];
    }
    initialized_=true;
    compactPorts.init(parameters,150,values,150,rate);
    return restore(values,count);
}
bool State::restore(const float *values,size_t count) {
    if(!initialized_ || !values_valid(values,count) ||
       (values!=parameters && overlap(values,150*sizeof(float),this,sizeof *this))) return false;
    // Snapshot accommodates parameters==values without a hidden partial overwrite.
    float saved[150]; std::memcpy(saved,values,sizeof saved);
    if(!compactPorts.restore(saved,150,sp->sr)) return false;
    for(unsigned i=0;i<150;++i) {
        if(s1p[i].usePortamento) {
            s1p[i].portamentoTarget=saved[i];
            sp_port_init(sp,s1p[i].portamento,saved[i]);
            s1p[i].portamento->htime=saved[portamentoHalfTime];
        }
    }
    previousProcessMonoPolyStatus=parameters[isMono];
    initializedNoteStates=false;
    // Original restore's sequencer.init creates 1024 pending entries. The caller
    // must explicitly establish reset(true) before this direct-event domain.
    sequencer.reset(true);
    return true;
}
void State::initializeNoteStates() {
    if(initializedNoteStates) return;
    initializedNoteStates=true;
    for(unsigned i=0;i<7;++i) {
        auto &v=i<6?poly[i]:mono; auto &o=objects_[i]; o={};
        v.kernel=this;
        v.adsr=&o.adsr; v.fadsr=&o.fadsr; v.oscmorph1=&o.morph1; v.oscmorph2=&o.morph2;
        v.morphCrossFade=&o.morph_crossfade; v.filterCrossFade=&o.filter_crossfade;
        v.subOsc=&o.sub; v.fmOsc=&o.fm; v.noise=&o.noise;
        v.loPass=&o.lowpass; v.bandPass=&o.bandpass; v.hiPass=&o.highpass;
        sp_adsr_init(sp,v.adsr); sp_adsr_init(sp,v.fadsr);
        sp_oscmorph2d_init(sp,v.oscmorph1,ft_array,4,13,ft_frequencyBand,0);
        v.oscmorph1->freq=0; v.oscmorph1->amp=0; v.oscmorph1->wtpos=0;
        v.oscmorph1->enableBandlimit=parameters[oscBandlimitEnable]; v.oscmorph1->bandlimitIndexOverride=-1;
        sp_oscmorph2d_init(sp,v.oscmorph2,ft_array,4,13,ft_frequencyBand,0);
        v.oscmorph2->freq=0; v.oscmorph2->amp=0; v.oscmorph2->wtpos=0;
        v.oscmorph2->enableBandlimit=parameters[oscBandlimitEnable]; v.oscmorph2->bandlimitIndexOverride=-1;
        sp_crossfade_init(sp,v.morphCrossFade); sp_crossfade_init(sp,v.filterCrossFade);
        sp_osc_init(sp,v.subOsc,sine,0); sp_fosc_init(sp,v.fmOsc,sine);
        sp_noise_init(sp,v.noise); sp_moogladder_init(sp,v.loPass);
        sp_butbp_init(sp,v.bandPass); sp_buthp_init(sp,v.hiPass);
        // Source setup initializes these four fields, not clear()'s amp.
        v.stage=S1NoteState::stageOff; v.internalGate=0; v.rootNoteNumber=-1; v.transpose=0;
    }
}
double State::tuningTableNoteToHz(int note) const { return tuningTable[clamp(note,0,127)]; }
void State::_setSynthParameter(int parameter,float value) {
    value=clampedValue(static_cast<S1Parameter>(parameter),value);
    if(s1p[parameter].usePortamento) s1p[parameter].portamentoTarget=value;
    else parameters[parameter]=value;
    if(parameter==portamentoHalfTime)
        for(auto &p:s1p) if(p.usePortamento) p.portamento->htime=clampedValue(portamentoHalfTime,parameters[parameter]);
}
void State::trace(unsigned phase,unsigned frame) const { if(trace_fn) trace_fn(*this,phase,frame,trace_context); }
bool State::valid_parameter(unsigned p,float v) const {
    return initialized_ && p<150 && std::isfinite(v) && (p!=arpIsOn || v==0.f);
}
bool State::valid_event(int pitch,int velocity,float frequency) const {
    return initialized_ && parameters[arpIsOn]==0 && sequencer.pending_empty && pitch>=0 && pitch<128 &&
        velocity>=0 && velocity<=127 && std::isfinite(frequency) && frequency>=0 && frequency<=sp->sr*.5f;
}
bool State::valid_output(float *l,float *r,unsigned n,unsigned offset) const {
    if(!initialized_ || !l || !r || n>65536 || offset>65536-n || parameters[arpIsOn]!=0 || !sequencer.pending_empty) return false;
    const size_t bytes=(static_cast<size_t>(n)+offset)*sizeof(float);
    if(overlap(l,bytes,r,bytes) || overlap(l,bytes,this,sizeof *this) || overlap(r,bytes,this,sizeof *this) ||
       overlap(l,bytes,tuningTable,128*sizeof(float)) || overlap(r,bytes,tuningTable,128*sizeof(float))) return false;
    for(const auto &d:descriptors_) if(overlap(l,bytes,d.tbl,4097*sizeof(float)) || overlap(r,bytes,d.tbl,4097*sizeof(float))) return false;
    return true;
}
bool Scheduler::set_target(unsigned p,float v) {
    return valid_parameter(p,v) && compactPorts.set_resolved_target(p,clampedValue(static_cast<S1Parameter>(p),v));
}
bool Scheduler::start(int pitch,int velocity,float ignored_frequency) {
    if(!valid_event(pitch,velocity,ignored_frequency)) return false;
    for(int i=0;i<heldNoteNumbers.count;++i) if(heldNoteNumbers[i].noteNumber==pitch) { heldNoteNumbers.remove(i); break; }
    heldNoteNumbers.insert({pitch,static_cast<int>(parameters[transpose]),velocity},0);
    heldNoteNumbersAE=heldNoteNumbers;
    on_valid(pitch,velocity,tuningTableNoteToHz(pitch+static_cast<int>(parameters[transpose])));
    return true;
}
bool Scheduler::on(int pitch,int velocity) {
    if(!valid_event(pitch,velocity,0)) return false;
    on_valid(pitch,velocity,tuningTableNoteToHz(pitch+static_cast<int>(parameters[transpose]))); return true;
}
bool Scheduler::on(int pitch,int velocity,float frequency) {
    if(!valid_event(pitch,velocity,frequency)) return false;
    on_valid(pitch,velocity,frequency); return true;
}
void Scheduler::on_valid(int pitch,int velocity,float frequency) {
    initializeNoteStates();
    if(parameters[isMono]>0) {
        monoFrequency=frequency;
        if(parameters[monoIsLegato]==0) {
            mono.internalGate=0; mono.stage=S1NoteState::stageRelease;
            sp_adsr_compute(sp,mono.adsr,&mono.internalGate,&mono.amp);
            sp_adsr_compute(sp,mono.fadsr,&mono.internalGate,&mono.filter);
        }
        mono.startNoteHelper(pitch,velocity,frequency);
    } else {
        int selected=-1;
        for(int i=0;i<6;++i) if(poly[i].rootNoteNumber==pitch) { selected=i; break; }
        if(selected<0) for(int i=0;i<6;++i) {
            int index=(playingNoteStatesIndex+i)%6;
            if(poly[index].rootNoteNumber==-1) { selected=index; break; }
        }
        playingNoteStatesIndex=selected<0?(playingNoteStatesIndex+1)%6:selected;
        poly[playingNoteStatesIndex].startNoteHelper(pitch,velocity,frequency);
    }
    heldNotesDidChange();
}
bool Scheduler::stop(int pitch) {
    if(!valid_event(pitch,0,0)) return false;
    for(int i=0;i<heldNoteNumbers.count;++i) if(heldNoteNumbers[i].noteNumber==pitch) {heldNoteNumbers.remove(i);break;}
    heldNoteNumbersAE=heldNoteNumbers; off_valid(pitch); return true;
}
bool Scheduler::off(int pitch) { if(!valid_event(pitch,0,0)) return false; off_valid(pitch); return true; }
void Scheduler::off_valid(int pitch) {
    initializeNoteStates();
    if(parameters[isMono]>0) {
        if(heldNoteNumbersAE.count==0) {
            if(mono.stage!=S1NoteState::stageOff) {mono.stage=S1NoteState::stageRelease;mono.internalGate=0;}
        } else {
            const auto &head=heldNoteNumbersAE[0];
            mono.rootNoteNumber=head.noteNumber; mono.transpose=static_cast<int>(parameters[transpose]);
            mono.velocity=head.velocity; mono.amp=pow2(head.velocity/127.f);
            mono.oscmorph1->amp=mono.oscmorph2->amp=mono.subOsc->amp=mono.fmOsc->amp=mono.amp;
            monoFrequency=tuningTableNoteToHz(head.noteNumber+static_cast<int>(parameters[transpose]));
            mono.oscmorph1->freq=mono.oscmorph2->freq=mono.subOsc->freq=mono.fmOsc->freq=monoFrequency;
            // The original fallback intentionally does not update noise->amp.
            if(parameters[monoIsLegato]==0) {
                mono.internalGate=0; mono.stage=S1NoteState::stageRelease;
                sp_adsr_compute(sp,mono.adsr,&mono.internalGate,&mono.amp);
                sp_adsr_compute(sp,mono.fadsr,&mono.internalGate,&mono.filter);
            }
            mono.stage=S1NoteState::stageOn; mono.internalGate=1;
        }
    } else for(auto &note:poly) if(note.rootNoteNumber==pitch) {
        if(note.stage!=S1NoteState::stageOff) {
            note.stage=S1NoteState::stageRelease; note.internalGate=0;
            sp_adsr_compute(sp,note.adsr,&note.internalGate,&note.amp);
            sp_adsr_compute(sp,note.fadsr,&note.internalGate,&note.filter);
        }
        break;
    }
    heldNotesDidChange();
}
bool Scheduler::stop_all() {
    if(!initialized_) return false;
    heldNoteNumbers.clear(); heldNoteNumbersAE=heldNoteNumbers;
    if(parameters[isMono]>0) return stop(60);
    for(int i=0;i<128;++i) stop(i);
    return true;
}
bool Scheduler::reset() {
    if(!initialized_) return false;
    for(auto &note:poly) note.clear(); mono.clear(); resetted=true; return true;
}
bool Scheduler::reset_dsp() {
    if(!initialized_) return false;
    heldNoteNumbers.clear(); heldNoteNumbersAE=heldNoteNumbers; sequencer.reset(true);
    set_target(arpIsOn,0); mono.clear(); for(auto &note:poly) note.clear(); return true;
}
bool Scheduler::reset_sequencer() { if(!initialized_) return false; sequencer.reset(false); beatCounterDidChange(); return true; }
bool Scheduler::process(unsigned n,unsigned offset,float *l,float *r) {
    if(!valid_output(l,r,n,offset)) return false;
    initializeNoteStates();
    if(parameters[isMono]>0) {
        if(mono.stage==S1NoteState::stageRelease && mono.amp<.01f) mono.clear();
    } else for(auto &note:poly) if(note.stage==S1NoteState::stageRelease && note.amp<.01f) note.clear();
    processSampleCounter+=n;
    if(processSampleCounter>2048.) {playingNotesDidChange();processSampleCounter=0;}
    trace(0,0);
    for(unsigned i=0;i<n;++i) {
        l[offset+i]=r[offset+i]=0;
        compactPorts.tick();
        monoFrequencyPort->htime=parameters[glide];
        sp_port_compute(sp,monoFrequencyPort,&monoFrequency,&monoFrequencySmooth);
        if(parameters[isMono]!=previousProcessMonoPolyStatus) {
            previousProcessMonoPolyStatus=parameters[isMono]; reset(); sequencer.reset(true);
        }
        processLfo(); sequencer.process(parameters,heldNoteNumbersAE);
        if(parameters[isMono]>0) {
            if(mono.rootNoteNumber!=-1 && mono.stage!=S1NoteState::stageOff) mono.run(offset+i,l,r);
        } else for(auto &note:poly)
            if(note.rootNoteNumber!=-1 && note.stage!=S1NoteState::stageOff) note.run(offset+i,l,r);
        trace(1,i);
    }
    return true;
}
} // namespace s1scheduler
