/* SPDX-License-Identifier: GPL-3.0-only */
/* Reference waveform evaluators, copied unchanged from experiments/wobble/wobble.c
 * before 18481a63 removed that DSP. ui_waveform.h mirrors these shapes. */
#include "../../../experiments/wobble/blep.h"
#include "../../../firmware/src/dsp/fastmath.h"
static float wrap(float p) { return p - fm_floorf(p); }
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
