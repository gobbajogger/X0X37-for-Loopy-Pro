/* SPDX-License-Identifier: GPL-3.0-only */
/* Independently composed wobble instrument. X-MOD is experimental PM, CRUSH
 * is experimental saturation, filter is an original bounded SVF approximation.
 * None is asserted to reproduce an undisclosed Miami algorithm. */
#include "wobble.h"
#include "blep.h"
#include "../../firmware/src/dsp/fastmath.h"

static float wrap(float p) { return p - fm_floorf(p); }
/* The existing helper shifts a signed exponent and UBSan flags negative n.
 * Keep its reviewed positive-domain approximation and use reciprocal here;
 * do not alter the production helper from this isolated experiment. */
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
static void update_rates(wobble_t *s)
{
    /* Original fixed 120 BPM note-division mapping, no host clock dependency. */
    static const float rates[7] = {1, 2, 3, 4, 6, 8, 12};
    unsigned rate = (unsigned)(s->p[W_WOBBLE_RATE] * 6.0f + 0.5f);
    s->lfo_step = rates[rate] / s->sample_rate;
    s->glide_step = s->p[W_GLIDE] == 0.0f ? 1.0f
        : 1.0f - fm_expf(-1.0f / (s->sample_rate
                * (0.002f + 0.998f * s->p[W_GLIDE] * s->p[W_GLIDE])));
}
void wobble_reset(wobble_t *s)
{
    s->carrier_phase = s->mod_phase = s->sub_phase = s->lfo_phase = 0.0f;
    s->hz = s->target_hz = 0.0f;
    s->envelope = s->velocity = 0.0f;
    s->filter_low = s->filter_band = s->stereo_memory = 0.0f;
    s->active_samples = 0;
    s->note = 0; s->gate = s->active = s->deferred_off = 0;
}
void wobble_init(wobble_t *s, float sample_rate)
{
    static const float defaults[W_PARAMETER_COUNT] = {
        0,.7f,0,.5f,0,0,0,.5f,0,.6f,0,0,0,0,.5f,0,.5f,0
    };
    s->sample_rate = finite_value(sample_rate)
        ? fm_clampf(sample_rate, 8000.0f, 192000.0f) : 48000.0f;
    for (unsigned i = 0; i < W_PARAMETER_COUNT; ++i) s->p[i] = defaults[i];
    s->attack_step = 1.0f / (0.002f * s->sample_rate);
    s->release_step = 1.0f / (0.030f * s->sample_rate);
    update_rates(s);
    wobble_reset(s);
}
int wobble_set(wobble_t *s, unsigned parameter, float value)
{
    if (parameter >= W_PARAMETER_COUNT || !finite_value(value)) return 0;
    s->p[parameter] = fm_clampf(value, 0, 1);
    if (parameter == W_CARRIER_PITCH || parameter == W_PITCH_BEND) update_pitch(s);
    if (parameter == W_GLIDE || parameter == W_WOBBLE_RATE) update_rates(s);
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
        if (s->gate) s->envelope = fm_minf(1, s->envelope + s->attack_step);
        else s->envelope = fm_maxf(0, s->envelope - s->release_step);
        if (s->envelope == 0 && !s->gate) {
            /* Exact silence; phase history is retained but filter memory cleared.
             * This is an original policy, not a conclusion about reference internals. */
            s->active = 0;
            s->filter_low = s->filter_band = s->stereo_memory = 0;
            left[i] = right[i] = 0;
            continue;
        }
        s->hz += (s->target_hz - s->hz) * s->glide_step;
        float step = fm_clampf(s->hz / s->sample_rate, 1e-6f, .12f);
        float mod_step = fm_clampf(step * .5f
            * exp2_bounded(10.0f * (s->p[W_XMOD_TUNE] - .5f) / 12.0f), 1e-6f, .24f);
        float mod = waveform(s->mod_phase, mod_step, s->p[W_XMOD_WAVE] >= .5f);
        /* Level0 gates both audible modulator and modulation. PM spectrum at high
         * depth is not fully bandlimited; aliasing remains a refinement target. */
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
        /* Original two-pass Chamberlin SVF, fixed damping, frequency capped for
         * stable twofold update. Not copied from an unreviewed filter module. */
        float f = 2.0f * fm_sinf(FM_PI * cutoff_hz / (2.0f * s->sample_rate));
        for (unsigned pass = 0; pass < 2; ++pass) {
            float high = mixed - s->filter_low - 1.41421356f * s->filter_band;
            s->filter_band = fm_flush(s->filter_band + f * high);
            s->filter_low = fm_flush(s->filter_low + f * s->filter_band);
        }
        float out = s->filter_low * s->envelope * s->velocity * s->p[W_OUTPUT];
        /* Fixed first-order allpass spread, not a claimed reference algorithm. */
        float shifted = -.5f * out + s->stereo_memory;
        s->stereo_memory = fm_flush(out + .5f * shifted);
        float spread = s->p[W_STEREO] * .5f * (shifted - out);
        left[i] = out - spread; right[i] = out + spread;
        s->carrier_phase = wrap(s->carrier_phase + step);
        s->mod_phase = wrap(s->mod_phase + mod_step);
        s->sub_phase = wrap(s->sub_phase + step * .5f);
        s->lfo_phase = wrap(s->lfo_phase + s->lfo_step);
        ++s->active_samples;
    }
}
