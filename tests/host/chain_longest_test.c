/* SPDX-License-Identifier: GPL-3.0-only */
/* Chain slot = longest 909 length in the chain. Shorter patterns loop inside
 * that window. Compile -DCHAIN_LONGEST_NO_WINDOW to link a sequencer that has
 * no seq_chain_window (the base): the playback checks still run. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/sequencer.h"

static int fails;
#define CHECK(c, ...) do { \
    if (!(c)) { \
        fails++; \
        printf("  FAIL %s:%d: ", __FILE__, __LINE__); \
        printf(__VA_ARGS__); \
        printf("\n"); \
    } \
} while (0)

enum { E_ON = 1 };
static seq_t S;
static pattern_t P[NPAT];
static song_t song;
static int nstep, n_on;
static uint32_t now;
static uint8_t ev_pat[1024], ev_step[1024];
static uint32_t ev_at[1024];

static int s_drum(void *x, int k, int v, float vel)
{
    (void)x; (void)k; (void)v; (void)vel;
    return 1;
}
static int s_on(void *x, int p, int n, int acc, int sl, int velocity)
{
    (void)x; (void)n; (void)acc; (void)sl; (void)velocity;
    if (p == 0)
        n_on++;
    return 1;
}
static void s_off(void *x, int p) { (void)x; (void)p; }
static void s_brk(void *x, int s16, int bar, float spb, int en)
{
    (void)x; (void)s16; (void)bar; (void)spb; (void)en;
}
static void s_brkstop(void *x) { (void)x; }
static void s_midi(void *x, uint8_t st, uint8_t d1, uint8_t d2)
{
    (void)x; (void)st; (void)d1; (void)d2;
}
static void s_step(void *x, int part, int st)
{
    (void)x;
    if (part != TRK_DRUM)
        return;
    if (nstep < (int)(sizeof ev_pat)) {
        ev_pat[nstep] = S.ppat[TRK_DRUM];
        ev_step[nstep] = (uint8_t)st;
        ev_at[nstep] = now;
    }
    nstep++;
}
static const seq_sink_t SINK = {s_drum, s_on, s_off, s_brk, s_brkstop, s_midi, s_step, 0, 0};

static void run(uint32_t samples)
{
    while (samples) {
        uint32_t n = samples < 256u ? samples : 256u;
        seq_advance(&S, 0, &SINK);
        while (n) {
            uint32_t k = seq_until_event(&S, n);
            now += k;
            seq_advance(&S, k, &SINK);
            n -= k;
            samples -= k;
        }
    }
}

static void reset(void)
{
    int i;
    for (i = 0; i < NPAT; i++)
        pattern_init(&P[i], 0x100u + (uint32_t)i, 0x200u + (uint32_t)i);
    seq_init(&S, P);
    S.send_clock = 0;
    S.bpm = 120.0f;
    memset(&song, 0, sizeof song);
    nstep = 0;
    n_on = 0;
    now = 0;
}

static void use_pat(int p)
{
    int t;
    for (t = 0; t < NTRACKS; t++)
        S.ppat[t] = (uint8_t)p;
}

static void dlen(int p, int n)
{
    P[p].drum[0].len = (uint8_t)n;
    P[p].drum[0].rate = RATE_16;
}

static void play(int n)
{
    int goal = nstep + n;
    uint32_t guard = 0;
    while (nstep < goal && guard < 44100u * 60u) {
        run(128);
        guard += 128;
    }
    CHECK(nstep >= goal, "play: wanted %d more 909 steps, have %d (pat %u)",
          n, nstep, (unsigned)S.ppat[TRK_DRUM]);
}

static int span_pat(int from, int n, int pat)
{
    int i;
    if (from < 0 || n < 0 || from + n > nstep || from + n > (int)(sizeof ev_pat))
        return 0;
    for (i = 0; i < n; i++)
        if (ev_pat[from + i] != (uint8_t)pat)
            return 0;
    return 1;
}

static int span_steps(int from, const uint8_t *st, int n)
{
    int i;
    if (from < 0 || nstep < from + n)
        return 0;
    for (i = 0; i < n; i++)
        if (ev_step[from + i] != st[i])
            return 0;
    return 1;
}

/* The long pattern is not the first in the chain. Cutting to the shorter
 * neighbour, or to the first pattern's length, leaves it early. */
