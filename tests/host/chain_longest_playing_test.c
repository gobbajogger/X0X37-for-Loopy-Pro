/* SPDX-License-Identifier: GPL-3.0-only */
/* A chain slot lasts for the longest playing part in the current pattern
 * (length and rate), rounded up to a whole 909 bar.
 * Owner 2026-11-12 12:33 UTC+10: "Pattern chaining is still only 1 bar
 * instead of using the longest playing pattern". */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../../firmware/src/seq/sequencer.h"

static int checks, failed;
#define VERIFY(c) do { \
    ++checks; \
    if (!(c)) { ++failed; fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } \
} while (0)

static int on_drum(void *c, int k, int v, float vel) { (void)c; (void)k; (void)v; (void)vel; return 1; }
static int on_bass(void *c, int b, int n, int a, int sl, int vel) { (void)c; (void)b; (void)n; (void)a; (void)sl; (void)vel; return 1; }
static void off_bass(void *c, int b) { (void)c; (void)b; }
static void on_brk(void *c, int s, int b, float p, int e) { (void)c; (void)s; (void)b; (void)p; (void)e; }
static void off_brk(void *c) { (void)c; }
static void out_midi(void *c, uint8_t s, uint8_t a, uint8_t b) { (void)c; (void)s; (void)a; (void)b; }
static const seq_sink_t sink = { on_drum, on_bass, off_bass, on_brk, off_brk, out_midi, 0, 0, 0 };

static seq_t S;
static pattern_t P[NPAT];

/* 16 sixteenths at 120 BPM: one 16-step 909 bar. 16 * 44100 * 60 / 120 / 4 = 88200. */
static const uint32_t BAR = (uint32_t)FS * 60u / 120u * 4u;

static void run(uint32_t samples)
{
    while (samples) {
        uint32_t n = samples < 256u ? samples : 256u;
        seq_advance(&S, 0, &sink);
        while (n) {
            uint32_t k = seq_until_event(&S, n);
            seq_advance(&S, k, &sink);
            n -= k;
            samples -= k;
        }
    }
}

static void bass_note(int pat, int voice, int len, int rate)
{
    int i;
    P[pat].bass[voice].len = (uint8_t)len;
    P[pat].bass[voice].rate = (uint8_t)rate;
    for (i = 0; i < NSTEPS; i++)
        P[pat].bass[voice].step[i].flags = (uint8_t)G_REST;
    P[pat].bass[voice].step[0].flags = (uint8_t)G_NOTE;
    P[pat].bass[voice].step[0].note = 36;
}

static void setup(void)
{
    int i, t;
    for (i = 0; i < NPAT; i++)
        pattern_init(&P[i], 0x100u + (uint32_t)i, 0x200u + (uint32_t)i);
    /* A hit on the 909 so the part is playing; length stays the default 16. */
    for (i = 0; i < NPAT; i++)
        P[i].drum[0].hit[0][0] = 1u;
    seq_init(&S, P);
    S.send_clock = 0;
    S.bpm = 120.0f;
    for (t = 0; t < NTRACKS; t++)
        S.ppat[t] = 0;
    seq_chain(&S, 0, 1);
}

static void start(void)
{
    seq_start(&S);
    seq_advance(&S, 0, &sink);
}

/* Bars of a 16-step 909 until the chain advances. -1 if it has not after max_bars. */
static int bars_until_advance(int max_bars)
{
    int start_pat = S.ppat[TRK_DRUM], i;
    for (i = 0; i < max_bars; i++) {
        run(BAR);
        if (S.ppat[TRK_DRUM] != start_pat)
            return i + 1;
    }
    return -1;
}

static void test_32_step_303_is_two_bars(void)
{
    setup();
    bass_note(0, 0, 32, RATE_16);
    bass_note(1, 0, 32, RATE_16);
    start();
    VERIFY(S.chain_win == 32);
    VERIFY(bars_until_advance(4) == 2);
    /* The next slot is also 2 bars (pattern 1 has the same 32-step line). */
    VERIFY(S.ppat[TRK_DRUM] == 1);
    VERIFY(S.chain_win == 32);
    VERIFY(bars_until_advance(4) == 2);
    VERIFY(S.ppat[TRK_DRUM] == 0);
}

static void test_all_16_is_one_bar(void)
{
    setup();
    bass_note(0, 0, 16, RATE_16);
    bass_note(1, 0, 16, RATE_16);
    start();
    VERIFY(S.chain_win == 16);
    VERIFY(bars_until_advance(4) == 1);
    VERIFY(S.ppat[TRK_DRUM] == 1);
    VERIFY(bars_until_advance(4) == 1);
    VERIFY(S.ppat[TRK_DRUM] == 0);
}

