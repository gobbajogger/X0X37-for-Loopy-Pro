/* SPDX-License-Identifier: GPL-3.0-only */
/* Prove equal-tempo skip leaves BPM Delay audio+state identical to always-set.
 * Baseline: call chi_fx2_insert_set on every tempo republish.
 * Candidate: only when tempo_times10 changes (compact tempo()).
 * Cases: silence-in spans, impulse+tail, tempo change mid-stream, restart note.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "chicago_fx2_compact.h"
#include "../../firmware/src/app/original_sound.h"

enum { N = 4096 };
static int16_t L[N], R[N], Lb[N], Rb[N];

static const original_chicago_fx2_kernel *cold(original_chicago_fx2_compact *s,
    unsigned e1, unsigned e2, int32_t tempo)
{
    original_chicago_fx2_cold_owner o = original_chicago_fx2_compact_owner(s);
    assert(o.supports(o.context, 13));
    const original_chicago_fx2_kernel *k = o.select_cold(o.context, 13);
    assert(k);
    k->begin(k->context);
    k->reset(k->context);
    k->parameter(k->context, 2, e1);
    k->parameter(k->context, 3, e2);
    k->tempo(k->context, tempo);
    return k;
}

/* Always-set tempo (pre-opt behavior). */
static void tempo_always(original_chicago_fx2_compact *s, int32_t value)
{
    s->tempo_times10 = value;
    if (s->selected && s->selected_type == 13)
        chi_fx2_insert_set(&s->arena.insert, 13, s->edit[0], s->edit[1], value);
}

static int fails;

static void check_eq(const char *tag, original_chicago_fx2_compact *a,
                     original_chicago_fx2_compact *b,
                     const int16_t *la, const int16_t *ra,
                     const int16_t *lb, const int16_t *rb, unsigned n)
{
    if (memcmp(la, lb, n * sizeof(int16_t)) || memcmp(ra, rb, n * sizeof(int16_t))) {
        fprintf(stderr, "FAIL %s audio\n", tag);
        fails++;
        return;
    }
    if (memcmp(&a->arena, &b->arena, sizeof a->arena)) {
        fprintf(stderr, "FAIL %s arena state\n", tag);
        fails++;
        return;
    }
    if (a->tempo_times10 != b->tempo_times10 || a->edit[0] != b->edit[0] ||
        a->edit[1] != b->edit[1] || a->selected_type != b->selected_type) {
        fprintf(stderr, "FAIL %s compact meta\n", tag);
        fails++;
    }
}

int main(void)
{
    original_chicago_fx2_compact cand, base;
    assert(original_chicago_fx2_compact_init(&cand));
    assert(original_chicago_fx2_compact_init(&base));
    const original_chicago_fx2_kernel *kc = cold(&cand, 100, 80, 1200);
    const original_chicago_fx2_kernel *kb = cold(&base, 100, 80, 1200);

    /* Silence-in: many equal-tempo republishes then process zeros. */
    for (int i = 0; i < 64; ++i) {
        kc->tempo(kc->context, 1200);          /* candidate: skip after first */
        tempo_always(&base, 1200);             /* baseline: always set */
    }
    memset(L, 0, sizeof L); memset(R, 0, sizeof R);
    memset(Lb, 0, sizeof Lb); memset(Rb, 0, sizeof Rb);
    for (unsigned i = 0; i < N; i += 128) {
        kc->process(kc->context, L + i, R + i, 128);
        kb->process(kb->context, Lb + i, Rb + i, 128);
    }
    check_eq("silence-equal-tempo", &cand, &base, L, R, Lb, Rb, N);

    /* Impulse + silence tail after equal-tempo spam. */
    assert(original_chicago_fx2_compact_init(&cand));
    assert(original_chicago_fx2_compact_init(&base));
    kc = cold(&cand, 110, 90, 1250);
    kb = cold(&base, 110, 90, 1250);
    for (int i = 0; i < 32; ++i) {
        kc->tempo(kc->context, 1250);
        tempo_always(&base, 1250);
    }
    memset(L, 0, sizeof L); memset(R, 0, sizeof R);
    memset(Lb, 0, sizeof Lb); memset(Rb, 0, sizeof Rb);
    for (unsigned i = 0; i < 48; ++i) L[i] = R[i] = Lb[i] = Rb[i] = (int16_t)(i & 8 ? -10000 : 10000);
    for (unsigned i = 0; i < N; i += 128) {
        kc->process(kc->context, L + i, R + i, 128);
        kb->process(kb->context, Lb + i, Rb + i, 128);
    }
    check_eq("impulse-tail", &cand, &base, L, R, Lb, Rb, N);

    /* Mid-stream tempo CHANGE must still update both identically. */
    assert(original_chicago_fx2_compact_init(&cand));
    assert(original_chicago_fx2_compact_init(&base));
    kc = cold(&cand, 80, 70, 1200);
    kb = cold(&base, 80, 70, 1200);
    memset(L, 0, sizeof L); memset(R, 0, sizeof R);
    memset(Lb, 0, sizeof Lb); memset(Rb, 0, sizeof Rb);
    for (unsigned i = 0; i < 64; ++i) L[i] = R[i] = Lb[i] = Rb[i] = 8000;
    for (unsigned i = 0; i < 1024; i += 128) {
        kc->process(kc->context, L + i, R + i, 128);
        kb->process(kb->context, Lb + i, Rb + i, 128);
    }
    kc->tempo(kc->context, 2400);
    tempo_always(&base, 2400);
    for (unsigned i = 1024; i < N; i += 128) {
        kc->process(kc->context, L + i, R + i, 128);
        kb->process(kb->context, Lb + i, Rb + i, 128);
    }
    check_eq("tempo-change", &cand, &base, L, R, Lb, Rb, N);

    /* Restart after silence with equal tempo. */
    assert(original_chicago_fx2_compact_init(&cand));
    assert(original_chicago_fx2_compact_init(&base));
    kc = cold(&cand, 100, 80, 1300);
    kb = cold(&base, 100, 80, 1300);
    memset(L, 0, sizeof L); memset(R, 0, sizeof R);
    memset(Lb, 0, sizeof Lb); memset(Rb, 0, sizeof Rb);
    for (unsigned i = 0; i < 2048; i += 256) {
        for (int r = 0; r < 8; ++r) {
            kc->tempo(kc->context, 1300);
            tempo_always(&base, 1300);
        }
        kc->process(kc->context, L + i, R + i, 256);
        kb->process(kb->context, Lb + i, Rb + i, 256);
    }
    for (unsigned i = 2048; i < 2048 + 64; ++i)
        L[i] = R[i] = Lb[i] = Rb[i] = 11000;
    for (unsigned i = 2048; i < N; i += 128) {
        kc->process(kc->context, L + i, R + i, 128);
        kb->process(kb->context, Lb + i, Rb + i, 128);
    }
    check_eq("restart-after-silence", &cand, &base, L, R, Lb, Rb, N);

    if (fails) {
        fprintf(stderr, "chicago_bpm_unchanged_tempo: %d FAIL\n", fails);
        return 1;
    }
    puts("chicago_bpm_unchanged_tempo: ok");
    return 0;
}
