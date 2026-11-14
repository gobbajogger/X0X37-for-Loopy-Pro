// Genuine pinned Synth One voice/LFO subset; ownership/scheduling prototype.
// Complete original MIT/module notices are retained in the parent experiment.
#pragma once
#include "../probe/portable_interface.hpp"

namespace synthone_shared {
constexpr size_t table_size = 4096;
constexpr size_t table_samples = table_size + 1;
constexpr size_t bands = 13;
constexpr size_t shapes = 4;
constexpr size_t morph_tables = bands * shapes;
constexpr unsigned voice_count = 6;
enum class Shape { triangle, square, pwm, saw };
struct Row {
    const float *samples;
    size_t count;
    unsigned band;
    Shape shape;
};
struct Tables {
    Row morph[morph_tables]; // Authentic rows overshoot one; bounded here to [-2,2].
    const float *sine;       // FM requires a separate normalized [-1,1] bound.
    size_t sine_count;
    float frequency_band[bands];
};
struct TestAccess;
class Graph {
public:
    Graph() = default;
    ~Graph() { teardown(); }
    Graph(const Graph &) = delete;
    Graph &operator=(const Graph &) = delete;
    Graph(Graph &&) = delete;
    Graph &operator=(Graph &&) = delete;
    bool init(const Tables &tables, int sample_rate);
    bool reset();
    void teardown();
    bool set_parameter(S1Parameter parameter, float value);
    bool set_mono_frequency(float frequency);
    bool set_active_count(unsigned count);
    bool note_on(unsigned index, int note, int velocity, float frequency);
    bool release(unsigned index);
    bool clear(unsigned index); // Source metadata/gate clear; does not reset DSP.
    // Explicit prefix scheduling: LFO once, then source notes [0,count) in order.
    // It deliberately omits upstream allocation, stage filtering and mono reset.
    bool process(float *left, float *right);
    bool initialized() const { return initialized_; }
    unsigned active_count() const { return active_count_; }
private:
    friend struct TestAccess;
    struct NoteObjects {
        sp_adsr adsr{}, fadsr{};
        sp_oscmorph2d morph1{}, morph2{};
        sp_crossfade morph_crossfade{}, filter_crossfade{};
        sp_osc sub{};
        sp_fosc fm{};
        sp_noise noise{};
        sp_moogladder lowpass{};
        sp_butbp bandpass{};
        sp_buthp highpass{};
    } objects_[voice_count]{};
    sp_phasor phasor1_{}, phasor2_{};
    sp_port port1_{}, port2_{};
    S1DSPKernel kernel_{};
    S1NoteState notes_[voice_count]{};
    sp_data context_{};
    float context_output_ = 0;
    sp_ftbl descriptors_[morph_tables + 1]{};
    unsigned active_count_ = 0;
    bool initialized_ = false;
};
} // namespace synthone_shared