static void test_long_holds(void)
{
    reset();
    dlen(0, 12);
    dlen(1, 16);
    use_pat(1);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(12);
    CHECK(S.ppat[TRK_DRUM] == 1, "long-holds: still on the long pattern after 12 steps");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 1,
          "long-holds: the long pattern is not cut to the shorter neighbour");
    play(3);
    CHECK(S.ppat[TRK_DRUM] == 1 && span_pat(0, 16, 1),
          "long-holds: 16 steps, the pattern's own length");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 0 && ev_step[16] == 0,
          "long-holds: the chain advances when that longest window ends");
}

/* Start on the short pattern. The chain must not move at its own end. */
static void test_short_holds(void)
{
    static const uint8_t want[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3
    };
    reset();
    dlen(0, 12);
    dlen(1, 16);
    use_pat(0);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(12);
    CHECK(S.ppat[TRK_DRUM] == 0, "short-holds: 12 steps have not left the pattern");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 0,
          "short-holds: does not advance at the first pattern's end");
    play(3);
    CHECK(span_pat(0, 16, 0) && span_steps(0, want, 16),
          "short-holds: 12 then a partial 0..3 inside the window of 16");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 1 && S.ppat[TRK_BASS0] == 1 && ev_step[16] == 0,
          "short-holds: every part advances together when the longest finishes");
}

static void test_odd_3(void)
{
    static const uint8_t want[] = {
        0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0
    };
    reset();
    dlen(0, 3);
    dlen(1, 16);
    use_pat(0);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(16);
    CHECK(span_pat(0, 16, 0) && span_steps(0, want, 16),
          "odd-3: five loops and a one-step partial, not a phase skip");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 1 && ev_step[16] == 0,
          "odd-3: the next pattern starts at step 0");
}

static void test_three_lengths(void)
{
    reset();
    dlen(0, 8);
    dlen(1, 16);
    dlen(2, 32);
    use_pat(0);
    seq_chain(&S, 0, 2);
    seq_start(&S);
    play(32);
    CHECK(span_pat(0, 32, 0), "lengths: the 8-step pattern loops to fill 32");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 1 && ev_step[32] == 0, "lengths: pattern 1 starts at 0");
    play(31);
    CHECK(S.ppat[TRK_DRUM] == 1, "lengths: pattern 1 (16) also fills 32");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 2 && ev_step[64] == 0, "lengths: the 32-step pattern starts");
    play(31);
    CHECK(S.ppat[TRK_DRUM] == 2, "lengths: the longest plays once");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 0 && ev_step[96] == 0, "chain-wrap: last returns to first at step 0");
}

static void test_revisit_phase(void)
{
    reset();
    dlen(0, 12);
    dlen(1, 16);
    use_pat(0);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(16);             /* pattern 0 ends on step index 3 */
    play(1);              /* pattern 1 step 0 */
    play(15);             /* rest of pattern 1 */
    CHECK(S.ppat[TRK_DRUM] == 1, "revisit: still on the second slot");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 0 && ev_step[32] == 0,
          "revisit: the same pattern starts at step 0, not at the old partial");
}

static void test_one_chain(void)
{
    reset();
    dlen(0, 12);
    use_pat(0);
    seq_chain(&S, 0, 0);
    seq_start(&S);
    play(12);
    CHECK(S.ppat[TRK_DRUM] == 0, "one-chain: 12 steps stay home");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 0 && ev_step[12] == 0,
          "one-chain: a single pattern still wraps at its own length");
    reset();
    dlen(3, 12);
    dlen(4, 16);
    use_pat(3);
    seq_chain(&S, 3, 3);
    seq_start(&S);
    play(1);
    seq_cue(&S, 4);
    play(11);
    CHECK(S.ppat[TRK_DRUM] == 3, "one-chain: a cue waits for this pattern's own end");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 4, "one-chain: the cue lands on the pattern's own bar");
}

static void test_empty_and_silent(void)
{
    int i;
    reset();
    dlen(0, 0);
    dlen(1, 8);
    use_pat(0);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(8);
    CHECK(span_pat(0, 8, 0), "empty: length 0 still occupies the window, it is not skipped");
    for (i = 0; i < 8; i++)
        CHECK(ev_step[i] == 0, "empty: clamped length 1 repeats step 0 (step %d)", i);
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 1, "empty: the chain then advances");

    reset();
    dlen(0, 4);
    dlen(1, 16);              /* no hits: pattern_init left the rows clear */
    use_pat(0);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(16);
    CHECK(span_pat(0, 16, 0),
          "silent-counts: a pattern with no events still counts by its length");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 1 && ev_step[16] == 0, "silent-counts: advance after that length");
}

