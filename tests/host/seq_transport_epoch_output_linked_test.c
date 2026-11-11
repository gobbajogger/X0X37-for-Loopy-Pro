/* SPDX-License-Identifier: GPL-3.0-only */
/* Linked production-C regression: MIDI clock epoch, transport echo, and NoteOff.
 * Input transport sets midi_req as engine.c does when SMK clip mode is enabled.
 * No firmware target, attached controller, or firmware package is exercised here.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "sequencer.h"

static pattern_t pat[NPAT];
static int checks, errors;
static uint32_t block;
typedef struct { uint8_t st, note; } event_t;
typedef struct { event_t events[96]; int used; } trace_t;
#define CHECK(cond) do { ++checks; if (!(cond)) { ++errors; fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); } } while (0)

static void capture(void *ctx, uint8_t st, uint8_t note, uint8_t velocity)
{
    trace_t *t = ctx;
    (void)velocity;
    if (t->used < (int)(sizeof t->events / sizeof t->events[0]))
        t->events[t->used++] = (event_t){st, note};
}
static int bass_on(void *ctx, int part, int note, int accent, int slide, int velocity)
{
    (void)ctx; (void)part; (void)note; (void)accent; (void)slide; (void)velocity;
 return 1; }
static void bass_off(void *ctx, int part) { (void)ctx; (void)part; }
static int count(const trace_t *t, uint8_t st, uint8_t note)
{
    int n = 0;
    for (int i = 0; i < t->used; ++i)
        if (t->events[i].st == st && t->events[i].note == note)
            ++n;
    return n;
}
static void init(seq_t *s, trace_t *t)
{
    for (unsigned i = 0; i < NPAT; ++i)
        pattern_init(&pat[i], 100u + i, 200u + i);
    pat[0].bass[0].step[0] = (bstep_t){60, G_NOTE | BS_SLIDE, 100};
    pat[0].bass[0].step[1] = (bstep_t){64, G_NOTE, 96};
    seq_init(s, pat);
    s->send_notes = 1;
    s->send_clock = 1;
    *t = (trace_t){0};
}
static void advance(seq_t *s, trace_t *t)
{
    seq_sink_t sink = {.midi = capture, .bass_on = bass_on,
        .bass_off = bass_off, .ctx = t};
    seq_advance(s, block, &sink);
}
static void incoming(seq_t *s, uint8_t st)
{
    if (st == 0xFA || st == 0xFB || st == 0xFC)
        s->midi_req = 1; /* Real engine.c marks host input in clip mode. */
    CHECK(seq_midi_rt(s, st) == 1);
}
static void before_start_clocks_must_not_fire_step(void)
{
    seq_t s; trace_t t;
    init(&s, &t);
    for (int i = 0; i < 7; ++i) incoming(&s, 0xF8);
    incoming(&s, 0xFA);
    advance(&s, &t);
    CHECK(s.playing && s.ext);
    CHECK(s.ext_q == 0);
    CHECK(count(&t, 0x91, 60) == 0);
    CHECK(count(&t, 0xFA, 0) == 0);
    incoming(&s, 0xF8);
    advance(&s, &t);
    CHECK(s.ext_q == 1);
    CHECK(count(&t, 0x91, 60) == 1);
}
static void post_start_clock_keeps_new_epoch(void)
{
    seq_t s; trace_t t;
    init(&s, &t);
    for (int i = 0; i < 5; ++i) incoming(&s, 0xF8);
    incoming(&s, 0xFA);
    incoming(&s, 0xF8);
    advance(&s, &t);
    CHECK(s.playing && s.ext);
    CHECK(s.ext_q == 1);
    CHECK(count(&t, 0xFA, 0) == 0);
    CHECK(count(&t, 0x91, 60) == 1);
    incoming(&s, 0xFC);
    advance(&s, &t);
    CHECK(!s.playing);
    CHECK(count(&t, 0xFC, 0) == 0);
    CHECK(count(&t, 0x81, 60) == 1);
    incoming(&s, 0xFC);
    advance(&s, &t);
    CHECK(count(&t, 0x81, 60) == 1);
}
static void local_play_echo_must_keep_phase(void)
{
    seq_t s; trace_t t;
    init(&s, &t);
    seq_start(&s);
    advance(&s, &t);
    CHECK(s.playing && s.start_local);
    CHECK(count(&t, 0xFA, 0) == 1);
    CHECK(count(&t, 0x91, 60) == 1);
    incoming(&s, 0xFA);
    advance(&s, &t);
    CHECK(s.playing && s.start_local);
    CHECK(count(&t, 0xFA, 0) == 1);
    CHECK(count(&t, 0x91, 60) == 1);
    CHECK(count(&t, 0x81, 60) == 0);
    seq_stop(&s);
    advance(&s, &t);
    CHECK(!s.playing);
    CHECK(count(&t, 0xFC, 0) == 1);
    CHECK(count(&t, 0x81, 60) == 1);
}
static void remote_stop_then_continue_preserves_order(void)
{
    seq_t s; trace_t t;
    init(&s, &t);
    seq_start(&s);
    advance(&s, &t);
    CHECK(count(&t, 0x91, 60) == 1);
    incoming(&s, 0xFC);
    incoming(&s, 0xFB);
    advance(&s, &t);
    CHECK(s.playing && !s.start_local);
    CHECK(count(&t, 0x81, 60) == 1);
    CHECK(count(&t, 0x91, 64) == 1);
    CHECK(count(&t, 0xFC, 0) == 0);
    CHECK(count(&t, 0xFB, 0) == 0);
    incoming(&s, 0xFC);
    advance(&s, &t);
    CHECK(count(&t, 0x81, 64) == 1);
}
/* The host may deliver an incoming Loopy transport and a physical PLAY/STOP
 * in one render block. midi_req then remains set even after PLAY supersedes
 * that MIDI request. Output suppression must follow the winning request.
 * In clip mode OFF, preserve legacy MIDI transport output (MID-07). */
