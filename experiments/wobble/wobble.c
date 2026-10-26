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
    (void)step;
    /* Open square: odd Fourier series pre-emphasized (boost≈2.15) so
     * prefilt+SVF lands on Miami-matrix strong-relative odds (square_open). */
    {
        float th = 2.0f * FM_PI * phase;
        return 1.0000000000f * fm_sinf(1.0f * th)
            + 0.3521721782f * fm_sinf(3.0f * th)
            + 0.2104254119f * fm_sinf(5.0f * th)
            + 0.1394680557f * fm_sinf(7.0f * th)
            + 0.0914483780f * fm_sinf(9.0f * th)
            + 0.0539063596f * fm_sinf(11.0f * th)
            + 0.0225654210f * fm_sinf(13.0f * th)
            + 0.0042509901f * fm_sinf(15.0f * th);
    }
}
/* X-MOD pulse: low-duty asymmetric BLEP pulse (matrix even-harmonic content).
 * Duty 0.10 matches ref H2..H7; 0.125 nulls H8 (sin(8*pi*D)=0) and fails spectrum. */
static float xmod_pulse_waveform(float phase, float step)
{
    const float duty = 0.10f;
    const float low = -duty / (1.0f - duty);
    float y = phase < duty ? 1.0f : low;
    y += wobble_blep(phase, step);
    y -= (1.0f - low) * wobble_blep(wrap(phase - duty), step);
    return y;
}
static float mod_waveform(float phase, float step, int pulse)
{
    return pulse ? xmod_pulse_waveform(phase, step) : waveform(phase, step, 0);
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
    /* Matrix reference release tail ~228 frames @ 48 kHz (MEASUREMENTS.md). */
    float rel_s = env_seconds(s->p[W_RELEASE], 0.00475f, 3.0f);
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
    s->filter_low = s->filter_band = s->stereo_memory = s->prefilt_lp = 0.0f;
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
            s->filter_low = s->filter_band = s->stereo_memory = s->prefilt_lp = 0;
            s->split_low = s->split_band = s->split2_low = s->split2_band = 0;
            left[i] = right[i] = 0;
            continue;
        }
        s->hz += (s->target_hz - s->hz) * s->glide_step;
        float step = fm_clampf(s->hz / s->sample_rate, 1e-6f, .12f);
        float mod_step = fm_clampf(step * .5f
            * exp2_bounded(10.0f * (s->p[W_XMOD_TUNE] - .5f) / 12.0f), 1e-6f, .24f);
        int xmod_pulse = s->p[W_XMOD_WAVE] >= .5f;
        float mod = mod_waveform(s->mod_phase, mod_step, xmod_pulse);
        float xmod_lvl = s->p[W_XMOD_LEVEL];
        float xmod_dep = s->p[W_XMOD_DEPTH];
        /* Depth>0 + level>0: Cheby-AM + odd sine bus (mix_depth H3/H5).
         * Level=0 must null depth (host modulation_null). Depth≈0: #397 pulse bus. */
        float m = mod;
        int xmod_live = (xmod_dep > 1e-6f) && (xmod_lvl > 1e-6f);
        if (xmod_live) {
            float m3 = 4.0f * mod * mod * mod - 3.0f * mod;
            float m5 = 16.0f * mod * mod * mod * mod * mod
                - 20.0f * mod * mod * mod + 5.0f * mod;
            m = fm_tanhf(mod + 1.3f * xmod_dep * m3 + 0.7f * xmod_dep * m5);
        }
        int carrier_muted_xmod = xmod_live && (s->p[W_CARRIER_LEVEL] <= 1e-6f);
        float carrier_phase_s = s->carrier_phase;
        /* Muted carrier + Depth>0 (x_form / trash_alias): PM the carrier so
         * X-mod remains audible; skip mix_depth odd residual (H4 spike). */
        if (carrier_muted_xmod)
            carrier_phase_s = wrap(s->carrier_phase
                + xmod_dep * xmod_lvl * 0.28f * m);
        int square_carrier = s->p[W_CARRIER_WAVE] >= .5f;
        float sub_lvl = s->p[W_SUB_LEVEL];
        float carrier;
        /* Sub-less muted X-mod (trash_alias): pulse at carrier Hz for even/high
         * partials. Sub-heavy muted rows (x_form) keep square carrier + PM. */
        if (carrier_muted_xmod && xmod_pulse && sub_lvl <= 1e-6f)
            carrier = xmod_pulse_waveform(carrier_phase_s, step);
        else
            carrier = waveform(carrier_phase_s, step, square_carrier);
        /* Square Fourier peak ~0.92 vs unity; restore Miami-matrix
         * square_open absolute steady envelope (~+2.23 dB). LFO shape
         * keeps unscaled waveform() so wobble depth is unchanged. */
        if (square_carrier && !(carrier_muted_xmod && xmod_pulse && sub_lvl <= 1e-6f))
            carrier *= 1.293f;
        if (xmod_live && !carrier_muted_xmod)
            carrier *= (1.0f + xmod_dep * xmod_lvl * (6.2f / 0.7f) * m);
        float sub = waveform(s->sub_phase, step * .5f, 0);
        float carrier_lvl = s->p[W_CARRIER_LEVEL];
        float mod_bus = 0.0f;
        if (!xmod_live) {
            mod_bus = xmod_lvl * mod;
            if (xmod_pulse && xmod_dep <= 1e-6f)
                mod_bus *= 1.8f; /* matrix mod_pulse was ~5 dB quiet at unity bus */
        } else if (carrier_muted_xmod) {
            carrier_lvl = xmod_lvl;
            mod_bus = 0.0f;
        } else {
            /* Cosine (not sine) odd partials of mod Hz: closes mix_depth
             * lag-correlation (≥0.99) while keeping strong-relative ≤1 dB.
             * Amplitudes 0.15/0.03 retuned vs prior 0.30/0.06 sine bus. */
            float odd = 0.15f * fm_cosf(6.0f * FM_PI * s->mod_phase)
                + 0.03f * fm_cosf(10.0f * FM_PI * s->mod_phase);
            mod_bus = xmod_lvl * odd;
        }
        float mixed = 0.07725f * (carrier_lvl * carrier
            + s->p[W_SUB_LEVEL] * sub + mod_bus);
        float crush = s->p[W_CRUSH];
        if (crush > 0) {
            mixed = fm_tanhf(mixed * (1.0f + 31.0f * crush));
            /* Muted-carrier X-mod rows (x_form / trash_alias): add reflective
             * Crush alias colour for H8..H14. Leave normal Crush as tanh. */
            if (carrier_muted_xmod) {
                float a = crush * crush;
                mixed += a * (0.40f * fm_sinf(8.0f * FM_PI * mixed)
                            + 0.55f * fm_sinf(14.0f * FM_PI * mixed));
            }
        }
        /* Pre-filter tone — softens pulse H11–H16 and square H15 vs Miami open. */
        s->prefilt_lp = fm_flush(s->prefilt_lp + 0.115f * (mixed - s->prefilt_lp));
        mixed = s->prefilt_lp;
        float lfo = waveform(s->lfo_phase, s->lfo_step, s->p[W_WOBBLE_SHAPE] >= .5f);
        float cutoff_control = fm_clampf(s->p[W_CUTOFF]
            - .65f * s->p[W_WOBBLE_DEPTH] * (lfo + 1.0f) * .5f, 0, 1);
        /* Span 8.0 + open-preserving damp: mid cutoff matches Miami
         * carrier_filter strong-relative; damp→1 at pot 1 keeps open rows. */
        float cutoff_hz = fm_minf(36.0f * fm_exp2f(cutoff_control * 8.0f),
                                  s->sample_rate * .18f);
        float f = 2.0f * fm_sinf(FM_PI * cutoff_hz / (2.0f * s->sample_rate));
        {
            float damp = 1.0f + 0.875f * (1.0f - cutoff_control); /* 1.35 @ pot 0.6 */
            float high = mixed - s->filter_low - damp * s->filter_band;
            s->filter_band = fm_flush(s->filter_band + f * high);
            s->filter_low = fm_flush(s->filter_low + f * s->filter_band);
        }
        float filt_gain = 0.82f + 0.18f * cutoff_control;
        float out = s->filter_low * filt_gain * s->envelope * s->velocity
            * s->p[W_OUTPUT];
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