static void test_part_len_and_rate(void)
{
    reset();
    dlen(0, 4);
    P[0].drum[1].len = 64;    /* 808 longer than any 909 in the chain */
    dlen(1, 8);
    P[1].drum[1].len = 4;
    use_pat(0);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(8);
    CHECK(span_pat(0, 8, 0), "part-len: the window is the 909 length, not the 808's 64");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 1, "part-len: advances at the longest 909 length");

    reset();
    dlen(0, 16);
    P[0].drum[0].rate = RATE_32;
    dlen(1, 8);
    P[1].drum[0].rate = RATE_16;
    use_pat(0);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(16);
    CHECK(span_pat(0, 16, 0), "rate: the window counts steps, not the other pattern's time");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 1, "rate: 16 steps of the playing rate, then the chain moves");
}

static void test_polymeter(void)
{
    reset();
    dlen(0, 4);
    dlen(1, 16);
    P[0].bass[0].len = 16;
    P[0].bass[0].step[0] = (bstep_t){36, G_NOTE, 0};
    P[1].bass[0].len = 16;
    P[1].bass[0].step[0] = (bstep_t){40, G_NOTE, 0};
    use_pat(0);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(16);
    CHECK(n_on == 1, "polymeter: the 303 does not restart when the short 909 loops (%d)", n_on);
    play(1);
    CHECK(S.ppat[TRK_BASS0] == 1 && n_on == 2,
          "polymeter: the chain boundary restarts the 303 (%d)", n_on);
}

static void test_lead_in(void)
{
    reset();
    dlen(5, 4);
    dlen(0, 12);
    dlen(1, 16);
    use_pat(5);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(4);
    CHECK(span_pat(0, 4, 5), "lead-in: a pattern outside the chain keeps its own length");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 0 && ev_step[4] == 0, "lead-in: the chain then starts at its first pattern");
    play(15);
    CHECK(S.ppat[TRK_DRUM] == 0, "lead-in: the first chain slot uses the longest window");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 1, "lead-in: then the next chain pattern");
}

static void test_cue_waits(void)
{
    reset();
    dlen(0, 12);
    dlen(1, 16);
    dlen(7, 8);
    use_pat(0);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(4);
    seq_cue(&S, 7);
    play(8);
    CHECK(S.ppat[TRK_DRUM] == 0, "cue-waits: 12 steps do not fire the cue");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 0, "cue-waits: the cue waits for the chain window");
    play(3);
    CHECK(S.ppat[TRK_DRUM] == 0, "cue-waits: still inside the latched window");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 7, "cue-wins: at the window the cue replaces the chain advance");
}

/* A part-only cue must not restart an unchanged 909 at a partial slot end. */
static void test_part_cue_partial_window(void)
{
    reset();
    dlen(0, 12);
    dlen(1, 16);
    use_pat(0);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(4);
    seq_cue_part(&S, TRK_BASS0, 7);
    play(13);
    CHECK(S.ppat[TRK_BASS0] == 7, "part-cue: the bass cue wins at the window end");
    CHECK(S.ppat[TRK_DRUM] == 0 && ev_step[16] == 4,
          "part-cue: unchanged 909 keeps its partial-loop phase (step %u)", ev_step[16]);
}

/* Leaving song record restores song playback's own 909 bar immediately. */
static void test_song_record_to_playback(void)
{
    int t;
    reset();
    dlen(0, 4);
    dlen(1, 16);
    use_pat(0);
    for (t = 0; t < NTRACKS; t++) {
        song.bar[0].pat[t] = 0;
        song.bar[1].pat[t] = 1;
    }
    song.len = 2;
    S.song = &song;
    S.song_on = 1;
    S.song_rec = 1;
    S.song_start = 0;
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(2);
    S.song_rec = 0;           /* REC is allowed to change while the song plays. */
    play(3);
    CHECK(S.song_pos == 1 && S.ppat[TRK_DRUM] == 1,
          "song-rec-off: playback advances at the current 909 wrap (pos %u pat %u)",
          S.song_pos, S.ppat[TRK_DRUM]);
}

