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
static void bass_on(void *ctx, int part, int note, int accent, int slide, int velocity)
{
    (void)ctx; (void)part; (void)note; (void)accent; (void)slide; (void)velocity;
}
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
int main(void)
{
    static const uint32_t blocks[] = {1u, 64u, 1024u};
    for (unsigned i = 0; i < sizeof blocks / sizeof blocks[0]; ++i) {
        block = blocks[i];
        before_start_clocks_must_not_fire_step();
        post_start_clock_keeps_new_epoch();
        local_play_echo_must_keep_phase();
        remote_stop_then_continue_preserves_order();
    }
    fprintf(stderr, "seq epoch/output linked: %d/%d checks %s\n", checks - errors, checks,
            errors ? "FAILED" : "PASSED");
    return errors ? EXIT_FAILURE : EXIT_SUCCESS;
}
