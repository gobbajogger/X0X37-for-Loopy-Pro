/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago Ens sweep: floor0.45 */
#pragma once
#include <stdint.h>
#include <string.h>
#include "../dsp/fastmath.h"
#include "chicago_fx.h"
#ifndef CHI_ENS_SR
#define CHI_ENS_SR 44100.0f
#endif
#define CHI_ENS_MAX 1024
#define CHI_ENS_VOICES 3
typedef struct {
    int16_t buf[CHI_ENS_MAX];
    uint16_t w;
    float phase[CHI_ENS_VOICES];
    float inc;
    float base[CHI_ENS_VOICES];
    float depth;
    float wet;
} chi_ens_t;
static inline int16_t chi_ens_q15(float x) {
    if (x > 1.0f) x = 1.0f;
    if (x < -1.0f) x = -1.0f;
    return (int16_t)(x * 32767.0f);
}
static inline void chi_ens_set(chi_ens_t *e, uint8_t e1, uint8_t e2, float sr) {
    static const float base_ms0[CHI_ENS_VOICES] = { 7.0f, 11.0f, 15.0f };
    float send, t, depth_ms, rate_hz, max_d; int v;
    if (!e) return;
    if (sr < 1.0f) sr = CHI_ENS_SR;
    send = (float)e1 * (1.0f / 127.0f); t = (float)e2 * (1.0f / 127.0f);
    e->wet = send < 0.45f ? 0.45f : send;
    depth_ms = 0.35f + 2.8f * t;
    rate_hz = 0.18f + 0.55f * t;
    e->depth = depth_ms * 0.001f * sr;
    max_d = (float)(CHI_ENS_MAX - 4);
    for (v = 0; v < CHI_ENS_VOICES; v++) {
        e->base[v] = base_ms0[v] * 0.001f * sr;
        if (e->base[v] + e->depth > max_d) {
            float scale = max_d / (e->base[v] + e->depth);
            e->base[v] *= scale; if (v == 0) e->depth *= scale;
        }
        if (e->base[v] < 2.0f) e->base[v] = 2.0f;
    }
    e->inc = rate_hz / sr; if (e->inc > 0.05f) e->inc = 0.05f;
}
static inline void chi_ens_init(chi_ens_t *e) {
    int v; if (!e) return; memset(e->buf, 0, sizeof(e->buf));
    e->w = 0; e->inc = 0; e->depth = 0; e->wet = 0;
    for (v = 0; v < CHI_ENS_VOICES; v++) {
        e->phase[v] = (float)v * (1.0f / (float)CHI_ENS_VOICES);
        e->base[v] = 8.0f + (float)v * 3.0f;
    }
}
static inline void chi_ens_process(chi_ens_t *e, float *buf, int n) {
    float wet, dry_g, depth, inc, base[CHI_ENS_VOICES], ph[CHI_ENS_VOICES];
    uint16_t w; int k, v;
    if (!e || !buf || n <= 0) return;
    wet = e->wet;
    if (wet <= 0.0f) return;
    dry_g = 1.0f - wet; depth = e->depth; inc = e->inc; w = e->w;
    for (v = 0; v < CHI_ENS_VOICES; v++) { base[v] = e->base[v]; ph[v] = e->phase[v]; }
    for (k = 0; k < n; k++) {
        float x = buf[k], sum = 0.0f, y;
        e->buf[w] = chi_ens_q15(x);
        for (v = 0; v < CHI_ENS_VOICES; v++) {
            float lfo, dly, di, frac, a, b, delayed; uint16_t i0, i1;
            lfo = fm_sin_turns(ph[v]);
            ph[v] += inc * (1.0f + 0.17f * (float)v);
            if (ph[v] >= 1.0f) ph[v] -= 1.0f;
            dly = base[v] + depth * lfo;
            if (dly < 1.0f) dly = 1.0f;
            if (dly > (float)(CHI_ENS_MAX - 2)) dly = (float)(CHI_ENS_MAX - 2);
            di = (float)w - dly; while (di < 0.0f) di += (float)CHI_ENS_MAX;
            i0 = (uint16_t)di; if (i0 >= CHI_ENS_MAX) i0 = (uint16_t)(i0 % CHI_ENS_MAX);
            i1 = (uint16_t)((i0 + 1u) % CHI_ENS_MAX);
            frac = di - (float)((uint16_t)di);
            a = (float)e->buf[i0] * (1.0f / 32767.0f);
            b = (float)e->buf[i1] * (1.0f / 32767.0f);
            delayed = a + (b - a) * frac; sum += delayed;
        }
        w = (uint16_t)((w + 1u) % CHI_ENS_MAX);
        y = dry_g * x + wet * (sum * (1.0f / (float)CHI_ENS_VOICES));
        buf[k] = y;
    }
    e->w = w; for (v = 0; v < CHI_ENS_VOICES; v++) e->phase[v] = ph[v];
}
static inline int chi_fx_is_ens_insert(uint8_t type) { return type == CHI_FX_ENS; }
