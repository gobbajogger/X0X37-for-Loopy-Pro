/* SPDX-License-Identifier: GPL-3.0-only */
/* Link the real sequencer, pattern and TB-3PO. Observe local and MIDI output;
 * no replacement gate algorithm, private-function extraction or hardware. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/sequencer.h"

enum { ON, OFF, MIDI, STEP };
typedef struct { unsigned at; int kind, part, note, slide; } event_t;
static event_t events[8192];
static unsigned now;
static int nevents, checks, failures;
static pattern_t patterns[NPAT];
static seq_t seq;
#define CHECK(c) do { checks++; if (!(c)) { failures++; fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); } } while (0)
static void log_event(int kind, int part, int note, int slide)
{
    if (nevents < (int)(sizeof events / sizeof events[0]))
        events[nevents++] = (event_t){now, kind, part, note, slide};
    else
        CHECK(0); /* Never silently truncate an event trace. */
}
static int drum(void *ctx, int k, int v, float vel)
{ (void)ctx; (void)k; (void)v; (void)vel; return 1; }
static void on(void *ctx, int b, int note, int accent, int slide, int vel)
{ (void)ctx; (void)accent; (void)vel; log_event(ON, b, note, slide); }
/* PR139 changes bass_on to return acceptance. Exercise accepted notes with either
 * exact production ABI, without casting incompatible function pointers. */
static int accepted_on(void *ctx, int b, int note, int accent, int slide, int vel)
{ on(ctx, b, note, accent, slide, vel); return 1; }
#define ON_CALLBACK _Generic(((seq_sink_t *)0)->bass_on, \
    void (*)(void *, int, int, int, int, int): on, \
    int (*)(void *, int, int, int, int, int): accepted_on)
static void off(void *ctx, int b)
{ (void)ctx; log_event(OFF, b, 0, 0); }
static void midi(void *ctx, uint8_t status, uint8_t note, uint8_t vel)
{ (void)ctx; (void)vel; log_event(MIDI, status, note, 0); }
static void step(void *ctx, int part, int pos)
{ (void)ctx; log_event(STEP, part, pos, 0); }
static const seq_sink_t sink = {.drum=drum, .bass_on=ON_CALLBACK, .bass_off=off, .midi=midi, .step=step};

