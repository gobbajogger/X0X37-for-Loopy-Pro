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
enum { E_DRUM, E_ON, E_OFF, E_BRK, E_BRKSTOP, E_MIDI, E_STEP };
typedef struct { uint32_t t; int kind, a, b, c, d; } ev_t;
static ev_t evs[100000];
static int nev;
static uint32_t now;
static int last_bass_velocity;
static void log_ev(int k, int a, int b, int c, int d)
{
    if (nev < (int)(sizeof evs / sizeof evs[0]))
        evs[nev++] = (ev_t){now, k, a, b, c, d};
}
static int s_drum(void *x, int k, int v, float vel) { (void)x; log_ev(E_DRUM, k * 100 + v, (int)(vel * 127.0f + 0.5f), 0, 0); return 1; }
static int s_on(void *x, int p, int n, int acc, int sl, int velocity) { (void)x; last_bass_velocity = velocity; log_ev(E_ON, p, n, acc, sl); return 1; }
static void s_off(void *x, int p) { (void)x; log_ev(E_OFF, p, 0, 0, 0); }
static void s_brk(void *x, int s16, int bar, float spb, int en) { (void)x; log_ev(E_BRK, s16, bar, (int)spb, en); }
static void s_brkstop(void *x) { (void)x; log_ev(E_BRKSTOP, 0, 0, 0, 0); }
static void s_midi(void *x, uint8_t st, uint8_t d1, uint8_t d2) { (void)x; log_ev(E_MIDI, st, d1, d2, 0); }
static const seq_sink_t SINK = {s_drum, s_on, s_off, s_brk, s_brkstop, s_midi, 0, 0};
static void s_step(void *x, int part, int st) { (void)x; if (part == 0) log_ev(E_STEP, st, 0, 0, 0); }
static const seq_sink_t SINK_STEP = {s_drum, s_on, s_off, s_brk, s_brkstop, s_midi, s_step, 0};   /* + the 909's steps */
static const seq_sink_t *sink = &SINK;
static uint8_t clk_src;                                        /* reset() sets this clock source (SEQ_CLK_*) */

static seq_t S;
static pattern_t P[NPAT];

