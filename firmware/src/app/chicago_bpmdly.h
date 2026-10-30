/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago Fx Type = BPM Delay — per-voice insert (not SEND / Mix delay theft).
 *
 * Factory BPM rows (Downer Arp, …) need a tempo-synced regenerative delay on the
 * dry 303 voice for strong-relative spectrum. AU compare previously rendered dry
 * native303 while BPM Delay only stole the shared Mix delay bus.
 *
 * Edit1 → wet (floor 0.5; former Mix send amount). Edit2 → note division 0..12
 * at the current sequencer tempo. The offline helper keeps CHI_BPMDLY_BPM
 * (default 170, AU compare tempo). Compact int16 ring; longer
 * divisions half until they fit (same law as fxbus). No libm. */
#pragma once
#include <stdint.h>
#include <string.h>
#include "chicago_fx.h"

#ifndef CHI_BPMDLY_SR
#define CHI_BPMDLY_SR 44100.0f
#endif

#ifndef CHI_BPMDLY_BPM
#define CHI_BPMDLY_BPM 170.0f
#endif

/* ~85 ms @ 48 kHz — covers 1/16..~1/8 at 170 after half-clamp; 2 voices ≈ 16 KB. */
#define CHI_BPMDLY_MAX 4096

typedef struct {
    int16_t buf[CHI_BPMDLY_MAX];
    uint16_t w;
    uint16_t delay;
    float fb;
    float wet;
} chi_bpmdly_t;

static inline int16_t chi_bpmdly_q15(float x)
{
    if (x > 1.0f)
        x = 1.0f;
    if (x < -1.0f)
        x = -1.0f;
    return (int16_t)(x * 32767.0f);
}

/* Same beat grid as fxbus DlTime 0..12. */
static inline float chi_bpmdly_beats(uint8_t div)
{
    static const float beats[13] = {
        0.125f, 1.0f / 6.0f, 0.25f, 1.0f / 3.0f, 0.375f, 0.5f, 2.0f / 3.0f,
        0.75f,  1.0f,        4.0f / 3.0f, 1.5f,  2.0f,  3.0f
    };
    if (div > 12)
        div = 12;
    return beats[div];
}

static inline void chi_bpmdly_set_bpm(chi_bpmdly_t *d, uint8_t e1, uint8_t e2, float sr, float bpm)
{
    float send, ms, samp, max_ms;
    uint8_t div;
    if (!d)
        return;
    if (sr < 1.0f)
        sr = CHI_BPMDLY_SR;
    if (bpm < 20.0f)
        bpm = 20.0f;
    if (bpm > 300.0f)
        bpm = 300.0f;
    div = (uint8_t)((uint32_t)e2 * 12u / 127u);
    ms = chi_bpmdly_beats(div) * (60000.0f / bpm);
    max_ms = (float)(CHI_BPMDLY_MAX - 1) * 1000.0f / sr;
    while (ms > max_ms)
        ms *= 0.5f;
    samp = ms * (sr * 0.001f);
    if (samp < 1.0f)
        samp = 1.0f;
    if (samp > (float)(CHI_BPMDLY_MAX - 1))
        samp = (float)(CHI_BPMDLY_MAX - 1);
    d->delay = (uint16_t)(samp + 0.5f);
    send = (float)e1 * (1.0f / 127.0f);
    d->wet = send < 0.5f ? 0.5f : send;
    d->fb = 0.35f + send * 0.5f;
}

/* Keep the fixed-tempo offline render/compare API unchanged. */
static inline void chi_bpmdly_set(chi_bpmdly_t *d, uint8_t e1, uint8_t e2, float sr)
{
    chi_bpmdly_set_bpm(d, e1, e2, sr, CHI_BPMDLY_BPM);
}

static inline void chi_bpmdly_init(chi_bpmdly_t *d)
{
    if (!d)
        return;
    memset(d->buf, 0, sizeof(d->buf));
    d->w = 0;
    d->delay = 512;
    d->fb = 0.0f;
    d->wet = 0.0f;
}

static inline void chi_bpmdly_process(chi_bpmdly_t *d, float *buf, int n)
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
    if (delay >= CHI_BPMDLY_MAX)
        delay = CHI_BPMDLY_MAX - 1;
    for (k = 0; k < n; k++) {
        float x = buf[k];
        float delayed, y, tap;
        r = (uint16_t)((w + CHI_BPMDLY_MAX - delay) % CHI_BPMDLY_MAX);
        delayed = (float)d->buf[r] * (1.0f / 32767.0f);
        tap = x + fb * delayed;
        if (tap > 1.0f)
            tap = 1.0f;
        else if (tap < -1.0f)
            tap = -1.0f;
        d->buf[w] = chi_bpmdly_q15(tap);
        w = (uint16_t)((w + 1u) % CHI_BPMDLY_MAX);
        y = dry_g * x + wet * delayed;
        buf[k] = y;
    }
    d->w = w;
}

static inline int chi_fx_is_bpmdly_insert(uint8_t type)
{
    return type == CHI_FX_BPM_DLY;
}
