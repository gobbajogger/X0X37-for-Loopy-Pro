/* SPDX-License-Identifier: GPL-3.0-only */
/* Archived Chicago inserts (#569/#577 era chicago_*.h) as compact FX2 kernels:
 * type 3 Ensemble, 12 Short Delay, 13 BPM Delay, 15 Hall, 16 Gate Reverb.
 *
 * The archived DSP is mono and float. Here it runs on the (L+R)/2 sum; the wet
 * signal goes to both channels and each channel keeps its own dry. All five
 * live in the compact object's 4096 B Chorus arena (one algorithm is selected
 * at a time), so they add no per-part history RAM:
 *   Ensemble, Short Delay, Gate Reverb: archived structs, unchanged.
 *   Hall: archived combs; the two allpass lines are Q12 int16, not float.
 *   BPM Delay: same ~92 ms range as the archived 4096-sample ring, stored at
 *   half rate (2-sample averages, linear read) in 2032 int16 slots. */
#ifndef ORIGINAL_CHICAGO_FX2_INSERTS_H
#define ORIGINAL_CHICAGO_FX2_INSERTS_H
#include <stdint.h>
#include <string.h>
#include "../../firmware/src/app/chicago_ens.h"
#include "../../firmware/src/app/chicago_sdly.h"
#include "../../firmware/src/app/chicago_gaterev.h"
#include "../../firmware/src/app/chicago_hall.h" /* CHI_HALL_* lengths and chi_hall_q12 */
#include "../../firmware/src/app/chicago_bpmdly.h" /* beat grid */

#define CHI_FX2_SR 44100.0f
#define CHI_FX2_BLOCK 32
#define CHI_BPM2_SLOTS 2032u

typedef struct {
    int16_t comb[CHI_HALL_COMB_N];
    int16_t ap0[CHI_HALL_AP0], ap1[CHI_HALL_AP1]; /* Q12 */
    float cdmp[4];
    uint16_t cpos[4], apos0, apos1;
    float fb, damp, wet, lvl;
} chi_hall_q_t;

typedef struct {
    int16_t buf[CHI_BPM2_SLOTS]; /* half-rate ring */
    uint32_t w;                  /* full-rate write count modulo 2*slots */
    float pair, fb, wet, delay;  /* delay in full-rate samples */
} chi_bpm2_t;

typedef union {
    chi_ens_t ens;
    chi_sdly_t sdly;
    chi_gaterev_t grev;
    chi_hall_q_t hall;
    chi_bpm2_t bpm;
} chi_fx2_insert_arena;

static inline int chi_fx2_insert_supports(unsigned type)
{
    return type == 3u || type == 12u || type == 13u || type == 15u || type == 16u;
}

static inline void chi_hall_q_process(chi_hall_q_t *h, float *buf, int n)
{
    static const uint16_t cl[4] = { CHI_HALL_C0, CHI_HALL_C1, CHI_HALL_C2, CHI_HALL_C3 };
    int16_t *cb[4];
    float wet = h->wet, dry_g, fb = h->fb, damp = h->damp, lvl = h->lvl;
    int k, c;
    if (wet <= 0.0f) return;
    dry_g = 1.0f - wet;
    cb[0] = h->comb; cb[1] = cb[0] + CHI_HALL_C0; cb[2] = cb[1] + CHI_HALL_C1; cb[3] = cb[2] + CHI_HALL_C2;
    for (k = 0; k < n; k++) {
        float x = buf[k], acc = 0.0f, y, bo;
        for (c = 0; c < 4; c++) {
            uint16_t p = h->cpos[c];
            float cy = (float)cb[c][p] * (1.0f / 2048.0f);
            acc += cy;
            h->cdmp[c] = cy + (h->cdmp[c] - cy) * damp;
            cb[c][p] = chi_hall_q12(x + h->cdmp[c] * fb);
            h->cpos[c] = (uint16_t)(p + 1u >= cl[c] ? 0u : p + 1u);
        }
        y = acc * 0.25f;
        bo = (float)h->ap0[h->apos0] * (1.0f / 2048.0f);
        h->ap0[h->apos0] = chi_hall_q12(y + bo * 0.5f);
        y = bo - y * 0.5f;
        h->apos0 = (uint16_t)(h->apos0 + 1u >= CHI_HALL_AP0 ? 0u : h->apos0 + 1u);
        bo = (float)h->ap1[h->apos1] * (1.0f / 2048.0f);
        h->ap1[h->apos1] = chi_hall_q12(y + bo * 0.5f);
        y = bo - y * 0.5f;
        h->apos1 = (uint16_t)(h->apos1 + 1u >= CHI_HALL_AP1 ? 0u : h->apos1 + 1u);
        buf[k] = dry_g * x + wet * y * lvl;
    }
}

