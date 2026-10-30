/* SPDX-License-Identifier: GPL-3.0-only */
/* Per-bus momentary FX and channel cutoff. One-pole filters, decay and tempo-synced
 * rhythm gates, stutter, and shuffle. Reverb and delay throws are send levels
 * (busfx_send); the existing fxbus keeps the tail. Effect ids match smc_fx_map.h. */
#pragma once
#include "fastmath.h"

enum {
    BFX_GATE = 0,
    BFX_REV = 1,
    BFX_HP = 2,
    BFX_DLY = 3,
    BFX_LP = 4,
    BFX_GATE8 = 5,
    BFX_GATE16 = 6,
    BFX_TRANCE = 7,
    BFX_STUT = 8,
    BFX_SHUF = 9,
    BFX_NFX = 10
};

#define BFX_RING 2u
#define BFX_STUT_CH 12u
#define BFX_SEND_NOM 0.4f
#define BFX_STUT_SCALE 32767.0f

typedef struct {
    int16_t ring[BFX_STUT_CH][BFX_RING];
    uint8_t ring_w[BFX_STUT_CH];
} busfx_stut_hub_t;

typedef busfx_stut_hub_t busfx_stut_t;

/* One state shape for 1..4 channels (send throws use nch=1, master filter nch=2,
 * drum buses up to 4). Sharing one apply body beats three near-copies. */