/* render like the engine: block of 256, split at events */
static void run(uint32_t samples)
{
    while (samples) {
        uint32_t n = samples < 256 ? samples : 256;
        seq_advance(&S, 0, sink);
        while (n) {
            uint32_t k = seq_until_event(&S, n);
            now += k;
            seq_advance(&S, k, sink);
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
    S.clk_src = clk_src;
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

/* T_VEL / seq.drum_vel: a recorded strike velocity overrides accent_q7 for that voice. */
static void test_drum_step_velocity(void)
{
    int i;
    reset();
    S.bpm = 120.0f;
    S.accent_q7 = 88;
    P[0].drum[0].hit[0][0] = 1u;                           /* BD on step 1 only */
    S.drum_vel[0][0] = 40;
    seq_start(&S);
    run(5513);
    i = find(E_DRUM, 0, 0);
    CHECK(i >= 0 && evs[i].b == 40, "recorded drum velocity reaches the sink (%d)", i >= 0 ? evs[i].b : -1);
    nev = 0;
    S.drum_vel[0][0] = 0;                                  /* legacy: accent row / accent_q7 */
    seq_start(&S);
    run(5513);
    i = find(E_DRUM, 0, 0);
    CHECK(i >= 0 && evs[i].b == 88, "zero drum_vel keeps accent_q7 (%d)", i >= 0 ? evs[i].b : -1);
    printf("  drum step velocity: recorded 40, legacy 88\n");
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
    b->step[0] = (bstep_t){36, G_NOTE, 0};                       /* plain: off at half step */
    b->step[1] = (bstep_t){38, G_NOTE | BS_SLIDE, 0};            /* slides into step 2 */
    b->step[2] = (bstep_t){41, G_NOTE | BS_ACCENT, 0};           /* reached by slide, accented */
    b->step[3] = (bstep_t){41, G_TIE, 0};                        /* tie: 2 held through 3 */
    b->step[4] = (bstep_t){0, G_REST, 0};
    b->step[5] = (bstep_t){43, G_TIE, 0};                        /* orphan tie after rest stays silent */
    b->step[6] = (bstep_t){45, G_NOTE, 0};                       /* later NOTE attacks cleanly */
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
    CHECK(i >= 0 && evs[i].b == 47 && evs[i].d == 0 && abs((int)evs[i].t - (int)(6 * step + 0.5)) <= 1, "orphan TIE stays silent; later NOTE attacks without slide");
    printf("  303 gate / slide / tie / transpose: checked\n");
}

static void test_303_velocity_slide_overlap(void)
{
    int i, on = -1, off = -1;
    reset();
    S.bpm = 120.0f;
    S.send_notes = 1;
    P[0].bass[0].len = 4;
    P[0].bass[0].step[0] = (bstep_t){36, G_NOTE | BS_SLIDE, 72};
    P[0].bass[0].step[1] = (bstep_t){41, G_NOTE, 115};
    seq_start(&S);
    run(7000);
    CHECK(last_bass_velocity == 115, "step velocity reaches the synth sink (%d)", last_bass_velocity);
    for (i = 0; i < nev; i++) {
        if (evs[i].kind != E_MIDI)
            continue;
        if (evs[i].a == 0x91 && evs[i].b == 36)
            CHECK(evs[i].c == 72, "first MIDI velocity %d", evs[i].c);
        if (evs[i].a == 0x91 && evs[i].b == 41) {
            on = i;
            CHECK(evs[i].c == 115, "slide target MIDI velocity %d", evs[i].c);
        }
        if (evs[i].a == 0x81 && evs[i].b == 36)
            off = i;
    }
    CHECK(on >= 0 && off > on && evs[off].t - evs[on].t == FS / 50u,
          "slide MIDI notes overlap 20 ms (%u samples)", on >= 0 && off >= 0 ? evs[off].t - evs[on].t : 0);
    CHECK(bstep_velocity(&P[0].bass[0].step[2]) == BASS_VELOCITY_DEFAULT,
          "unset step velocity follows legacy default");
}

static void test_303_midi_lifecycle(void)
{
    int i, on, off, mode;
    reset();
    S.send_notes = 1;
    P[0].bass[0].len = 2;
    P[0].bass[0].transpose = 0;                 /* note 24 down to MIDI pitch zero */
    P[0].bass[0].step[0] = (bstep_t){24, G_NOTE, 100};
    seq_start(&S);
    run(3000);
    on = off = -1;
    for (i = 0; i < nev; i++) {
        if (evs[i].kind == E_MIDI && evs[i].a == 0x91 && evs[i].b == 0) on = i;
        if (evs[i].kind == E_MIDI && evs[i].a == 0x81 && evs[i].b == 0) off = i;
    }
    CHECK(on >= 0 && off > on, "MIDI note zero must have a Note Off");

    reset();
    S.send_notes = 1;
    P[0].bass[0].len = 4;
    P[0].bass[0].step[0] = (bstep_t){36, G_NOTE | BS_SLIDE, 80};
    P[0].bass[0].step[1] = (bstep_t){41, G_NOTE, 100};
    seq_start(&S);
    run(5600);                                    /* old note's delayed off is pending */
    S.ch[TRK_BASS0] = 5;
    run(900);
    off = on = -1;
    for (i = 0; i < nev; i++)
        if (evs[i].kind == E_MIDI && evs[i].b == 36 && (evs[i].a & 0xF0) == 0x80)
            off = i;
    for (i = 0; i < nev; i++)
        if (evs[i].kind == E_MIDI && evs[i].b == 41 && (evs[i].a & 0xF0) == 0x80)
            on = i;
    CHECK(off >= 0 && on > off && evs[off].a == 0x81 && evs[on].a == 0x81,
          "channel change flushes both old and current notes on their original channel");

    for (mode = 0; mode < 2; mode++) {
        int old_off = -1, current_off = -1, request_at;
        reset();
        S.send_notes = 1;
        P[0].bass[0].len = 4;
        P[0].bass[0].step[0] = (bstep_t){36, G_NOTE | BS_SLIDE, 100};
        P[0].bass[0].step[1] = (bstep_t){41, G_NOTE, 100};
        seq_start(&S);
        run(5600);
        request_at = nev;
        if (mode) seq_continue(&S); else seq_start(&S);
        run(1);
        for (i = request_at; i < nev; i++) {
            if (evs[i].kind == E_MIDI && evs[i].a == 0x81 && evs[i].b == 36) old_off = i;
            if (evs[i].kind == E_MIDI && evs[i].a == 0x81 && evs[i].b == 41) current_off = i;
        }
        CHECK(old_off >= request_at && current_off > old_off,
              "%s flushes both pending and active MIDI notes", mode ? "Continue" : "Start");
    }

    reset();
    S.send_notes = 1;
    P[0].bass[0].len = 2;
    P[0].bass[0].step[0] = (bstep_t){36, G_NOTE | BS_SLIDE, 100};
    P[0].bass[0].step[1] = (bstep_t){36, G_NOTE, 100};
    seq_start(&S);
    run(5600);
    on = off = -1;
    for (i = 0; i < nev; i++) {
        if (evs[i].kind == E_MIDI && evs[i].a == 0x81 && evs[i].b == 36) off = i;
        if (off >= 0 && evs[i].kind == E_MIDI && evs[i].a == 0x91 && evs[i].b == 36) on = i;
    }
    CHECK(off >= 0 && on > off && evs[on].t == evs[off].t,
          "same-pitch slide ends old MIDI note before retrigger at the same sample");

    reset();
    for (i = 0; i < NSTEPS; i++)
        P[0].bass[0].step[i].velocity = 42;
    tb3po_generate(&P[0].bass[0]);
    for (i = 0; i < NSTEPS; i++)
        CHECK(P[0].bass[0].step[i].velocity == 0,
              "generator must clear stale velocity at step %d", i);
}

static void test_polymeter_cue_chain(void)
{
    int i, n, last;
    double step = 44100.0 * 60.0 / 120.0 / 4.0;
    reset();
    S.bpm = 120.0f;
    P[0].bass[0].len = 3;
    P[0].bass[0].step[0] = (bstep_t){36, G_NOTE, 0};
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
    P[0].bass[1].step[0] = (bstep_t){40, G_NOTE, 0};
    P[1].bass[0].step[0] = (bstep_t){50, G_NOTE, 0};               /* 303 A in pattern 2 */
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
static int ble_rt;                                            /* 1: the ticks go through seq_midi_rt (as engine.c) */
static void ble_clock(double bpm, double ev_ms, double secs)
{
    double tick = 44100.0 * 60.0 / bpm / 24.0, ev = 44100.0 * ev_ms / 1000.0, end = now + secs * 44100.0;
    while (now < end) {
        while (ble_ev <= now) {                               /* the events since the last block */
            while (ble_tick <= ble_ev) {
                if (ble_rt)
                    seq_midi_rt(&S, 0xF8);
                else
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

/* a73 (owner OK 2026-10-16 20:11): a live hit records on the nearest step. After half of the step it goes
 * to the next step (samples on the internal clock, ticks on an external clock). A hit on a step that did
 * not play yet on this pass does not sound again on that pass (the key sounded it); the next pass plays it. */
static int rec_hit(int t, int v)                              /* as ui.c drum_key and bass_kbd do */
{
    int st = seq_rec_step(&S, t);
    if (t < TRK_BASS0)
        sm_set(P[S.ppat[t]].drum[t].hit[v], st);
    else {
        P[S.ppat[t]].bass[t - TRK_BASS0].step[st].note = (uint8_t)v;
        P[S.ppat[t]].bass[t - TRK_BASS0].step[st].flags = G_NOTE;
    }
    seq_rec_skip(&S, t, st, t < TRK_BASS0 ? v : 0);
    return st;
}
static int count_ev(int kind, int a, int b, int from, int to)  /* events [from, to) of that kind (b < 0: any) */
{
    int i, n = 0;
    for (i = from; i < to && i < nev; i++)
        if (evs[i].kind == kind && evs[i].a == a && (b < 0 || evs[i].b == b))
            n++;
    return n;
}
static void test_rec_nearest(void)
{
    static const struct { int step, off_ms, v, want; } H[] = {
        {4, -20, 3, 4},                                       /* early: the step it is for */
        {8, 20, 4, 8},                                        /* late: the step that plays */
        {12, -65, 6, 11},                                     /* 60 ms late: stays */
        {12, -60, 5, 12},                                     /* 60 ms early (half: 62.5 ms): the next step */
        {16, -20, 7, 0},                                      /* just before the loop start: step 1 */
    };
    static const int WANT[][3] = {{0, 1, 1}, {0, 1, 1}, {0, 1, 1}, {0, 1, 1}, {0, 0, 1}};   /* fires in bars 0, 1, 2 */
    double L = 44100.0 * 60.0 / 120.0 / 4.0;
    uint32_t t0;
    int i, b, mark[4], st;
    /* internal clock, 120 BPM */
    reset();
    S.bpm = 120.0f;
    P[0].drum[0].hit[0][0] = 0xFFFFu;                         /* voice 0 on each step: the step times */
    seq_start(&S);
    run(256);
    i = find(E_DRUM, 0, 0);
    CHECK(i >= 0, "internal: the first step played");
    t0 = i >= 0 ? evs[i].t : 0;
    mark[0] = 0;
    for (i = 0; i < 5; i++) {
        uint32_t at = t0 + (uint32_t)(H[i].step * L + H[i].off_ms * 44.1);
        run(at - now);
        st = rec_hit(TRK_DRUM, H[i].v);
        CHECK(st == H[i].want, "internal: hit %+d ms from step %d: on step %d, want %d", H[i].off_ms, H[i].step + 1,
              st + 1, H[i].want + 1);
    }
    for (b = 1; b < 4; b++) {
        run(t0 + (uint32_t)(b * 16 * L - 441.0) - now);     /* 10 ms before the bar */
        mark[b] = nev;
    }
    for (i = 0; i < 5; i++)
        for (b = 0; b < 3; b++)
            CHECK(count_ev(E_DRUM, H[i].v, -1, mark[b], mark[b + 1]) == WANT[i][b],
                  "internal: voice %d (hit %+d ms from step %d) in bar %d: %d hits, want %d", H[i].v, H[i].off_ms,
                  H[i].step + 1, b + 1, count_ev(E_DRUM, H[i].v, -1, mark[b], mark[b + 1]), WANT[i][b]);
    for (b = 0; b < 3; b++)
        CHECK(count_ev(E_DRUM, 0, -1, mark[b], mark[b + 1]) == 16, "internal: bar %d: voice 0 plays %d steps",
              b + 1, count_ev(E_DRUM, 0, -1, mark[b], mark[b + 1]));

    /* external clock, 120 BPM: 6 ticks for each step; hit after tick k of the step */
    {
        static const struct { int tick, v, want; } X[] = {
            {4 * 6 + 2, 3, 4},                                /* 2 ticks into step 5: step 5 */
            {6 * 6 + 3, 4, 7},                                /* 3 ticks into step 7: step 8 */
            {15 * 6 + 4, 5, 0},                               /* 4 ticks into step 16: step 1 */
        };
        static const int XW[][3] = {{0, 1, 1}, {0, 1, 1}, {0, 0, 1}};
        int k, x = 0;
        reset();
        P[0].drum[0].hit[0][0] = 0xFFFFu;
        for (k = 0; k < 48; k++) {                            /* the clock runs, X0X stopped */
            seq_ext_clock(&S);
            run(919);
        }
        seq_start(&S);
        for (k = 0; k < 4 * 96; k++) {
            if (k % 96 == 0)
                mark[k / 96] = nev;
            seq_ext_clock(&S);
            run(919);
            if (x < 3 && X[x].tick == k) {
                st = rec_hit(TRK_DRUM, X[x].v);
                CHECK(st == X[x].want, "external: hit %d ticks into step %d: on step %d, want %d", k % 6, k / 6 % 16 + 1,
                      st + 1, X[x].want + 1);
                x++;
            }
        }
        for (i = 0; i < 3; i++)
            for (b = 0; b < 3; b++)
                CHECK(count_ev(E_DRUM, X[i].v, -1, mark[b], mark[b + 1]) == XW[i][b],
                      "external: voice %d (tick %d) in bar %d: %d hits, want %d", X[i].v, X[i].tick, b + 1,
                      count_ev(E_DRUM, X[i].v, -1, mark[b], mark[b + 1]), XW[i][b]);
    }

    /* 303 A, internal clock: a key 20 ms early for step 7 (the next step), a key 20 ms late on step 10 */
    {
        int n7 = 40, n10 = 45, tr = P[0].bass[0].transpose - 24;
        reset();
        S.bpm = 120.0f;
        P[0].drum[0].hit[0][0] = 0xFFFFu;
        P[0].bass[0].len = 16;
        P[0].bass[0].dir = DIR_FWD;
        for (i = 0; i < 16; i++)
            P[0].bass[0].step[i].flags = G_REST;
        seq_start(&S);
        run(256);
        i = find(E_DRUM, 0, 0);
        t0 = i >= 0 ? evs[i].t : 0;
        mark[0] = 0;
        run(t0 + (uint32_t)(6 * L - 20 * 44.1) - now);
        st = rec_hit(TRK_BASS0, n7);
        CHECK(st == 6, "303: key 20 ms early for step 7: on step %d", st + 1);
        run(t0 + (uint32_t)(9 * L + 20 * 44.1) - now);
        st = rec_hit(TRK_BASS0, n10);
        CHECK(st == 9, "303: key 20 ms late on step 10: on step %d", st + 1);
        for (b = 1; b < 4; b++) {
            run(t0 + (uint32_t)(b * 16 * L - 441.0) - now);     /* 10 ms before the bar */
            mark[b] = nev;
        }
        for (b = 0; b < 3; b++) {
            CHECK(count_ev(E_ON, 0, n7 + tr, mark[b], mark[b + 1]) == (b ? 1 : 0), "303: step 7 note in bar %d: %d, want %d",
                  b + 1, count_ev(E_ON, 0, n7 + tr, mark[b], mark[b + 1]), b ? 1 : 0);
            CHECK(count_ev(E_ON, 0, n10 + tr, mark[b], mark[b + 1]) == (b ? 1 : 0), "303: step 10 note in bar %d: %d, want %d",
                  b + 1, count_ev(E_ON, 0, n10 + tr, mark[b], mark[b + 1]), b ? 1 : 0);
        }
    }
    printf("  live record: the nearest step, no second sound on the pass of the hit: checked\n");
}


/* Three MIDI ticks form a 1/32 step. At tick 1 the hit is only one third of the way
 * through the step, so nearest-step recording must not advance until tick 2. */
static void test_rec_32nd_tick_midpoint(void)
{
    static const int part[] = {TRK_DRUM, 1, TRK_BASS0};
    int i;
    reset();
    P[0].drum[0].rate = RATE_32;                     /* 909 */
    P[0].drum[1].rate = RATE_32;                     /* 808 */
    P[0].bass[0].rate = RATE_32;                     /* 303 */
    S.clk_src = SEQ_CLK_EXT;
    seq_advance(&S, 0, sink);                       /* select EXTERNAL before PLAY */
    seq_start(&S);
    seq_advance(&S, 0, sink);
    seq_ext_clock(&S);                              /* tick 0: downbeat step */
    seq_advance(&S, 0, sink);
    for (i = 0; i < 3; i++)
        CHECK(S.t[part[i]].started && seq_rec_step(&S, part[i]) == 0,
              "external 1/32: part %d first tick plays step 1", part[i]);
    seq_ext_clock(&S);                              /* tick 1/3: before halfway */
    seq_advance(&S, 0, sink);
    for (i = 0; i < 3; i++)
        CHECK(seq_rec_step(&S, part[i]) == 0,
              "external 1/32: part %d after one tick stays on step 1", part[i]);
    seq_ext_clock(&S);                              /* tick 2/3: past halfway */
    seq_advance(&S, 0, sink);
    for (i = 0; i < 3; i++)
        CHECK(seq_rec_step(&S, part[i]) == 1,
              "external 1/32: part %d after two ticks records on step 2", part[i]);
    for (i = 0; i < 3; i++) {                        /* wrapping step 16 -> step 1 */
        S.t[part[i]].pos = 15;
        S.t[part[i]].ext_ticks = 1;
        CHECK(seq_rec_step(&S, part[i]) == 15,
              "external 1/32: part %d before midpoint stays on step 16", part[i]);
        S.t[part[i]].ext_ticks = 2;
        CHECK(seq_rec_step(&S, part[i]) == 0,
              "external 1/32: part %d past midpoint wraps to step 1", part[i]);
    }
    printf("  external 1/32 recording midpoint: 909, 808, 303 and 16-step wrap checked\n");
}

/* ---- a74: GLO > MIDI > CLOCK SOURCE (AUTO, INTERNAL, EXTERNAL) */
static int count_midi(int st, int from)                       /* MIDI out of status st since event from */
{
    int i, n = 0;
    for (i = from; i < nev; i++)
        if (evs[i].kind == E_MIDI && evs[i].a == st)
            n++;
    return n;
}
static void bass_held(int part)                               /* every step a note that slides: always held */
{
    int i;
    P[0].bass[part].len = 16;
    P[0].bass[part].dir = DIR_FWD;
    for (i = 0; i < 16; i++) {
        P[0].bass[part].step[i].note = 40;
        P[0].bass[part].step[i].flags = G_NOTE | BS_SLIDE;
    }
}

/* INTERNAL: Clock, Start, Continue and Stop in are ignored; the steps and the clock out use the internal tempo */
static void test_clksrc_internal(void)
{
    int i, k, n;
    uint32_t t0, end;
    double L = 44100.0 * 60.0 / 120.0 / 4.0;
    reset();
    S.clk_src = SEQ_CLK_INT;
    S.send_clock = 1;
    S.bpm = 120.0f;
    P[0].drum[0].hit[0][0] = 0xFFFFu;
    CHECK(seq_midi_rt(&S, 0xF8) == 1 && seq_midi_rt(&S, 0xFA) == 1 && seq_midi_rt(&S, 0xFB) == 1 &&
          seq_midi_rt(&S, 0xFC) == 1 && seq_midi_rt(&S, 0xF2) == 0 && seq_midi_rt(&S, 0x90) == 0,
          "seq_midi_rt takes only F8 FA FB FC");
    run(256);
    CHECK(!S.playing && !S.ext && S.ext_pending == 0, "INTERNAL: MIDI Start and Clock in ignored");
    seq_start(&S);                                            /* PLAY */
    nev = 0;
    t0 = now;
    end = t0 + 44100 * 2 - 1;
    for (i = 0; now < end; i++) {                             /* a clock at 140 BPM comes in all the time */
        seq_midi_rt(&S, 0xF8);
        if (i == 20)
            seq_midi_rt(&S, 0xFC);
        if (i == 40)
            seq_midi_rt(&S, 0xFA);
        if (i == 60)
            seq_midi_rt(&S, 0xFB);
        run(end - now < 788 ? end - now : 788);
    }
    CHECK(S.playing && !S.ext && S.bpm == 120.0f, "INTERNAL: playing %d, ext %d, tempo %.1f", S.playing, S.ext, (double)S.bpm);
    CHECK(count_midi(0xF8, 0) == 96, "INTERNAL: %d clocks out in 2 s at 120 BPM, want 96", count_midi(0xF8, 0));
    CHECK(find(E_MIDI, 0xFA, 0) == 0 && count_midi(0xFA, 0) == 1 && count_midi(0xFC, 0) == 0 && count_midi(0xFB, 0) == 0,
          "INTERNAL: one Start out, no Stop or Continue");
    for (i = 0, k = 0, n = 0; (i = find(E_DRUM, 0, i)) >= 0; i++, k++)
        n += abs((int)(evs[i].t - t0) - (int)(k * L + 0.5)) <= 1;
    CHECK(k == 16 && n == 16, "INTERNAL: %d steps, %d on the internal grid, want 16", k, n);
    seq_stop(&S);                                             /* PLAY */
    run(256);
    CHECK(!S.playing && count_midi(0xFC, 0) == 1, "INTERNAL: PLAY stops, Stop out");
    /* clock that came in before the switch is not used */
    reset();
    S.bpm = 120.0f;
    P[0].drum[0].hit[0][0] = 0xFFFFu;
    seq_start(&S);
    run(4000);
    t0 = evs[find(E_DRUM, 0, 0)].t;
    seq_ext_clock(&S);
    seq_ext_clock(&S);
    seq_ext_clock(&S);
    S.clk_src = SEQ_CLK_INT;
    nev = 0;
    run(256);
    CHECK(S.ext == 0 && S.ext_pending == 0, "to INTERNAL: the pending clock is dropped (ext %d, pending %u)", S.ext,
          S.ext_pending);
    run(4000);
    i = find(E_DRUM, 0, 0);
    CHECK(i >= 0 && abs((int)(evs[i].t - t0) - (int)(L + 0.5)) <= 1, "to INTERNAL: step 2 on the internal grid");
    printf("  clock source INTERNAL: clock in ignored, clock out, PLAY: checked\n");
}

/* EXTERNAL: PLAY with no clock waits; the first tick plays step 1; a lost clock pauses X0X at its place */
static void test_clksrc_external(void)
{
    int i, k, n, mark;
    uint32_t t0, last;
    reset();
    S.clk_src = SEQ_CLK_EXT;
    S.send_clock = 1;
    S.bpm = 100.0f;
    P[0].drum[0].hit[0][0] = 0xFFFFu;
    bass_held(0);
    run(256);
    CHECK(S.ext == 1 && !S.playing && seq_tempo(&S) == 100.0f, "EXTERNAL: stopped, the internal tempo shows");
    seq_start(&S);                                            /* PLAY, no clock */
    nev = 0;
    run(44100 * 2);
    CHECK(S.playing && find(E_DRUM, 0, 0) < 0 && find(E_ON, 0, 0) < 0 && find(E_MIDI, -1, 0) < 0,
          "EXTERNAL: PLAY with no clock: no step and no MIDI out in 2 s");
    CHECK(seq_clock_waiting(&S), "EXTERNAL: waiting for the clock");
    CHECK(seq_rec_step(&S, 0) == 0, "EXTERNAL: waiting: a hit records on step 1");
    nev = 0;
    t0 = now;
    for (k = 0; k < 99; k++) {                                /* 120 BPM: one bar and 3 ticks */
        seq_midi_rt(&S, 0xF8);
        run(919);
    }
    last = now - 919;
    for (i = 0, k = 0, n = 0; (i = find(E_DRUM, 0, i)) >= 0; i++, k++)
        n += evs[i].t - t0 == (uint32_t)(k % 16) * 6u * 919u + (k / 16) * 96u * 919u;
    CHECK(k == 17 && n == 17, "EXTERNAL: %d steps (%d on the ticks), want 17: step 1 on the first tick", k, n);
    CHECK(count_midi(0xF8, 0) + count_midi(0xFA, 0) + count_midi(0xFC, 0) == 0, "EXTERNAL: no clock or transport out");
    CHECK(S.ext_bpm > 119.0f && S.ext_bpm < 121.0f && !seq_clock_waiting(&S), "EXTERNAL: tempo %.2f", (double)S.ext_bpm);
    CHECK(S.t[TRK_BASS0].sounding, "EXTERNAL: the 303 note sounds");
    k = S.t[0].pos;
    mark = nev;
    run(44100 * 6 / 10);                                      /* the clock stops */
    i = find(E_OFF, 0, mark);
    CHECK(i >= 0 && evs[i].t - last <= 44100u / 2u + 256u, "EXTERNAL: clock lost: 303 off %u samples after the last tick",
          i >= 0 ? evs[i].t - last : 0);
    CHECK(find(E_BRKSTOP, -1, mark) >= 0, "EXTERNAL: clock lost: the break stops");
    CHECK(S.playing && S.ext && S.t[0].pos == k && find(E_DRUM, 0, mark) < 0 && !S.t[TRK_BASS0].sounding,
          "EXTERNAL: clock lost: paused at step %d (now %d)", k + 1, S.t[0].pos + 1);
    CHECK(S.bpm == 120.0f, "EXTERNAL: clock lost: the tempo keeps the clock's, %.1f", (double)S.bpm);
    CHECK(seq_clock_waiting(&S) && seq_rec_step(&S, 0) == k, "EXTERNAL: paused: waiting, a hit records on step %d", k + 1);
    run(44100 * 2);
    CHECK(S.playing && S.ext && find(E_DRUM, 0, mark) < 0, "EXTERNAL: paused for 2 s more");
    mark = nev;
    for (n = 0; n < 12; n++) {                                /* the clock comes back */
        seq_midi_rt(&S, 0xF8);
        run(919);
    }
    i = find(E_STEP, -1, mark);
    for (i = mark, n = 0; (i = find(E_DRUM, 0, i)) >= 0; i++)
        n++;
    CHECK(n == 2 && S.t[0].pos == (k + 2) % 16 && !seq_clock_waiting(&S),
          "EXTERNAL: clock back: %d steps in 12 ticks (want 2), at step %d (want %d)", n, S.t[0].pos + 1, (k + 2) % 16 + 1);
    seq_midi_rt(&S, 0xFA);                                    /* Start: from step 1 */
    seq_midi_rt(&S, 0xF8);
    run(919);
    CHECK(S.playing && S.t[0].pos == 0, "EXTERNAL: Start: step %d, want 1", S.t[0].pos + 1);
    seq_midi_rt(&S, 0xFC);                                    /* Stop and Start in one block: a restart */
    seq_midi_rt(&S, 0xFA);
    run(256);
    for (n = 0; n < 7; n++) {
        seq_midi_rt(&S, 0xF8);
        run(919);
    }
    CHECK(S.playing && S.t[0].pos == 1, "EXTERNAL: Stop + Start in one block: step %d, want 2", S.t[0].pos + 1);
    mark = nev;
    seq_midi_rt(&S, 0xFC);
    run(256);
    CHECK(!S.playing && count_midi(0xFC, mark) == 0, "EXTERNAL: Stop: stopped, no Stop out");
    run(44100);                                               /* PLAY with no clock, then PLAY again: stopped */
    seq_start(&S);
    run(44100);
    CHECK(S.playing && seq_clock_waiting(&S), "EXTERNAL: PLAY waits");
    seq_stop(&S);
    run(256);
    CHECK(!S.playing && !seq_clock_waiting(&S), "EXTERNAL: PLAY again cancels the wait");
    /* the clock in Bluetooth bursts, as test_ext_tempo_ble and test_ext_burst_steps */
    {
        static const double EV[] = {7.5, 15.0, 30.0}, BPM[] = {120.0, 93.0, 140.0};
        unsigned e, b, ph;
        int r, want;
        clk_src = SEQ_CLK_EXT;
        ble_rt = 1;
        for (e = 0; e < 3; e++)
            for (b = 0; b < 3; b++)
                for (ph = 0; ph < 2; ph++) {
                    reset();
                    S.bpm = 100.0f;
                    ble_clock_start(BPM[b], EV[e], 0.1 + 0.4 * ph);
                    ble_clock(BPM[b], EV[e], 10.0);
                    CHECK(S.ext_bpm > BPM[b] - 0.5 && S.ext_bpm < BPM[b] + 0.5, "EXTERNAL: %.0f BPM in %.1f ms events: %.2f",
                          BPM[b], EV[e], (double)S.ext_bpm);
                    run(44100);
                    CHECK(S.ext == 1 && S.bpm == (float)BPM[b], "EXTERNAL: after the link end: ext %d, tempo %.1f", S.ext,
                          (double)S.bpm);
                }
        for (r = 0; r < 2; r++)
            for (e = 0; e < 3; e++)
                for (ph = 0; ph < 3; ph++) {
                    n = burst_steps(r ? RATE_32 : RATE_16, 180.0, EV[e], 0.1 + 0.3 * ph, ph == 2 ? 400.0 : 0.0, &want);
                    CHECK(n == want, "EXTERNAL: %s 180 BPM, %.1f ms events (phase %u): %d steps, the clock's %d",
                          r ? "1/32" : "1/16", EV[e], ph, n, want);
                }
        ble_rt = 0;
        clk_src = SEQ_CLK_AUTO;
    }
    printf("  clock source EXTERNAL: wait, pause on a lost clock, transport, Bluetooth bursts: checked\n");
}

/* all 6 switches while playing, with and without a clock: no double step, no lost step, no stuck 303 note */
static void test_clksrc_switch(void)
{
    static const char *const NM[] = {"AUTO", "INTERNAL", "EXTERNAL"};
    static const uint8_t SW[6][2] = {{0, 1}, {0, 2}, {1, 0}, {1, 2}, {2, 0}, {2, 1}};
    double L = 44100.0 * 60.0 / 120.0 / 4.0;
    int c, w, ph;
    sink = &SINK_STEP;
    for (c = 0; c < 2; c++)                                   /* c = 1: a 120 BPM clock in 15 ms events */
        for (w = 0; w < 6; w++)
            for (ph = 0; ph < 3; ph++) {
                int from = SW[w][0], to = SW[w][1], i, prev, n = 0, bad = 0, mark;
                uint32_t gmin = 0xFFFFFFFFu, gmax = 0, tsw;
                reset();
                S.clk_src = (uint8_t)from;
                S.bpm = 120.0f;
                P[0].drum[0].hit[0][0] = 0xFFFFu;
                bass_held(0);
                ble_rt = 1;
                ble_clock_start(120.0, 15.0, 0.5);
                seq_start(&S);
                if (c)
                    ble_clock(120.0, 15.0, 1.0 + 0.031 * ph);
                else
                    run(44100 + 1400 * (uint32_t)ph);
                mark = nev;
                for (i = nev - 1; i >= 0 && evs[i].kind != E_STEP; i--)
                    ;
                if (i >= 0)
                    mark = i;                                 /* (from the last step before the switch) */
                tsw = now;
                S.clk_src = (uint8_t)to;
                if (c)
                    ble_clock(120.0, 15.0, 2.0);
                else
                    run(44100 * 2);
                for (i = mark, prev = -1; (i = find(E_STEP, -1, i)) >= 0; prev = i, i++) {
                    if (prev >= 0) {
                        uint32_t g = evs[i].t - evs[prev].t;
                        gmin = g < gmin ? g : gmin;
                        gmax = g > gmax ? g : gmax;
                        bad += evs[i].a != (evs[prev].a + 1) % 16;
                    }
                    n += evs[i].t > tsw;
                }
                if (to == SEQ_CLK_EXT && !c) {                /* no clock: X0X waits at its place */
                    CHECK(n == 0 && !S.t[TRK_BASS0].sounding && seq_clock_waiting(&S),
                          "no clock, %s to %s: %d steps, 303 sounding %d, waiting %d", NM[from], NM[to], n,
                          S.t[TRK_BASS0].sounding, seq_clock_waiting(&S));
                } else {
                    CHECK(bad == 0, "%s, %s to %s (%d): %d steps out of order", c ? "clock" : "no clock", NM[from], NM[to],
                          ph, bad);
                    /* (a clock in 15 ms events moves a step by up to one event; the first tick after INTERNAL is
                     * tick 0 of the step that plays: up to 2 steps, as LIM-11) */
                    CHECK(gmin >= (uint32_t)((c ? 0.8 : 0.99) * L) &&
                          gmax <= (uint32_t)((c ? (to == SEQ_CLK_EXT || from == SEQ_CLK_INT) ? 2.25 : 1.25 : 1.01) * L),
                          "%s, %s to %s (%d): steps %u to %u samples apart (a step: %.0f)", c ? "clock" : "no clock",
                          NM[from], NM[to], ph, gmin, gmax, L);
                    CHECK(n >= 15, "%s, %s to %s (%d): %d steps in 2 s", c ? "clock" : "no clock", NM[from], NM[to], ph, n);
                    CHECK(S.ext == (c && to != SEQ_CLK_INT), "%s, %s to %s: ext %d", c ? "clock" : "no clock", NM[from], NM[to],
                          S.ext);
                }
                seq_stop(&S);
                run(256);
                CHECK(!S.t[TRK_BASS0].sounding && !S.t[TRK_BASS0 + 1].sounding, "%s to %s: a 303 note stays on", NM[from],
                      NM[to]);
                ble_rt = 0;
            }
    sink = &SINK;
    printf("  clock source: 6 switches while playing, with and without a clock: checked\n");
}

/* ---- a74 (MID-14): the PLAY note with Loopy Pro as the clock (Toggle Clock Pause on note 0x5B) */
static void test_play_note_with_loopy(void)
{
    static const int DELAY_MS[] = {5, 20, 40, 80, 120};
    double tick = 44100.0 * 60.0 / 120.0 / 24.0, L = 6.0 * tick;
    int d, k, i, n, prev, bad, ons, mark;
    sink = &SINK_STEP;
    ble_rt = 1;
    /* X0X follows Loopy; PLAY stops X0X; Loopy pauses and sends Stop: X0X stays stopped */
    reset();
    S.send_clock = 1;
    P[0].drum[0].hit[0][0] = 0xFFFFu;
    ble_clock_start(120.0, 15.0, 0.5);
    seq_midi_rt(&S, 0xFA);
    ble_clock(120.0, 15.0, 2.0);
    CHECK(S.playing && S.ext, "Loopy clock: X0X follows");
    seq_stop(&S);                                             /* PLAY (the note goes to Loopy) */
    mark = nev;
    ble_clock(120.0, 15.0, 0.03);                             /* Loopy: its clock for 30 ms more */
    seq_midi_rt(&S, 0xFC);                                    /* then Stop, and no clock */
    run(44100 * 2);
    CHECK(!S.playing && find(E_STEP, -1, mark) < 0, "PLAY stop, then Loopy Stop: X0X stays stopped");
    CHECK(count_midi(0xFA, mark) + count_midi(0xFB, mark) + count_midi(0xFC, mark) == 0, "PLAY stop: no Start or Stop out");
    /* Loopy paused (no clock); PLAY starts X0X; Loopy answers with Start (or Continue) and its clock */
    for (k = 0; k < 2; k++)
        for (d = 0; d < (int)(sizeof DELAY_MS / sizeof DELAY_MS[0]); d++) {
            uint32_t t0, tc;
            reset();
            S.send_clock = 1;
            S.bpm = 120.0f;
            P[0].drum[0].hit[0][0] = 0xFFFFu;
            P[0].bass[0].len = 16;
            for (i = 0; i < 16; i++) {
                P[0].bass[0].step[i].note = (uint8_t)(30 + i);
                P[0].bass[0].step[i].flags = G_NOTE;
            }
            run(44100);
            seq_start(&S);                                    /* PLAY: X0X plays step 1 now */
            run(256);
            t0 = evs[find(E_STEP, -1, 0)].t;
            run((uint32_t)(DELAY_MS[d] * 44.1) - (now - t0));
            seq_midi_rt(&S, k ? 0xFB : 0xFA);                 /* Loopy: Start or Continue, the first tick 1 ms later */
            run(44);
            tc = now;
            ble_clock_start(120.0, 15.0, 0.9);
            ble_clock(120.0, 15.0, 4.0);
            {
                uint32_t gmin = 0xFFFFFFFFu, gmax = 0, bar = t0 + (uint32_t)(15.5 * L);
                int first = find(E_ON, 0, 0), note = first >= 0 ? evs[first].b : -1;
                for (i = 0, prev = -1, n = 0, bad = 0; (i = find(E_STEP, -1, i)) >= 0; prev = i, i++) {
                    if (prev >= 0) {
                        uint32_t g = evs[i].t - evs[prev].t;
                        bad += evs[i].a != (evs[prev].a + 1) % 16;
                        gmin = g < gmin ? g : gmin;
                        gmax = g > gmax ? g : gmax;
                    }
                    n += evs[i].a == 0 && evs[i].t < bar;
                }
                for (i = 0, ons = 0; i < nev && evs[i].t < bar; i++)
                    ons += evs[i].kind == E_ON && evs[i].a == 0 && evs[i].b == note;
                CHECK(n == 1 && bad == 0, "PLAY, then Loopy %s %d ms later: step 1 %d times in the first bar, %d steps out of order",
                      k ? "Continue" : "Start", DELAY_MS[d], n, bad);
                CHECK(ons == 1, "PLAY, then Loopy %s %d ms later: the 303 note of step 1 %d times in the first bar, want 1",
                      k ? "Continue" : "Start", DELAY_MS[d], ons);
                /* (the first tick is tick 0 of the step that plays, LIM-11: one step can be up to 2 steps long;
                 * the clock in 15 ms events moves a step by up to one event) */
                CHECK(gmin >= (uint32_t)(0.8 * L) && gmax <= (uint32_t)(2.25 * L),
                      "PLAY, then Loopy %s %d ms later: steps %u to %u samples apart (a step: %.0f)", k ? "Continue" : "Start",
                      DELAY_MS[d], gmin, gmax, L);
                (void)tc;
            }
            CHECK(S.playing && S.ext, "PLAY, then Loopy: X0X follows the clock");
        }
    /* a Start more than one step after PLAY starts X0X again from step 1, as before */
    reset();
    S.bpm = 120.0f;
    P[0].drum[0].hit[0][0] = 0xFFFFu;
    seq_start(&S);
    run(44100 / 2);
    mark = nev;
    seq_midi_rt(&S, 0xFA);
    seq_midi_rt(&S, 0xF8);
    run(256);
    CHECK(find(E_STEP, 0, mark) >= 0, "a Start 0.5 s after PLAY: from step 1, as before");
    ble_rt = 0;
    sink = &SINK;
    printf("  PLAY note: PLAY with Loopy as the clock, no second start: checked\n");
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

/* A malformed chain range must not become an out-of-bounds pattern index at
 * the next bar, including endpoint values that would wrap when cast to u8. */
static void test_chain_endpoint_bounds(void)
{
    static const int INVALID[][2] = {
        {-1, 5}, {3, -1}, {NPAT, 5}, {3, NPAT},
        {NPAT + 256, 5}, {3, NPAT + 256}, {-256, 5}, {3, -256}
    };
    unsigned i;
    int t;
    reset();
    S.bpm = 120.0f;
    seq_chain(&S, 3, 5);
    CHECK(S.chain_a == 3 && S.chain_b == 5, "valid chain endpoints accepted");
    for (i = 0; i < sizeof INVALID / sizeof INVALID[0]; i++) {
        seq_chain(&S, INVALID[i][0], INVALID[i][1]);
        CHECK(S.chain_a == 3 && S.chain_b == 5,
              "invalid chain endpoints %d,%d preserved valid range (got %u,%u)",
              INVALID[i][0], INVALID[i][1],
              (unsigned)S.chain_a, (unsigned)S.chain_b);
        seq_chain(&S, 3, 5);             /* baseline must not index a bad pattern */
    }
    for (t = 0; t < NTRACKS; t++)
        S.ppat[t] = 3;
    seq_chain(&S, -1, NPAT);
    if (S.chain_a == 3 && S.chain_b == 5) {
        seq_start(&S);
        run(FS * 2u + 2048u);           /* bar boundary at 120 BPM */
        CHECK(S.ppat[TRK_DRUM] == 4 && S.ppat[TRK_BRK] == 4,
              "rejected chain leaves valid 909/break bar transition");
    }
    seq_chain(&S, 5, 3);
    CHECK(S.chain_a == 5 && S.chain_b == 3, "valid reversed endpoints accepted");
    seq_chain(&S, 3, 3);
    CHECK(S.chain_a == 3 && S.chain_b == 3, "equal endpoints disable chain");
    printf("  chain endpoints: valid, reversed, disabled and eight invalid ranges checked\n");
}

/* REC-06: a swung internal step is long or short. Recording must use the midpoint
 * of that ACTUAL interval (fire_step), not half an unswung step. */
static void test_rec_swing_midpoint(void)
{
    static const int tracks[] = {TRK_DRUM, 1, TRK_BASS0, TRK_BASS0 + 1, TRK_BRK};
    uint32_t L;
    int i;
    reset();
    S.bpm = 120.0f;
    P[0].swing = 100;                           /* maximum swing: long 1.5L, short 0.5L */
    L = seq_step_q8(&S, RATE_16);
    for (i = 0; i < (int)(sizeof tracks / sizeof tracks[0]); i++) {
        seq_track_t *tr = &S.t[tracks[i]];
        tr->started = 1;
        tr->pos = 0;
        tr->to_next = L * 3u / 5u;             /* 60% through long step: next */
        CHECK(seq_rec_step(&S, tracks[i]) == 1, "swung long midpoint track %d", tracks[i]);
        tr->pos = 1;
        tr->to_next = L * 3u / 8u;             /* 25% through short step: current */
        CHECK(seq_rec_step(&S, tracks[i]) == 1, "swung short first quarter track %d", tracks[i]);
        tr->to_next = L / 8u;                  /* 75% through short step: next */
        CHECK(seq_rec_step(&S, tracks[i]) == 2, "swung short last quarter track %d", tracks[i]);
    }

    P[0].drum[0].rate = RATE_32;
    P[0].bass[0].rate = RATE_32;
    L = seq_step_q8(&S, RATE_32);
    S.t[TRK_DRUM].pos = 0;
    S.t[TRK_DRUM].to_next = L * 3u / 5u;
    CHECK(seq_rec_step(&S, TRK_DRUM) == 1, "1/32 swung long midpoint drum");
    S.t[TRK_BASS0].pos = 1;
    S.t[TRK_BASS0].to_next = L * 3u / 8u;
    CHECK(seq_rec_step(&S, TRK_BASS0) == 1, "1/32 swung short midpoint 303");

    P[0].bass[0].rate = RATE_16;
    P[0].bass[0].dir = DIR_REV;
    P[0].bass[0].len = 16;
    L = seq_step_q8(&S, RATE_16);
    S.t[TRK_BASS0].pos = 4;                    /* reverse: 4 -> 3, long */
    S.t[TRK_BASS0].to_next = L * 3u / 5u;
    CHECK(seq_rec_step(&S, TRK_BASS0) == 3, "reverse 303 long step");
    S.t[TRK_BASS0].pos = 5;                    /* reverse: 5 -> 4, short */
    S.t[TRK_BASS0].to_next = L * 3u / 8u;
    CHECK(seq_rec_step(&S, TRK_BASS0) == 5, "reverse 303 short step");
    P[0].bass[0].dir = DIR_PINGPONG;
    S.t[TRK_BASS0].pos = 15;
    S.t[TRK_BASS0].pp_dir = 1;                  /* endpoint reverses to even step 14 */
    S.t[TRK_BASS0].to_next = L * 3u / 8u;
    CHECK(seq_rec_step(&S, TRK_BASS0) == 15, "ping-pong short step at endpoint");

    P[0].drum[0].rate = RATE_16;            /* restore rate after the RATE_32 cases */
    P[0].drum[0].len = 3;                      /* even step 2 -> even step 0: no swing */
    L = seq_step_q8(&S, RATE_16);
    S.t[TRK_DRUM].pos = 2;
    S.t[TRK_DRUM].to_next = L * 2u / 5u;
    CHECK(seq_rec_step(&S, TRK_DRUM) == 0, "odd-length even-to-even wrap midpoint");

    P[0].drum[0].len = 16;
    P[0].drum[0].rate = RATE_16T;              /* triplets are not swung */
    L = seq_step_q8(&S, RATE_16T);
    S.t[TRK_DRUM].pos = 0;
    S.t[TRK_DRUM].to_next = L * 3u / 5u;
    CHECK(seq_rec_step(&S, TRK_DRUM) == 0, "triplet retains unswung midpoint");
    P[0].swing = 0;
    P[0].drum[0].rate = RATE_16;
    L = seq_step_q8(&S, RATE_16);
    S.t[TRK_DRUM].to_next = L * 2u / 5u;
    CHECK(seq_rec_step(&S, TRK_DRUM) == 1, "straight rate retains nearest-step midpoint");
    S.t[TRK_DRUM].to_next = 0xFFFFFFFFu;
    CHECK(seq_rec_step(&S, TRK_DRUM) == 0, "no next event stays on current step");
    S.t[TRK_DRUM].started = 0;
    CHECK(seq_rec_step(&S, TRK_DRUM) == 0, "before first step remains step 0");
    printf("  recording midpoint follows swung internal step durations: checked\n");
}

int main(void)
{
    test_chain_endpoint_bounds();
    test_drum_timing();
    test_drum_step_velocity();
    test_swing_and_rates();
    test_303();
    test_303_velocity_slide_overlap();
    test_303_midi_lifecycle();
    test_polymeter_cue_chain();
    test_part_cue();
    test_song();
    test_ext_clock_and_midi_out();
    test_ext_tempo_ble();
    test_ext_transport_ble();
    test_ext_burst_steps();
    test_rec_nearest();
    test_rec_32nd_tick_midpoint();
    test_rec_swing_midpoint();
    test_clksrc_internal();
    test_clksrc_external();
    test_clksrc_switch();
    test_play_note_with_loopy();
    test_break_clock();
#ifdef REF_TB3PO
    test_tb3po_matches_reference();
#endif
    printf("seq: %s\n", fails ? "FAIL" : "ok");
    return fails != 0;
}
