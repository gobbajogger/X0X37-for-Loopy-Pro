/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago Fx Type = Crush (Decimator) — per-voice insert (not SEND / master Dist).
 *
 * AU factory presets that use Decimator (Yigh Yigh, GRIME, …) need bit-depth +
 * sample-rate reduction on the dry 303 voice for strong-relative spectrum.
 * Edit1 → crush amount (drive into d9-style quantise+hold). Edit2 unused for
 * mix — AU Decimator is full-wet insert (low Edit2 on GRIME still crushes).
 * Reuses the Crush law from drum909_dsp d9_shape type 6; no libm.
 *
 * Retune after #466: tip Edit2-as-wet left GRIME ~69 dB strongRel (worse than
 * pre-insert); full-wet + d9 drive map cuts Yigh/GRIME residuals. */
#pragma once
#include <stdint.h>
#include "../dsp/fastmath.h"
#include "chicago_fx.h"

typedef struct {
    float hold;  /* last held sample */
    float phase; /* samples until next hold (0..period) */
    float c1;    /* quantise scale */
    float g;     /* 1/c1 */
    float c2;    /* hold period in samples (>=1) */
    float wet;   /* 0 dry .. 1 full crush */
} chi_crush_t;

/* Edit1 0..127 → drive into crush prep (mild → aggressive). */
static inline float chi_crush_drive_from_edit1(uint8_t e1)
{
    float t = (float)e1 * (1.0f / 127.0f);
    /* 0.4 .. ~12.4 — matches fxbus Crush usefulness without needing CV_EXP. */
    return 0.4f + t * 12.0f;
}

static inline void chi_crush_prep_drive(chi_crush_t *c, float drive)
{
    float k = drive > 0.01f ? drive : 0.01f;
    c->c1 = 1.5f + 9.0f / k;
    c->g = 1.0f / c->c1;
    c->c2 = k < 1.0f ? 1.0f : 1.0f + (k - 1.0f) * 1.7f;
}

static inline void chi_crush_set(chi_crush_t *c, uint8_t e1, uint8_t e2)
{
    (void)e2;
    chi_crush_prep_drive(c, chi_crush_drive_from_edit1(e1));
    /* Full wet: Decimator is an insert algorithm, not a parallel blend. */
    c->wet = 1.0f;
}

static inline void chi_crush_init(chi_crush_t *c)
{
    c->hold = 0.0f;
    c->phase = 0.0f;
    c->c1 = 10.5f;
    c->g = 1.0f / 10.5f;
    c->c2 = 1.0f;
    c->wet = 0.0f;
}

/* In-place mono insert. wet=0 is a no-op. */
static inline void chi_crush_process(chi_crush_t *c, float *buf, int n)
{
    float wet, dry_g, hold, ph, c1, g, c2;
    int k;
    if (!c || !buf || n <= 0)
        return;
    wet = c->wet;
    if (wet <= 0.0f)
        return;
    dry_g = 1.0f - wet;
    hold = c->hold;
    ph = c->phase;
    c1 = c->c1;
    g = c->g;
    c2 = c->c2;
    for (k = 0; k < n; k++) {
        float x = buf[k];
        float q = fm_floorf(x * c1 + 0.5f) * g;
        ph += 1.0f;
        if (ph >= c2) {
            ph -= c2;
            hold = q;
        }
        buf[k] = dry_g * x + wet * hold;
    }
    c->hold = hold;
    c->phase = ph;
}

static inline int chi_fx_is_crush_insert(uint8_t type)
{
    return type == CHI_FX_DECIM;
}