static void mixed_transport_origin_output(void)
{
    seq_t s; trace_t t;
    init(&s, &t);
    incoming(&s, 0xFA);
    seq_start(&s);                     /* local PLAY wins over MIDI Start */
    advance(&s, &t);
    CHECK(s.playing && s.start_local);
    CHECK(count(&t, 0xFA, 0) == 1);
    CHECK(count(&t, 0x91, 60) == 1);

    init(&s, &t);
    incoming(&s, 0xFB);
    seq_continue(&s);                  /* local Continue wins */
    advance(&s, &t);
    CHECK(s.playing && s.start_local);
    CHECK(count(&t, 0xFB, 0) == 1);

    init(&s, &t);
    seq_start(&s);
    advance(&s, &t);
    t.used = 0;
    incoming(&s, 0xFC);
    seq_stop(&s);                      /* local STOP wins over MIDI Stop */
    advance(&s, &t);
    CHECK(!s.playing);
    CHECK(count(&t, 0xFC, 0) == 1);
    CHECK(count(&t, 0x81, 60) == 1);  /* held 303 NoteOff once */

    init(&s, &t);
    seq_start(&s);
    advance(&s, &t);
    t.used = 0;
    incoming(&s, 0xFC);
    seq_start(&s);                     /* remote Stop, then local restart */
    advance(&s, &t);
    CHECK(s.playing && s.start_local);
    CHECK(count(&t, 0xFA, 0) == 1);
    CHECK(count(&t, 0xFC, 0) == 0);
    CHECK(count(&t, 0x81, 60) == 1);

    init(&s, &t);
    seq_start(&s);
    advance(&s, &t);
    t.used = 0;
    seq_stop(&s);                      /* local Stop, then remote restart */
    incoming(&s, 0xFA);
    advance(&s, &t);
    CHECK(s.playing && !s.start_local);
    CHECK(count(&t, 0xFC, 0) == 1);
    CHECK(count(&t, 0xFA, 0) == 0);
    CHECK(count(&t, 0x81, 60) == 1);

    init(&s, &t);
    CHECK(seq_midi_rt(&s, 0xFA) == 1); /* clip mode OFF: no midi_req */
    advance(&s, &t);
    CHECK(s.playing && !s.start_local);
    CHECK(count(&t, 0xFA, 0) == 1);    /* untouched legacy echo */

    init(&s, &t);
    incoming(&s, 0xFA);              /* clip mode ON: no remote echo */
    advance(&s, &t);
    CHECK(count(&t, 0xFA, 0) == 0);

    init(&s, &t);
    s.clk_src = SEQ_CLK_INT;
    advance(&s, &t);
    incoming(&s, 0xFA);              /* ignored remotely, still sets midi_req */
    seq_start(&s);
    advance(&s, &t);
    CHECK(s.playing && !s.ext);
    CHECK(count(&t, 0xFA, 0) == 1);

    init(&s, &t);
    s.send_clock = 0;
    incoming(&s, 0xFA);
    seq_start(&s);
    advance(&s, &t);
    CHECK(count(&t, 0xFA, 0) == 0);

    init(&s, &t);
    s.clk_src = SEQ_CLK_EXT;
    advance(&s, &t);
    incoming(&s, 0xFA);
    seq_start(&s);
    advance(&s, &t);
    CHECK(s.playing && s.ext);
    CHECK(count(&t, 0xFA, 0) == 0);
}

