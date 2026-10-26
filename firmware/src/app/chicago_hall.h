/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago Fx Type = Hall — per-voice insert (not SEND / Mix reverb theft).
 *
 * Factory Hall rows (Snail, …) need diffuse reverb colour on the dry 303 voice
 * for strong-relative spectrum. AU compare previously rendered dry native303
 * while Hall only stole the shared Mix reverb bus (and host render never ran
 * fxbus).
 *
 * Compact mono Schroeder: 4 int16 combs + 2 float allpasses (~2 KB/voice).
 * Edit1 → wet (floor 0.5; former Mix send). Edit2 → decay (low values floor
 * like the old SEND RvDec map). Fixed damp / return level. No libm. */
#pragma once
#include <stdint.h>
#include <string.h>
#include "chicago_fx.h"

#ifndef CHI_HALL_SR
#define CHI_HALL_SR 44100.0f
#endif

#define CHI_HALL_C0 379
#define CHI_HALL_C1 421
#define CHI_HALL_C2 457
#define CHI_HALL_C3 503
#define CHI_HALL_COMB_N (CHI_HALL_C0 + CHI_HALL_C1 + CHI_HALL_C2 + CHI_HALL_C3)
#define CHI_HALL_AP0 139
#define CHI_HALL_AP1 107

typedef struct {
    int16_t comb[CHI_HALL_COMB_N];
    float ap0[CHI_HALL_AP0];
    float ap1[CHI_HALL_AP1];
    float cdmp[4];
    uint16_t cpos[4];
    uint16_t apos0, apos1;
    float fb;
    float damp;
    float wet;
    float lvl;
} chi_hall_t;

static inline int16_t chi_hall_q12(float x)
{
    /* 12-bit sat toward zero — same density law as fxbus combs. */
    if (x > 7.999f)
        x = 7.999f;
    if (x < -8.0f)
        x = -8.0f;
    return (int16_t)(x * 2048.0f);
}

static inline void chi_hall_set(chi_hall_t *h, uint8_t e1, uint8_t e2, float sr)
{
    float send, dec;
    (void)sr;
    if (!h)
        return;
    send = (float)e1 * (1.0f / 127.0f);
    /* Former SEND: tone < 20 → RvDec pot 73 (mid-long). */
    if (e2 < 20)
        dec = 0.62f;
    else
        dec = 0.45f + (float)e2 * (1.0f / 127.0f) * 0.45f;
    h->fb = dec;
    h->damp = 0.42f;
    h->wet = send < 0.5f ? 0.5f : send;
    h->lvl = 0.67f; /* former fixed RvLvl ≈ 85/127 */
}

static inline void chi_hall_init(chi_hall_t *h)
{
    if (!h)
        return;
    memset(h, 0, sizeof(*h));
    h->fb = 0.0f;
    h->damp = 0.42f;
    h->wet = 0.0f;
    h->lvl = 0.67f;
}

/* In-place mono insert. wet=0 is a no-op. */
static inline void chi_hall_process(chi_hall_t *h, float *buf, int n)
{
    static const uint16_t cl[4] = { CHI_HALL_C0, CHI_HALL_C1, CHI_HALL_C2, CHI_HALL_C3 };
    float wet, dry_g, fb, damp, lvl;
    int16_t *cb[4];
    uint16_t cpos[4], a0, a1;
    float *ap0, *ap1;
    int k, c;
    if (!h || !buf || n <= 0)
        return;
    wet = h->wet;
    if (wet <= 0.0f)
        return;
    dry_g = 1.0f - wet;
    fb = h->fb;
    damp = h->damp;
    lvl = h->lvl;
    cb[0] = h->comb;
    cb[1] = cb[0] + CHI_HALL_C0;
    cb[2] = cb[1] + CHI_HALL_C1;
    cb[3] = cb[2] + CHI_HALL_C2;
    ap0 = h->ap0;
    ap1 = h->ap1;
    for (c = 0; c < 4; c++)
        cpos[c] = h->cpos[c];
    a0 = h->apos0;
    a1 = h->apos1;
    for (k = 0; k < n; k++) {
        float x = buf[k];
        float acc = 0.0f, y, bo;
        for (c = 0; c < 4; c++) {
            int16_t *b = cb[c];
            uint16_t p = cpos[c];
            float cy = (float)b[p] * (1.0f / 2048.0f);
            acc += cy;
            h->cdmp[c] = cy + (h->cdmp[c] - cy) * damp;
            b[p] = chi_hall_q12(x + h->cdmp[c] * fb);
            cpos[c] = (uint16_t)(p + 1u >= cl[c] ? 0u : p + 1u);
        }
        y = acc * 0.25f;
        bo = ap0[a0];
        ap0[a0] = y + bo * 0.5f;
        y = bo - y * 0.5f;
        a0 = (uint16_t)(a0 + 1u >= CHI_HALL_AP0 ? 0u : a0 + 1u);
        bo = ap1[a1];
        ap1[a1] = y + bo * 0.5f;
        y = bo - y * 0.5f;
        a1 = (uint16_t)(a1 + 1u >= CHI_HALL_AP1 ? 0u : a1 + 1u);
        buf[k] = dry_g * x + wet * y * lvl;
    }
    for (c = 0; c < 4; c++)
        h->cpos[c] = cpos[c];
    h->apos0 = a0;
    h->apos1 = a1;
}

static inline int chi_fx_is_hall_insert(uint8_t type)
{
    return type == CHI_FX_HALL;
}
