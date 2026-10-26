/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago Fx Type = Crush (Decimator) — per-voice insert (not SEND / Mix theft).
 *
 * AU factory presets that use Crush (Yigh Yigh, GRIME) need sample-rate reduction
 * + mild bit quantize for strong-relative spectrum. Edit1 → intensity (hold period
 * + bit depth); Edit2 → wet 0..1. Quiet native303 must not null — boost into the
 * quantizer, then scale back. No libm. */
#pragma once
#include <stdint.h>
#include "../dsp/fastmath.h"
#include "chicago_fx.h"

typedef struct {
    float hold;  /* sample-hold counter */
    float lat;   /* last held sample */
    float c1;    /* quantizer steps */
    float g;     /* 1/c1 */
    float c2;    /* hold period in samples (>=1) */
    float wet;   /* 0 dry .. 1 full crush */
    uint8_t primed; /* first sample latches immediately */
} chi_crush_t;

static inline void chi_crush_set(chi_crush_t *c, uint8_t e1, uint8_t e2)
{
    float t = (float)e1 * (1.0f / 127.0f);
    /* Hold 1..40 samples — higher Edit1 = coarser rate (aliases / grit). */
    c->c2 = 1.0f + t * 39.0f;
    /* ~9→5 bit feel; enough levels that typical 303 amplitude still resolves. */
    {
        float bits = 9.0f - t * 4.0f;
        c->c1 = fm_powf(2.0f, bits);
        if (c->c1 < 8.0f)
            c->c1 = 8.0f;
        c->g = 1.0f / c->c1;
    }
    c->wet = (float)e2 * (1.0f / 127.0f);
    c->primed = 0;
    c->hold = 0.0f;
}

static inline void chi_crush_init(chi_crush_t *c)
{
    c->hold = 0.0f;
    c->lat = 0.0f;
    c->c1 = 512.0f;
    c->g = 1.0f / 512.0f;
    c->c2 = 1.0f;
    c->wet = 0.0f;
    c->primed = 0;
}

/* In-place mono insert. wet=0 is a no-op. */
static inline void chi_crush_process(chi_crush_t *c, float *buf, int n)
{
    float wet, dry_g, hold, lat, c1, g, c2;
    const float boost = 8.0f; /* lift quiet 303 into the quantizer */
    int k;
    uint8_t primed;
    if (!c || !buf || n <= 0)
        return;
    wet = c->wet;
    if (wet <= 0.0f)
        return;
    dry_g = 1.0f - wet;
    hold = c->hold;
    lat = c->lat;
    c1 = c->c1;
    g = c->g;
    c2 = c->c2;
    primed = c->primed;
    for (k = 0; k < n; k++) {
        float x = buf[k];
        float q = fm_floorf(x * boost * c1 + 0.5f) * g * (1.0f / boost);
        hold += 1.0f;
        if (!primed || hold >= c2) {
            if (primed)
                hold -= c2;
            else
                hold = 0.0f;
            lat = q;
            primed = 1;
        }
        buf[k] = dry_g * x + wet * lat;
    }
    c->hold = hold;
    c->lat = lat;
    c->primed = primed;
}

static inline int chi_fx_is_crush_insert(uint8_t type)
{
    return type == CHI_FX_DECIM;
}
