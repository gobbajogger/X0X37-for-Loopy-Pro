/* SPDX-License-Identifier: GPL-3.0-only */
/* Generator ported from schwung-breakbeat (BB Gen) by mestela, used with the author's permission.
 *
 * BREAK part: an 8-slice ADPCM loop player driven by BB Gen's generator.
 *
 * What is BB Gen's, kept as its code has it (src/dsp/slice_select.c, perf.c, breakbeat.c):
 *   - slice choice per trigger: Complexity 0 bypasses the generator and plays the trigger's
 *     beat position; otherwise a MOVE branch (probability 1 - Roll: swap to a uniform random
 *     slice with probability Complexity * weight(beat position, Anchor), else play the beat
 *     position) or a STAY branch (5 % escape jump of 2..4 forward, else repeat with probability
 *     1 - weight(current slice, Anchor), else walk +-1). Same draw order.
 *   - the Anchor weight curve {0, .5, 1, .7, 0, .5, 1, 1.2}, linearly blended from 1.0.
 *   - fill bar = last bar of a phrase: Complexity -> c + (1 - c) * Fill, Anchor and Roll * (1 - Fill).
 *   - trigger cadence: one trigger every Length / 8 bars (BB Gen's 12 * length clock ticks);
 *     beat position = trigger count mod 8, the count restarting at transport start and at a bank
 *     switch, where slice 0 is forced.
 *   - retrigger: each of 2x/3x/4x/8x rolls per trigger with p = 1 - (1 - P)^(1 / triggers per bar),
 *     one of the fired rates is picked at random; the slice's head is replayed div times.
 *   - phrase A/B: B is rolled (B chance) at the start of the bar before the fill bar and switched
 *     in at the fill bar's start; A is always scheduled back for the first bar of the next phrase.
 *     (X0X: with Phrase off, B chance rolls at the start of every bar, for that bar.)
 *   - live layer: held slices on a last-note-priority stack (dedup on re-press), held slice beats
 *     the engine (the engine still draws), reverse wraps within the slice, jungle repeats short grains at unchanged pitch
 *     while sequencer timing stays unchanged; stutter forces a 4x retrigger on each trigger.
 *
 * What is adapted (the reasons are the FM-1's, see the design doc):
 *   - time comes from breaks_step (every 16th) instead of MIDI clock ticks; the 1/4-bar length's
 *     second trigger inside a 16th is scheduled half a 16th later.
 *   - playback rate follows the loop's stated `bars` (source samples per 16th / output samples per
 *     16th) times Pitch, instead of BB Gen's slice length / trigger interval. The two agree when
 *     A Length equals the loop's bars; otherwise Length only sets how often slices are triggered.
 *   - retrigger sub-hits are timed in output samples (interval / div) instead of source samples,
 *     so they stay on the beat whatever Pitch is; in reverse the reversed head repeats.
 *   - every jump (trigger, sub-hit, reverse flip, wrap at the loop end is seamless) crossfades two
 *     voices over 2 ms; BB Gen jumps play_pos and clicks.
 *   - the pattern's step bit gates the output (2 ms ramp); the generator runs underneath, so a
 *     re-enabled step resumes exactly where the break is. A held slice key opens the gate.
 *   - transport start returns the engine to bank A; randomness is one xorshift32 seeded in init.
 *
 * IMA ADPCM tables and decoder step: ima_adpcm.h (from Felucca's eng_sample.c,
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hugelton Instruments, GPL-3.0). */
#include "breaks.h"
#include "../app/plat.h"
#if !defined(X0X_HOST) && __SIZEOF_POINTER__ == 4
#define brk_config_lock plat_cq_lock
#define brk_config_unlock plat_cq_unlock
#else
static uint32_t brk_config_lock(void) { return 0; }
static void brk_config_unlock(uint32_t state) { (void)state; }
#endif
#include "../x0x_cold.h"          /* X0X_COLD: breaks_init is boot only, not the render */

/* Build-time A/B: 0 runs original per-sample reverse advancement. */
#ifndef BRK_FAST_REV_SKIP
#define BRK_FAST_REV_SKIP 1
#endif
#include "fastmath.h"
#include "ima_adpcm.h"

/* ---- parameters ------------------------------------------------------------------------- */

static const char *const N_PHRASE[] = {"Off", "2", "4", "8", "16"};
static const char *const N_LEN[] = {"1/4", "1/2", "1", "2", "4", "8"};
static const char *const N_PITCH[] = {"-12", "-11", "-10", "-9", "-8", "-7", "-6", "-5", "-4", "-3", "-2",
                                      "-1", "0", "+1", "+2", "+3", "+4", "+5", "+6", "+7", "+8", "+9",
                                      "+10", "+11", "+12"};
/* defaults are BB Gen's module.json defaults */
static const x0x_param_t BRK_PARAMS[BRK_NPARAM] = {
    {"Cmplx", 100, 50, 0},  {"Anchor", 100, 0, 0}, {"Roll", 100, 0, 0},   {"Fill", 100, 0, 0},
    {"Rtg2x", 100, 0, 0},   {"Rtg3x", 100, 0, 0},  {"Rtg4x", 100, 0, 0},  {"Rtg8x", 100, 0, 0},
    {"Phrase", 4, 0, N_PHRASE}, {"BChnc", 100, 0, 0}, {"A Len", 5, 2, N_LEN}, {"B Len", 5, 2, N_LEN},
    {"Level", 127, 100, 0}, {"Pitch", 24, 12, N_PITCH}, {"Rev", 127, 0, 0}, {"Dly", 127, 0, 0},
};
static const uint8_t PHRASE_BARS[5] = {0, 2, 4, 8, 16};
static const uint8_t LEN_TICKS[6] = {3, 6, 12, 24, 48, 96};   /* BB Gen: 12 * length, 24 ppq */

