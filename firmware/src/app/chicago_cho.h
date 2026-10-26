/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago Fx Type = Cho/Flg — per-voice insert (not SEND / Mix theft).
 *
 * Factory Cho rows (Detuned Square, Palm Bass, Trax Bass, …) need modulated
 * delay colour on the dry 303 voice for strong-relative spectrum. AU compare
 * previously rendered dry native303 while Cho stayed NOT_YET.
 *
 * Compact mono chorus/flanger: int16 delay line (~21 ms @ 48 kHz) + sine LFO
 * with linear tap interp. Edit1 → wet via sqrt (no Hall/SDly floor — those
 * floors left Detuned Square too wet). Edit2 → mild quadratic depth + feedback
 * (low = chorus detune; high = flanger whoosh). Slow LFO rate. No libm. */
#pragma once
#include <stdint.h>
#include <string.h>
#include "../dsp/fastmath.h"
#include "chicago_fx.h"

#ifndef CHI_CHO_SR
#define CHI_CHO_SR 44100.0f
#endif

/* ~21 ms @ 48 kHz — chorus base + depth headroom without a large pool hit. */
#define CHI_CHO_MAX 1024

typedef struct {
    int16_t buf[CHI_CHO_MAX];
    uint16_t w;     /* write index */
    float phase;    /* LFO phase in turns 0..1 */
    float inc;      /* LFO turns per sample */
    float base;     /* centre delay in samples */
    float depth;    /* ± modulation in samples */
    float fb;       /* feedback 0..~0.55 */
    float wet;      /* 0 dry .. 1 full insert */
} chi_cho_t;

static inline int16_t chi_cho_q15(float x)
{
    if (x > 1.0f)
        x = 1.0f;
    if (x < -1.0f)
        x = -1.0f;
    return (int16_t)(x * 32767.0f);
}

static inline void chi_cho_set(chi_cho_t *c, uint8_t e1, uint8_t e2, float sr)
{
    float send, t, base_ms, depth_ms, rate_hz, max_d;
    if (!c)
        return;
    if (sr < 1.0f)
        sr = CHI_CHO_SR;
    send = (float)e1 * (1.0f / 127.0f);
    t = (float)e2 * (1.0f / 127.0f);
    /* Sqrt wet (no floor): linear wet + Hall-style floor raised Detuned strongRel. */
    c->wet = fm_powf(send, 0.5f);
    /* Edit2 low: nearly-static slap (Detuned Square Edit2≈16). Mild quadratic
     * depth; high Edit2 opens flanger depth/fb. */
    base_ms = 9.0f - 4.0f * t;           /* ~9 ms → ~5 ms */
    depth_ms = 0.05f + 2.0f * t * t;     /* ~0.05..2.05 ms */
    c->fb = t < 0.35f ? 0.0f : (t - 0.35f) * (0.55f / 0.65f);
    rate_hz = 0.25f + 1.4f * t;          /* ~0.25..1.65 Hz */
    c->base = base_ms * 0.001f * sr;
    c->depth = depth_ms * 0.001f * sr;
    max_d = (float)(CHI_CHO_MAX - 3);
    if (c->base + c->depth > max_d) {
        float scale = max_d / (c->base + c->depth);
        c->base *= scale;
        c->depth *= scale;
    }
    if (c->base < 2.0f)
        c->base = 2.0f;
    c->inc = rate_hz / sr;
    if (c->inc > 0.05f)
        c->inc = 0.05f;
}

static inline void chi_cho_init(chi_cho_t *c)
{
    if (!c)
        return;
    memset(c->buf, 0, sizeof(c->buf));
    c->w = 0;
    c->phase = 0.0f;
    c->inc = 0.0f;
    c->base = 8.0f;
    c->depth = 0.0f;
    c->fb = 0.0f;
    c->wet = 0.0f;
}

/* In-place mono insert. wet=0 is a no-op. */
static inline void chi_cho_process(chi_cho_t *c, float *buf, int n)
{
    float wet, dry_g, fb, base, depth, ph, inc;
    uint16_t w;
    int k;
    if (!c || !buf || n <= 0)
        return;
    wet = c->wet;
    if (wet <= 0.0f)
        return;
    dry_g = 1.0f - wet;
    fb = c->fb;
    base = c->base;
    depth = c->depth;
    ph = c->phase;
    inc = c->inc;
    w = c->w;
    for (k = 0; k < n; k++) {
        float x = buf[k];
        float lfo, dly, di, frac, a, b, delayed, tap, y;
        uint16_t i0, i1;
        lfo = fm_sin_turns(ph);
        ph += inc;
        if (ph >= 1.0f)
            ph -= 1.0f;
        dly = base + depth * lfo;
        if (dly < 1.0f)
            dly = 1.0f;
        if (dly > (float)(CHI_CHO_MAX - 2))
            dly = (float)(CHI_CHO_MAX - 2);
        di = (float)w - dly;
        while (di < 0.0f)
            di += (float)CHI_CHO_MAX;
        i0 = (uint16_t)di;
        if (i0 >= CHI_CHO_MAX)
            i0 = (uint16_t)(i0 % CHI_CHO_MAX);
        i1 = (uint16_t)((i0 + 1u) % CHI_CHO_MAX);
        frac = di - (float)((uint16_t)di);
        a = (float)c->buf[i0] * (1.0f / 32767.0f);
        b = (float)c->buf[i1] * (1.0f / 32767.0f);
        delayed = a + (b - a) * frac;
        tap = x + fb * delayed;
        if (tap > 1.0f)
            tap = 1.0f;
        else if (tap < -1.0f)
            tap = -1.0f;
        c->buf[w] = chi_cho_q15(tap);
        w = (uint16_t)((w + 1u) % CHI_CHO_MAX);
        y = dry_g * x + wet * delayed;
        buf[k] = y;
    }
    c->w = w;
    c->phase = ph;
}

static inline int chi_fx_is_cho_insert(uint8_t type)
{
    return type == CHI_FX_CHO;
}
