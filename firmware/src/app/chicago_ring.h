/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago Fx Type = Ring Mod — per-voice insert (not SEND / Mix theft).
 *
 * AU factory presets that use Ring (Hightone FX, Loop Me 2, Network Gone Bad FX)
 * need sidebands for strong-relative spectrum. Edit1 → carrier Hz (exp 20..~5 kHz);
 * Edit2 → wet 0..1 (1 = dry×carrier). Uses fm_sin_turns; no libm. */
#pragma once
#include <stdint.h>
#include "../dsp/fastmath.h"
#include "chicago_fx.h"

#ifndef CHI_RING_SR
#define CHI_RING_SR 44100.0f
#endif

typedef struct {
    float phase; /* carrier phase in turns 0..1 */
    float inc;   /* turns per sample */
    float wet;   /* 0 dry .. 1 full ring */
} chi_ring_t;

static inline float chi_ring_hz_from_edit1(uint8_t e1)
{
    /* Edit1 0..127 → ~20 Hz .. ~4 kHz. 8-octave map (max ~5.1 kHz) left
     * loop_me_2 strong-relative ~63 dB; 20·200^t cuts that residual ~26 dB
     * on tip-QUICK Ring rows without regressing hightone / network. */
    float t = (float)e1 * (1.0f / 127.0f);
    return 20.0f * fm_powf(200.0f, t);
}

static inline void chi_ring_set(chi_ring_t *r, uint8_t e1, uint8_t e2, float sr)
{
    float hz;
    if (sr < 1.0f)
        sr = CHI_RING_SR;
    hz = chi_ring_hz_from_edit1(e1);
    r->inc = hz / sr;
    if (r->inc > 0.49f)
        r->inc = 0.49f;
    r->wet = (float)e2 * (1.0f / 127.0f);
}

static inline void chi_ring_init(chi_ring_t *r)
{
    r->phase = 0.0f;
    r->inc = 0.0f;
    r->wet = 0.0f;
}

/* In-place mono insert. wet=0 is a no-op. */
static inline void chi_ring_process(chi_ring_t *r, float *buf, int n)
{
    float ph, inc, wet, dry_g;
    int k;
    if (!r || !buf || n <= 0)
        return;
    wet = r->wet;
    if (wet <= 0.0f)
        return;
    ph = r->phase;
    inc = r->inc;
    dry_g = 1.0f - wet;
    for (k = 0; k < n; k++) {
        float c = fm_sin_turns(ph);
        ph += inc;
        if (ph >= 1.0f)
            ph -= 1.0f;
        buf[k] = buf[k] * (dry_g + wet * c);
    }
    r->phase = ph;
}

static inline int chi_fx_is_ring_insert(uint8_t type)
{
    return type == CHI_FX_RING;
}