static void test_live_edit_and_continue(void)
{
    reset();
    dlen(0, 12);
    dlen(1, 16);
    use_pat(0);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(4);
    dlen(1, 32);          /* would stretch the window if it were recomputed now */
    play(12);
    CHECK(S.ppat[TRK_DRUM] == 0, "latch: a longer edit does not stretch the open slot");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 1 && ev_step[16] == 0, "latch: the slot still ends at 16");
    play(31);
    CHECK(S.ppat[TRK_DRUM] == 1, "latch: the next slot uses the edited longest length");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 0, "latch: that next slot is 32 steps");

    reset();
    dlen(0, 12);
    dlen(1, 16);
    use_pat(0);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(4);
    seq_stop(&S);
    run(256);
    dlen(1, 8);           /* shorter neighbour: a fresh window would be 12 */
    seq_continue(&S);
    run(256);             /* Continue fires the next step */
    CHECK(S.ppat[TRK_DRUM] == 0, "continue-holds: resume stays on the open slot");
    while (nstep < 16 && nstep < 40)
        play(1);
    CHECK(nstep >= 16 && S.ppat[TRK_DRUM] == 0,
          "continue-holds: the original window is not restarted");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 1, "continue-holds: advances at the original window");
}

static void test_tempo_swing_clock(void)
{
    uint32_t into_odd, into_even, into_wrap;
    reset();
    dlen(0, 12);
    dlen(1, 16);
    use_pat(0);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(4);
    S.bpm = 90.0f;
    play(12);
    CHECK(span_pat(0, 16, 0), "tempo-steps: a tempo edit does not change the step count");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 1, "tempo-steps: the chain still advances after 16 steps");

    reset();
    dlen(0, 12);
    dlen(1, 16);
    P[0].swing = 100;
    use_pat(0);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(16);
    into_odd = ev_at[1] - ev_at[0];
    into_even = ev_at[2] - ev_at[1];
    CHECK(into_odd > into_even, "swing-gap: odd steps stay late inside the chain slot");
    CHECK(span_pat(0, 16, 0), "swing-steps: swing does not shorten the window");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 1, "swing-steps: advance after 16 swung steps");

    reset();
    dlen(0, 3);
    dlen(1, 16);
    P[0].swing = 100;
    use_pat(0);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(4);              /* steps 0,1,2,0 — the wrap 2→0 is even to even */
    into_odd = ev_at[1] - ev_at[0];
    into_wrap = ev_at[3] - ev_at[2];
    CHECK(into_odd > into_wrap, "swing-wrap: an even-to-even loop has no extra shuffle");

    reset();
    dlen(0, 12);
    dlen(1, 16);
    use_pat(0);
    S.send_clock = 1;
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(16);
    CHECK(span_pat(0, 16, 0), "midi-clock: clock out does not change the window");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 1, "midi-clock: advance after the same 16 steps");

    reset();
    dlen(0, 12);
    dlen(1, 16);
    use_pat(0);
    S.clk_src = SEQ_CLK_EXT;
    seq_chain(&S, 0, 1);
    seq_start(&S);
    {
        int guard = 0;
        while (nstep < 12 && guard < 400) {
            seq_ext_clock(&S);
            run(64);
            guard++;
        }
        CHECK(S.ppat[TRK_DRUM] == 0 && nstep == 12, "midi-sync: 12 external steps stay on the short pattern");
        while (nstep < 16 && guard < 800) {
            seq_ext_clock(&S);
            run(64);
            guard++;
        }
        CHECK(span_pat(0, 16, 0), "midi-sync: external clock waits for the longest window");
        while (nstep < 17 && guard < 900) {
            seq_ext_clock(&S);
            run(64);
            guard++;
        }
        CHECK(S.ppat[TRK_DRUM] == 1 && ev_step[16] == 0,
              "midi-sync: the next external step advances the chain");
    }
}