static inline void chi_bpm2_set(chi_bpm2_t *d, uint8_t e1, uint8_t e2, int32_t bpm_times10)
{
    float bpm = (float)bpm_times10 * 0.1f, ms, samp, send;
    const float max_samp = (float)(2u * CHI_BPM2_SLOTS - 4u);
    if (bpm < 20.0f) bpm = 20.0f;
    if (bpm > 300.0f) bpm = 300.0f;
    ms = chi_bpmdly_beats((uint8_t)((uint32_t)e2 * 12u / 127u)) * (60000.0f / bpm);
    samp = ms * (CHI_FX2_SR * 0.001f);
    while (samp > max_samp) samp *= 0.5f; /* archived half-clamp */
    if (samp < 2.0f) samp = 2.0f;
    d->delay = samp;
    send = (float)e1 * (1.0f / 127.0f);
    d->wet = send < 0.5f ? 0.5f : send;
    d->fb = 0.35f + send * 0.5f;
}

static inline void chi_bpm2_process(chi_bpm2_t *d, float *buf, int n)
{
    const uint32_t span = 2u * CHI_BPM2_SLOTS;
    float wet = d->wet, dry_g, fb = d->fb;
    int k;
    if (wet <= 0.0f) return;
    dry_g = 1.0f - wet;
    for (k = 0; k < n; k++) {
        float x = buf[k], r, frac, a, b, delayed, tap;
        uint32_t i0;
        /* half-rate position of (w - delay); slot i holds samples 2i,2i+1 */
        r = ((float)d->w - d->delay) * 0.5f - 0.5f;
        while (r < 0.0f) r += (float)CHI_BPM2_SLOTS;
        i0 = (uint32_t)r;
        frac = r - (float)i0;
        i0 %= CHI_BPM2_SLOTS;
        a = (float)d->buf[i0] * (1.0f / 32767.0f);
        b = (float)d->buf[(i0 + 1u) % CHI_BPM2_SLOTS] * (1.0f / 32767.0f);
        delayed = a + (b - a) * frac;
        tap = x + fb * delayed;
        if (tap > 1.0f) tap = 1.0f; else if (tap < -1.0f) tap = -1.0f;
        if (d->w & 1u) d->buf[d->w >> 1] = chi_bpmdly_q15((d->pair + tap) * 0.5f);
        else d->pair = tap;
        d->w = (d->w + 1u) % span;
        buf[k] = dry_g * x + wet * delayed;
    }
}

static inline void chi_fx2_insert_init(chi_fx2_insert_arena *a, unsigned type)
{
    memset(a, 0, sizeof *a);
    switch (type) {
    case 3: chi_ens_init(&a->ens); break;
    case 12: chi_sdly_init(&a->sdly); break;
    case 16: chi_gaterev_init(&a->grev); break;
    case 15: a->hall.damp = 0.42f; a->hall.lvl = 0.67f; break;
    default: a->bpm.delay = 512.0f; break;
    }
}