typedef struct {
    float a;
    float lp[4];
    float hp[4];
    float tone_lp[4];
    float hp_mix;
    float lp_mix;
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

static inline void busfx_stut_init(busfx_stut_t *st)
{
    unsigned i;
    uint8_t *p;
    if (!st)
        return;
    p = (uint8_t *)st;
    for (i = 0; i < sizeof *st; i++)
        p[i] = 0;
}

static inline int busfx_is_decay_gate(int effect) { return effect == BFX_GATE; }

static inline int busfx_is_rhythm(int effect)
{
    return effect == BFX_GATE8 || effect == BFX_GATE16 || effect == BFX_TRANCE || effect == BFX_STUT ||
           effect == BFX_SHUF;
}

static inline int busfx_settling(const busfx_t *s)
{
    return s->gate < 0.9995f || s->hp_mix > 0.002f || s->lp_mix > 0.002f || s->a < 0.9995f;
}

static inline float busfx_bpm_clamp(float bpm)
{
    if (bpm < 20.0f)
        bpm = 20.0f;
    if (bpm > 300.0f)
        bpm = 300.0f;
    return bpm;
}

static inline uint32_t busfx_sp16(float bpm)
{
    bpm = busfx_bpm_clamp(bpm);
    return (uint32_t)(44100.0f * 15.0f / bpm + 0.5f);
}

static inline uint32_t busfx_sp8(float bpm)
{
    return busfx_sp16(bpm) * 2u;
}

static inline float busfx_rhythm_target(int effect, float bpm, uint32_t abs_s)
{
    uint32_t sp16 = busfx_sp16(bpm), sp8 = busfx_sp8(bpm), period, pos, sixteenth;
    float ph, duty = 0.5f;
    if (sp16 < 32u)
        sp16 = 32u;
    if (effect == BFX_GATE8) {
        period = sp8;
        duty = 0.5f;
    } else if (effect == BFX_GATE16 || effect == BFX_STUT) {
        period = sp16;
        duty = (effect == BFX_STUT) ? 0.22f : 0.5f;
    } else if (effect == BFX_TRANCE) {
        period = sp16;
        duty = 0.38f;
    } else if (effect == BFX_SHUF) {
        sixteenth = abs_s / sp16;
        pos = abs_s % sp16;
        ph = (float)pos / (float)sp16;
        if (sixteenth & 1u)
            ph -= 0.34f;
        if (ph < 0.0f)
            ph = 0.0f;
        return ph < 0.48f ? 1.0f : 0.0f;
    } else
        return 1.0f;
    pos = abs_s % period;
    ph = (float)pos / (float)period;
    return ph < duty ? 1.0f : 0.0f;
}

static inline void busfx_ring_write(busfx_stut_t *st, int ch, float x)
{
    uint8_t w;
    float s;
    if (!st || ch < 0 || ch >= (int)BFX_STUT_CH)
        return;
    w = st->ring_w[ch];
    s = x * BFX_STUT_SCALE;
    if (s > 32767.0f)
        s = 32767.0f;
    else if (s < -32768.0f)
        s = -32768.0f;
    st->ring[ch][w] = (int16_t)s;
    st->ring_w[ch] = (uint8_t)((w + 1u) % BFX_RING);
}

static inline float busfx_ring_read(const busfx_stut_t *st, int ch, uint8_t back)
{
    int idx;
    if (!st || ch < 0 || ch >= (int)BFX_STUT_CH)
        return 0.0f;
    idx = (int)st->ring_w[ch] - 1 - (int)back;
    while (idx < 0)
        idx += (int)BFX_RING;
    return (float)st->ring[ch][(unsigned)idx] * (1.0f / BFX_STUT_SCALE);
}

static inline float busfx_send(float saved, int held, int effect, int which)
{
    if (held && effect == which)
        return 1.0f;
    return saved < 0.0f ? 0.0f : saved;
}

/* One body for every bus width — callers must not inline three copies. */
static __attribute__((noinline)) void busfx_apply(busfx_t *s, busfx_stut_t *stut, int stut_base, float *c0,
                                                  float *c1, float *c2, float *c3, int nch, int n, float cut,
                                                  int held0, int effect, int press_at, int release_at, float bpm,
                                                  uint32_t sample0)
{
    float *ch[4];
    int i, k, held = held0 ? 1 : 0;
    const float slew = 0.0022651f;
    const float hp_c = 0.0395f;
    const float rg_a = 0.35f;
    const float rg_r = 0.18f;
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
    bpm = busfx_bpm_clamp(bpm);
    for (i = 0; i < n; i++) {
        float target, want_hp, want_lp, peak = 0.0f, rg_tgt = 1.0f;
        uint32_t abs_s = sample0 + (uint32_t)i;
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
        want_lp = (held && effect == BFX_LP) ? 1.0f : 0.0f;
        s->lp_mix += (want_lp - s->lp_mix) * 0.01f;
        if (fm_fabsf(want_lp - s->lp_mix) < 1.0e-4f)
            s->lp_mix = want_lp;
        if (held && busfx_is_rhythm(effect))
            rg_tgt = busfx_rhythm_target(effect, bpm, abs_s);
        if (!(held && (busfx_is_decay_gate(effect) || busfx_is_rhythm(effect)))) {
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
            s->tone_lp[k] += (x - s->tone_lp[k]) * 0.008f;
            hp = y - s->hp[k];
            if (s->hp_mix > 0.0f)
                y += (hp - y) * s->hp_mix;
            if (s->lp_mix > 0.0f)
                y += (s->tone_lp[k] - y) * s->lp_mix;
            ax = fm_fabsf(y);
            if (ax > peak)
                peak = ax;
            if (held && effect == BFX_STUT && stut) {
                int ring_ch = stut_base + k;
                uint32_t sp16 = busfx_sp16(bpm);
                uint32_t pos = abs_s % sp16;
                uint32_t chunk = sp16 / 4u;
                if (chunk < 8u)
                    chunk = 8u;
                busfx_ring_write(stut, ring_ch, y);
                if (pos % chunk < chunk / 2u)
                    y = busfx_ring_read(stut, ring_ch, (uint8_t)(pos & (BFX_RING - 1u)));
            }
            ch[k][i] = y;
        }
        if (held && busfx_is_decay_gate(effect)) {
            s->follow += (peak - s->follow) * 0.002f;
            if (peak > s->follow * 1.7f && peak > 0.02f)
                s->gate += (1.0f - s->gate) * 0.3f;
            else
                s->gate += (0.0f - s->gate) * 0.0012f;
            if (s->gate < 0.0f)
                s->gate = 0.0f;
            if (s->gate > 1.0f)
                s->gate = 1.0f;
        } else if (held && busfx_is_rhythm(effect)) {
            float step = rg_tgt > s->gate ? rg_a : rg_r;
            s->gate += (rg_tgt - s->gate) * step;
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