static void test_303_eighth_rate_is_two_bars(void)
{
    /* 16 steps at 1/8 = 32 sixteenths = two 16-step 909 bars. */
    setup();
    bass_note(0, 0, 16, RATE_8);
    bass_note(1, 0, 16, RATE_16);
    start();
    VERIFY(S.chain_win == 32);
    VERIFY(bars_until_advance(4) == 2);
    /* Pattern 1 is all 16 at 1/16: one bar. Current pattern, not the range max. */
    VERIFY(S.ppat[TRK_DRUM] == 1);
    VERIFY(S.chain_win == 16);
    VERIFY(bars_until_advance(4) == 1);
    VERIFY(S.ppat[TRK_DRUM] == 0);
}

static void test_24_step_303_rounds_up(void)
{
    /* 24 sixteenths vs a 16-step 909 bar: ceil(24/16) = 2 bars. */
    setup();
    bass_note(0, 0, 24, RATE_16);
    start();
    VERIFY(S.chain_win == 32);
    VERIFY(bars_until_advance(4) == 2);
}

static void test_808_quarter_rate(void)
{
    /* 16-step 808 at 1/4 = 64 sixteenths = four 16-step 909 bars. */
    setup();
    P[0].drum[1].len = 16;
    P[0].drum[1].rate = RATE_4;
    P[0].drum[1].hit[0][0] = 1u;
    start();
    VERIFY(S.chain_win == 64);
    VERIFY(bars_until_advance(8) == 4);
}

static void test_empty_32_step_303_does_not_extend(void)
{
    int i;
    setup();
    P[0].bass[0].len = 32;
    P[0].bass[0].rate = RATE_16;
    for (i = 0; i < NSTEPS; i++)
        P[0].bass[0].step[i].flags = (uint8_t)G_REST;
    start();
    VERIFY(S.chain_win == 16);
    VERIFY(bars_until_advance(4) == 1);
}

static void test_muted_32_step_303_does_not_extend(void)
{
    setup();
    bass_note(0, 0, 32, RATE_16);
    S.mute = seq_part_mask(TRK_BASS0);
    start();
    VERIFY(S.chain_win == 16);
    VERIFY(bars_until_advance(4) == 1);
}

static void test_part_off_32_step_303_does_not_extend(void)
{
    setup();
    bass_note(0, 0, 32, RATE_16);
    S.part_off = (uint8_t)(1u << TRK_BASS0);
    start();
    VERIFY(S.chain_win == 16);
    VERIFY(bars_until_advance(4) == 1);
}

static void test_triplet_rate_rounds_up(void)
{
    /* 32 steps at 1/16T (4 ticks) = 128 ticks; 16-step 909 at 1/16 (6 ticks)
     * is 96 ticks: ceil(128/96) = 2 bars. */
    setup();
    bass_note(0, 0, 32, RATE_16T);
    start();
    VERIFY(S.chain_win == 32);
    VERIFY(bars_until_advance(4) == 2);
}

static void test_break_extends_short_909(void)
{
    /* BREAK is always 16 sixteenths. An 8-step 909 bar is half of that, so
     * two 909 passes. Both patterns use an 8-step 909 so a range-max of 909
     * lengths would still be 8; only BREAK extends the first slot. */
    setup();
    P[0].drum[0].len = 8;
    P[1].drum[0].len = 8;
    P[0].brk.steps = 1u; /* step 0 on */
    start();
    VERIFY(S.chain_win == 16);
    /* 16 909 steps at length 8 = two 8-step bars = one standard 16-step bar. */
    run(BAR);
    VERIFY(S.ppat[TRK_DRUM] == 1);
    /* Pattern 1 has no BREAK hits: one 8-step 909 pass = half a standard bar. */
    VERIFY(S.chain_win == 8);
}

static void test_64_step_808_quarter_fits_u16(void)
{
    /* 64 steps at 1/4 = 256 sixteenths = 16 bars. Must fit in chain_win (uint16). */
    setup();
    P[0].drum[1].len = 64;
    P[0].drum[1].rate = RATE_4;
    P[0].drum[1].hit[0][0] = 1u;
    start();
    VERIFY(S.chain_win == 256);
    VERIFY(bars_until_advance(20) == 16);
}


/* Real sequencer regressions for independently selected parts and bar controls. */
static void test_mixed_patterns(void)
{
    setup(); bass_note(1, 0, 64, RATE_4); S.ppat[TRK_BASS0] = 1;
    start(); VERIFY(S.chain_win == 256); run(BAR); VERIFY(S.ppat[TRK_DRUM] == 0);
    setup(); P[1].drum[1].len = 32; P[1].drum[1].hit[0][0] = 1;
    S.ppat[1] = 1; start(); VERIFY(S.chain_win == 32);
    setup(); P[0].drum[0].len = 8; P[1].brk.steps = 1;
    S.ppat[TRK_BRK] = 1; start(); VERIFY(S.chain_win == 16);
}

