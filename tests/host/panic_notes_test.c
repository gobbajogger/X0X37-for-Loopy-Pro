/* SPDX-License-Identifier: GPL-3.0-only */
/* M1: release sounding sequencer notes without stopping transport.
 * Links production sequencer.c. Device and native builds are not this test. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/sequencer.h"

enum { EV_ON, EV_OFF, EV_MIDI, EV_BRK };
typedef struct { unsigned at; int kind, a, b; } event_t;
static event_t events[4096];
static unsigned now, nevents, checks, failures;
static pattern_t patterns[NPAT];
static seq_t seq;
static uint8_t pattern_before[sizeof(pattern_t)];

#define CHECK(c) do { \
    checks++; \
    if (!(c)) { failures++; fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); } \
} while (0)

static void log_event(int kind, int a, int b)
{
    if (nevents < (unsigned)(sizeof events / sizeof events[0]))
        events[nevents++] = (event_t){now, kind, a, b};
    else
        CHECK(0);
}
static int bass_on(void *ctx, int part, int note, int accent, int slide, int vel)
{
    (void)ctx; (void)accent; (void)slide; (void)vel;
    log_event(EV_ON, part, note);
    return 1;
}
static void bass_off(void *ctx, int part)
{
    (void)ctx;
    log_event(EV_OFF, part, 0);
}
static void brk_stop(void *ctx)
{
    (void)ctx;
    log_event(EV_BRK, 0, 0);
}
static void midi_out(void *ctx, uint8_t st, uint8_t d1, uint8_t d2)
{
    (void)ctx; (void)d2;
    log_event(EV_MIDI, st, d1);
}
static const seq_sink_t sink = {
    .bass_on = bass_on, .bass_off = bass_off, .brk_stop = brk_stop, .midi = midi_out
};

static void run(unsigned n)
{
    seq_advance(&seq, 0, &sink);
    while (n) {
        unsigned k = seq_until_event(&seq, n > 256u ? 256u : n);
        now += k;
        seq_advance(&seq, k, &sink);
        n -= k;
    }
}
static int count_kind(int kind, int a)
{
    unsigned i, n = 0;
    for (i = 0; i < nevents; i++)
        n += events[i].kind == kind && (a < 0 || events[i].a == a);
    return (int)n;
}
static int saw_status(unsigned status)
{
    unsigned i;
    for (i = 0; i < nevents; i++)
        if (events[i].kind == EV_MIDI && (unsigned)events[i].a == status)
            return 1;
    return 0;
}
static void hold_line(int part, int first, int later)
{
    bpart_t *bp = &patterns[0].bass[part];
    int i;
    bp->len = 8;
    bp->rate = 0;
    bp->dir = DIR_FWD;
    for (i = 0; i < 8; i++)
        bp->step[i] = (bstep_t){99, G_TIE, 100};
    bp->step[0] = (bstep_t){(uint8_t)first, G_NOTE, 100};
    bp->step[4] = (bstep_t){(uint8_t)later, G_NOTE, 100};
}
static void setup(void)
{
    int i;
    memset(patterns, 0, sizeof patterns);
    for (i = 0; i < NPAT; i++)
        pattern_init(&patterns[i], 123, 456);
    seq_init(&seq, patterns);
    seq.bpm = 150.0f;
    seq.send_notes = 1;
    seq.send_clock = 1;
    hold_line(0, 36, 48);
    hold_line(1, 40, 52);
    patterns[0].brk.steps = 0;
    nevents = 0;
    now = 0;
}

static void notes_off_keeps_transport(void)
{
    uint8_t pos;
    uint32_t to_next;
    float bpm;
    uint16_t song;
    setup();
    memcpy(pattern_before, &patterns[0], sizeof pattern_before);
    seq_start(&seq);
    run(64);
    CHECK(seq.playing == 1);
    CHECK(count_kind(EV_ON, 0) == 1);
    CHECK(count_kind(EV_ON, 1) == 1);
    CHECK(saw_status(0x90 | seq.ch[TRK_BASS0]));
    CHECK(saw_status(0xFA));
    pos = seq.t[TRK_BASS0].pos;
    to_next = seq.t[TRK_BASS0].to_next;
    bpm = seq.bpm;
    song = seq.song_pos;
    seq_notes_off(&seq, &sink);
    CHECK(seq.playing == 1);
    CHECK(seq.req_stop == 0);
    CHECK(seq.req_start == 0);
    CHECK(seq.t[TRK_BASS0].pos == pos);
    CHECK(seq.t[TRK_BASS0 + 1].pos == pos);
    CHECK(seq.t[TRK_BASS0].to_next == to_next);
    CHECK(seq.t[TRK_BASS0].sounding == 0);
    CHECK(seq.t[TRK_BASS0 + 1].sounding == 0);
    CHECK(seq.t[TRK_BASS0].to_off == 0);
    CHECK(seq.bpm == bpm);
    CHECK(seq.song_pos == song);
    CHECK(count_kind(EV_OFF, 0) == 1);
    CHECK(count_kind(EV_OFF, 1) == 1);
    CHECK(count_kind(EV_BRK, -1) == 1);
    CHECK(saw_status(0x80 | seq.ch[TRK_BASS0]));
    CHECK(saw_status(0x80 | seq.ch[TRK_BASS0 + 1]));
    CHECK(!saw_status(0xFC));
    CHECK(memcmp(pattern_before, &patterns[0], sizeof pattern_before) == 0);
    run(4u * 4410u);
    CHECK(seq.playing == 1);
    CHECK(seq.t[TRK_BASS0].pos == 4);
    CHECK(count_kind(EV_ON, 0) == 2);
    CHECK(count_kind(EV_ON, 1) == 2);
    CHECK(saw_status(0xF8));
    CHECK(!saw_status(0xFC));
    CHECK(seq.req_stop == 0);
}

/* Panic must retire capture ownership as well as the sounding gate. */
static void panic_finishes_recording_without_overwriting_future_notes(void)
{
    int src, b;
    for (src = 0; src <= 3; src++) {
        bstep_t future[NBASS];
        uint8_t gen[NBASS], pos[NBASS];
        uint32_t next[NBASS];
        setup();
        seq.rec_mask = src == 3 ? (1u << NBASS) - 1u : 0;
        seq.rec_ext_mask = src < 3 ? (1u << NBASS) - 1u : 0;
        seq_start(&seq);
        run(64);
        for (b = 0; b < NBASS; b++) {
            seq_bass_rec_t *r = &seq.rec_bass[b];
            seq_rec_bass(&seq, b, 60 + b, 100, 1, src, 0);
            r->own_ok = 1;
            r->own_gen = r->gen;
            r->own_src = (uint8_t)src;
            r->own_note = (uint8_t)(60 + b);
            CHECK(r->active);
            future[b] = patterns[0].bass[b].step[4];
            gen[b] = r->gen;
            pos[b] = seq.t[TRK_BASS0 + b].pos;
            next[b] = seq.t[TRK_BASS0 + b].to_next;
        }
        seq_notes_off(&seq, &sink);
        for (b = 0; b < NBASS; b++) {
            CHECK(!seq.rec_bass[b].active);
            CHECK(!seq.rec_bass[b].own_ok);
            CHECK(seq.rec_bass[b].gen != gen[b]);
            CHECK(seq.t[TRK_BASS0 + b].to_off == 0);
            CHECK(seq.t[TRK_BASS0 + b].pos == pos[b]);
            CHECK(seq.t[TRK_BASS0 + b].to_next == next[b]);
        }
        CHECK(seq.playing && !seq.req_stop && !seq.req_start);
        CHECK(seq.rec_mask == (src == 3 ? (1u << NBASS) - 1u : 0));
        CHECK(seq.rec_ext_mask == (src < 3 ? (1u << NBASS) - 1u : 0));
        run(4u * 4410u);
        for (b = 0; b < NBASS; b++) {
            CHECK(memcmp(&future[b], &patterns[0].bass[b].step[4], sizeof future[b]) == 0);
            CHECK(count_kind(EV_ON, b) == 2);
        }
        memcpy(pattern_before, &patterns[0], sizeof pattern_before);
        /* The eventual release of the cancelled press is harmless. */
        for (b = 0; b < NBASS; b++)
            seq_rec_bass(&seq, b, 60 + b, 0, 0, src, 0);
        CHECK(memcmp(pattern_before, &patterns[0], sizeof pattern_before) == 0);
        for (b = 0; b < NBASS; b++) {
            /* REC remains armed, and a new press can record normally. */
            seq_rec_bass(&seq, b, 67 + b, 100, 1, src, 0);
            CHECK(seq.rec_bass[b].active);
            CHECK(seq.rec_bass[b].note == 67 + b);
        }
        CHECK(!saw_status(0xFC));
    }
}

static void silent_call_is_safe(void)
{
    setup();
    seq_notes_off(&seq, &sink);
    CHECK(seq.playing == 0);
    CHECK(count_kind(EV_BRK, -1) == 1);
    CHECK(!saw_status(0xFC));
    CHECK(count_kind(EV_OFF, 0) == 0);
}

int main(void)
{
    notes_off_keeps_transport();
    panic_finishes_recording_without_overwriting_future_notes();
    silent_call_is_safe();
    if (failures) {
        fprintf(stderr, "panic notes: %u checks, %u failures\n", checks, failures);
        return 1;
    }
    printf("panic notes: %u checks, 0 failures\n", checks);
    return 0;
}
