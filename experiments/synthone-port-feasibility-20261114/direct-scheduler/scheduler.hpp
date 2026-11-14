// Caller-owned direct-event pre-FX Synth One scheduler. See README and notices.
#pragma once
#include "shared_graph.hpp"
#include "parameter_ports.hpp"
#include <array>
#include <cstddef>

namespace s1scheduler {
struct NoteNumber { int noteNumber, transpose, velocity; };
struct Held {
    std::array<NoteNumber,128> values{};
    int count = 0;
    NoteNumber &operator[](size_t i) { return values[i]; }
    const NoteNumber &operator[](size_t i) const { return values[i]; }
    void remove(size_t i);
    void insert(NoteNumber value, size_t i);
    void clear() { count = 0; }
};
// Deliberately only the sequencer reset(true), arp-off, empty-pending domain.
// No arp algorithm or timing is supplied by this adapter.
struct EmptySequencer {
    bool pending_empty = true;
    unsigned resets = 0;
    void reset(bool) { pending_empty = true; ++resets; }
    void process(float *, const Held &) const;
};
struct S1Info { bool usePortamento = false; sp_port *portamento = nullptr; float portamentoTarget = 0; };
class State : public S1DSPKernel {
public:
    State() = default;
    State(const State &) = delete;
    State &operator=(const State &) = delete;
    bool init(const synthone_shared::Tables &, int rate, const float *values,
              size_t value_count, const float *tuning, size_t tuning_count);
    // Source restoreValues note/control boundary only; absent FX/rates excluded.
    bool restore(const float *values, size_t count);
    void initializeNoteStates();
    double tuningTableNoteToHz(int note) const;
    void heldNotesDidChange() {}
    void playingNotesDidChange() {}
    void beatCounterDidChange() {}
    void _setSynthParameter(int parameter, float value); // Oracle control adapter.
    void trace(unsigned phase, unsigned frame) const;
    using Trace = void (*)(const State &,unsigned,unsigned,void *);
    Trace trace_fn = nullptr;
    void *trace_context = nullptr;

    std::array<S1NoteState,6> poly{};
    std::array<S1NoteState,6> *noteStates = &poly;
    S1NoteState mono{};
    S1NoteState *monoNote = &mono;
    int polyphony = 6, playingNoteStatesIndex = 0;
    Held heldNoteNumbers{}, heldNoteNumbersAE{};
    float monoFrequency = 440.f * exp2((60.f - 69.f)/12.f);
    float previousProcessMonoPolyStatus = 0;
    sp_port *monoFrequencyPort = &mono_port_;
    const float *tuningTable = nullptr;
    bool resetted = false, initializedNoteStates = false;
    double processSampleCounter = 0;
    S1Info s1p[150]{};
    EmptySequencer sequencer{};
    void *delayL = nullptr, *delayR = nullptr, *delayRR = nullptr, *delayFillIn = nullptr;
    synthone_ports::Ports compactPorts;
    bool initialized() const { return initialized_; }
    bool valid_parameter(unsigned, float) const;
    bool valid_event(int pitch, int velocity, float frequency) const;
    bool valid_output(float *, float *, unsigned, unsigned) const;
protected:
    struct NoteObjects {
        sp_adsr adsr{}, fadsr{};
        sp_oscmorph2d morph1{}, morph2{};
        sp_crossfade morph_crossfade{}, filter_crossfade{};
        sp_osc sub{}; sp_fosc fm{}; sp_noise noise{};
        sp_moogladder lowpass{}; sp_butbp bandpass{}; sp_buthp highpass{};
    } objects_[7]{};
    sp_phasor phasor1_{}, phasor2_{};
    sp_port port1_{}, port2_{}, mono_port_{}, reference_ports_[150]{};
    sp_data context_{};
    float context_output_ = 0;
    sp_ftbl descriptors_[53]{};
    bool initialized_ = false;
};

class Scheduler : public State {
public:
    bool set_target(unsigned parameter, float resolved_value);
    bool start(int pitch, int velocity, float ignored_frequency = 1.f);
    bool on(int pitch, int velocity, float explicit_frequency);
    bool on(int pitch, int velocity);
    bool stop(int pitch);
    bool off(int pitch);
    bool stop_all();
    bool reset();
    bool reset_dsp();
    bool reset_sequencer();
    bool process(unsigned frames, unsigned offset, float *left, float *right);
private:
    void on_valid(int,int,float);
    void off_valid(int);
};
} // namespace s1scheduler
