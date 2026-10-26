/* SPDX-License-Identifier: GPL-3.0-only */
/* Independently composed wobble instrument. X-MOD is experimental PM, CRUSH
 * is experimental saturation, filter is an original bounded SVF approximation.
 * None is asserted to reproduce an undisclosed Miami algorithm. */
#include "wobble.h"
#include "blep.h"
#include "../../firmware/src/dsp/fastmath.h"

static float wrap(float p) { return p - fm_floorf(p); }
static float exp2_bounded(float x)
{
    return x < 0 ? 1.0f / fm_exp2f(-x) : fm_exp2f(x);
}
static int finite_value(float x)
{
    fm_bits_t b; b.f = x;
    return (b.u & 0x7f800000u) != 0x7f800000u;
}
static float waveform(float phase, float step, int square)
{
    if (!square) return 4.0f * fm_fabsf(phase - 0.5f) - 1.0f;
    return (phase < 0.5f ? 1.0f : -1.0f)
        + wobble_blep(phase, step) - wobble_blep(wrap(phase + 0.5f), step);
}
static void update_pitch(wobble_t *s)
{
    float semitones = (float)s->note - 69.0f
        + 24.0f * (s->p[W_CARRIER_PITCH] - 0.5f)
        + 4.0f * (s->p[W_PITCH_BEND] - 0.5f);
    s->target_hz = fm_minf(440.0f * exp2_bounded(semitones / 12.0f),
                          s->sample_rate * 0.12f);
}
#define WOBBLE_SPLIT_LOG_RATIO 2.05491958f
float wobble_bass_split_hz(float norm)
{
    norm = fm_clampf(norm, 0.0f, 1.0f);
    return 60.0f * exp2_bounded(norm * WOBBLE_SPLIT_LOG_RATIO);
}
float wobble_bass_split_alpha(float norm)
{
    float hz = wobble_bass_split_hz(norm);
    return fm_clampf((250.0f - hz) / 190.0f, 0.0f, 1.0f);
}
static float svf_lowpass(float in, float f, float *low, float *band)
{
    float high = in - *low - 1.41421356f * *band;
    *band = fm_flush(*band + f * high);
    *low = fm_flush(*low + f * *band);
    return *low;
}
static void update_rates(wobble_t *s)
{
    static const float rates[7] = {1, 2, 3, 4, 6, 8, 12};
    unsigned rate = (unsigned)(s->p[W_WOBBLE_RATE] * 6.0f + 0.5f);
    s->lfo_step = rates[rate] / s->sample_rate;
    s->glide_step = s->p[W_GLIDE] == 0.0f ? 1.0f
        : 1.0f - fm_expf(-1.0f / (s->sample_rate
                * (0.002f + 0.998f * s->p[W_GLIDE] * s->p[W_GLIDE])));
}
/* Map 0..1 pot to seconds (square law). Attack/Release defaults match legacy AR. */
static float env_seconds(float pot, float min_s, float max_s)
{
    float t = pot * pot;
    return min_s + t * (max_s - min_s);
}
static void update_amp_env(wobble_t *s)
{
    float atk_s = env_seconds(s->p[W_ATTACK], 0.002f, 2.0f);
    float dec_s = env_seconds(s->p[W_DECAY], 0.0001f, 2.5f);
    float rel_s = env_seconds(s->p[W_RELEASE], 0.030f, 3.0f);
    s->sustain_level = fm_clampf(s->p[W_SUSTAIN], 0.0f, 1.0f);
    s->attack_step = 1.0f / (atk_s * s->sample_rate);
    float span = 1.0f - s->sustain_level;
    s->decay_step = span > 1e-6f ? span / (dec_s * s->sample_rate) : 1.0f;
    s->release_step = 1.0f / (rel_s * s->sample_rate);
}
void wobble_reset(wobble_t *s)
{
    s->carrier_phase = s->mod_phase = s->sub_phase = s->lfo_phase = 0.0f;
    s->hz = s->target_hz = 0.0f;
    s->envelope = s->velocity = 0.0f;
    s->env_stage = 0;
    s->filter_low = s->filter_band = s->stereo_memory = 0.0f;
    s->split_low = s->split_band = s->split2_low = s->split2_band = 0.0f;
    s->active_samples = 0;
    s->note = 0; s->gate = s->active = s->deferred_off = 0;
}
void wobble_init(wobble_t *s, float sample_rate)
{
    static const float defaults[W_PARAMETER_COUNT] = {
        0,.7f,0,.5f,0,0,0,.5f,0,.6f,0,0,0,0,.5f,0,.5f,0,
        0,0,1,0, /* Attack Decay Sustain Release — AU-like at init */
        0.285f /* Bass Split ~90 Hz */
    };
    s->sample_rate = finite_value(sample_rate)
        ? fm_clampf(sample_rate, 8000.0f, 192000.0f) : 48000.0f;
    for (unsigned i = 0; i < W_PARAMETER_COUNT; ++i) s->p[i] = defaults[i];
    update_rates(s);
    update_amp_env(s);
    wobble_reset(s);
}
int wobble_set(wobble_t *s, unsigned parameter, float value)
{
    if (parameter >= W_PARAMETER_COUNT || !finite_value(value)) return 0;
    s->p[parameter] = fm_clampf(value, 0, 1);
    if (parameter == W_CARRIER_PITCH || parameter == W_PITCH_BEND) update_pitch(s);
    if (parameter == W_GLIDE || parameter == W_WOBBLE_RATE) update_rates(s);
    if (parameter >= W_ATTACK && parameter <= W_RELEASE) update_amp_env(s);
    if (parameter == W_DAMPER && value < .5f && s->deferred_off) {
        s->gate = s->deferred_off = 0;
    }
    return 1;
}
void wobble_note_on(wobble_t *s, uint8_t note, uint8_t velocity)
{
    if (!velocity) { wobble_note_off(s, note); return; }
    s->note = note > 127 ? 127 : note;
    update_pitch(s);
    if (!s->active || s->p[W_GLIDE] == 0.0f) s->hz = s->target_hz;
    s->velocity = (float)(velocity > 127 ? 127 : velocity) / 127.0f;
    s->gate = s->active = 1; s->deferred_off = 0;
    s->envelope = 0.0f;
    s->env_stage = 0;
}
void wobble_note_off(wobble_t *s, uint8_t note)
{
    if (note != s->note) return;
    if (s->p[W_DAMPER] >= .5f) s->deferred_off = 1;
    else s->gate = 0;
}
void wobble_all_notes_off(wobble_t *s) { s->gate = s->deferred_off = 0; }
void wobble_render(wobble_t *s, float *left, float *right, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        if (!s->active) { left[i] = right[i] = 0; continue; }
        if (s->gate) {
            if (s->env_stage == 0) {
                s->envelope = fm_minf(1.0f, s->envelope + s->attack_step);
                if (s->envelope >= 1.0f) {
                    s->envelope = 1.0f;
                    s->env_stage = s->sustain_level < 0.999f ? 1 : 2;
                }
            } else if (s->env_stage == 1) {
                s->envelope = fm_maxf(s->sustain_level, s->envelope - s->decay_step);
                if (s->envelope <= s->sustain_level + 1e-6f) {
                    s->envelope = s->sustain_level;
                    s->env_stage = 2;
                }
            }
        } else if (s->env_stage != 3) {
            s->env_stage = 3;
        }
        if (s->env_stage == 3)
            s->envelope = fm_maxf(0.0f, s->envelope - s->release_step);
        if (s->envelope == 0 && !s->gate) {
            s->active = 0;
            s->filter_low = s->filter_band = s->stereo_memory = 0;
            s->split_low = s->split_band = s->split2_low = s->split2_band = 0;
            left[i] = right[i] = 0;
            continue;
        }
        s->hz += (s->target_hz - s->hz) * s->glide_step;
        float step = fm_clampf(s->hz / s->sample_rate, 1e-6f, .12f);
        float mod_step = fm_clampf(step * .5f
            * exp2_bounded(10.0f * (s->p[W_XMOD_TUNE] - .5f) / 12.0f), 1e-6f, .24f);
        float mod = waveform(s->mod_phase, mod_step, s->p[W_XMOD_WAVE] >= .5f);
        float pm = mod * s->p[W_XMOD_DEPTH] * s->p[W_XMOD_LEVEL] * .5f;
        float carrier = waveform(wrap(s->carrier_phase + pm), step,
                                 s->p[W_CARRIER_WAVE] >= .5f);
        float sub = waveform(s->sub_phase, step * .5f, 0);
        float mixed = .1f * (s->p[W_CARRIER_LEVEL] * carrier
            + s->p[W_SUB_LEVEL] * sub + s->p[W_XMOD_LEVEL] * mod);
        float crush = s->p[W_CRUSH];
        if (crush > 0) mixed = fm_tanhf(mixed * (1.0f + 31.0f * crush));
        float lfo = waveform(s->lfo_phase, s->lfo_step, s->p[W_WOBBLE_SHAPE] >= .5f);
        float cutoff_control = fm_clampf(s->p[W_CUTOFF]
            - .65f * s->p[W_WOBBLE_DEPTH] * (lfo + 1.0f) * .5f, 0, 1);
        float cutoff_hz = fm_minf(40.0f * fm_exp2f(cutoff_control * 8.7f),
                                  s->sample_rate * .18f);
        float f = 2.0f * fm_sinf(FM_PI * cutoff_hz / (2.0f * s->sample_rate));
        for (unsigned pass = 0; pass < 2; ++pass) {
            float high = mixed - s->filter_low - 1.41421356f * s->filter_band;
            s->filter_band = fm_flush(s->filter_band + f * high);
            s->filter_low = fm_flush(s->filter_low + f * s->filter_band);
        }
        float out = s->filter_low * s->envelope * s->velocity * s->p[W_OUTPUT];
        float split_hz = wobble_bass_split_hz(s->p[W_BASS_SPLIT]);
        float split_alpha = wobble_bass_split_alpha(s->p[W_BASS_SPLIT]);
        float split_f = 2.0f * fm_sinf(FM_PI * split_hz / s->sample_rate);
        float low12 = svf_lowpass(out, split_f, &s->split_low, &s->split_band);
        float low24 = svf_lowpass(low12, split_f, &s->split2_low, &s->split2_band);
        float low_mono = split_alpha * low24 + (1.0f - split_alpha) * low12;
        float high = out - low_mono;
        float shifted = -.5f * high + s->stereo_memory;
        s->stereo_memory = fm_flush(high + .5f * shifted);
        float spread = s->p[W_STEREO] * .5f * (shifted - high);
        left[i] = low_mono + high - spread;
        right[i] = low_mono + high + spread;
        s->carrier_phase = wrap(s->carrier_phase + step);
        s->mod_phase = wrap(s->mod_phase + mod_step);
        s->sub_phase = wrap(s->sub_phase + step * .5f);
        s->lfo_phase = wrap(s->lfo_phase + s->lfo_step);
        ++s->active_samples;
    }
}