int breaks_nparams(void) { return BRK_NPARAM; }
const x0x_param_t *breaks_param(int i) { return (i >= 0 && i < BRK_NPARAM) ? &BRK_PARAMS[i] : 0; }
void breaks_set(breaks_t *b, int i, int value)
{
    if (i < 0 || i >= BRK_NPARAM)
        return;
    if (value < 0)
        value = 0;
    if (value > BRK_PARAMS[i].max)
        value = BRK_PARAMS[i].max;
    b->set[i] = (uint8_t)value;
}
int breaks_get(const breaks_t *b, int i) { return (i >= 0 && i < BRK_NPARAM) ? b->set[i] : 0; }

/* ---- randomness ------------------------------------------------------------------------- */

__attribute__((noinline)) static uint32_t brk_rand_u(breaks_t *b)
{
    uint32_t x = b->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    b->rng = x;
    return x;
}
__attribute__((noinline)) static float brk_rand(breaks_t *b) { return (float)(brk_rand_u(b) >> 8) * (1.0f / 16777216.0f); }

/* ---- BB Gen's slice selection (slice_select.c) ------------------------------------------ */

typedef struct {
    int current_slice, beat_position, slices;
    float complexity, anchor, roll, fill;     /* 0..1 */
    int phrase_bars, bar_in_phrase;
} brk_sel_t;

#define BRK_ESCAPE_P 0.05f

static float brk_weight_at(int slice, float anchor)
{
    static const float locked[8] = {0.0f, 0.5f, 1.0f, 0.7f, 0.0f, 0.5f, 1.0f, 1.2f};
    slice = slice < 0 ? 0 : slice > 7 ? 7 : slice;
    anchor = fm_clampf(anchor, 0.0f, 1.0f);
    return (1.0f - anchor) + locked[slice] * anchor;
}

static void brk_apply_phrase(const brk_sel_t *in, float *c, float *a, float *r)
{
    float f;
    *c = in->complexity;
    *a = in->anchor;
    *r = in->roll;
    if (in->phrase_bars <= 0 || in->bar_in_phrase != in->phrase_bars - 1)
        return;
    f = fm_clampf(in->fill, 0.0f, 1.0f);
    *c = in->complexity + (1.0f - in->complexity) * f;
    *a = in->anchor * (1.0f - f);
    *r = in->roll * (1.0f - f);
}

static int brk_select_next(const brk_sel_t *in, breaks_t *b)
{
    float c, a, r;
    brk_apply_phrase(in, &c, &a, &r);
    if (brk_rand(b) < 1.0f - r) {                          /* MOVE */
        if (brk_rand(b) < c * brk_weight_at(in->beat_position * 8 / in->slices, a)) {
            int j = (int)(brk_rand(b) * (float)in->slices);
            return j < 0 ? 0 : j >= in->slices ? in->slices - 1 : j;
        }
        return in->beat_position;                          /* no swap: the beat's own slice */
    }
    if (brk_rand(b) < BRK_ESCAPE_P) {                      /* STAY: escape 2..4 forward */
        int j = 2 + (int)(brk_rand(b) * 3.0f);
        j = j < 2 ? 2 : j > 4 ? 4 : j;
        return (in->current_slice + j) % in->slices;
    }
    if (brk_rand(b) < 1.0f - brk_weight_at(in->current_slice * 8 / in->slices, a))
        return in->current_slice;                          /* repeat */
    return (in->current_slice + (brk_rand(b) < 0.5f ? 1 : in->slices - 1)) % in->slices;   /* walk +-1 */
}

/* ---- BB Gen's live layer (perf.c), this part's subset ------------------------------------ */

__attribute__((noinline)) static void perf_release(brk_perf_t *p, int slice)
{
    int i, found = -1;
    for (i = (int)p->count - 1; i >= 0; i--)
        if (p->stack[i] == slice) {
            found = i;
            break;
        }
    if (found < 0)
        return;
    for (i = found; i < (int)p->count - 1; i++)
        p->stack[i] = p->stack[i + 1];
    p->count--;
}
__attribute__((noinline)) static void perf_push(brk_perf_t *p, int slice)
{
    if (slice < 0 || slice > 31)
        return;
    perf_release(p, slice);                                /* a re-press moves it to the top */
    if (p->count >= 32)
        return;
    p->stack[p->count++] = (int8_t)slice;
}
static int perf_top(const brk_perf_t *p) { return p->count ? p->stack[p->count - 1] : -1; }
/* ---- ADPCM ------------------------------------------------------------------------------ */

#if defined(X0X_HOST) || (defined(__GNUC__) && !defined(__clang__)) || __SIZEOF_POINTER__ != 4
static inline void brk_barrier(void)
{
#if defined(X0X_HOST) || __SIZEOF_POINTER__ != 4
    __asm__ volatile("" ::: "memory");
#else
    __sync_synchronize();
#endif
}
#else
#include "../../hal/fm1_sync.h"
static inline void brk_barrier(void) { fm1_data_sync(); }
#endif

#if !defined(X0X_HOST)
/* Immutable spare snapshots use pool; native qualification must retain the 8 KiB floor. */
static brk_bank_t brk_side[2] X0X_POOL;
#endif

/* Set/drop path only — breaks_bank uses brk_side_slot_c (X0X_COLD) */
X0X_COLD static brk_bank_t *brk_side_slot(breaks_t *b, unsigned j)
{
#if defined(X0X_HOST)
    return &b->side[j];
#else
    (void)b;
    return &brk_side[j];
#endif
}
__attribute__((noinline)) static const brk_bank_t *brk_side_slot_c(const breaks_t *b, unsigned j)
{
#if defined(X0X_HOST)
    return &b->side[j];
#else
    (void)b;
    return &brk_side[j];
#endif
}

