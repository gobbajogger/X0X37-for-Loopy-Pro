/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago Fx Type = Short Delay — per-voice insert (not SEND / Mix delay theft).
 *
 * Factory SDly rows (Feedback FX, Broken System FX, …) need a short regenerative
 * delay on the dry 303 voice for strong-relative spectrum. AU compare previously
 * rendered dry native303 while SDly only stole the shared Mix delay bus.
 *
 * Retune after #495: tip Edit1→time + hard fb left Broken System FX slightly
 * worse on strongRel. Fixed ~18 ms slap, Edit1→wet (floor 0.5), softer
 * Edit2→fb improves Feedback FX and Broken System FX together. No libm. */
#pragma once
#include <stdint.h>
#include <string.h>
#include "../dsp/fastmath.h"
#include "chicago_fx.h"

#ifndef CHI_SDLY_SR
#define CHI_SDLY_SR 44100.0f
#endif

/* ~32 ms @ 48 kHz; enough comb/feedback colour without a large pool hit. */
#define CHI_SDLY_MAX 1536

typedef struct {
    int16_t buf[CHI_SDLY_MAX];
    uint16_t w;     /* write index */
    uint16_t delay; /* samples 1..CHI_SDLY_MAX-1 */
    float fb;       /* feedback 0..~0.92 */
    float wet;      /* 0 dry .. 1 full insert */
} chi_sdly_t;

static inline int16_t chi_sdly_q15(float x)
{
    if (x > 1.0f)
        x = 1.0f;
    if (x < -1.0f)
        x = -1.0f;
    return (int16_t)(x * 32767.0f);
}

static inline void chi_sdly_set(chi_sdly_t *d, uint8_t e1, uint8_t e2, float sr)
{
    float send, f, samp;
    if (!d)
        return;
    if (sr < 1.0f)
        sr = CHI_SDLY_SR;
    /* Fixed ~18 ms slap — AU Short Delay is short, not BPM-synced. */
    samp = 0.018f * sr;
    if (samp < 1.0f)
        samp = 1.0f;
    if (samp > (float)(CHI_SDLY_MAX - 1))
        samp = (float)(CHI_SDLY_MAX - 1);
    d->delay = (uint16_t)(samp + 0.5f);
    send = (float)e1 * (1.0f / 127.0f);
    f = (float)e2 * (1.0f / 127.0f);
    /* Edit1 ≈ former Mix send amount; Edit2 ≈ regeneration. */
    d->wet = send < 0.5f ? 0.5f : send;
    d->fb = 0.35f + f * 0.5f;
}

static inline void chi_sdly_init(chi_sdly_t *d)
{
    if (!d)
        return;
    memset(d->buf, 0, sizeof(d->buf));
    d->w = 0;
    d->delay = 64;
    d->fb = 0.0f;
    d->wet = 0.0f;
}

/* In-place mono insert. wet=0 is a no-op. */
static inline void chi_sdly_process(chi_sdly_t *d, float *buf, int n)
{
    float wet, dry_g, fb;
    uint16_t w, delay, r;
    int k;
    if (!d || !buf || n <= 0)
        return;
    wet = d->wet;
    if (wet <= 0.0f)
        return;
    dry_g = 1.0f - wet;
    fb = d->fb;
    w = d->w;
    delay = d->delay;
    if (delay < 1)
        delay = 1;
    if (delay >= CHI_SDLY_MAX)
        delay = CHI_SDLY_MAX - 1;
    for (k = 0; k < n; k++) {
        float x = buf[k];
        float delayed, y, tap;
        r = (uint16_t)((w + CHI_SDLY_MAX - delay) % CHI_SDLY_MAX);
        delayed = (float)d->buf[r] * (1.0f / 32767.0f);
        tap = x + fb * delayed;
        /* Soft clip write so feedback stays bounded without libm. */
        if (tap > 1.0f)
            tap = 1.0f;
        else if (tap < -1.0f)
            tap = -1.0f;
        d->buf[w] = chi_sdly_q15(tap);
        w = (uint16_t)((w + 1u) % CHI_SDLY_MAX);
        y = dry_g * x + wet * delayed;
        buf[k] = y;
    }
    d->w = w;
}

static inline int chi_fx_is_sdly_insert(uint8_t type)
{
    return type == CHI_FX_SHORT_DLY;
}