static void test_muted_voice_hits(void)
{
    setup(); P[0].drum[1].len = 64; P[0].drum[1].rate = RATE_4;
    P[0].drum[1].hit[0][0] = 1; S.vmute = 1u << NDRUM;
    start(); VERIFY(S.chain_win == 16); VERIFY(bars_until_advance(2) == 1);
    setup(); P[0].drum[1].len = 64; P[0].drum[1].rate = RATE_4;
    P[0].drum[1].hit[0][0] = P[0].drum[1].hit[1][0] = 1;
    S.vmute = 1u << NDRUM; start(); VERIFY(S.chain_win == 256);
    setup(); P[0].drum[1].len = 64; P[0].drum[1].rate = RATE_4;
    P[0].drum[1].hit[0][0] = 1; S.mute = 1u << NDRUM;
    start(); VERIFY(S.chain_win == 16);
}

static void test_invalid_pattern_indices(void)
{
    VERIFY(seq_chain_slot_len(0) == 1);
    for (int t = 0; t < NTRACKS; t++) {
        setup(); S.ppat[t] = NPAT; VERIFY(seq_chain_slot_len(&S) == 1);
    }
}

static void test_bar_actions_inside_long_slot(void)
{
    setup(); bass_note(0, 0, 32, RATE_16); start();
    S.fill_req = 1; S.mute_q = seq_part_mask(TRK_BASS0 + 1);
    run(BAR); VERIFY(S.ppat[TRK_DRUM] == 0); VERIFY(S.fill_on == 1);
    VERIFY(seq_part_muted(&S, TRK_BASS0 + 1)); VERIFY(S.mute_q == 0);
    run(BAR); VERIFY(S.ppat[TRK_DRUM] == 1);
    setup(); bass_note(0, 0, 32, RATE_16); start();
    seq_cue_part(&S, TRK_BASS0 + 1, 2); run(BAR);
    VERIFY(S.ppat[TRK_DRUM] == 0); VERIFY(S.ppat[TRK_BASS0 + 1] == 2);
    VERIFY(S.pcue[TRK_BASS0 + 1] == 0xff);
    run(BAR); VERIFY(S.ppat[TRK_DRUM] == 1); /* cue does not restart the chain clock */
}

static void test_tie_eligibility(void)
{
    setup(); P[0].bass[0].len = 64; P[0].bass[0].rate = RATE_4;
    for (int i = 0; i < NSTEPS; i++) P[0].bass[0].step[i].flags = G_TIE;
    start(); VERIFY(!S.t[TRK_BASS0].sounding); VERIFY(S.chain_win == 16);
    VERIFY(bars_until_advance(2) == 1);
    /* A NOTE followed by ties really sounds and must retain the long slot. */
    setup(); bass_note(0, 0, 32, RATE_16);
    for (int i = 1; i < 32; i++) P[0].bass[0].step[i].flags = G_TIE;
    P[0].bass[0].step[31].flags |= BS_SLIDE; /* explicit held continuation at boundary */
    P[1].bass[0].len = 32;
    for (int i = 0; i < 32; i++) P[1].bass[0].step[i].flags = G_TIE;
    start(); VERIFY(S.t[TRK_BASS0].sounding); VERIFY(S.chain_win == 32);
    run(BAR * 2); VERIFY(S.ppat[TRK_DRUM] == 1);
    VERIFY(S.t[TRK_BASS0].sounding); VERIFY(S.chain_win == 32);
    run(BAR); VERIFY(S.ppat[TRK_DRUM] == 1); VERIFY(S.t[TRK_BASS0].sounding);
}

static void test_rest_breaks_carried_tie(void)
{
    for (int reverse = 0; reverse < 2; reverse++) {
        setup(); bass_note(0, 0, 32, RATE_16);
        for (int i = 1; i < 32; i++) P[0].bass[0].step[i].flags = G_TIE;
        P[0].bass[0].step[31].flags |= BS_SLIDE;
        P[1].bass[0].len = 64; P[1].bass[0].rate = RATE_4;
        P[1].bass[0].dir = reverse ? DIR_REV : DIR_FWD;
        for (int i = 0; i < 64; i++) P[1].bass[0].step[i].flags = G_TIE;
        P[1].bass[0].step[reverse ? 63 : 0].flags = G_REST;
        start(); run(BAR * 2);
        VERIFY(S.ppat[TRK_DRUM] == 1); VERIFY(!S.t[TRK_BASS0].sounding);
        VERIFY(S.chain_win == 16); run(BAR); VERIFY(S.ppat[TRK_DRUM] == 0);
    }
}

int main(void)
{
    test_rest_breaks_carried_tie();
    test_invalid_pattern_indices();
    test_mixed_patterns();
    test_muted_voice_hits();
    test_bar_actions_inside_long_slot();
    test_tie_eligibility();
    test_32_step_303_is_two_bars();
    test_all_16_is_one_bar();
    test_303_eighth_rate_is_two_bars();
    test_24_step_303_rounds_up();
    test_808_quarter_rate();
    test_empty_32_step_303_does_not_extend();
    test_muted_32_step_303_does_not_extend();
    test_part_off_32_step_303_does_not_extend();
    test_triplet_rate_rounds_up();
    test_break_extends_short_909();
    test_64_step_808_quarter_fits_u16();
    printf("chain longest playing: %d checks, %d failed\n", checks, failed);
    return failed != 0;
}