/* Set/drop path only — render uses bank_slot_c / breaks_bank (X0X_COLD) */
X0X_COLD static brk_bank_t *bank_slot(breaks_t *b, unsigned i)
{
    return i < 2u ? &b->bank[i] : brk_side_slot(b, i - 2u);
}
__attribute__((noinline)) static const brk_bank_t *bank_slot_c(const breaks_t *b, unsigned i)
{
    return i < 2u ? &b->bank[i] : brk_side_slot_c(b, i - 2u);
}
const brk_bank_t *breaks_bank(const breaks_t *b, int which)
{
    if (which < 0 || which > 1)
        return 0;
    return bank_slot_c(b, b->pub[which]);
}
/* Set-loop path only — not breaks_render (X0X_COLD) */
X0X_COLD static int bank_referenced(const breaks_t *b, const brk_bank_t *k)
{
    return (b->v[0].on && b->v[0].snap == k) || (b->v[1].on && b->v[1].snap == k);
}
static unsigned loop_count(const brk_loop_t *loop)
{
    unsigned n = loop->slices;
    return n == 4 || n == 16 || n == 32 ? n : 8;
}
int breaks_count(const breaks_t *b, int bank)
{
    const brk_bank_t *k = breaks_bank(b, bank);
    return k && k->slices ? k->slices : 8;
}
/* Set-loop path only — not breaks_render (X0X_COLD) */
X0X_COLD static int loop_same(const brk_bank_t *k, const brk_loop_t *loop)
{
    float sp16;
    if (!k->valid || !loop || !loop->adpcm || loop->nsamples < 64 || !loop->bars)
        return 0;
    if (k->adpcm != loop->adpcm || k->n != loop->nsamples)
        return 0;
    sp16 = (float)loop->nsamples / ((float)loop->bars * 16.0f);
    return k->sp16 == sp16 && k->slices == loop_count(loop);
}
/* a snapshot nobody is reading and that is not the other published loop (set path) */
X0X_COLD static int pick_slot(const breaks_t *b, int which)
{
    unsigned i, pub = b->pub[which], other = b->pub[which ^ 1];
    for (i = 0; i < 4u; i++) {
        if (i == pub || i == other)
            continue;
        if (bank_referenced(b, bank_slot_c(b, i)))
            continue;
        return (int)i;
    }
    return -1;
}
/* Main-loop bank decode — not breaks_render (X0X_COLD) */
X0X_COLD static void bank_fill(brk_bank_t *k, const brk_loop_t *loop)
{
    uint32_t n, i, s, sh;
    int32_t pred = 0, idx = 0;
    n = loop->nsamples;
    k->valid = 0;
    k->gen++;
    for (sh = 6; ((n + (1u << sh) - 1u) >> sh) > BRK_NCK; sh++)
        ;
    k->ck_shift = (uint8_t)sh;
    k->adpcm = loop->adpcm;
    k->n = n;
    k->sp16 = (float)n / ((float)loop->bars * 16.0f);
    k->slices = (uint8_t)loop_count(loop);
    for (s = 0; s <= 32; s++)
        k->start[s] = (n >> 5) * s + ((n & 31u) * s >> 5);  /* n * s / 32 without overflow */
    for (i = 0, s = 0; i < n; i++) {                       /* one pass: record states, decode on */
        if (!(i & ((1u << sh) - 1u))) {
            k->ck[i >> sh].pred = (int16_t)pred;
            k->ck[i >> sh].idx = (uint8_t)idx;
        }
        while (s < 32 && k->start[s] == i) {
            k->at_start[s].pred = (int16_t)pred;
            k->at_start[s].idx = (uint8_t)idx;
            s++;
        }
        ima_decode(k->adpcm, i, &pred, &idx);
    }
    brk_barrier();
    k->valid = 1;
}
/* Set-loop path only — not breaks_render (X0X_COLD) */
X0X_COLD static void pend_store(breaks_t *b, int which, const brk_loop_t *loop)
{
    b->pend[which] = *loop;
    b->pend_on[which] = 1;
}

/* decoder state before nibble p (p < n) */
static void bank_seek(const brk_bank_t *k, uint32_t p, int32_t *pred, int32_t *idx)
{
    uint32_t c = p >> k->ck_shift, i = c << k->ck_shift;
    *pred = k->ck[c].pred;
    *idx = k->ck[c].idx;
    while (i < p)
        ima_decode(k->adpcm, i++, pred, idx);
}