static void test_song(void)
{
    int t;
    reset();
    dlen(0, 8);
    dlen(1, 16);
    dlen(2, 32);
    for (t = 0; t < NTRACKS; t++) {
        song.bar[0].pat[t] = 0;
        song.bar[1].pat[t] = 1;
    }
    song.len = 2;
    S.song = &song;
    S.song_on = 1;
    S.song_start = 0;
    seq_chain(&S, 0, 2);      /* ignored while the song is playing */
    seq_start(&S);
    play(8);
    CHECK(S.ppat[TRK_DRUM] == 0 && S.song_pos == 0, "song-bar: the song bar uses its own 909 length");
    play(1);
    CHECK(S.ppat[TRK_DRUM] == 1 && S.song_pos == 1,
          "song-bar: the chain window does not hold a song bar");

    reset();
    dlen(0, 8);
    dlen(1, 16);
    for (t = 0; t < NTRACKS; t++) {
        song.bar[0].pat[t] = 0;
        song.bar[1].pat[t] = 1;
    }
    song.len = 2;
    S.song = &song;
    S.song_on = 1;
    S.song_start = 1;
    seq_chain(&S, 0, 1);
    seq_start(&S);
    run(256);
    CHECK(S.song_pos == 1 && S.ppat[TRK_DRUM] == 1,
          "song-play-from: PLAY starts at song_start immediately");

    reset();
    dlen(0, 12);
    dlen(1, 16);
    use_pat(0);
    S.song = &song;
    song.len = 0;
    S.song_on = 1;
    S.song_rec = 1;
    S.song_start = 0;
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(12);
    CHECK(song.len == 1, "song-rec: the short loop does not write another bar (len %u)", song.len);
    play(1);
    CHECK(song.len == 1 && S.ppat[TRK_DRUM] == 0,
          "song-rec-holds: recording waits for the chain window");
    play(3);
    CHECK(song.len == 1, "song-rec: still one bar at the end of the window");
    play(1);
    CHECK(song.len == 2 && song.bar[1].pat[TRK_DRUM] == 1,
          "song-rec: the next chain slot is the next song bar");
}

#ifndef CHAIN_LONGEST_NO_WINDOW
static void test_window_fn(void)
{
    reset();
    dlen(0, 12);
    dlen(1, 16);
    dlen(2, 8);
    P[0].drum[1].len = 64;
    P[0].bass[0].len = 64;
    CHECK(seq_chain_window(P, 0, 2) == 16, "window-longest: max 909 length is 16, not 64 or 8");
    CHECK(seq_chain_window(P, 2, 0) == 16, "window-longest: reversed endpoints");
    CHECK(seq_chain_window(P, 1, 1) == 16, "window-one: a one-pattern range is that length");
    dlen(0, 0);
    dlen(1, 0);
    CHECK(seq_chain_window(P, 0, 1) == 1, "window-empty: stored length 0 counts as 1");
    dlen(1, 4);
    CHECK(seq_chain_window(P, 0, 1) == 4, "window-empty: length 0 does not raise the window");
    dlen(0, 80);
    dlen(1, 10);
    CHECK(seq_chain_window(P, 0, 1) == 64, "window-longest: lengths above 64 clamp to 64");
    CHECK(seq_chain_window(P, -1, 1) == 1, "window-range: a negative endpoint returns 1");
    CHECK(seq_chain_window(P, 0, NPAT) == 1, "window-range: an endpoint past the bank returns 1");
    CHECK(seq_chain_window(0, 0, 1) == 1, "window-range: no pattern bank returns 1");

    reset();
    dlen(0, 12);
    dlen(1, 16);
    use_pat(0);
    seq_chain(&S, 0, 1);
    seq_start(&S);
    play(1);
    CHECK(S.chain_win == 16 && S.chain_at == 1, "window-latched: the open slot freezes the longest length");
    dlen(1, 32);
    play(1);
    CHECK(S.chain_win == 16, "window-latched: a live edit does not move the open slot");
    CHECK(seq_chain_window(P, 0, 1) == 32, "window-longest: the function sees the edit at once");
}
#endif

int main(void)
{
    test_long_holds();
    test_short_holds();
    test_odd_3();
    test_three_lengths();
    test_revisit_phase();
    test_one_chain();
    test_empty_and_silent();
    test_part_len_and_rate();
    test_polymeter();
    test_lead_in();
    test_cue_waits();
    test_part_cue_partial_window();
    test_song_record_to_playback();
    test_live_edit_and_continue();
    test_tempo_swing_clock();
    test_song();
#ifndef CHAIN_LONGEST_NO_WINDOW
    test_window_fn();
#endif
    if (fails)
        printf("chain-longest: %d failures\n", fails);
    else
        printf("chain-longest: ok\n");
    return fails ? 1 : 0;
}
