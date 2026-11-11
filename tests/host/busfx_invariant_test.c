/* SPDX-License-Identifier: GPL-3.0-only */
/* Bit-identical bus FX. busfx_apply_ref and busfx_rhythm_target_ref are the parent
 * functions. The apply reference calls the parent rhythm helper, not the new wrapper.
 * The mutant uses the sixteenth period for eighth-note gates and must not match. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "busfx.h"

static inline float busfx_rhythm_target_ref(int effect, float bpm, uint32_t abs_s)
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

static __attribute__((noinline)) void busfx_apply_ref(busfx_t *s, busfx_stut_t *stut, int stut_base, float *c0,
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
            rg_tgt = busfx_rhythm_target_ref(effect, bpm, abs_s);
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

static __attribute__((noinline)) void busfx_apply_mutant(busfx_t *s, busfx_stut_t *stut, int stut_base, float *c0,
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
    if (n <= 0)
        return;
    /* Arguments are fixed for this call; keep every sample recurrence below. */
    float target;
    if (cut >= 126.5f)
        target = 1.0f;
    else {
        float fc = 40.0f * fm_exp2f(cut * (8.2f / 127.0f));
        target = 1.0f - fm_expf(-2.0f * FM_PI * fc / 44100.0f);
        if (target > 1.0f)
            target = 1.0f;
    }
    uint32_t sp16 = busfx_is_rhythm(effect) ? busfx_sp16(bpm) : 0u;
    for (i = 0; i < n; i++) {
        float want_hp, want_lp, peak = 0.0f, rg_tgt = 1.0f;
        uint32_t abs_s = sample0 + (uint32_t)i;
        if (i == press_at)
            held = 1;
        if (i == release_at)
            held = 0;
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
            rg_tgt = busfx_rhythm_target_periods(effect, sp16, sp16, abs_s); /* mutant */
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

#define MAXN 256
static int mutant;
static int fails;
static unsigned cases;
static int saw_lo, saw_hi, saw_rhythm, saw_stut, saw_nch4, saw_blocks;

static int same_bits(const void *a, const void *b, unsigned n, const char *what)
{
    if (!n || !memcmp(a, b, n))
        return 1;
    if (!fails)
        printf("mismatch %s (%u bytes)\n", what, n);
    fails++;
    return 0;
}

static void prep_state(busfx_t *s, int dirty)
{
    unsigned i;
    if (!dirty) {
        busfx_init(s);
        return;
    }
    memset(s, 0, sizeof *s);
    s->a = 0.35f;
    s->gate = 0.55f;
    s->hp_mix = 0.2f;
    s->lp_mix = 0.15f;
    s->follow = 0.08f;
    for (i = 0; i < 4u; i++) {
        s->lp[i] = 0.01f * (float)(i + 1u);
        s->hp[i] = -0.02f * (float)(i + 1u);
        s->tone_lp[i] = 0.03f * (float)(i + 1u);
    }
}

static void prep_stut(busfx_stut_t *st)
{
    unsigned ch, k;
    busfx_stut_init(st);
    for (ch = 0; ch < BFX_STUT_CH; ch++) {
        st->ring_w[ch] = (uint8_t)(ch & 1u);
        for (k = 0; k < BFX_RING; k++)
            st->ring[ch][k] = (int16_t)(1000 + (int)ch * 17 - (int)k * 40);
    }
}

static void fill_in(float *x, int n, int seed)
{
    int i;
    for (i = 0; i < n; i++) {
        int v = (seed * 17 + i * 13) % 2000;
        x[i] = (float)(v - 1000) * 0.001f;
    }
}

static void note_case(float cut, int effect, int nch, int n)
{
    if (cut < 126.5f)
        saw_lo = 1;
    else
        saw_hi = 1;
    if (busfx_is_rhythm(effect))
        saw_rhythm = 1;
    if (effect == BFX_STUT)
        saw_stut = 1;
    if (nch >= 4)
        saw_nch4 = 1;
    if (n == 0 || n == 1 || n == 7 || n == 16 || n == 64 || n == 256)
        saw_blocks = 1;
}

static int one_apply(float cut, float bpm, int effect, int nch, int n, int held0,
                     int press_at, int release_at, uint32_t sample0, int use_stut,
                     int stut_base, int dirty)
{
    busfx_t a, b;
    busfx_stut_t sa, sb;
    float in[4][MAXN], oa[4][MAXN], ob[4][MAXN];
    int c, samples;
    char what[96];
    if (fails)
        return 0;
    if (n > MAXN)
        return 0;
    samples = n > 0 ? n : 0;
    prep_state(&a, dirty);
    b = a;
    prep_stut(&sa);
    sb = sa;
    for (c = 0; c < 4; c++) {
        fill_in(in[c], samples, 3 + c * 9 + effect);
        memcpy(oa[c], in[c], (unsigned)samples * sizeof(float));
        memcpy(ob[c], in[c], (unsigned)samples * sizeof(float));
    }
    note_case(cut, effect, nch, n);
    cases++;
    if (mutant)
        busfx_apply_mutant(&a, use_stut ? &sa : 0, stut_base, oa[0], oa[1], oa[2], oa[3],
                           nch, n, cut, held0, effect, press_at, release_at, bpm, sample0);
    else
        busfx_apply(&a, use_stut ? &sa : 0, stut_base, oa[0], oa[1], oa[2], oa[3],
                    nch, n, cut, held0, effect, press_at, release_at, bpm, sample0);
    busfx_apply_ref(&b, use_stut ? &sb : 0, stut_base, ob[0], ob[1], ob[2], ob[3],
                    nch, n, cut, held0, effect, press_at, release_at, bpm, sample0);
    snprintf(what, sizeof what, "fx %d cut %.4f bpm %.2f nch %d n %d held %d s0 %u",
             effect, (double)cut, (double)bpm, nch, n, held0, sample0);
    if (!same_bits(&a, &b, sizeof a, what))
        return 0;
    if (use_stut && !same_bits(&sa, &sb, sizeof sa, what))
        return 0;
    for (c = 0; c < 4; c++) {
        unsigned nbytes = (unsigned)samples * sizeof(float);
        if (!same_bits(oa[c], ob[c], nbytes, what))
            return 0;
        if (c >= nch && c >= 1 && nbytes && memcmp(oa[c], in[c], nbytes)) {
            if (!fails)
                printf("channel %d changed with nch %d\n", c, nch);
            fails++;
            return 0;
        }
    }
    return 1;
}

static int grid(void)
{
    static const float cuts_init[] = {0.0f, 40.0f, 64.0f, 100.0f, 126.0f, 126.5f, 127.0f, 200.0f};
    static const float bpms[] = {19.0f, 20.0f, 120.0f, 173.5f, 300.0f, 301.0f};
    static const int ns[] = {0, 1, 7, 16, 64};
    static const int nchs[] = {1, 2, 3, 4};
    float cuts[9];
    unsigned ci, bi, ei, ni, hi, pi, si, di;
    union { float f; uint32_t u; } bits;
    memcpy(cuts, cuts_init, sizeof cuts_init);
    bits.f = 126.5f;
    bits.u -= 1u;
    cuts[8] = bits.f;
    for (di = 0; di < 2u && !fails; di++)
        for (ei = 0; ei < (unsigned)BFX_NFX && !fails; ei++)
            for (ci = 0; ci < sizeof cuts / sizeof cuts[0] && !fails; ci++)
                for (bi = 0; bi < sizeof bpms / sizeof bpms[0] && !fails; bi++)
                    for (ni = 0; ni < sizeof ns / sizeof ns[0] && !fails; ni++)
                        for (hi = 0; hi < sizeof nchs / sizeof nchs[0] && !fails; hi++)
                            for (pi = 0; pi < 2u && !fails; pi++) {
                                int n = ns[ni];
                                int press = pi ? (n > 1 ? 1 : 0) : -1;
                                int release = pi && n > 4 ? n / 2 : -1;
                                for (si = 0; si < 3u && !fails; si++) {
                                    uint32_t s0 = si == 0u ? 0u :
                                                  si == 1u ? 0xFFFFFFF0u :
                                                  busfx_sp16(bpms[bi]) / 2u + 8u;
                                    int use_stut = (ei == (unsigned)BFX_STUT) ? (int)si : 0;
                                    if (!one_apply(cuts[ci], bpms[bi], (int)ei, nchs[hi], n,
                                                   pi ? 0 : 1, press, release, s0, use_stut, 0, (int)di))
                                        return 0;
                                }
                            }
    /* Longer blocks, every channel count, stutter ring base, and a null upper channel. */
    for (ei = 0; ei < (unsigned)BFX_NFX && !fails; ei++) {
        if (!one_apply(40.0f, 120.0f, (int)ei, 4, 256, 1, 8, 200, 1000u, ei == (unsigned)BFX_STUT, 4, 1))
            return 0;
        if (!one_apply(126.5f, 96.0f, (int)ei, 2, 256, 0, -1, -1, busfx_sp16(96.0f) - 1u, 0, 0, 0))
            return 0;
    }
    if (!one_apply(10.0f, 120.0f, BFX_LP, 1, 32, 1, -1, -1, 0u, 0, 0, 1))
        return 0;
    if (!one_apply(80.0f, 120.0f, BFX_STUT, 4, 64, 1, 0, -1, 50u, 1, 10, 1))
        return 0;
    if (!one_apply(0.0f, 120.0f, BFX_GATE, 0, 16, 1, -1, -1, 0u, 0, 0, 1))
        return 0;
    if (!one_apply(127.0f, 120.0f, BFX_HP, 5, 16, 1, -1, -1, 0u, 0, 0, 1))
        return 0;
    if (!one_apply(40.0f, 120.0f, BFX_GATE8, 1, -1, 1, -1, -1, 0u, 0, 0, 1))
        return 0;
    return fails == 0;
}

static int rhythm_ok(void)
{
    static const float bpms[] = {20.0f, 96.5f, 120.0f, 300.0f};
    int effect, b;
    for (effect = 0; effect < BFX_NFX && !fails; effect++) {
        for (b = 0; b < 4 && !fails; b++) {
            uint32_t sp = busfx_sp16(bpms[b]);
            uint32_t step = sp / 16u;
            uint32_t abs, p;
            uint32_t pts[8];
            if (step < 1u)
                step = 1u;
            for (abs = 0; abs < sp * 4u && !fails; abs += step) {
                float x = busfx_rhythm_target(effect, bpms[b], abs);
                float y = busfx_rhythm_target_ref(effect, bpms[b], abs);
                cases++;
                if (memcmp(&x, &y, sizeof x)) {
                    printf("rhythm mismatch fx %d abs %u\n", effect, abs);
                    fails++;
                    return 0;
                }
            }
            pts[0] = 0u;
            pts[1] = 1u;
            pts[2] = sp - 1u;
            pts[3] = sp;
            pts[4] = sp + 1u;
            pts[5] = sp * 2u;
            pts[6] = 0xFFFFFFF0u;
            pts[7] = 0xFFFFFFFFu;
            for (p = 0; p < 8u && !fails; p++) {
                float x = busfx_rhythm_target(effect, bpms[b], pts[p]);
                float y = busfx_rhythm_target_ref(effect, bpms[b], pts[p]);
                cases++;
                if (memcmp(&x, &y, sizeof x)) {
                    printf("rhythm boundary fx %d abs %u\n", effect, pts[p]);
                    fails++;
                    return 0;
                }
            }
        }
    }
    return fails == 0;
}

static int moved(void)
{
    busfx_t s, init;
    float x[64];
    int i;
    prep_state(&init, 1);
    s = init;
    for (i = 0; i < 64; i++)
        x[i] = 0.25f;
    busfx_apply(&s, 0, 0, x, 0, 0, 0, 1, 64, 0.0f, 0, BFX_LP, -1, -1, 120.0f, 0u);
    if (!memcmp(&s, &init, sizeof s)) {
        puts("apply left the filter state untouched");
        return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    mutant = argc > 1 && argv[1] && !strcmp(argv[1], "mutant");
    if (mutant) {
        puts("MUTANT busfx-invariants");
        /* Past the wrong eighth-note edge, still inside the real one. */
        one_apply(40.0f, 120.0f, BFX_GATE8, 1, 128, 1, -1, -1,
                  busfx_sp16(120.0f) / 2u + 8u, 0, 0, 1);
        if (fails) {
            puts("RED busfx-invariants: mutant output differs from the old loop");
            return 1;
        }
        puts("RED busfx-invariants: mutant matched the old loop");
        return 1;
    }
    if (!moved() || !rhythm_ok() || !grid()) {
        printf("RED busfx-invariants: %d mismatches\n", fails ? fails : 1);
        return 1;
    }
    if (!saw_lo || !saw_hi || !saw_rhythm || !saw_stut || !saw_nch4 || !saw_blocks) {
        printf("RED busfx-invariants: coverage lo %d hi %d rhythm %d stut %d nch4 %d blocks %d\n",
               saw_lo, saw_hi, saw_rhythm, saw_stut, saw_nch4, saw_blocks);
        return 1;
    }
    printf("GREEN busfx-invariants: bit-identical samples and filter state (%u cases)\n", cases);
    return 0;
}