/* Main-loop loop assign — not breaks_render (X0X_COLD) */
X0X_COLD void breaks_set_loop(breaks_t *b, int which, const brk_loop_t *loop)
{
    brk_bank_t *k;
    int idx, j;
    if (which < 0 || which > 1)
        return;
    k = bank_slot(b, b->pub[which]);
    if (loop && which == 0 && (b->slices || b->perf.count || b->local_guard) &&
        breaks_count(b, 0) != (int)loop_count(loop)) {
        pend_store(b, which, loop);
        return;
    }
    if (!loop || !loop->adpcm || loop->nsamples < 64 || !loop->bars) {
        uint32_t irq = brk_config_lock();
        if (loop && which == 0 && breaks_count(b, 0) != (int)loop_count(loop) &&
            (b->slices || b->perf.count || b->local_guard)) {
            brk_config_unlock(irq);
            pend_store(b, which, loop);
            return;
        }
        /* the samples may be erased: do not keep a voice on them */
        for (j = 0; j < 2; j++)
            if (b->v[j].bank == (uint8_t)which)
                b->v[j].on = 0;
        k->valid = 0;
        k->gen++;
        if (loop) k->slices = (uint8_t)loop_count(loop);
        brk_config_unlock(irq);
        b->pend_on[which] = 0;
        return;
    }
    if (loop_same(k, loop)) {
        b->pend_on[which] = 0;
        return;                                         /* already sounding this loop: leave the voice */
    }
    /* A held source note keeps its original mapping until release. */
    if (which == 0 && (b->slices || b->perf.count || b->local_guard) && k->slices != loop_count(loop)) {
        pend_store(b, which, loop);
        return;
    }
    idx = pick_slot(b, which);
    if (idx >= 0) {
        brk_bank_t *next = bank_slot(b, (unsigned)idx);
        if (k->valid && k->adpcm == loop->adpcm && k->n == loop->nsamples &&
            k->sp16 == (float)loop->nsamples / ((float)loop->bars * 16.0f)) {
            /* Count-only publication reuses immutable, exact max-32 decoder states. */
            *next = *k;
            next->slices = (uint8_t)loop_count(loop);
            next->gen++;
        } else
            bank_fill(next, loop);
        uint32_t irq = brk_config_lock();
        if (which == 0 && k->slices != loop_count(loop) &&
            (b->slices || b->perf.count || b->local_guard)) {
            brk_config_unlock(irq);
            pend_store(b, which, loop);
            return;
        }
        brk_barrier();
        b->pub[which] = (uint8_t)idx;
        brk_config_unlock(irq);                   /* triggers keep reading valid audio */
        b->pend_on[which] = 0;
        return;
    }
    if (k->slices == loop_count(loop) && !bank_referenced(b, k)) {
        k->valid = 0;                                   /* voice_start will not take a half-built bank */
        brk_barrier();
        if (!bank_referenced(b, k)) {
            bank_fill(k, loop);
            b->pend_on[which] = 0;
            return;
        }
        k->valid = 1;                                   /* a voice claimed it; do not rewrite its samples */
    }
    pend_store(b, which, loop);                         /* every spare snapshot is still sounding */
}

/* Main-loop pending publish — not breaks_render (X0X_COLD) */
X0X_COLD void breaks_flush(breaks_t *b)
{
    int w;
    for (w = 0; w < 2; w++)
        if (b->pend_on[w])
            breaks_set_loop(b, w, &b->pend[w]);
}

/* Sample unload path — not breaks_render (X0X_COLD) */
X0X_COLD void breaks_drop_range(breaks_t *b, const uint8_t *base, uint32_t nbytes)
{
    const uint8_t *end;
    int j, w;
    if (!base || !nbytes)
        return;
    end = base + nbytes;
    for (j = 0; j < 2; j++) {
        const brk_bank_t *k = b->v[j].snap;
        if (b->v[j].on && k && k->adpcm >= base && k->adpcm < end)
            b->v[j].on = 0;
    }
    for (w = 0; w < 2; w++) {
        brk_bank_t *k = bank_slot(b, b->pub[w]);
        if (k->adpcm >= base && k->adpcm < end)
            k->valid = 0;
        if (b->pend_on[w] && b->pend[w].adpcm >= base && b->pend[w].adpcm < end)
            b->pend_on[w] = 0;
    }
}

/* ---- voices ----------------------------------------------------------------------------- */

__attribute__((noinline)) static float brk_inc(const breaks_t *b, const brk_bank_t *k)
{
    float sp16 = b->sp16 > 1.0f ? b->sp16 : (float)BRK_FS * 0.125f;   /* 120 BPM until told */
    float semi = (float)((int)b->set[BRK_PITCH] - 12);
    float bend = (float)b->bend / (b->bend < 0 ? 8192.0f : 8191.0f);
    return k->sp16 / sp16 * fm_exp2f(semi * (1.0f / 12.0f) + bend);
}

/* Only one decoder state is cached for the preceding reverse-cache block.
 * An unrelated seek still starts from authoritative bank checkpoints; the saved
 * state is invalidated when a voice is started on any bank or direction. */
static void bank_seek_reverse(brk_voice_t *v, const brk_bank_t *k, uint32_t p, int32_t *pred,
                              int32_t *idx)
{
    uint32_t c, i;
    if (v->seek_pos == p) {
        *pred = v->seek_state.pred;
        *idx = v->seek_state.idx;
        v->seek_pos = UINT32_MAX;
        return;
    }
    c = p >> k->ck_shift;
    i = c << k->ck_shift;
    *pred = k->ck[c].pred;
    *idx = k->ck[c].idx;
    /* rev_fetch passes a BRK_RCH-aligned base. If it is also an exact
     * checkpoint, no decoding or adjacent-state lookup is necessary. */
    if (i == p) {
        v->seek_pos = UINT32_MAX;
        return;
    }
    if (p >= BRK_RCH && p - BRK_RCH >= i) {
        uint32_t prev = p - BRK_RCH;
        while (i < prev)
            ima_decode(k->adpcm, i++, pred, idx);
        v->seek_state.pred = (int16_t)*pred;
        v->seek_state.idx = (uint8_t)*idx;
        v->seek_pos = prev;
    } else {
        v->seek_pos = UINT32_MAX;
    }
    while (i < p)
        ima_decode(k->adpcm, i++, pred, idx);
}

static float rev_fetch(brk_voice_t *v, const brk_bank_t *k, uint32_t q)
{
    if (!v->rvalid || q < v->rbase || q >= v->rbase + BRK_RCH) {
        uint32_t base = q & ~(uint32_t)(BRK_RCH - 1), m = k->n - base, i;
        int32_t pred, idx;
        if (m > BRK_RCH)
            m = BRK_RCH;
        bank_seek_reverse(v, k, base, &pred, &idx);
        for (i = 0; i < m; i++)
            v->rbuf[i] = (int16_t)ima_decode(k->adpcm, base + i, &pred, &idx);
        v->rbase = base;
        v->rvalid = 1;
    }
    return (float)v->rbuf[q - v->rbase];
}
__attribute__((noinline)) static uint32_t rev_next(const brk_voice_t *v, uint32_t q) { return q > v->lo ? q - 1u : v->hi - 1u; }

