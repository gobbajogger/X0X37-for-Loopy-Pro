/* SPDX-License-Identifier: GPL-3.0-only */
/* Per-bus momentary FX and channel cutoff. One-pole filters and a decay gate,
 * small enough to share with the mixer. Reverb and delay throws are send levels
 * (busfx_send); the existing fxbus keeps the tail. Effect ids match smc_fx_map.h:
 * 0 gate, 1 reverb, 2 high-pass, 3 delay. */
#pragma once
#include "fastmath.h"

enum { BFX_GATE = 0, BFX_REV = 1, BFX_HP = 2, BFX_DLY = 3 };

typedef struct {
    float a;
    float lp[4];
    float hp[4];
    float hp_mix;
    float gate;
    float follow;
} busfx_t;

static inline void busfx_init(busfx_t *s)
{
    unsigned i;
    uint8_t *p = (uint8_t *)s;
    for (i = 0; i < sizeof *s; i++)
        p[i] = 0;
    s->a = 1.0f;
    s->gate = 1.0f;
}

/* Held reverb/delay throws the existing send to full. Release returns to the
 * saved send. The fxbus lines are left running, so the tail rings out. */
static inline float busfx_send(float saved, int held, int effect, int which)
{
    if (held && effect == which)
        return 1.0f;
    return saved < 0.0f ? 0.0f : saved;
}

/* Shape n samples on 1..4 buffers that belong to one bus.
 * held0 is the button at the first sample. press_at / release_at are indexes
 * inside the block, or n when that edge is not in this block. A press and a
 * release in the same block slew back to dry before the block ends. Cutoff 127
 * is a wire (sample-exact). Cutoff 0 closes the tone without a hard mute. */
static inline void busfx_apply(busfx_t *s, float *c0, float *c1, float *c2, float *c3,
                               int nch, int n, float cut, int held0, int effect,
                               int press_at, int release_at)
{
    float *ch[4];
    int i, k, held = held0 ? 1 : 0;
    const float slew = 0.0022651f;
    const float hp_c = 0.0395f;
    ch[0] = c0;
    ch[1] = c1;
    ch[2] = c2;
    ch[3] = c3;
    if (nch < 1)
        nch = 1;
    if (nch > 4)
        nch = 4;
    if (press_at < 0)
        press_at = n;
    if (release_at < 0)
        release_at = n;
    for (i = 0; i < n; i++) {
        float target, want_hp, peak = 0.0f;
        if (i == press_at)
            held = 1;
        if (i == release_at)
            held = 0;
        if (cut >= 126.5f)
            target = 1.0f;
        else {
            float fc = 40.0f * fm_exp2f(cut * (8.2f / 127.0f));
            target = 1.0f - fm_expf(-2.0f * FM_PI * fc / 44100.0f);
            if (target > 1.0f)
                target = 1.0f;
        }
        s->a += (target - s->a) * slew;
        if (fm_fabsf(target - s->a) < 1.0e-4f)
            s->a = target;
        want_hp = (held && effect == BFX_HP) ? 1.0f : 0.0f;
        s->hp_mix += (want_hp - s->hp_mix) * 0.01f;
        if (fm_fabsf(want_hp - s->hp_mix) < 1.0e-4f)
            s->hp_mix = want_hp;
        if (!(held && effect == BFX_GATE)) {
            s->gate += (1.0f - s->gate) * 0.02f;
            if (s->gate > 0.999f)
                s->gate = 1.0f;
        }
        for (k = 0; k < nch; k++) {
            float x = ch[k][i], y, hp, ax;
            if (s->a >= 0.999f) {
                s->lp[k] = x;
                y = x;
            } else {
                s->lp[k] += (x - s->lp[k]) * s->a;
                y = s->lp[k];
            }
            s->hp[k] += (y - s->hp[k]) * hp_c;
            hp = y - s->hp[k];
            if (s->hp_mix > 0.0f)
                y += (hp - y) * s->hp_mix;
            ax = fm_fabsf(y);
            if (ax > peak)
                peak = ax;
            ch[k][i] = y;
        }
        if (held && effect == BFX_GATE) {
            s->follow += (peak - s->follow) * 0.002f;
            if (peak > s->follow * 1.7f && peak > 0.02f)
                s->gate += (1.0f - s->gate) * 0.3f;
            else
                s->gate += (0.0f - s->gate) * 0.0012f;
            if (s->gate < 0.0f)
                s->gate = 0.0f;
            if (s->gate > 1.0f)
                s->gate = 1.0f;
        }
        if (s->gate < 1.0f)
            for (k = 0; k < nch; k++)
                ch[k][i] *= s->gate;
    }
}
