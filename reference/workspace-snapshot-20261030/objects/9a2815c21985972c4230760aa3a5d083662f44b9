/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago Fx Type = Gate Rev — per-voice insert (not SEND / Mix reverb theft).
 *
 * Factory GateRev rows (Summer Bass, …) need short gated-reverb colour on the
 * dry 303 voice for strong-relative spectrum. AU compare previously rendered
 * dry native303 while GateRev stayed NOT_YET.
 *
 * Compact mono gated reverb: 3 int16 combs + 1 float allpass (~1.7 KB/voice),
 * envelope gate, and a one-pole tone filter on the dry path. AU Summer Bass is
 * much darker than dry native303; tone+crossfade ducks bright partials into a
 * damped wash. Edit1 → wet (floor 0.72). Edit2 → decay/fb + gate hold + tone.
 * No libm. */
#pragma once
#include <stdint.h>
#include <string.h>
#include "chicago_fx.h"

#ifndef CHI_GREV_SR
#define CHI_GREV_SR 44100.0f
#endif

#define CHI_GREV_C0 181
#define CHI_GREV_C1 211
#define CHI_GREV_C2 241
#define CHI_GREV_COMB_N (CHI_GREV_C0 + CHI_GREV_C1 + CHI_GREV_C2)
#define CHI_GREV_AP0 97

typedef struct {
    int16_t comb[CHI_GREV_COMB_N];
    float ap0[CHI_GREV_AP0];
    float cdmp[3];
    uint16_t cpos[3];
    uint16_t apos0;
    float fb;
    float damp;
    float wet;
    float lvl;
    float tone;     /* dry-path one-pole coeff 0..1 (lower = darker) */
    float lp;       /* dry LP state */
    float env;      /* |dry| follower */
    float gate;     /* 0..1 wet gate */
    float hold;     /* samples left while open after env drop */
    float hold_max; /* Edit2 → hold length in samples */
    float atk;
    float rel;
    float open_th;
    float close_th;
} chi_gaterev_t;

static inline int16_t chi_grev_q12(float x)
{
    if (x > 7.999f)
        x = 7.999f;
    if (x < -8.0f)
        x = -8.0f;
    return (int16_t)(x * 2048.0f);
}

static inline void chi_gaterev_set(chi_gaterev_t *g, uint8_t e1, uint8_t e2, float sr)
{
    float send, t, hold_ms;
    if (!g)
        return;
    if (sr < 1.0f)
        sr = CHI_GREV_SR;
    send = (float)e1 * (1.0f / 127.0f);
    t = (float)e2 * (1.0f / 127.0f);
    /* Wet floor — Summer Bass Edit1≈28 still crossfades enough to darken. */
    g->wet = send < 0.85f ? 0.85f : send;
    g->fb = 0.18f + 0.40f * t;
    g->damp = 0.78f - 0.18f * t;
    g->lvl = 1.15f;
    /* Dark dry tone; higher Edit2 opens a bit of air. */
    g->tone = 0.035f + 0.08f * t; /* ~0.035..0.115 */
    hold_ms = 60.0f + 160.0f * t;
    g->hold_max = hold_ms * 0.001f * sr;
    g->atk = 0.40f;
    g->rel = 0.003f;
    g->open_th = 0.005f;
    g->close_th = 0.0015f;
}

static inline void chi_gaterev_init(chi_gaterev_t *g)
{
    if (!g)
        return;
    memset(g, 0, sizeof(*g));
    g->fb = 0.0f;
    g->damp = 0.78f;
    g->wet = 0.0f;
    g->lvl = 1.15f;
    g->tone = 0.035f;
    g->hold_max = 0.0f;
    g->atk = 0.40f;
    g->rel = 0.003f;
    g->open_th = 0.005f;
    g->close_th = 0.0015f;
}

/* In-place mono insert. wet=0 is a no-op. */
static inline void chi_gaterev_process(chi_gaterev_t *g, float *buf, int n)
{
    static const uint16_t cl[3] = { CHI_GREV_C0, CHI_GREV_C1, CHI_GREV_C2 };
    float wet, fb, damp, lvl, tone, lp, env, gate, hold, atk, rel, oth, cth;
    int16_t *cb[3];
    uint16_t cpos[3], a0;
    float *ap0;
    int k, c;
    if (!g || !buf || n <= 0)
        return;
    wet = g->wet;
    if (wet <= 0.0f)
        return;
    fb = g->fb;
    damp = g->damp;
    lvl = g->lvl;
    tone = g->tone;
    lp = g->lp;
    env = g->env;
    gate = g->gate;
    hold = g->hold;
    atk = g->atk;
    rel = g->rel;
    oth = g->open_th;
    cth = g->close_th;
    cb[0] = g->comb;
    cb[1] = cb[0] + CHI_GREV_C0;
    cb[2] = cb[1] + CHI_GREV_C1;
    ap0 = g->ap0;
    for (c = 0; c < 3; c++)
        cpos[c] = g->cpos[c];
    a0 = g->apos0;
    for (k = 0; k < n; k++) {
        float x = buf[k];
        float ax = x >= 0.0f ? x : -x;
        float acc = 0.0f, y, bo, ge, wet_g, xd;
        if (ax > env)
            env += (ax - env) * atk;
        else
            env += (ax - env) * rel;
        if (env >= oth) {
            gate = 1.0f;
            hold = g->hold_max;
        } else if (hold > 0.0f) {
            hold -= 1.0f;
            gate = 1.0f;
        } else if (env < cth) {
            gate *= 0.75f;
            if (gate < 0.002f)
                gate = 0.0f;
        }
        /* Darken dry path — AU GateRev rows are far less bright than native303. */
        lp += (x - lp) * tone;
        xd = lp;
        for (c = 0; c < 3; c++) {
            int16_t *b = cb[c];
            uint16_t p = cpos[c];
            float cy = (float)b[p] * (1.0f / 2048.0f);
            acc += cy;
            g->cdmp[c] = cy + (g->cdmp[c] - cy) * damp;
            b[p] = chi_grev_q12(xd + g->cdmp[c] * fb);
            cpos[c] = (uint16_t)(p + 1u >= cl[c] ? 0u : p + 1u);
        }
        y = acc * (1.0f / 3.0f);
        bo = ap0[a0];
        ap0[a0] = y + bo * 0.5f;
        y = bo - y * 0.5f;
        a0 = (uint16_t)(a0 + 1u >= CHI_GREV_AP0 ? 0u : a0 + 1u);
        ge = gate;
        wet_g = wet * (0.35f + 0.65f * ge); /* keep some wash after gate closes */
        buf[k] = (1.0f - wet_g) * xd + wet_g * y * lvl;
    }
    for (c = 0; c < 3; c++)
        g->cpos[c] = cpos[c];
    g->apos0 = a0;
    g->lp = lp;
    g->env = env;
    g->gate = gate;
    g->hold = hold;
}

static inline int chi_fx_is_gaterev_insert(uint8_t type)
{
    return type == CHI_FX_GATE_REV;
}