/* start a new voice at sample p of snapshot k (logical bank bk), slice sl, crossfading the current one.
 * have_state: (pred, idx) is the decoder state before nibble p; otherwise it is sought.
 * The claim of k happens before the fade, and a bank that goes invalid under us is refused,
 * so a loop rebuild cannot silence the voice that is already playing. */
static int voice_start_on(breaks_t *b, const brk_bank_t *k, int bk, int sl, uint32_t p, int rev,
                           int have_state, int32_t pred, int32_t idx)
{
    brk_voice_t *o = &b->v[b->cur], *v;
    const brk_bank_t *prev;
    if (!k || !k->valid)
        return 0;
    v = &b->v[b->cur ^ 1u];
    prev = v->snap;
    v->snap = k;                                        /* hold this snapshot so a reload will not rewrite it */
    brk_barrier();
    if (!k->valid) {
        v->snap = prev;
        return 0;
    }
    if (o->on)
        o->dg = -1.0f / (float)BRK_FADE;                   /* fade the old one out from where it is */
    b->cur ^= 1u;
    v = &b->v[b->cur];
    v->on = 1;
    v->bank = (uint8_t)bk;
    v->gen = k->gen;
    v->snap = k;
    v->rev = (uint8_t)rev;
    v->seek_pos = UINT32_MAX;       /* never reuse state from a prior voice */
    v->lo = k->start[sl * (32 / k->slices)];
    v->hi = k->start[(sl + 1) * (32 / k->slices)];
    v->frac = 0.0f;
    v->g = 0.0f;
    v->dg = 1.0f / (float)BRK_FADE;
    if (p >= k->n)
        p = 0;
    b->grain_anchor = p;
    if (rev) {
        v->rvalid = 0;
        v->s0 = rev_fetch(v, k, p);
        v->pos = rev_next(v, p);
        v->s1 = rev_fetch(v, k, v->pos);
    } else {
        if (!have_state)
            bank_seek(k, p, &pred, &idx);
        v->s0 = (float)ima_decode(k->adpcm, p, &pred, &idx);
        if (++p >= k->n) {
            p = 0;
            pred = 0;
            idx = 0;
        }
        v->s1 = (float)ima_decode(k->adpcm, p, &pred, &idx);
        v->pos = p;
        v->pred = pred;
        v->idx = idx;
    }
    b->grain_left = BRK_GRAIN_HOP;
    b->grain_distance = 0.0f;
    return 1;
}

/* (re)start slice sl of bank bk from its beginning: forward from its first sample, reverse
 * from its first sample then wrapping to its end (BB Gen's reverse) */
__attribute__((noinline)) static int slice_start(breaks_t *b, int bk, int sl)
{
    const brk_bank_t *k = breaks_bank(b, bk);
    if (!k || !k->valid)
        return 0;
    sl %= k->slices;
    unsigned boundary = (unsigned)sl * (32u / k->slices);
    return voice_start_on(b, k, bk, sl, k->start[boundary], b->perf.reverse, 1,
                   k->at_start[boundary].pred, k->at_start[boundary].idx);
}

/* ---- the generator ---------------------------------------------------------------------- */

__attribute__((noinline)) static int bank_ticks(const breaks_t *b, int bk) { return LEN_TICKS[b->set[bk ? BRK_BLEN : BRK_ALEN] % 6u]; }

/* BB_FIRE_TRIGGER: decide, then sound. forced = transport start / bank switch: slice 0, no retrig */
static void fire_trigger(breaks_t *b, int bp, int forced)
{
    int slice, held, rb, tpt = bank_ticks(b, b->engine_bank), i, n = 0, fired[4];
    held = perf_top(&b->perf);
    rb = held >= 0 ? 0 : b->engine_bank;
    if (held < 0 && !breaks_bank(b, rb)->valid && breaks_bank(b, rb ^ 1)->valid)
        rb ^= 1;
    int count = breaks_count(b, rb);
    if (forced)
        slice = 0;
    else if (b->set[BRK_COMPLEX] == 0)
        slice = bp * count / 8;
    else {
        brk_sel_t in;
        int pb = PHRASE_BARS[b->set[BRK_PHRASE] % 5u];
        in.slices = count;
        in.current_slice = b->current_slice % count;
        in.beat_position = bp * count / 8;
        float ramp = fm_minf((float)b->set[BRK_COMPLEX] * 0.04f, 1.0f);   /* X0X: COMPLEXITY is the amount: */
        in.complexity = (float)b->set[BRK_COMPLEX] * 0.01f;      /* ROLL and FILL come in with it, full */
        in.anchor = (float)b->set[BRK_ANCHOR] * 0.01f;           /* from 25 % (low settings stay gentle) */
        in.roll = (float)b->set[BRK_ROLL] * 0.01f * ramp;
        in.fill = (float)b->set[BRK_FILL] * 0.01f * ramp;
        in.phrase_bars = pb;
        in.bar_in_phrase = pb ? b->bar % pb : 0;
        slice = brk_select_next(&in, b);
    }
    held = perf_top(&b->perf);
    if (held >= 0)
        slice = held;                                      /* held slice beats the engine */
    b->current_slice = slice;
    b->sub_div = 0;
    if (!forced) {
        static const uint8_t RDIV[4] = {2, 3, 4, 8};
        float inv_tpb = (float)tpt * (1.0f / 96.0f);       /* 1 / triggers per bar */
        for (i = 0; i < 4; i++) {
            float pbar = (float)b->set[BRK_R2 + i] * 0.01f, p;
            if (pbar <= 0.0f)
                continue;
            p = pbar >= 1.0f ? 1.0f : 1.0f - fm_powf(1.0f - pbar, inv_tpb);
            if (brk_rand(b) < p)
                fired[n++] = i;
        }
        if (n)
            b->sub_div = RDIV[fired[brk_rand_u(b) % (uint32_t)n]];
        if (b->perf.stutter)
            b->sub_div = 4;                                /* BB Gen's Stutter 4x pad */
    }
    b->render_bank = (uint8_t)rb;
    b->sub_count = 0;
    b->sub_elapsed = 0;
    b->sub_len = b->sub_div ? b->spt / (float)b->sub_div : 0.0f;
    int accepted = slice_start(b, rb, slice);
    if (accepted && held < 0 && b->running && b->step_on && !b->quiet &&
        b->v[b->cur].on && b->v[b->cur].snap == breaks_bank(b, rb) && b->accepted)
        b->accepted(b->accepted_ctx, slice, rb, b->v[b->cur].gen);
    b->st_trigs++;
    b->st_slice = (int8_t)slice;
    b->st_bank = (int8_t)rb;
    b->st_div = (int8_t)b->sub_div;
    b->st_bp = (int8_t)bp;
    b->st_forced = (uint8_t)forced;
}

