/* SPDX-License-Identifier: GPL-3.0-only */
/* Sequencer and TB-3PO tests on the host: event times to the sample, 303 gate / slide /
 * tie semantics, swing, rates, polymeter, pattern cue + chain, external clock, MIDI clock
 * out, the break clock, and TB-3PO lines identical to schwung-tb3po's for the same seed. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../firmware/src/seq/sequencer.h"
#include "../../firmware/src/seq/tb3po.h"  /* IWYU pragma: keep (tb3po_* below) */

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("  FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* ---- event log */
enum { E_DRUM, E_ON, E_OFF, E_BRK, E_BRKSTOP, E_MIDI };
typedef struct { uint32_t t; int kind, a, b, c, d; } ev_t;
static ev_t evs[100000];
static int nev;
static uint32_t now;
static void log_ev(int k, int a, int b, int c, int d)
{
    if (nev < (int)(sizeof evs / sizeof evs[0]))
        evs[nev++] = (ev_t){now, k, a, b, c, d};
}
static int s_drum(void *x, int k, int v, float vel) { (void)x; log_ev(E_DRUM, k * 100 + v, (int)(vel * 127.0f + 0.5f), 0, 0); return 1; }
static void s_on(void *x, int p, int n, int acc, int sl) { (void)x; log_ev(E_ON, p, n, acc, sl); }
static void s_off(void *x, int p) { (void)x; log_ev(E_OFF, p, 0, 0, 0); }
static void s_brk(void *x, int s16, int bar, float spb, int en) { (void)x; log_ev(E_BRK, s16, bar, (int)spb, en); }
static void s_brkstop(void *x) { (void)x; log_ev(E_BRKSTOP, 0, 0, 0, 0); }
static void s_midi(void *x, uint8_t st, uint8_t d1, uint8_t d2) { (void)x; log_ev(E_MIDI, st, d1, d2, 0); }
static const seq_sink_t SINK = {s_drum, s_on, s_off, s_brk, s_brkstop, s_midi, 0, 0};

static seq_t S;
static pattern_t P[NPAT];

/* render like the engine: block of 256, split at events */
static void run(uint32_t samples)
{
    while (samples) {
        uint32_t n = samples < 256 ? samples : 256;
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
        pattern_init(&P[i], 0x100 + i, 0x200 + i);
    seq_init(&S, P);
    S.send_clock = 0;
    nev = 0;
    now = 0;
}

static int find(int kind, int a, int from)
{
    int i;
    for (i = from; i < nev; i++)
        if (evs[i].kind == kind && (a < 0 || evs[i].a == a))
            return i;
    return -1;
}

static void test_drum_timing(void)
{
    int i, k, n = 0;
    double step = 44100.0 * 60.0 / 120.0 / 4.0;      /* 5512.5 samples */
    reset();
    S.bpm = 120.0f;
    for (i = 0; i < 16; i += 4)
        P[0].drum[0].hit[0][0] |= 1u << i;                 /* BD on the quarters */
    P[0].drum[0].hit[7][0] = 0xAAAAu;                      /* CH on the off-16ths */
    P[0].drum[0].accent[0] = 1u;                           /* accent on step 1 */
    seq_start(&S);
    run(44100 * 4 - 1);                              /* 2 bars, not the downbeat of the 3rd */
    for (i = 0, k = 0; (i = find(E_DRUM, 0, i)) >= 0; i++, k++) {
        double want = k * 4 * step;
        CHECK(abs((int)evs[i].t - (int)(want + 0.5)) <= 1, "BD %d at %u, want %.1f", k, evs[i].t, want);
        if (k % 4 == 0)
            CHECK(evs[i].b == 127, "BD %d accented vel %d", k, evs[i].b);
        else
            CHECK(evs[i].b == 88, "BD %d vel %d, want 88", k, evs[i].b);
        n++;
    }
    CHECK(n == 8, "BD hits %d, want 8 in 2 bars", n);
    for (i = 0, k = 0; (i = find(E_DRUM, 7, i)) >= 0; i++, k++)
        CHECK(abs((int)evs[i].t - (int)((2 * k + 1) * step + 0.5)) <= 1, "CH %d at %u", k, evs[i].t);
    CHECK(k == 16, "CH hits %d", k);
    printf("  drum timing: %d BD, %d CH on the sample\n", n, k);
}

static void test_swing_and_rates(void)
{
    int i, k;
    double step = 44100.0 * 60.0 / 120.0 / 4.0;
    reset();
    S.bpm = 120.0f;
    P[0].swing = 100;                                /* 75 %: odd 16ths half a step late */
    P[0].drum[0].hit[7][0] = 0xFFFFu;
    seq_start(&S);
    run(44100 * 2);
    for (i = 0, k = 0; (i = find(E_DRUM, 7, i)) >= 0 && k < 16; i++, k++) {
        double want = k * step + ((k & 1) ? 0.5 * step : 0.0);
        CHECK(abs((int)evs[i].t - (int)(want + 0.5)) <= 1, "swung CH %d at %u want %.1f", k, evs[i].t, want);
    }
    /* triplets: 1/16T = 1/6 quarter, never swung */
    reset();
    S.bpm = 120.0f;
    P[0].swing = 100;
    P[0].drum[0].rate = RATE_16T;
    P[0].drum[0].len = 12;
    P[0].drum[0].hit[7][0] = 0xFFFu;
    seq_start(&S);
    run(44100);
    for (i = 0, k = 0; (i = find(E_DRUM, 7, i)) >= 0 && k < 12; i++, k++) {
        double want = k * 44100.0 * 60.0 / 120.0 / 6.0;
        CHECK(abs((int)evs[i].t - (int)(want + 0.5)) <= 1, "triplet %d at %u want %.1f", k, evs[i].t, want);
    }
    printf("  swing 75%%, 1/16T unswung: checked\n");
}

static void test_303(void)
{
    bpart_t *b;
    int i, on, off;
    double step = 44100.0 * 60.0 / 120.0 / 4.0;
    reset();
    S.bpm = 120.0f;
    b = &P[0].bass[0];
    b->len = 8;
    b->step[0] = (bstep_t){36, G_NOTE};                       /* plain: off at half step */
    b->step[1] = (bstep_t){38, G_NOTE | BS_SLIDE};            /* slides into step 2 */
    b->step[2] = (bstep_t){41, G_NOTE | BS_ACCENT};           /* reached by slide, accented */
    b->step[3] = (bstep_t){41, G_TIE};                        /* tie: 2 held through 3 */
    b->step[4] = (bstep_t){0, G_REST};
    b->step[5] = (bstep_t){43, G_TIE};                        /* tie after rest = a note */
    b->transpose = 26;                                        /* +2 */
    seq_start(&S);
    run((uint32_t)(step * 8));
    on = find(E_ON, 0, 0);
    CHECK(on >= 0 && evs[on].b == 38 && evs[on].d == 0 && evs[on].t == 0, "step 1 note 36+2, no slide");
    off = find(E_OFF, 0, on);
    CHECK(off >= 0 && abs((int)evs[off].t - (int)(step / 2)) <= 1, "step 1 off at half step (%u)", off >= 0 ? evs[off].t : 0);
    on = find(E_ON, 0, on + 1);
    CHECK(on >= 0 && evs[on].b == 40 && evs[on].d == 0, "step 2 on");
    i = find(E_ON, 0, on + 1);
    off = find(E_OFF, 0, on + 1);
    CHECK(i >= 0 && evs[i].b == 43 && evs[i].d == 1 && evs[i].c == 1, "step 3 slides (slide=%d acc=%d)", i >= 0 ? evs[i].d : -1, i >= 0 ? evs[i].c : -1);
    CHECK(off > i, "no gate off between a slide and its target");
    CHECK(off >= 0 && abs((int)evs[off].t - (int)(3.5 * step + 0.5)) <= 1, "tie holds step 3 into step 4, whose gate falls at its half (%u, want %.0f)", off >= 0 ? evs[off].t : 0, 3.5 * step);
    i = find(E_ON, 0, off);
    CHECK(i >= 0 && evs[i].b == 45 && abs((int)evs[i].t - (int)(5 * step + 0.5)) <= 1, "tie after a rest plays as a note");
    printf("  303 gate / slide / tie / transpose: checked\n");
}

static void test_polymeter_cue_chain(void)
{
    int i, n, last;
    double step = 44100.0 * 60.0 / 120.0 / 4.0;
    reset();
    S.bpm = 120.0f;
    P[0].bass[0].len = 3;
    P[0].bass[0].step[0] = (bstep_t){36, G_NOTE};
    P[0].drum[0].hit[0][0] = 1u;
    P[1].drum[0].hit[1][0] = 1u;                                    /* SD only in pattern 2 */
    seq_start(&S);
    run((uint32_t)(step * 15) - 1);                         /* not the note due exactly at step 15 */
    for (i = 0, n = 0; (i = find(E_ON, 0, i)) >= 0; i++, n++)
        CHECK(abs((int)evs[i].t - (int)(n * 3 * step + 0.5)) <= 1, "3-step 303 loop %d at %u", n, evs[i].t);
    CHECK(n == 5, "3-step line over 15 steps: %d notes", n);
    seq_cue(&S, 1);
    nev = 0;
    run((uint32_t)(step * 17));                               /* to the bar line and one more */
    i = find(E_DRUM, 1, 0);
    CHECK(i >= 0 && abs((int)evs[i].t - (int)(16 * step + 0.5)) <= 1, "cued pattern starts on the bar (%u)", i >= 0 ? evs[i].t : 0);
    CHECK(S.ppat[0] == 1 && S.ppat[4] == 1, "now playing pattern 2");
    i = find(E_ON, 0, 0);
    /* pattern 2's 303 A line is empty: no notes after the switch */
    for (n = 0; (i = find(E_ON, 0, i < 0 ? 0 : i)) >= 0; i++)
        if (evs[i].t >= (uint32_t)(16 * step))
            n++;
    CHECK(n == 0, "303 restarts with the new (empty) pattern");
    /* chain 3..5 */
    reset();
    S.bpm = 120.0f;
    for (i = 0; i < NPAT; i++)
        P[i].drum[0].hit[0][0] = 1u;
    for (i = 0; i < NTRACKS; i++)
        S.ppat[i] = 3;
    seq_chain(&S, 3, 5);
    seq_start(&S);
    for (i = 0, last = -1; i < 7; i++) {
        uint8_t c = S.ppat[0];
        run((uint32_t)(16 * step));
        if (i < 6)
            CHECK(c == 3 + (i % 3), "chain bar %d plays pattern %d (got %d)", i, 3 + i % 3, c + 1 - 1);
        last = c;
    }
    (void)last;
    printf("  polymeter, cue on the bar, chain 4-6: checked\n");
}

/* one part's cue: it switches on the bar and restarts; the others keep their place */
static void test_part_cue(void)
{
    int i, n;
    double step = 44100.0 * 60.0 / 120.0 / 4.0;
    reset();
    S.bpm = 120.0f;
    P[0].drum[0].hit[0][0] = 1u;
    P[0].bass[1].len = 3;                                       /* 303 B: a 3-step loop */
    P[0].bass[1].step[0] = (bstep_t){40, G_NOTE};
    P[1].bass[0].step[0] = (bstep_t){50, G_NOTE};               /* 303 A in pattern 2 */
    seq_start(&S);
    run((uint32_t)(step * 5));
    seq_cue_part(&S, TRK_BASS0, 1);
    run((uint32_t)(step * 11) - 1);                             /* to just before the bar */
    CHECK(S.ppat[TRK_BASS0] == 0, "a part's cue waits for the bar");
    nev = 0;
    run((uint32_t)(step * 4));
    CHECK(S.ppat[TRK_BASS0] == 1 && S.ppat[0] == 0 && S.ppat[TRK_BASS0 + 1] == 0, "only 303 A switched");
    i = find(E_ON, 0, 0);
    CHECK(i >= 0 && evs[i].b == 50 && abs((int)evs[i].t - (int)(16 * step + 0.5)) <= 1, "303 A's new line on the bar");
    for (i = 0, n = 0; (i = find(E_ON, 1, i)) >= 0; i++, n++)   /* 303 B: steps 18 (its 7th loop), not 16 */
        CHECK(abs((int)evs[i].t - (int)(18 * step + 0.5)) <= 1, "303 B kept its place (note at %u)", evs[i].t);
    CHECK(n == 1, "303 B: one note in the window, %d", n);
    printf("  a part's own cue: checked\n");
}

static void test_song(void)
{
    static song_t sg;
    double bar = 44100.0 * 60.0 / 120.0 * 4.0;
    int i;
    /* playback: three bars, looping; a bar's mutes */
    reset();
    S.bpm = 120.0f;
    S.song = &sg;
    sg.len = 3;
    for (i = 0; i < NTRACKS; i++) {
        sg.bar[0].pat[i] = 0;
        sg.bar[1].pat[i] = 1;
        sg.bar[2].pat[i] = 2;
    }
    sg.bar[1].pat[TRK_BRK] = 7;
    sg.bar[0].mute = sg.bar[1].mute = 0;
    sg.bar[2].mute = 1u << 1;                                   /* the 808 out in bar 3 */
    S.song_on = 1;
    S.song_start = 1;
    seq_start(&S);
    run(1);
    CHECK(S.ppat[0] == 1 && S.ppat[TRK_BRK] == 7 && S.song_pos == 1, "starts at the start bar");
    run((uint32_t)bar);
    CHECK(S.ppat[0] == 2 && S.song_pos == 2 && seq_part_muted(&S, 1) && !seq_part_muted(&S, 0), "bar 3, 808 muted");
    run((uint32_t)bar);
    CHECK(S.ppat[0] == 0 && S.song_pos == 0 && !seq_part_muted(&S, 1), "loops to bar 1, 808 back");
    seq_cue(&S, 9);
    run((uint32_t)bar);
    CHECK(S.ppat[0] == 1, "playing the song, a cue does not override it");
    /* recording: from an empty song, the live cues and mutes are written bar by bar */
    reset();
    S.bpm = 120.0f;
    S.song = &sg;
    sg.len = 0;
    S.song_on = S.song_rec = 1;
    S.song_start = 0;
    seq_start(&S);
    run(1);
    CHECK(sg.len == 1 && sg.bar[0].pat[0] == 0, "record: bar 1 written at the start");
    seq_cue_part(&S, TRK_BASS0, 5);
    S.mute |= seq_part_mask(TRK_BRK);
    run((uint32_t)bar);
    CHECK(sg.len == 2 && sg.bar[1].pat[TRK_BASS0] == 5 && sg.bar[1].pat[0] == 0 && sg.bar[1].mute == 1u << TRK_BRK,
          "record: bar 2 has the cue and the mute (len %d, pat %d, mute %d)", sg.len, sg.bar[1].pat[TRK_BASS0], sg.bar[1].mute);
    run((uint32_t)bar * 2);
    CHECK(sg.len == 4 && sg.bar[3].pat[TRK_BASS0] == 5, "record: the song grows while it runs (%d)", sg.len);
    /* overdub from bar 2 plays bar 2 as it is, then writes on */
    seq_stop(&S);
    run(1);
    S.song_start = 1;
    sg.bar[1].pat[0] = 6;
    seq_start(&S);
    run(1);
    CHECK(S.ppat[0] == 6 && sg.len == 4, "overdub starts from the song's bar");
    printf("  song: playback, loop, mutes, record, overdub: checked\n");
}

static void test_ext_clock_and_midi_out(void)
{
    int i, k, clocks;
    uint32_t per_tick = 919;                                  /* ~120 BPM: 44100*60/120/24 = 918.75 */
    reset();
    P[0].drum[0].hit[0][0] = 0xFFFFu;
    for (i = 0; i < 2; i++) {                                 /* clocks before start: tempo only */
        seq_ext_clock(&S);
        run(per_tick);
    }
    seq_start(&S);
    nev = 0;
    {
    uint32_t t_start = now;
    for (i = 0; i < 24 * 4; i++) {                            /* one bar of ticks */
        uint32_t t0 = now;
        seq_ext_clock(&S);
        run(per_tick);
        (void)t0;
    }
    CHECK(S.ext == 1, "following the external clock");
    for (i = 0, k = 0; (i = find(E_DRUM, 0, i)) >= 0; i++, k++)
        CHECK((evs[i].t - t_start) == (uint32_t)k * 6u * per_tick, "ext step %d at +%u, want tick %d", k, evs[i].t - t_start, 6 * k);
    }
    CHECK(k == 16, "16 steps in 96 ticks, got %d", k);
    CHECK(S.ext_bpm > 119.0f && S.ext_bpm < 121.0f, "tempo estimate %.2f", (double)S.ext_bpm);
    run(44100);                                               /* clock gone: back to internal */
    CHECK(S.ext == 0, "internal again after the clock stops");
    /* MIDI clock out: 24 per quarter */
    reset();
    S.send_clock = 1;
    S.bpm = 120.0f;
    seq_start(&S);
    run(44100 * 2 - 1);                                       /* 4 quarters, not the next downbeat */
    for (i = 0, clocks = 0; i < nev; i++)
        if (evs[i].kind == E_MIDI && evs[i].a == 0xF8)
            clocks++;
    CHECK(clocks == 96, "MIDI clocks in 2 s at 120 BPM: %d, want 96", clocks);
    CHECK(find(E_MIDI, 0xFA, 0) == 0, "Start sent first");
    seq_stop(&S);
    run(256);
    CHECK(find(E_MIDI, 0xFC, 0) > 0, "Stop sent");
    printf("  external clock follow + tempo, MIDI clock out: checked\n");
}

/* ---- MIDI clock as Bluetooth MIDI gives it: the ticks of one connection event come in one block */
static double ble_tick, ble_ev;                               /* the next tick and the next connection event */
static int ble_n;                                             /* the ticks given */
static void ble_clock(double bpm, double ev_ms, double secs)
{
    double tick = 44100.0 * 60.0 / bpm / 24.0, ev = 44100.0 * ev_ms / 1000.0, end = now + secs * 44100.0;
    while (now < end) {
        while (ble_ev <= now) {                               /* the events since the last block */
            while (ble_tick <= ble_ev) {
                seq_ext_clock(&S);
                ble_n++;
                ble_tick += tick;
            }
            ble_ev += ev;
        }
        run(256);
    }
}
static void ble_clock_start(double bpm, double ev_ms, double phase)
{
    ble_tick = now + phase * 44100.0 * 60.0 / bpm / 24.0;
    ble_ev = now + (1.0 - phase) * 44100.0 * ev_ms / 1000.0;
}

/* ext 0xF8 at 120 BPM (and 93, 140) in Bluetooth bursts: the measured tempo, then the link ends:
 * the internal tempo keeps the last external tempo */
static void test_ext_tempo_ble(void)
{
    static const double EV[] = {1.0, 7.5, 11.25, 15.0, 30.0}, BPM[] = {120.0, 93.0, 140.0};
    unsigned e, b, ph;
    for (e = 0; e < sizeof EV / sizeof EV[0]; e++)
        for (b = 0; b < sizeof BPM / sizeof BPM[0]; b++)
            for (ph = 0; ph < 4; ph++) {
                reset();
                S.bpm = 100.0f;
                ble_clock_start(BPM[b], EV[e], 0.1 + 0.2 * ph);
                ble_clock(BPM[b], EV[e], 10.0);
                CHECK(S.ext == 1, "following the clock (%.2f ms events)", EV[e]);
                CHECK(S.ext_bpm > BPM[b] - 0.5 && S.ext_bpm < BPM[b] + 0.5 && seq_tempo(&S) == S.ext_bpm,
                      "%.0f BPM in %.2f ms events (phase %u): measured %.2f", BPM[b], EV[e], ph, (double)S.ext_bpm);
                run(44100);                                   /* the link ends: no tick for 1 s */
                CHECK(S.ext == 0 && S.bpm == (float)BPM[b], "after the link end: internal %.1f, want %.0f",
                      (double)S.bpm, BPM[b]);
            }
    reset();                                                  /* a tempo change: 120, then 140 */
    ble_clock_start(120.0, 15.0, 0.5);
    ble_clock(120.0, 15.0, 6.0);
    ble_clock(140.0, 15.0, 6.0);
    CHECK(S.ext_bpm > 139.5f && S.ext_bpm < 140.5f, "tempo change to 140: measured %.2f", (double)S.ext_bpm);
    printf("  external clock in Bluetooth bursts: tempo, tempo kept after the link end: checked\n");
}

/* Start, Stop and Continue drive the transport (clock in 7.5 ms Bluetooth events) */
static void test_ext_transport_ble(void)
{
    int i, k, n;
    uint32_t t0;
    double tick = 44100.0 * 60.0 / 120.0 / 24.0;
    reset();
    P[0].drum[0].hit[0][0] = 0xFFFFu;
    ble_clock_start(120.0, 7.5, 0.5);
    ble_clock(120.0, 7.5, 1.0);                               /* the clock runs, X0X stopped */
    CHECK(!S.playing && S.ext, "stopped, the clock runs");
    seq_start(&S);                                            /* Start */
    nev = 0;
    ble_clock(120.0, 7.5, 2.0);                               /* one bar at 120 */
    CHECK(S.playing, "Start: playing");
    for (i = 0, n = 0; (i = find(E_DRUM, 0, i)) >= 0; i++)
        n++;
    CHECK(n == 16, "Start: 16 steps in one bar, got %d", n);
    seq_stop(&S);                                             /* Stop */
    run(256);
    CHECK(!S.playing, "Stop: stopped");
    k = S.t[0].pos;
    nev = 0;
    ble_clock(120.0, 7.5, 0.25);
    CHECK(find(E_DRUM, 0, 0) < 0, "Stop: no step while stopped");
    seq_continue(&S);                                         /* Continue: from the next step */
    ble_clock(120.0, 7.5, 0.05);                              /* (2 ticks: the first step only) */
    CHECK(S.playing && S.t[0].pos == (k + 1) % 16, "Continue: step %d, want %d", S.t[0].pos, (k + 1) % 16);
    seq_stop(&S);                                             /* Stop and Start in one burst: a restart */
    seq_start(&S);
    run(256);
    CHECK(S.playing, "Stop + Start in one block: playing");
    ble_clock(120.0, 7.5, 0.05);
    CHECK(S.t[0].pos == 0, "Stop + Start in one block: from step 1, at step %d", S.t[0].pos + 1);
    seq_start(&S);                                            /* Start + Stop in one burst: stopped */
    seq_stop(&S);
    run(256);
    CHECK(!S.playing, "Start + Stop in one block: stopped");

    /* no clock for 1 s, then Start, and the first tick one event later: no double step */
    reset();
    P[0].drum[0].hit[0][0] = 0xFFFFu;
    S.bpm = 120.0f;
    seq_start(&S);
    run(256);
    CHECK(S.playing && !S.ext, "Start without clock: internal");
    t0 = now;
    ble_clock_start(120.0, 7.5, 0.0);
    ble_tick = now + 330.0;                                   /* the first tick 7.5 ms after Start */
    ble_ev = ble_tick;
    ble_clock(120.0, 7.5, 2.0 - 330.0 / 44100.0);
    for (i = 0, n = 0; (i = find(E_DRUM, 0, i)) >= 0; i++)
        n++;
    CHECK(n == 16, "Start, then the clock: 16 steps in one bar, got %d", n);
    i = find(E_DRUM, 0, find(E_DRUM, 0, 0) + 1);              /* step 2: 6 ticks after the first tick */
    CHECK(i >= 0 && evs[i].t - t0 >= (uint32_t)(330.0 + 6.0 * tick - 400.0) && evs[i].t - t0 <= (uint32_t)(330.0 + 6.0 * tick + 600.0),
          "Start, then the clock: step 2 at +%u, want about +%.0f", i >= 0 ? evs[i].t - t0 : 0, 330.0 + 6.0 * tick);
    printf("  Start, Stop, Continue with the clock in Bluetooth bursts: checked\n");
}

/* a73: one block with the ticks of two or more step boundaries (a long connection event, or a stall).
 * Each boundary plays its step: the 909 plays a step on each step, so the steps it plays = the clock's steps. */
static int burst_steps(int rate, double bpm, double ev_ms, double phase, double stall_ms, int *want)
{
    int i, n, tps = rate == RATE_32 ? 3 : 6;
    reset();
    P[0].drum[0].hit[0][0] = 0xFFFFFFFFu;
    P[0].drum[0].hit[0][1] = 0xFFFFFFFFu;
    P[0].drum[0].rate = (uint8_t)rate;
    ble_clock_start(bpm, ev_ms, phase);
    ble_clock(bpm, ev_ms, 1.0);                               /* the clock runs, X0X stopped */
    seq_start(&S);
    nev = 0;
    ble_n = 0;
    ble_clock(bpm, ev_ms, 2.0);
    if (stall_ms > 0.0) {                                     /* no connection event, then all its ticks in one */
        ble_ev += 44100.0 * stall_ms / 1000.0;
        ble_clock(bpm, ev_ms, 2.0);
    }
    run(4410);                                                /* (the last block's steps) */
    for (i = 0, n = 0; (i = find(E_DRUM, 0, i)) >= 0; i++)
        n++;
    *want = (ble_n + tps - 1) / tps;
    return n;
}
static void test_ext_burst_steps(void)
{
    static const double BPM[] = {120.0, 180.0, 240.0, 280.0}, EV[] = {7.5, 15.0, 30.0, 45.0};
    unsigned b, e, ph;
    int r, n, want;
    for (r = 0; r < 2; r++)
        for (b = 0; b < sizeof BPM / sizeof BPM[0]; b++)
            for (e = 0; e < sizeof EV / sizeof EV[0]; e++)
                for (ph = 0; ph < 3; ph++) {
                    n = burst_steps(r ? RATE_32 : RATE_16, BPM[b], EV[e], 0.1 + 0.3 * ph, 0.0, &want);
                    CHECK(n == want, "%s %.0f BPM, %.1f ms events (phase %u): %d steps, the clock's %d",
                          r ? "1/32" : "1/16", BPM[b], EV[e], ph, n, want);
                }
    for (r = 0; r < 2; r++)                                   /* a 400 ms stall at 120 BPM (and 220-460 ms) */
        for (e = 0; e < 4; e++)
            for (ph = 0; ph < 3; ph++) {
                double st = e == 0 ? 400.0 : 100.0 + 120.0 * e;
                n = burst_steps(r ? RATE_32 : RATE_16, 120.0, 7.5, 0.1 + 0.3 * ph, st, &want);
                CHECK(n == want, "%s 120 BPM, 7.5 ms events, a %.0f ms stall (phase %u): %d steps, the clock's %d",
                      r ? "1/32" : "1/16", st, ph, n, want);
            }
    printf("  external clock, two or more step boundaries in one block: no step lost\n");
}

static void test_break_clock(void)
{
    int i, k;
    reset();
    S.bpm = 170.0f;
    P[0].drum[0].rate = RATE_16T;                                /* the break stays on 16ths regardless */
    P[0].drum[0].len = 12;
    P[0].brk.steps = 0x00FFu;                                 /* first half of the bar */
    seq_start(&S);
    run(44100 * 3);
    for (i = 0, k = 0; (i = find(E_BRK, -1, i)) >= 0; i++, k++) {
        CHECK(evs[i].a == k % 16, "break step %d is %d", k, evs[i].a);
        CHECK(evs[i].b == k / 16, "break bar %d", evs[i].b);
        CHECK(evs[i].d == (k % 16 < 8), "break step %d enabled=%d", k, evs[i].d);
    }
    CHECK(k > 16 * 2, "break clocked %d times", k);
    seq_stop(&S);
    run(256);
    CHECK(find(E_BRKSTOP, -1, 0) >= 0, "break told to stop");
    printf("  break clock: %d 16ths, bars counted, step mask honoured\n", k);
}

/* ---- TB-3PO against schwung-tb3po: include its plugin source and call its generator */
#ifdef REF_TB3PO
#define move_plugin_init_v2 ref_move_plugin_init_v2
#define slot_init_defaults ref_slot_init_defaults
#define generate_pattern ref_generate_pattern
#define mutate_pattern ref_mutate_pattern
#include REF_TB3PO
static void test_tb3po_matches_reference(void)
{
    int seed, len, i, same = 0, total = 0;
    for (seed = 1; seed < 400; seed++) {
        for (len = 8; len <= 32; len += 8) {
            tb3po_slot_t ref;
            bpart_t b;
            ref_slot_init_defaults(&ref);
            ref.length = len;
            ref.density = (float)(30 + seed % 70) / 100.0f;
            ref.accent = (float)(seed % 90) / 100.0f;
            ref.slide = (float)(seed % 60) / 100.0f;
            ref.octave_range = 1 + seed % 3;
            ref.scale = seed % 6;
            ref.root = seed % 12;
            ref_generate_pattern(&ref, (uint32_t)seed * 7919u);
            memset(&b, 0, sizeof b);
            b.len = (uint8_t)len;
            tb3po_defaults(&b.gen, (uint32_t)seed * 7919u);
            b.gen.density = (uint8_t)(30 + seed % 70);
            b.gen.accent = (uint8_t)(seed % 90);
            b.gen.slide = (uint8_t)(seed % 60);
            b.gen.oct_range = (uint8_t)(1 + seed % 3);
            b.gen.scale = (uint8_t)(seed % 6);
            b.gen.root = (uint8_t)(seed % 12);
            tb3po_generate(&b);
            for (i = 0; i < len; i++) {
                int rk = ref.steps[i], gate = bstep_gate(&b.step[i]);
                int kind = gate == G_REST ? 0 : (b.step[i].flags & BS_SLIDE) ? 3 : (b.step[i].flags & BS_ACCENT) ? 2 : 1;
                int ok = rk == kind;
                if (ok && kind)
                    ok = b.step[i].note == note_for_step(&ref, i);
                same += ok;
                total++;
                if (!ok && total - same < 5)
                    printf("  seed %d len %d step %d: ref kind %d note %d, ours %d note %d\n", seed, len, i, rk,
                           note_for_step(&ref, i), kind, b.step[i].note);
            }
        }
    }
    CHECK(same == total, "TB-3PO steps identical to schwung-tb3po: %d of %d", same, total);
    printf("  TB-3PO vs schwung-tb3po: %d / %d steps identical over 396 seeds x 4 lengths\n", same, total);
}
#endif

int main(void)
{
    test_drum_timing();
    test_swing_and_rates();
    test_303();
    test_polymeter_cue_chain();
    test_part_cue();
    test_song();
    test_ext_clock_and_midi_out();
    test_ext_tempo_ble();
    test_ext_transport_ble();
    test_ext_burst_steps();
    test_break_clock();
#ifdef REF_TB3PO
    test_tb3po_matches_reference();
#endif
    printf("seq: %s\n", fails ? "FAIL" : "ok");
    return fails != 0;
}
