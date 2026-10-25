/* SPDX-License-Identifier: GPL-3.0-only */
/* Linked production-C regression: render-chunk invariance of the scheduled MIDI
 * clock, local gestures and 303 NoteOff. No device or target firmware is run.
 * The test drives seq_until_event exactly as the documented engine render loop. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sequencer.h"

#define NEVENTS 256
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); } } while (0)

typedef struct { uint32_t at; uint8_t st, note, vel; } event_t;
typedef struct { event_t event[NEVENTS]; unsigned len; } trace_t;
typedef struct { trace_t trace; uint32_t now; unsigned chunks, limit; } observer_t;
static pattern_t patterns[NPAT];
static int checks, failures;

static void midi_out(void *p, uint8_t st, uint8_t d1, uint8_t d2)
{
    observer_t *o = p;
    if (o->trace.len >= NEVENTS) { ++failures; return; }
    o->trace.event[o->trace.len++] = (event_t){o->now, st, d1, d2};
}
static int bass_on(void *ctx, int part, int note, int accent, int slide, int velocity)
{
    (void)ctx; (void)part; (void)note; (void)accent; (void)slide; (void)velocity;
 return 1; }
static void bass_off(void *ctx, int part) { (void)ctx; (void)part; }
static int drum(void *ctx, int kit, int voice, float vel)
{ (void)ctx; (void)kit; (void)voice; (void)vel; return 1; }
static unsigned count_status(const observer_t *o, uint8_t st, int note)
{
    unsigned n = 0;
    for (unsigned i = 0; i < o->trace.len; i++)
        if (o->trace.event[i].st == st && (note < 0 || o->trace.event[i].note == note))
            n++;
    return n;
}
static void init(seq_t *s, observer_t *o, unsigned limit)
{
    for (unsigned i = 0; i < NPAT; ++i)
        pattern_init(&patterns[i], 100 + i, 200 + i);
    patterns[0].bass[0].step[0] = (bstep_t){60, G_NOTE, 100};
    patterns[0].bass[0].step[1] = (bstep_t){64, G_NOTE, 100};
    seq_init(s, patterns);
    s->send_notes = s->send_clock = 1;
    *o = (observer_t){0};
    o->limit = limit;
}
static seq_sink_t sink(observer_t *o)
{
    seq_sink_t s = {0};
    s.ctx = o;
    s.midi = midi_out;
    s.drum = drum;
    s.bass_on = bass_on;
    s.bass_off = bass_off;
    return s;
}
static void advance(seq_t *s, observer_t *o, seq_sink_t *out, uint32_t k)
{
    o->now += k;
    seq_advance(s, k, out);
}
/* This is the actual firmware scheduling contract: render at most the next due
 * event, then let the sequencer advance. A test that calls seq_advance once per
 * arbitrary 1024-frame chunk would miss sample-accurate timing regressions. */
static void render(seq_t *s, observer_t *o, seq_sink_t *out, uint32_t frames)
{
    while (frames) {
        uint32_t avail = frames < o->limit ? frames : o->limit;
        uint32_t k = seq_until_event(s, avail);
        CHECK(k >= 1 && k <= avail);
        if (!k || k > avail) { ++failures; return; }
        advance(s, o, out, k);
        frames -= k;
        o->chunks++;
    }
}
static observer_t scenario(unsigned limit)
{
    seq_t s; observer_t o;
    init(&s, &o, limit);
    seq_sink_t out = sink(&o);
    seq_start(&s);
    /* Start is picked up on the first audio sample. All partitions share it. */
    advance(&s, &o, &out, 1);
    CHECK(s.playing && !s.ext);
    CHECK(count_status(&o, 0xFA, -1) == 1);
    CHECK(count_status(&o, 0x91, 60) == 1);
    /* STOP while step one's 303 gate is held must send exactly one NoteOff. */
    render(&s, &o, &out, 1500);
    seq_stop(&s);
    advance(&s, &o, &out, 1);
    CHECK(!s.playing);
    CHECK(count_status(&o, 0x81, 60) == 1);
    seq_continue(&s);
    advance(&s, &o, &out, 1);
    CHECK(s.playing);
    CHECK(count_status(&o, 0xFB, -1) == 1);
    CHECK(count_status(&o, 0x91, 64) == 1);
    render(&s, &o, &out, 12000);
    seq_stop(&s);
    advance(&s, &o, &out, 1);
    seq_stop(&s); /* redundant release cannot send a second NoteOff */
    advance(&s, &o, &out, 1);
    CHECK(!s.playing);
    CHECK(count_status(&o, 0x81, 60) == 1);
    CHECK(count_status(&o, 0x81, 64) == 1);
    CHECK(count_status(&o, 0x90, -1) == 0);
    CHECK(count_status(&o, 0xFA, -1) == 1);
    CHECK(count_status(&o, 0xFB, -1) == 1);
    CHECK(count_status(&o, 0xFC, -1) == 2);
    CHECK(count_status(&o, 0xF8, -1) >= 10);
    return o;
}
static void compare(const observer_t *base, const observer_t *other)
{
    CHECK(base->trace.len == other->trace.len);
    unsigned n = base->trace.len < other->trace.len ? base->trace.len : other->trace.len;
    for (unsigned i = 0; i < n; ++i) {
        const event_t *a = &base->trace.event[i], *b = &other->trace.event[i];
        CHECK(a->at == b->at);
        CHECK(a->st == b->st);
        CHECK(a->note == b->note);
        CHECK(a->vel == b->vel);
    }
}
int main(void)
{
    const unsigned limits[] = {1, 2, 7, 63, 64, 65, 127, 128, 255, 256, 511, 512, 1024};
    observer_t base = scenario(limits[0]);
    /* 44.1 kHz, 125 BPM -> 882 samples per F8 and 2646 per 303 half-step.
     * Fixed anchor times guard against all partitions drifting identically. */
    static const struct { unsigned index; event_t event; } anchors[] = {
        {0, {1, 0xFA, 0, 0}}, {1, {1, 0x91, 60, 100}},
        {2, {1, 0xF8, 0, 0}}, {3, {883, 0xF8, 0, 0}},
        {4, {1502, 0x81, 60, 0}}, {5, {1502, 0xFC, 0, 0}},
        {6, {1503, 0xFB, 0, 0}}, {7, {1503, 0x91, 64, 100}},
        {9, {2385, 0xF8, 0, 0}}, {11, {4149, 0x81, 64, 0}},
        {22, {12969, 0xF8, 0, 0}}, {23, {13504, 0xFC, 0, 0}}
    };
    CHECK(base.trace.len == 24);
    for (unsigned i = 0; i < sizeof anchors / sizeof anchors[0]; ++i) {
        const event_t *a = &base.trace.event[anchors[i].index];
        const event_t *b = &anchors[i].event;
        CHECK(a->at == b->at && a->st == b->st && a->note == b->note && a->vel == b->vel);
    }
    for (unsigned i = 1; i < sizeof limits / sizeof limits[0]; ++i) {
        observer_t o = scenario(limits[i]);
        compare(&base, &o);
    }
    fprintf(stderr, "seq sample scheduling: %d/%d checks %s; %u events\n",
            checks - failures, checks, failures ? "FAILED" : "PASSED", base.trace.len);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
