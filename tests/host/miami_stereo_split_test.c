/* SPDX-License-Identifier: GPL-3.0-only */
/* Miami product: sub stays mono; stereo width applies above Bass Split. */
#include "../../experiments/wobble/wobble.h"
#include "dsp/fastmath.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

enum { SR = 48000, N = SR / 2 };

static float l[N], r[N], lm[N], rm[N];

static void apply_bass_patch(wobble_t *s)
{
    wobble_set(s, W_SUB_LEVEL, 1.0f);
    wobble_set(s, W_CARRIER_LEVEL, 0.15f);
    wobble_set(s, W_CUTOFF, 1.0f);
    wobble_set(s, W_BASS_SPLIT, 0.285f);
}

static void render_blocks(wobble_t *s, float *ol, float *or, unsigned frames)
{
    for (unsigned p = 0; p < frames; ) {
        unsigned chunk = 256;
        if (p + chunk > frames)
            chunk = frames - p;
        wobble_render(s, ol + p, or + p, chunk);
        p += chunk;
    }
}

static void render_note(wobble_t *s, float *ol, float *or)
{
    wobble_note_on(s, 36, 100);
    render_blocks(s, ol, or, N);
    wobble_note_off(s, 36);
    render_blocks(s, ol, or, SR / 8);
}

static double corr_band(const float *xl, const float *xr, unsigned i0, unsigned i1, float lp_hz)
{
    double sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
    float lx = 0, ly = 0, ax = expf(-2.0f * (float)M_PI * lp_hz / SR);
    for (unsigned i = i0; i < i1; ++i) {
        lx = ax * lx + (1.0f - ax) * xl[i];
        ly = ax * ly + (1.0f - ax) * xr[i];
        sx += lx;
        sy += ly;
        sxx += lx * lx;
        syy += ly * ly;
        sxy += lx * ly;
    }
    double n = (double)(i1 - i0);
    double vx = sxx - sx * sx / n, vy = syy - sy * sy / n, cov = sxy - sx * sy / n;
    if (vx <= 0 || vy <= 0)
        return 1.0;
    return cov / sqrt(vx * vy);
}

static double side_rms(const float *xl, const float *xr, unsigned i0, unsigned i1, float hp_hz)
{
    double e = 0;
    float hp = 0, prev = 0;
    float a = expf(-2.0f * (float)M_PI * hp_hz / SR);
    for (unsigned i = i0; i < i1; ++i) {
        float d = xl[i] - xr[i];
        hp = a * (hp + d - prev);
        prev = d;
        e += hp * hp;
    }
    return sqrt(e / (double)(i1 - i0));
}

int main(void)
{
    wobble_t s, mono;
    int fails = 0;
    const unsigned i0 = 8000, i1 = 20000;
    float hz_def = wobble_bass_split_hz(0.285f);

    if (fabsf(wobble_bass_split_hz(0.0f) - 60.0f) > 0.5f
        || fabsf(wobble_bass_split_hz(1.0f) - 250.0f) > 1.0f)
        fails++, puts("FAIL split Hz endpoints");
    if (hz_def < 80.0f || hz_def > 100.0f)
        fails++, printf("FAIL default split %.2f Hz (want 80-100)\n", hz_def);
    if (fabsf(wobble_bass_split_alpha(0.0f) - 1.0f) > 0.02f
        || fabsf(wobble_bass_split_alpha(1.0f)) > 0.02f)
        fails++, puts("FAIL slope alpha endpoints");

    wobble_init(&s, (float)SR);
    apply_bass_patch(&s);
    wobble_set(&s, W_STEREO, 1.0f);
    render_note(&s, l, r);

    if (corr_band(l, r, i0, i1, 45.0f) < 0.995)
        fails++, printf("FAIL sub correlation %.4f\n", corr_band(l, r, i0, i1, 45.0f));
    if (side_rms(l, r, i0, i1, 150.0f) < 1e-5)
        fails++, puts("FAIL no stereo body side energy");

    wobble_init(&s, (float)SR);
    apply_bass_patch(&s);
    wobble_set(&s, W_STEREO, 1.0f);
    wobble_set(&s, W_BASS_SPLIT, 0.0f);
    render_note(&s, l, r);
    double side_lo = side_rms(l, r, i0, i1, 150.0f);

    wobble_init(&s, (float)SR);
    apply_bass_patch(&s);
    wobble_set(&s, W_STEREO, 1.0f);
    wobble_set(&s, W_BASS_SPLIT, 1.0f);
    render_note(&s, lm, rm);
    double side_hi = side_rms(lm, rm, i0, i1, 150.0f);
    if (fabs(side_lo - side_hi) < side_lo * 0.08)
        fails++, printf("FAIL split knob moves body width (side %.6g vs %.6g)\n", side_lo, side_hi);

    wobble_init(&s, (float)SR);
    apply_bass_patch(&s);
    wobble_set(&s, W_STEREO, 1.0f);
    render_note(&s, l, r);
    wobble_init(&mono, (float)SR);
    apply_bass_patch(&mono);
    wobble_set(&mono, W_STEREO, 0.0f);
    render_note(&mono, lm, rm);
    for (unsigned i = i0; i < i1; ++i) {
        if (fabsf(0.5f * (l[i] + r[i]) - lm[i]) > 2e-4f) {
            fails++, printf("FAIL mono downmix at %u\n", i);
            break;
        }
    }

    if (fails)
        printf("miami_stereo_split: %d FAILED\n", fails);
    else
        puts("PASS miami stereo split: sub centred, body width, knob law, downmix");
    return fails ? 1 : 0;
}
