/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef X0X_UI_WAVEFORM_H
#define X0X_UI_WAVEFORM_H
#include "../dsp/fastmath.h"
#include "../../../experiments/wobble/blep.h"
/* Raw periodic SHAPE cue, not an oscilloscope or the modulated/filtered output.
 * One normalized cycle, fixed reference phase increment. These renderer-only
 * equations mirror wobble.c; host proof compares the actual private evaluator.
 * No audio state, note frequency, gain, envelope or phase is read or changed. */
#define UI_WAVE_REFERENCE_STEP (1.0f / 128.0f)
static float ui_miami_wave_value(int pulse, int selected, float phase)
{
    phase -= fm_floorf(phase);
    if (!selected) return 4.0f * fm_fabsf(phase - 0.5f) - 1.0f;
    if (pulse) {
        const float duty=0.10f, low=-duty/(1.0f-duty);
        float shifted=phase-duty; shifted-=fm_floorf(shifted);
        float v=phase<duty?1.0f:low;
        v+=wobble_blep(phase,UI_WAVE_REFERENCE_STEP);
        v-=(1.0f-low)*wobble_blep(shifted,UI_WAVE_REFERENCE_STEP);
        return v;
    }
    {
        float th=2.0f*FM_PI*phase;
        return 1.0000000000f*fm_sinf(th)
            +0.3521721782f*fm_sinf(3.0f*th)
            +0.2104254119f*fm_sinf(5.0f*th)
            +0.1394680557f*fm_sinf(7.0f*th)
            +0.0914483780f*fm_sinf(9.0f*th)
            +0.0539063596f*fm_sinf(11.0f*th)
            +0.0225654210f*fm_sinf(13.0f*th)
            +0.0042509901f*fm_sinf(15.0f*th);
    }
}
#endif
