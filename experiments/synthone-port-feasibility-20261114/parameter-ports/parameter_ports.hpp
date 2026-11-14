// Caller-owned master parameter-port representation; see ../notices/.
#pragma once
#include "generated/parameter_map.hpp"
extern "C" {
#include "soundpipe.h"
}

namespace synthone_ports {
struct TargetView { bool valid; float value; };
struct PortView { bool valid; sp_port state; };

class Ports {
public:
    Ports() = default;
    ~Ports() { teardown(); }
    Ports(const Ports &) = delete;
    Ports &operator=(const Ports &) = delete;
    Ports(Ports &&) = delete;
    Ports &operator=(Ports &&) = delete;
    // Exactly 150 caller-owned outputs. Inputs may equal outputs, not partially
    // overlap; neither may overlap this object. Inputs are already resolved.
    bool init(float *outputs, size_t output_count, const float *values,
              size_t value_count, int sample_rate);
    // Source setupParameterTree/restoreValues boundary, retaining prior c1/c2.
    bool restore(const float *values, size_t value_count, int sample_rate);
    // Source target storage plus index144's current-output half-time side effect.
    bool set_resolved_target(unsigned parameter, float value);
    // Low-level eligible uniform update; accepts finite [1e-6,0.99] only.
    bool update_half_time(float half_time);
    bool tick();
    TargetView target(unsigned parameter) const;
    PortView port(unsigned parameter) const;
    void teardown();
    bool initialized() const { return outputs_ != nullptr; }
private:
    struct Selected { float target = 0, yt1 = 0; } selected_[selected_count]{};
    struct Common {
        float htime = 0, c1 = 0, c2 = 0, prvhtim = 0, sr = 0, onedsr = 0;
    } common_{};
    float *outputs_ = nullptr;
    bool valid_values(const float *values, size_t count, float *outputs, size_t output_count) const;
    void apply_restore(const float *values, int sample_rate);
};
} // namespace synthone_ports