/* Engine render loop: split actual sample time at production events. */
static void run(unsigned n)
{
    seq_advance(&seq, 0, &sink);
    while (n) {
        unsigned k = seq_until_event(&seq, n > 256 ? 256 : n);
        now += k;
        seq_advance(&seq, k, &sink);
        n -= k;
    }
}
static void setup(void)
{
    for (int i = 0; i < NPAT; i++) pattern_init(&patterns[i], 123, 456);
    seq_init(&seq, patterns);
    seq.bpm = 150; /* quarter = 17640 samples; straight sixteenth = 4410 */
    seq.send_clock = 0;
    seq.send_notes = 1;
    nevents = 0;
    now = 0;
}
static int count(int kind, int part)
{
    int n = 0;
    for (int i = 0; i < nevents; i++) n += events[i].kind == kind && events[i].part == part;
    return n;
}
static int at(int kind, int part, int note, unsigned time, int slide)
{
    for (int i = 0; i < nevents; i++) {
        const event_t *e = &events[i];
        if (e->kind == kind && e->part == part && e->at == time &&
            (note < 0 || e->note == note) && (slide < 0 || e->slide == slide)) return 1;
    }
    return 0;
}
static void chain(int b, int length, int reverse)
{
    bpart_t *bp = &patterns[0].bass[b];
    bp->len = (uint8_t)length;
    bp->dir = reverse ? DIR_REV : DIR_FWD;
    for (int i = 0; i < length; i++) bp->step[i] = (bstep_t){99, G_TIE, 42};
    bp->step[reverse ? length - 1 : 0] = (bstep_t){36, G_NOTE, 100};
}
static void long_chains(void)
{
    const unsigned lengths[] = {2, 4, 16, 64};
    const unsigned samples[] = {4410, 2940, 2205, 5880}; /* independent musical rates */
    for (int rev = 0; rev < 2; rev++) for (int rate = 0; rate < NRATES; rate++)
        for (unsigned l = 0; l < sizeof lengths / sizeof lengths[0]; l++) {
            setup();
            for (int b = 0; b < NBASS; b++) { chain(b, (int)lengths[l], rev); patterns[0].bass[b].rate = (uint8_t)rate; }
            unsigned end = samples[rate] * lengths[l];
            seq_start(&seq);
            run(end - 1);
            for (int b = 0; b < NBASS; b++) {
                CHECK(count(ON, b) == 1); /* TIE's stored pitch/velocity never retriggers. */
                CHECK(count(OFF, b) == 1);
                CHECK(at(ON, b, 36, 0, 0));
                CHECK(at(OFF, b, -1, end - samples[rate] / 2 - samples[rate] % 2, -1));
                CHECK(count(MIDI, 0x91 + b) == 1);
                CHECK(count(MIDI, 0x81 + b) == 1);
                CHECK(at(MIDI, 0x81 + b, 36, end - samples[rate] / 2 - samples[rate] % 2, -1));
                CHECK(count(STEP, TRK_BASS0 + b) == (int)lengths[l]);
            }
        }
}
static void wrap_and_rest(void)
{
    setup();
    bpart_t *bp = &patterns[0].bass[0];
    bp->len = 4;
    bp->step[2] = (bstep_t){40, G_NOTE, 100};
    bp->step[3] = (bstep_t){99, G_TIE, 100};
    seq_start(&seq);
    run(2 * 4410 + 1);
    bp->step[0] = (bstep_t){12, G_TIE, 100}; /* Edit before wrap, after initial REST. */
    run(3 * 4410 - 2);
    CHECK(count(ON, 0) == 1);
    CHECK(count(OFF, 0) == 1);
    CHECK(at(OFF, 0, -1, 4 * 4410 + 2205, -1));
    CHECK(at(MIDI, 0x81, 40, 4 * 4410 + 2205, -1));
    CHECK(at(STEP, TRK_BASS0, 0, 4 * 4410, -1));
    /* A REST must release immediately even when a preceding SLIDE held the gate. */
    setup();
    patterns[0].bass[0].step[0] = (bstep_t){40, G_NOTE | BS_SLIDE, 100};
    seq_start(&seq); run(4411);
    CHECK(at(OFF, 0, -1, 4410, -1));
    CHECK(at(MIDI, 0x81, 40, 4410, -1));
}
static void retrigger_and_slide(void)
{
    for (int slide = 0; slide < 2; slide++) {
        setup();
        bpart_t *bp = &patterns[0].bass[0];
        bp->step[0] = (bstep_t){36, G_NOTE, 100};
        bp->step[1] = (bstep_t){99, G_TIE | (slide ? BS_SLIDE : 0), 100};
        bp->step[2] = (bstep_t){slide ? 43 : 36, G_NOTE, 100};
        seq_start(&seq); run(2 * 4410 + FS / 50 + 1);
        CHECK(count(ON, 0) == 2);
        CHECK(at(ON, 0, slide ? 43 : 36, 8820, slide));
        CHECK(at(OFF, 0, -1, 6615, -1) == !slide);
        CHECK(at(MIDI, 0x81, 36, slide ? 8820 + FS / 50 : 6615, -1));
    }
}
static void edits_transport_and_cue(void)
{
    setup(); chain(0, 16, 0);
    seq_start(&seq); run(4411);
    seq.bpm = 300; /* A tie remains held; subsequent step timing follows new tempo. */
    run(4410);
    patterns[0].bass[0].rate = RATE_8T;
    run(2205);
    CHECK(count(ON, 0) == 1 && count(OFF, 0) == 0);
    CHECK(at(STEP, TRK_BASS0, 3, 11025, -1));
    run(2940); /* Observe the new eighth-triplet interval, not just its edit. */
    CHECK(at(STEP, TRK_BASS0, 4, 13965, -1));
    CHECK(count(ON, 0) == 1 && count(OFF, 0) == 0);
    seq_stop(&seq); run(1);
    CHECK(count(OFF, 0) == 1 && count(MIDI, 0x81) == 1);
    CHECK(!seq.t[TRK_BASS0].sounding && !seq.bass_out_valid[0]);
    seq_start(&seq); run(1);
    CHECK(count(ON, 0) == 2);
    seq_continue(&seq); run(1); /* Restarting timers flushes a held note first. */
    CHECK(count(OFF, 0) == 2 && count(MIDI, 0x81) == 2);
    setup(); chain(0, 16, 0);
    patterns[0].drum[0].len = 2;
    patterns[1].bass[0].step[0] = (bstep_t){52, G_NOTE, 100};
    seq_start(&seq); run(4411);
    seq_cue_part(&seq, TRK_BASS0, 1); run(4410 + 2205);
    CHECK(seq.ppat[TRK_BASS0] == 1);
    CHECK(at(ON, 0, 52, 8820, 0));
    CHECK(at(MIDI, 0x81, 36, 8820, -1));
    CHECK(at(OFF, 0, -1, 11025, -1));
}
static void external_clock(void)
{
    setup();
    seq.clk_src = SEQ_CLK_EXT;
    patterns[0].bass[0].step[0] = (bstep_t){36, G_NOTE, 100};
    patterns[0].bass[0].step[1] = (bstep_t){99, G_TIE, 100};
    patterns[0].bass[0].step[2] = (bstep_t){99, G_TIE, 100};
    seq_start(&seq);
    /* Feed real 24ppqn events, 735 samples/tick at 150 BPM. */
    for (int tick = 0; tick < 18; tick++) {
        seq_ext_clock(&seq);
        run(735);
    }
    CHECK(count(ON, 0) == 1 && count(OFF, 0) == 1);
    CHECK(at(ON, 0, 36, 0, 0));
    CHECK(at(OFF, 0, -1, 11025, -1));
    CHECK(at(MIDI, 0x81, 36, 11025, -1));
    CHECK(at(STEP, TRK_BASS0, 1, 4410, -1));
    CHECK(at(STEP, TRK_BASS0, 2, 8820, -1));
}
int main(void)
{
    long_chains(); wrap_and_rest(); retrigger_and_slide(); edits_transport_and_cue(); external_clock();
    printf("303 long notes: %d/%d checks passed\n", checks - failures, checks);
    return failures != 0;
}