/* X0X: a step with a slice of its own plays it, from its start, in place of the generator's choice
 * (no retrigger); the generator still counts the trigger the step would have made */
static void pin_trigger(breaks_t *b, int slice)
{
    slice %= breaks_count(b, 0);
    b->current_slice = slice;
    b->render_bank = 0;
    b->sub_div = 0;
    b->sub_count = 0;
    b->sub_elapsed = 0;
    b->sub_len = 0.0f;
    int accepted = slice_start(b, 0, slice);
    if (accepted && b->running && b->step_on && !b->quiet && breaks_bank(b, 0)->valid && b->accepted)
        b->accepted(b->accepted_ctx, slice, 0, breaks_bank(b, 0)->gen);
    b->st_trigs++;
    b->st_slice = (int8_t)slice;
    b->st_bank = 0;
    b->st_div = 0;
    b->st_forced = 0;
}

void breaks_pin(breaks_t *b, int slice) { b->pin = (int8_t)(slice >= 0 && slice < 32 ? slice : -1); }

/* Grains never change the sequencer trigger cadence. */
__attribute__((noinline)) static void clock_trigger(breaks_t *b)
{
    int bp = b->trigger_count % 8;
    b->trigger_count++;
    fire_trigger(b, bp, 0);
}

__attribute__((noinline)) static void gate_update(breaks_t *b)
{
    int held = b->perf.count > 0;
    b->gate_to = (held || (b->running && b->step_on)) ? 1.0f : 0.0f;
}

/* apply live key changes (keys is written by breaks_live, possibly from another context) */
static void perf_sync(breaks_t *b)
{
    uint32_t k = b->slices, d = k ^ b->slices_seen;
    uint16_t macro = b->keys;
    b->perf.reverse = (macro >> 8) & 1;
    uint8_t jungle = (macro >> 9) & 1;
    if (jungle != b->perf.jungle) {
        brk_voice_t *v = &b->v[b->cur];
        b->grain_anchor = v->rev ? v->pos : (v->pos ? v->pos - 1u : 0u);
        if (b->grain_anchor < v->lo || b->grain_anchor >= v->hi) b->grain_anchor = v->lo;
        b->grain_distance = 0.0f;
        b->grain_left = BRK_GRAIN_HOP;
    }
    b->perf.jungle = jungle;
    b->perf.stutter = (macro >> 10) & 1;
    b->keys_seen = macro;
    int key, edged = -1;
    uint8_t restart = b->restart;
    if (restart)
        b->restart = 0;
    if (!d && !restart)
        return;
    if (d) {
        for (key = 0; key < 32; key++) {
            uint32_t bit = 1u << key;
            int down = (k & bit) != 0;
            if (!(d & bit))
                continue;
            {
                if (down) {
                    perf_push(&b->perf, key);
                    if (breaks_bank(b, 0)->valid) {            /* instant hit */
                        b->current_slice = key;
                        b->render_bank = 0;
                        b->sub_div = 0;
                        slice_start(b, 0, key);
                    }
                    edged = key;
                } else
                    perf_release(&b->perf, key);
            }
        }
        b->slices_seen = k;
        gate_update(b);
    }
    /* A second note for a slice that is already held restarts that slice.
     * slice_start crossfades. It does not stop the voice, and it is not a loop change. */
    if (restart) {
        int sl = (int)restart - 1;
        if ((unsigned)sl < 32u && sl != edged && (b->slices & (1u << sl)) && breaks_bank(b, 0)->valid) {
            perf_push(&b->perf, sl);
            b->current_slice = sl;
            b->render_bank = 0;
            b->sub_div = 0;
            slice_start(b, 0, sl);
            gate_update(b);
        }
    }
}

void breaks_slice_on(breaks_t *b, int slice)
{
    if (slice < 0 || slice > 31)
        return;
    breaks_slice_hold(b, slice, 1);
    b->restart = (uint8_t)(slice + 1);
}

