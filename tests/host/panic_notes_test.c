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
    if (!(c)) { failures++; fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } \
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
    silent_call_is_safe();
    if (failures) {
        fprintf(stderr, "panic notes: %u checks, %u failures\n", checks, failures);
        return 1;
    }
    printf("panic notes: %u checks, 0 failures\n", checks);
    return 0;
}