static inline void chi_fx2_insert_set(chi_fx2_insert_arena *a, unsigned type, uint8_t e1, uint8_t e2, int32_t bpm_times10)
{
    switch (type) {
    case 3: chi_ens_set(&a->ens, e1, e2, CHI_FX2_SR); break;
    case 12: chi_sdly_set(&a->sdly, e1, e2, CHI_FX2_SR); break;
    case 13: chi_bpm2_set(&a->bpm, e1, e2, bpm_times10); break;
    case 15: {
        float send = (float)e1 * (1.0f / 127.0f);
        a->hall.fb = e2 < 20 ? 0.62f : 0.45f + (float)e2 * (1.0f / 127.0f) * 0.45f;
        a->hall.damp = 0.42f; a->hall.lvl = 0.67f;
        a->hall.wet = send < 0.5f ? 0.5f : send;
        break; }
    case 16: chi_gaterev_set(&a->grev, e1, e2, CHI_FX2_SR); break;
    default: break;
    }
}

static inline int16_t chi_fx2_s16(float x)
{
    x *= 32768.0f;
    if (x > 32767.0f) return 32767;
    if (x < -32768.0f) return -32768;
    return (int16_t)(x >= 0.0f ? x + 0.5f : x - 0.5f);
}

/* In place, stereo int16. Wet = mono insert(sum) - dry_g*sum, added to each
 * channel's own dry_g*input, so a centred voice renders like the archive. */
static inline void chi_fx2_insert_process(chi_fx2_insert_arena *a, unsigned type, int16_t *l, int16_t *r, uint32_t n)
{
    float m[CHI_FX2_BLOCK], d[CHI_FX2_BLOCK], wet;
    while (n) {
        uint32_t c = n > CHI_FX2_BLOCK ? CHI_FX2_BLOCK : n, i;
        for (i = 0; i < c; ++i) d[i] = m[i] = ((float)l[i] + (float)r[i]) * (0.5f / 32768.0f);
        switch (type) {
        case 3: wet = a->ens.wet; chi_ens_process(&a->ens, m, (int)c); break;
        case 12: wet = a->sdly.wet; chi_sdly_process(&a->sdly, m, (int)c); break;
        case 13: wet = a->bpm.wet; chi_bpm2_process(&a->bpm, m, (int)c); break;
        case 15: wet = a->hall.wet; chi_hall_q_process(&a->hall, m, (int)c); break;
        default: wet = 0.0f; chi_gaterev_process(&a->grev, m, (int)c); break;
        }
        if (type == 16u) { /* Gate Reverb filters its dry path: apply the mono result */
            for (i = 0; i < c; ++i) {
                float delta = m[i] - d[i];
                l[i] = chi_fx2_s16((float)l[i] * (1.0f / 32768.0f) + delta);
                r[i] = chi_fx2_s16((float)r[i] * (1.0f / 32768.0f) + delta);
            }
        } else if (wet > 0.0f) {
            float dry_g = 1.0f - wet;
            for (i = 0; i < c; ++i) {
                float w = m[i] - dry_g * d[i];
                l[i] = chi_fx2_s16((float)l[i] * (dry_g / 32768.0f) + w);
                r[i] = chi_fx2_s16((float)r[i] * (dry_g / 32768.0f) + w);
            }
        }
        l += c; r += c; n -= c;
    }
}

/* Exact idle span only when the selected ring is silent (no tail to lose). */
static inline int chi_fx2_insert_advance_zero(const chi_fx2_insert_arena *a, unsigned type, uint32_t n)
{
    const int16_t *p; uint32_t len, i;
    (void)n;
    switch (type) {
    case 3: p = a->ens.buf; len = CHI_ENS_MAX; break;
    case 12: p = a->sdly.buf; len = CHI_SDLY_MAX; break;
    case 13: p = a->bpm.buf; len = CHI_BPM2_SLOTS; if (a->bpm.pair != 0.0f) return 0; break;
    case 15: p = a->hall.comb; len = CHI_HALL_COMB_N + CHI_HALL_AP0 + CHI_HALL_AP1;
        for (i = 0; i < 4; ++i) if (a->hall.cdmp[i] != 0.0f) return 0;
        break;
    default: return 0; /* Gate Reverb: float allpass/envelope state, always process */
    }
    for (i = 0; i < len; ++i) if (p[i]) return 0;
    return 1;
}
#endif