void breaks_slice_hold(breaks_t *b, int slice, int down)
{
    if ((unsigned)slice >= 32u) return;
    uint32_t bit = 1u << slice;
    b->slices = down ? b->slices | bit : b->slices & ~bit;
}
void breaks_live(breaks_t *b, int key, int down)
{
    if ((unsigned)key < 8u) { breaks_slice_hold(b, key, down); return; }
    uint16_t bit;
    if (key < 0 || key > 10)
        return;
    bit = (uint16_t)(1u << key);
    b->keys = (uint16_t)(down ? (b->keys | bit) : (b->keys & ~bit));
}

void breaks_step(breaks_t *b, int step16, int bar, float samples_per_16th, int enabled)
{
    int pb, tpt, tick, forced = 0, pin;
    perf_sync(b);
    b->sp16 = samples_per_16th;
    b->bar = bar;
    b->step_on = (uint8_t)(enabled != 0);
    if (!b->running) {                                     /* transport start (bb_reset_transport) */
        b->running = 1;
        b->trigger_count = 0;
        b->current_slice = 0;
        b->engine_bank = 0;
        b->pending_bank = -1;
        forced = 1;
    }
    pb = PHRASE_BARS[b->set[BRK_PHRASE] % 5u];
    if (step16 == 0) {                                     /* bar boundary */
        if (!forced && b->pending_bank >= 0) {
            b->engine_bank = (uint8_t)b->pending_bank;
            forced = 1;
        }
        b->pending_bank = -1;
        if (pb) {
            int bip = bar % pb;
            if (bip == pb - 2) {
                if (brk_rand(b) < (float)b->set[BRK_BCHANCE] * 0.01f && breaks_bank(b, 1)->valid)
                    b->pending_bank = 1;
            } else if (bip == pb - 1)
                b->pending_bank = 0;
        } else if (b->set[BRK_BCHANCE] && !forced) {      /* X0X: no phrase: B chance rolls for every bar */
            int bank = brk_rand(b) < (float)b->set[BRK_BCHANCE] * 0.01f && breaks_bank(b, 1)->valid;
            if (bank != b->engine_bank) {
                b->engine_bank = (uint8_t)bank;
                forced = 1;
            }
        } else if (!forced && b->engine_bank) {            /* B CHANCE turned to 0 while B played */
            b->engine_bank = 0;
            forced = 1;
        }
    }
    tpt = bank_ticks(b, b->engine_bank);
    b->spt = samples_per_16th * (float)tpt * (1.0f / 6.0f);
    tick = 6 * (step16 & 15);
    b->trig_left = 0;
    pin = b->pin;
    b->pin = -1;
    if (pin >= 0 && (!enabled || !breaks_bank(b, 0)->valid || b->perf.count))
        pin = -1;                                          /* off, no loop A, or a held slice key wins */
    if (pin >= 0) {
        if (forced)
            b->trigger_count = 1;
        else if (tick % tpt == 0)
            b->trigger_count++;
        pin_trigger(b, pin);
    } else if (forced) {
        b->trigger_count = 1;
        fire_trigger(b, 0, 1);
    } else if (tick % tpt == 0)
        clock_trigger(b);
    if (tpt == 3)                                          /* 1/4 bar: a 32nd-note trigger too */
        b->trig_left = (int32_t)(samples_per_16th * 0.5f + 0.5f) + 1;   /* fires at that sample */
    gate_update(b);
}

void breaks_stop(breaks_t *b)
{
    b->running = 0;
    b->trig_left = 0;
    b->sub_div = 0;
    gate_update(b);
}

/* Boot only (engine_init). The render, the step, and the live keys do not call it. */
X0X_COLD void breaks_init(breaks_t *b)
{
    uint8_t *p = (uint8_t *)b;
    uint32_t i;
    for (i = 0; i < sizeof(*b); i++)
        p[i] = 0;
    for (i = 0; i < BRK_NPARAM; i++)
        b->set[i] = BRK_PARAMS[i].def;
    b->pub[0] = 0;
    b->pub[1] = 1;                                       /* A and B start on their own snapshots */
    b->rng = 0x2545F491u;
    b->pending_bank = -1;
    b->st_slice = -1;
    b->pin = -1;
}

/* ---- render ----------------------------------------------------------------------------- */

void breaks_bend(breaks_t *b, int value)
{
    b->bend = (int16_t)(value < -8192 ? -8192 : value > 8191 ? 8191 : value);
}

void breaks_set_quiet(breaks_t *b, int quiet) { b->quiet = (uint8_t)(quiet != 0); }

/* X0X: nothing can sound this block: no voice, the gate shut and staying shut, no trigger or
 * retrigger due inside it. The render would write zeros and change nothing else. */
int breaks_silent(const breaks_t *b)
{
    return !b->v[0].on && !b->v[1].on && b->gate == 0.0f && b->gate_to == 0.0f && b->trig_left == 0 &&
           b->sub_count + 1 >= b->sub_div && b->keys == b->keys_seen && b->slices == b->slices_seen;
}

