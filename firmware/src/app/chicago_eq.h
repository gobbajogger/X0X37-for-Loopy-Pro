/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago Fx Type = EQ — per-voice insert (not SEND / Mix theft).
 *
 * AU factory presets that use EQ (Bloghaus, Dist Sq Line, …) need a tilt shelf
 * on the dry 303 voice for strong-relative spectrum. Edit1 → tilt (dark↔bright
 * via one-pole LP vs HP residual); Edit2 → wet 0..1. No libm.
 *
 * Retune after #479: tip corner 120·50^t left Bloghaus Edit1=0 as a ~120 Hz LP
 * and grew strongRel (55.8→59.1 dB). Floor the corner at 750 Hz so dark EQ
 * still tilts without dumping the 303 body; bright map unchanged. */
#pragma once
#include <stdint.h>
#include "../dsp/fastmath.h"
#include "chicago_fx.h"

#ifndef CHI_EQ_SR
#define CHI_EQ_SR 44100.0f
#endif

typedef struct {
    float z;   /* one-pole LP state */
    float a;   /* LP feedback coeff */
    float g;   /* 1-a (feed) */
    float wet; /* 0 dry .. 1 full EQ */
    float tilt; /* 0 = dark (LP) .. 1 = bright (HP residual) */
} chi_eq_t;

static inline void chi_eq_set(chi_eq_t *e, uint8_t e1, uint8_t e2, float sr)
{
    float t, hz;
    if (sr < 1.0f)
        sr = CHI_EQ_SR;
    t = (float)e1 * (1.0f / 127.0f);
    /* Corner ~120 Hz .. ~6 kHz, floored at 750 Hz — tip dark extreme was too
     * low for Bloghaus/Dist SQ Line strong-relative spectrum. */
    hz = 120.0f * fm_powf(50.0f, t);
    if (hz < 750.0f)
        hz = 750.0f;
    e->a = fm_expf(-FM_TWO_PI * hz / sr);
    if (e->a > 0.999f)
        e->a = 0.999f;
    if (e->a < 0.0f)
        e->a = 0.0f;
    e->g = 1.0f - e->a;
    e->tilt = t;
    e->wet = (float)e2 * (1.0f / 127.0f);
}

static inline void chi_eq_init(chi_eq_t *e)
{
    e->z = 0.0f;
    e->a = 0.0f;
    e->g = 1.0f;
    e->wet = 0.0f;
    e->tilt = 0.5f;
}

/* In-place mono insert. wet=0 is a no-op. */
static inline void chi_eq_process(chi_eq_t *e, float *buf, int n)
{
    float z, a, g, wet, dry_g, tilt, inv_tilt;
    int k;
    if (!e || !buf || n <= 0)
        return;
    wet = e->wet;
    if (wet <= 0.0f)
        return;
    z = e->z;
    a = e->a;
    g = e->g;
    dry_g = 1.0f - wet;
    tilt = e->tilt;
    inv_tilt = 1.0f - tilt;
    for (k = 0; k < n; k++) {
        float x = buf[k];
        float lp, hp, shaped;
        z = a * z + g * x;
        lp = z;
        hp = x - z;
        /* Edit1 low → dark (LP); Edit1 high → bright (HP). Mid keeps body. */
        shaped = inv_tilt * lp + tilt * (x + hp);
        buf[k] = dry_g * x + wet * shaped;
    }
    e->z = z;
}

static inline int chi_fx_is_eq_insert(uint8_t type)
{
    return type == CHI_FX_EQ;
}
