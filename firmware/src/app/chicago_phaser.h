/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago Fx Type = Phaser — per-voice insert (not SEND / Mix theft).
 *
 * Factory Phaser rows (Hot Tube, Phase Line, Phaze Craze, RAZR FX) need moving
 * notches on the dry 303 voice for strong-relative spectrum. AU compare
 * previously rendered dry native303 while Phaser stayed NOT_YET.
 *
 * Compact mono 4-stage first-order allpass + mild feedback. Edit1 → linear wet
 * (no floor — Phase Line Edit1≈41 stays light; Hot Tube≈95 still thick).
 * Edit2 → LFO rate/depth + feedback. No delay-line pool hit; no libm. */
#pragma once
#include <stdint.h>
#include <string.h>
#include "../dsp/fastmath.h"
#include "chicago_fx.h"

#ifndef CHI_PHASER_SR
#define CHI_PHASER_SR 44100.0f
#endif

#define CHI_PHASER_STAGES 4

typedef struct {
    float z[CHI_PHASER_STAGES]; /* allpass delay states */
    float last_y;               /* previous chain out (feedback) */
    float phase;                /* LFO phase in turns 0..1 */
    float inc;                  /* LFO turns per sample */
    float g_centre;             /* allpass coeff centre */
    float g_depth;              /* ± LFO sweep of coeff */
    float fb;                   /* feedback into first stage */
    float wet;                  /* 0 dry .. 1 full insert */
} chi_phaser_t;

static inline void chi_phaser_set(chi_phaser_t *p, uint8_t e1, uint8_t e2, float sr)
{
    float send, t, rate_hz;
    if (!p)
        return;
    if (sr < 1.0f)
        sr = CHI_PHASER_SR;
    send = (float)e1 * (1.0f / 127.0f);
    t = (float)e2 * (1.0f / 127.0f);
    /* Linear wet (no floor): Phase Line Edit1≈41 → ~0.32; Hot Tube≈95 → ~0.75. */
    p->wet = send;
    /* Edit2 mid (Hot Tube≈64): gentle sweep; high → faster/deeper whoosh. */
    rate_hz = 0.10f + 1.8f * t;
    p->g_centre = 0.40f + 0.28f * t; /* ~0.40..0.68 */
    p->g_depth = 0.10f + 0.22f * t;  /* keep |g| in ~0.1..0.90 */
    p->fb = 0.02f + 0.28f * t;
    p->inc = rate_hz / sr;
    if (p->inc > 0.05f)
        p->inc = 0.05f;
}

static inline void chi_phaser_init(chi_phaser_t *p)
{
    if (!p)
        return;
    memset(p->z, 0, sizeof(p->z));
    p->last_y = 0.0f;
    p->phase = 0.0f;
    p->inc = 0.0f;
    p->g_centre = 0.55f;
    p->g_depth = 0.20f;
    p->fb = 0.0f;
    p->wet = 0.0f;
}

/* One first-order allpass: y = -g·x + z; z = x + g·y */
static inline float chi_phaser_ap(float x, float g, float *z)
{
    float y = -g * x + *z;
    *z = x + g * y;
    return y;
}

/* In-place mono insert. wet=0 is a no-op. */
static inline void chi_phaser_process(chi_phaser_t *p, float *buf, int n)
{
    float wet, dry_g, ph, inc, g0, gd, fb;
    int k, s;
    if (!p || !buf || n <= 0)
        return;
    wet = p->wet;
    if (wet <= 0.0f)
        return;
    dry_g = 1.0f - wet;
    ph = p->phase;
    inc = p->inc;
    g0 = p->g_centre;
    gd = p->g_depth;
    fb = p->fb;
    for (k = 0; k < n; k++) {
        float x = buf[k];
        float lfo, g, y, in;
        lfo = fm_sin_turns(ph);
        ph += inc;
        if (ph >= 1.0f)
            ph -= 1.0f;
        g = g0 + gd * lfo;
        if (g > 0.95f)
            g = 0.95f;
        else if (g < 0.05f)
            g = 0.05f;
        in = x + fb * p->last_y;
        if (in > 1.0f)
            in = 1.0f;
        else if (in < -1.0f)
            in = -1.0f;
        y = in;
        for (s = 0; s < CHI_PHASER_STAGES; s++)
            y = chi_phaser_ap(y, g, &p->z[s]);
        p->last_y = y;
        buf[k] = dry_g * x + wet * y;
    }
    p->phase = ph;
}

static inline int chi_fx_is_phaser_insert(uint8_t type)
{
    return type == CHI_FX_PHASER;
}