/* A Loopy Start/Continue echo and the first external clock can arrive together after
 * local PLAY already sounded step 1. Rephase must not sound that step twice,
 * and the sixth FOLLOWING F8 is the next step boundary (24 ppqn/16th). */
static void local_play_echo_first_external_tick_phase(uint8_t echo)
{
    seq_t s; trace_t t;
    init(&s, &t);
    seq_start(&s);
    advance(&s, &t);
    CHECK(s.playing && s.start_local);
    CHECK(count(&t, 0x91, 60) == 1);
    incoming(&s, echo);
    incoming(&s, 0xF8);    /* first external tick establishes phase */
    advance(&s, &t);
    CHECK(s.playing && s.start_local && s.ext);
    CHECK(count(&t, 0x91, 60) == 1);
    CHECK(count(&t, 0x81, 60) == 0);
    CHECK(count(&t, 0x91, 64) == 0);
    for (int i = 0; i < 5; ++i) incoming(&s, 0xF8);
    advance(&s, &t);
    CHECK(count(&t, 0x91, 60) == 1);
    CHECK(count(&t, 0x91, 64) == 0);
    incoming(&s, 0xF8);
    advance(&s, &t);
    CHECK(count(&t, 0x91, 60) == 1);
    CHECK(count(&t, 0x91, 64) == 1);
    incoming(&s, 0xFC);
    advance(&s, &t);
    CHECK(count(&t, 0x81, 60) == 1);
    CHECK(count(&t, 0x81, 64) == 1);
    CHECK(count(&t, 0xFA, 0) == 1);
    CHECK(count(&t, 0xFB, 0) == 0);
    CHECK(count(&t, 0xFC, 0) == 0);
}


/* Same-block MIDI and physical gestures must partition queued F8 ticks at the
 * LAST winning request, not at the first MIDI request or start of the block.
 * Test both zero and nonzero post-request clock counts, with held 303 notes. */
static void mixed_origin_epoch_partition(void)
{
    seq_t s; trace_t t;
    init(&s, &t);
    for (int i = 0; i < 3; ++i) incoming(&s, 0xF8);
    incoming(&s, 0xFA);                      /* remote Start is superseded */
    for (int i = 0; i < 2; ++i) incoming(&s, 0xF8);
    seq_start(&s);                           /* LAST request is local Start */
    incoming(&s, 0xF8);                     /* sole new-epoch downbeat */
    advance(&s, &t);
    CHECK(s.playing && s.start_local && s.ext);
    CHECK(s.ext_q == 1u);
    CHECK(count(&t, 0x91, 60) == 1);
    CHECK(count(&t, 0x91, 64) == 0);
    CHECK(count(&t, 0xFA, 0) == 0);         /* external clock: no local echo */
    for (int i = 0; i < 5; ++i) incoming(&s, 0xF8);
    advance(&s, &t);
    CHECK(count(&t, 0x91, 64) == 0);
    incoming(&s, 0xF8);
    advance(&s, &t);
    CHECK(count(&t, 0x91, 64) == 1);
    seq_stop(&s);
    advance(&s, &t);
    CHECK(!s.playing);
    CHECK(count(&t, 0x81, 60) == 1);
    CHECK(count(&t, 0x81, 64) == 1);

    init(&s, &t);
    for (int i = 0; i < 7; ++i) incoming(&s, 0xF8);
    seq_start(&s);                           /* all ticks belong to old epoch */
    advance(&s, &t);
    CHECK(s.playing && s.ext);
    CHECK(s.ext_q == 0u);
    CHECK(count(&t, 0x91, 60) == 0);
    seq_stop(&s);                            /* Stop before the first downbeat */
    advance(&s, &t);
    CHECK(count(&t, 0x81, 60) == 0);         /* cannot release a never-sent note */
    CHECK(!s.playing);

    init(&s, &t);
    for (int i = 0; i < 4; ++i) incoming(&s, 0xF8);
    seq_start(&s);
    incoming(&s, 0xF8);
    incoming(&s, 0xFB);                      /* remote Continue supersedes Start */
    incoming(&s, 0xF8);
    seq_continue(&s);                        /* LAST request local Continue */
    incoming(&s, 0xF8);
    advance(&s, &t);
    CHECK(s.playing && s.start_local && s.ext);
    CHECK(s.ext_q == 1u);
    CHECK(count(&t, 0x91, 60) == 1);
    CHECK(count(&t, 0x91, 64) == 0);
    CHECK(count(&t, 0xFB, 0) == 0);
    incoming(&s, 0xFC);
    advance(&s, &t);
    CHECK(count(&t, 0x81, 60) == 1);
    CHECK(count(&t, 0xFC, 0) == 0);
}


