// Original parameter-port storage for the source oracle only. See MIT notices.
#pragma once
#include "../scheduler.hpp"
#include <cstring>

namespace s1scheduler {
struct S1Info { bool usePortamento = false; sp_port *portamento = nullptr; float portamentoTarget = 0; };
class OracleState : public State {
public:
    S1Info s1p[150]{};
    void *delayL = nullptr, *delayR = nullptr, *delayRR = nullptr, *delayFillIn = nullptr;
    using Trace = void (*)(const OracleState &,unsigned,unsigned,void *);
    Trace trace_fn = nullptr;
    void *trace_context = nullptr;
    void trace(unsigned phase,unsigned frame) const {
        if(trace_fn) trace_fn(*this,phase,frame,trace_context);
    }
    void _setSynthParameter(int parameter,float value) {
        value=clampedValue(static_cast<S1Parameter>(parameter),value);
        if(s1p[parameter].usePortamento) s1p[parameter].portamentoTarget=value;
        else parameters[parameter]=value;
        if(parameter==portamentoHalfTime)
            for(auto &p:s1p) if(p.usePortamento) p.portamento->htime=clampedValue(portamentoHalfTime,parameters[parameter]);
    }
protected:
    OracleState() = default;
    bool init_oracle(const synthone_shared::Tables &tables,int rate,const float *values,
                     size_t count,const float *tuning,size_t tuning_count,
                     const bool (&original_flags)[150],InstanceSpan self) {
        if(!valid_init(tables,rate,values,count,tuning,tuning_count,self)) return false;
        init_graph(tables,rate,tuning);
        for(unsigned i=0;i<150;++i) {
            reference_ports_[i]={}; s1p[i]={};
            s1p[i].usePortamento=original_flags[i];
            s1p[i].portamento=&reference_ports_[i];
        }
        // Already validated; do not dispatch a hidden non-virtual base restore.
        return restore_oracle(values,count,self);
    }
    bool restore_oracle(const float *values,size_t count,InstanceSpan self) {
        if(!valid_restore(values,count,self)) return false;
        float saved[150]; std::memcpy(saved,values,sizeof saved);
        std::memcpy(parameters,saved,sizeof saved);
        for(unsigned i=0;i<150;++i) if(s1p[i].usePortamento) {
            s1p[i].portamentoTarget=saved[i];
            // Actual port init retains c1/c2, clears history, marks coefficients dirty.
            sp_port_init(sp,s1p[i].portamento,saved[i]);
            s1p[i].portamento->htime=saved[portamentoHalfTime];
        }
        restore_graph();
        return true;
    }
    sp_port reference_ports_[150]{};
};
} // namespace s1scheduler