void breaks_render(breaks_t *b, float *out, int n)
{
    float inc[2], lvl, gstep = 1.0f / (float)BRK_FADE;
    int i, j;
    if (b->quiet && b->gate == 0.0f && b->gate_to == 0.0f)
        b->v[0].on = b->v[1].on = 0;                       /* X0X: unheard: stop decoding */
    if (breaks_silent(b)) {                                /* X0X: skip a block that writes zeros */
        for (i = 0; i < n; i++)
            out[i] = 0.0f;
        return;
    }
    perf_sync(b);
    for (j = 0; j < 2; j++) {
        brk_voice_t *v = &b->v[j];
        const brk_bank_t *k = v->snap;
        if (v->on && !k)
            v->on = 0;
        inc[j] = v->on ? brk_inc(b, k) : 0.0f;
    }
    {   /* reverse pressed or released: continue from where the playhead is, the other way.
         * Stay on the snapshot already sounding, so a loop change does not steer the reverse. */
        brk_voice_t *v = &b->v[b->cur];
        const brk_bank_t *k = v->snap;
        if (v->on && k && v->rev != b->perf.reverse && v->dg >= 0.0f) {
            int bk = v->bank, sl = 0;
            uint32_t p = v->rev ? v->pos : (v->pos ? v->pos - 1u : 0u);
            while (sl + 1 < k->slices && k->start[(sl + 1) * (32 / k->slices)] <= v->lo)
                sl++;
            voice_start_on(b, k, bk, sl, p, b->perf.reverse, 0, 0, 0);
            inc[b->cur] = brk_inc(b, b->v[b->cur].snap ? b->v[b->cur].snap : k);
        }
    }
    lvl = (float)b->set[BRK_LEVEL] * (1.0f / 127.0f);
    lvl = lvl * lvl * (1.0f / 32768.0f);
    for (i = 0; i < n; i++) {
        float y = 0.0f;
        if (b->trig_left > 0 && --b->trig_left == 0 && b->running) {
            clock_trigger(b);
            if (b->v[b->cur].snap)
                inc[b->cur] = brk_inc(b, b->v[b->cur].snap);
        }
        if (b->sub_count + 1 < b->sub_div) {                /* retrigger: replay the slice head */
            if ((float)b->sub_elapsed >= (float)(b->sub_count + 1) * b->sub_len) {
                b->sub_count++;
                slice_start(b, b->render_bank, b->current_slice);
                inc[b->cur] = b->v[b->cur].snap ? brk_inc(b, b->v[b->cur].snap) : 0.0f;
            }
            b->sub_elapsed++;
        }
        /* Repeat 90 ms grains at unchanged playback pitch. Each next grain
         * advances half the source distance, with the existing two-voice fade.
         * A new sequencer/pin/stutter hit resets the grain origin and timer.
         * Forward grain tails retain dry BREAK continuity across slice edges.
         * Only origins wrap; all origin jumps crossfade. No grain emits MIDI. */
        if (b->perf.jungle && b->v[b->cur].on) {
            brk_voice_t *v = &b->v[b->cur];
            if (!b->grain_left && v->hi > v->lo) {
                const brk_bank_t *k = v->snap;
                uint32_t span = v->hi - v->lo;
                uint32_t advance = (uint32_t)b->grain_distance % span;
                uint32_t off = b->grain_anchor - v->lo;
                uint32_t pos = v->lo + (v->rev ? (off + span - advance) % span : (off + advance) % span);
                int sl = 0;
                while (sl + 1 < k->slices && k->start[(sl + 1) * (32 / k->slices)] <= v->lo) sl++;
                if (voice_start_on(b, k, v->bank, sl, pos, v->rev, 0, 0, 0))
                    inc[b->cur] = brk_inc(b, k);
            }
            if (b->grain_left) b->grain_left--;
            b->grain_distance += inc[b->cur] * 0.5f;
        }
        for (j = 0; j < 2; j++) {
            brk_voice_t *v = &b->v[j];
            const brk_bank_t *k;
            if (!v->on)
                continue;
            k = v->snap;
            if (!k)
                continue;
            y += (v->s0 + (v->s1 - v->s0) * v->frac) * v->g;
            v->g += v->dg;
            if (v->g >= 1.0f) {
                v->g = 1.0f;
                v->dg = 0.0f;
            } else if (v->g <= 0.0f) {
                v->on = 0;
                continue;
            }
            v->frac += inc[j];
            /* Reverse ADPCM is position-addressable. At high playback rates, do
             * not fetch every skipped interpolation sample. Preserve the final
             * two samples and the fraction for exactly the same PCM result.
             * The forward path is stateful and deliberately stays unchanged.
             * Limit the shortcut to floats with an exactly representable
             * integral sample count, and retain the original path otherwise. */
            #if BRK_FAST_REV_SKIP
            if (v->rev && v->frac >= 32.0f && v->frac < 16777216.0f &&
                v->hi > v->lo && v->pos >= v->lo && v->pos < v->hi) {
                uint32_t steps = (uint32_t)v->frac;
                uint32_t span = v->hi - v->lo;
                uint32_t shift = steps % span;
                uint32_t off = v->pos - v->lo;
                uint32_t next_off = off >= shift ? off - shift : span - (shift - off);
                uint32_t newpos = v->lo + next_off;
                uint32_t prevpos = newpos < v->hi - 1u ? newpos + 1u : v->lo;
                v->frac -= (float)steps;
                v->s0 = rev_fetch(v, k, prevpos);
                v->pos = newpos;
                v->s1 = rev_fetch(v, k, newpos);
            } else
            #endif
            {
                while (v->frac >= 1.0f) {
                    v->frac -= 1.0f;
                    v->s0 = v->s1;
                    if (v->rev) {
                        v->pos = rev_next(v, v->pos);
                        v->s1 = rev_fetch(v, k, v->pos);
                    } else {
                        if (++v->pos >= k->n) {           /* BB Gen loops at the end; state 0/0 */
                            v->pos = 0;
                            v->pred = 0;
                            v->idx = 0;
                        }
                        v->s1 = (float)ima_decode(k->adpcm, v->pos, &v->pred, &v->idx);
                    }
                }
            }
        }
        if (b->gate < b->gate_to)
            b->gate = fm_minf(b->gate + gstep, b->gate_to);
        else if (b->gate > b->gate_to)
            b->gate = fm_maxf(b->gate - gstep, b->gate_to);
        out[i] = y * b->gate * lvl;
    }
    if (b->gate == 0.0f && b->gate_to == 0.0f && !b->running) {
        b->v[0].on = 0;                                    /* stopped and silent: stop decoding */
        b->v[1].on = 0;
    }
}