/* Exhaust the small same-render-block F8/Start/Continue/gesture boundary grid.
 * Clock source is forced EXTERNAL before queuing traffic so a zero-post-tick
 * winning gesture MUST stay silent, regardless of stale pre-gesture ticks.
 * Each scenario checks the outgoing MIDI trace, exact clock epoch, and final
 * NoteOff idempotency after two physical stops. */
static void mixed_origin_burst_grid(void)
{
    for (int remote = 0; remote < 2; ++remote)
    for (int local = 0; local < 2; ++local)
    for (int pre = 0; pre <= 4; ++pre)
    for (int between = 0; between <= 3; ++between)
    for (int after = 0; after <= 6; ++after) {
        seq_t s; trace_t t;
        init(&s, &t);
        s.clk_src = SEQ_CLK_EXT;
        advance(&s, &t); /* commit the source before any queued MIDI bytes */
        CHECK(s.ext && !s.playing);
        for (int k = 0; k < pre; ++k)
            incoming(&s, 0xF8);
        incoming(&s, remote ? 0xFB : 0xFA);
        for (int k = 0; k < between; ++k)
            incoming(&s, 0xF8);
        if (local)
            seq_continue(&s);
        else
            seq_start(&s);
        for (int k = 0; k < after; ++k)
            incoming(&s, 0xF8);
        advance(&s, &t);
        CHECK(s.playing && s.start_local && s.ext);
        CHECK(s.ext_q == (uint8_t)after);
        CHECK(count(&t, 0x91, 60) == (after ? 1 : 0));
        CHECK(count(&t, 0x91, 64) == 0);
        CHECK(count(&t, 0xFA, 0) == 0 && count(&t, 0xFB, 0) == 0);
        seq_stop(&s);
        advance(&s, &t);
        seq_stop(&s); /* duplicate STOP may not release twice */
        advance(&s, &t);
        CHECK(!s.playing);
        CHECK(count(&t, 0x81, 60) == (after ? 1 : 0));
        CHECK(count(&t, 0xFC, 0) == 0);
    }
}

int main(void)
{
    static const uint32_t blocks[] = {1u, 64u, 1024u};
    for (unsigned i = 0; i < sizeof blocks / sizeof blocks[0]; ++i) {
        block = blocks[i];
        before_start_clocks_must_not_fire_step();
        post_start_clock_keeps_new_epoch();
        local_play_echo_must_keep_phase();
        remote_stop_then_continue_preserves_order();
        mixed_transport_origin_output();
        local_play_echo_first_external_tick_phase(0xFA);
        local_play_echo_first_external_tick_phase(0xFB);
        mixed_origin_epoch_partition();
        mixed_origin_burst_grid();
    }
    fprintf(stderr, "seq epoch/output linked: %d/%d checks %s\n", checks - errors, checks,
            errors ? "FAILED" : "PASSED");
    return errors ? EXIT_FAILURE : EXIT_SUCCESS;
}
