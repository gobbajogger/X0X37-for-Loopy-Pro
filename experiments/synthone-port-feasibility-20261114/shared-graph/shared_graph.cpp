// Ownership/scheduling adaptation only. Pinned run/LFO arithmetic is unchanged.
#include "shared_graph.hpp"
#include "../probe/wave_frequencies.h"
#include <cmath>
#include <cstring>

namespace synthone_shared {
namespace {
bool valid_payload(const float *samples, size_t count, float bound) {
    if (!samples || count != table_samples) return false;
    for (size_t i = 0; i < count; ++i)
        if (!std::isfinite(samples[i]) || samples[i] < -bound || samples[i] > bound)
            return false;
    return samples[table_size] == 0.0f && !std::signbit(samples[table_size]);
}
bool valid_tables(const Tables &tables, int rate) {
    if (rate != 44100 && rate != 48000 && rate != 96000) return false;
    for (size_t band = 0; band < bands; ++band) {
        if (tables.frequency_band[band] != wave_frequencies[band]) return false;
        for (size_t shape = 0; shape < shapes; ++shape) {
            const auto &row = tables.morph[band * shapes + shape];
            if (row.band != band || static_cast<size_t>(row.shape) != shape ||
                !valid_payload(row.samples, row.count, 2.0f)) return false;
        }
    }
    return valid_payload(tables.sine, tables.sine_count, 1.0f);
}
void bind_descriptor(sp_data *context, sp_ftbl &descriptor, const float *samples) {
    descriptor = {};
    sp_ftbl_init(context, &descriptor, table_size);
    // Soundpipe's mutable ABI; traced init/compute closure only reads payloads.
    descriptor.tbl = const_cast<float *>(samples);
    descriptor.del = 0;
}
} // namespace

bool Graph::init(const Tables &tables, int rate) {
    if (!valid_tables(tables, rate)) return false;
    context_.sr = rate;
    for (size_t i = 0; i < morph_tables; ++i) {
        bind_descriptor(&context_, descriptors_[i], tables.morph[i].samples);
        kernel_.ft_array[i] = &descriptors_[i];
    }
    bind_descriptor(&context_, descriptors_[morph_tables], tables.sine);
    kernel_.sine = &descriptors_[morph_tables];
    std::memcpy(kernel_.ft_frequencyBand, tables.frequency_band,
                sizeof kernel_.ft_frequencyBand);
    initialized_ = true;
    return reset();
}

bool Graph::reset() {
    if (!initialized_) return false;
    const int rate = context_.sr;
    context_ = {};
    context_output_ = 0;
    context_.out = &context_output_;
    context_.sr = rate;
    context_.nchan = 1;
    context_.len = 5ul * static_cast<unsigned>(rate);
    std::memcpy(context_.filename, "test.wav", sizeof "test.wav");
    kernel_.sp = &context_;
    std::memcpy(kernel_.parameters, defaults, sizeof defaults);
    kernel_.lfo1_0_1 = kernel_.lfo2_0_1 = kernel_.lfo3_0_1 = 0;
    kernel_.lfo1_1_0 = kernel_.lfo2_1_0 = kernel_.lfo3_1_0 = 1;
    kernel_.lfo1Smooth = kernel_.lfo2Smooth = 0;
    kernel_.monoFrequencySmooth = 261.6255653006f;
    phasor1_ = {}; phasor2_ = {}; port1_ = {}; port2_ = {};
    kernel_.lfo1Phasor = &phasor1_; kernel_.lfo2Phasor = &phasor2_;
    kernel_.lfo1Port = &port1_; kernel_.lfo2Port = &port2_;
    sp_phasor_init(&context_, kernel_.lfo1Phasor, 0);
    sp_phasor_init(&context_, kernel_.lfo2Phasor, 0);
    sp_port_init(&context_, kernel_.lfo1Port, .0053125f);
    sp_port_init(&context_, kernel_.lfo2Port, .0053125f);
    for (unsigned i = 0; i < voice_count; ++i) {
        objects_[i] = {};
        notes_[i] = {};
        auto &v = notes_[i]; auto &o = objects_[i];
        v.kernel = &kernel_;
        v.adsr = &o.adsr; v.fadsr = &o.fadsr;
        v.oscmorph1 = &o.morph1; v.oscmorph2 = &o.morph2;
        v.morphCrossFade = &o.morph_crossfade; v.filterCrossFade = &o.filter_crossfade;
        v.subOsc = &o.sub; v.fmOsc = &o.fm; v.noise = &o.noise;
        v.loPass = &o.lowpass; v.bandPass = &o.bandpass; v.hiPass = &o.highpass;
        // Original per-note init order/arguments, without any create calls.
        sp_adsr_init(&context_, v.adsr); sp_adsr_init(&context_, v.fadsr);
        sp_oscmorph2d_init(&context_, v.oscmorph1, kernel_.ft_array, 4, 13,
                         kernel_.ft_frequencyBand, 0);
        v.oscmorph1->freq = 0; v.oscmorph1->amp = 0; v.oscmorph1->wtpos = 0;
        v.oscmorph1->enableBandlimit = kernel_.parameters[oscBandlimitEnable];
        v.oscmorph1->bandlimitIndexOverride = -1;
        sp_oscmorph2d_init(&context_, v.oscmorph2, kernel_.ft_array, 4, 13,
                         kernel_.ft_frequencyBand, 0);
        v.oscmorph2->freq = 0; v.oscmorph2->amp = 0; v.oscmorph2->wtpos = 0;
        v.oscmorph2->enableBandlimit = kernel_.parameters[oscBandlimitEnable];
        v.oscmorph2->bandlimitIndexOverride = -1;
        sp_crossfade_init(&context_, v.morphCrossFade);
        sp_crossfade_init(&context_, v.filterCrossFade);
        sp_osc_init(&context_, v.subOsc, kernel_.sine, 0);
        sp_fosc_init(&context_, v.fmOsc, kernel_.sine);
        sp_noise_init(&context_, v.noise);
        sp_moogladder_init(&context_, v.loPass);
        sp_butbp_init(&context_, v.bandPass); sp_buthp_init(&context_, v.hiPass);
        v.clear();
    }
    active_count_ = 0;
    return true;
}

void Graph::teardown() {
    initialized_ = false;
    active_count_ = 0;
    for (unsigned i = 0; i < voice_count; ++i) { objects_[i] = {}; notes_[i] = {}; }
    phasor1_ = {}; phasor2_ = {}; port1_ = {}; port2_ = {};
    kernel_ = {}; context_ = {}; context_output_ = 0;
    for (auto &descriptor : descriptors_) descriptor = {};
}
bool Graph::set_parameter(S1Parameter parameter, float value) {
    const int p = static_cast<int>(parameter);
    if (!initialized_ || p < 0 || p >= S1ParameterCount || !std::isfinite(value)) return false;
    kernel_.parameters[p] = kernel_.clampedValue(parameter, value);
    return true;
}
bool Graph::set_mono_frequency(float frequency) {
    if (!initialized_ || !std::isfinite(frequency) || frequency < 0 || frequency > context_.sr * .5f)
        return false;
    kernel_.monoFrequencySmooth = frequency;
    return true;
}
bool Graph::set_active_count(unsigned count) {
    if (!initialized_ || count > voice_count) return false;
    active_count_ = count;
    return true;
}
bool Graph::note_on(unsigned index, int note, int velocity, float frequency) {
    if (!initialized_ || index >= voice_count || note < 0 || note > 127 ||
        velocity < 0 || velocity > 127 || !std::isfinite(frequency) ||
        frequency < 0 || frequency > context_.sr * .5f) return false;
    notes_[index].startNoteHelper(note, velocity, frequency);
    return true;
}
bool Graph::release(unsigned index) {
    if (!initialized_ || index >= voice_count) return false;
    notes_[index].internalGate = 0;
    notes_[index].stage = S1NoteState::stageRelease;
    return true;
}
bool Graph::clear(unsigned index) {
    if (!initialized_ || index >= voice_count) return false;
    notes_[index].clear();
    return true;
}
bool Graph::process(float *left, float *right) {
    if (!initialized_ || !left || !right) return false;
    kernel_.processLfo();
    for (unsigned i = 0; i < active_count_; ++i) notes_[i].run(0, left, right);
    return true;
}
} // namespace synthone_shared
