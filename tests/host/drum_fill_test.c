/* SPDX-License-Identifier: GPL-3.0-only */
/* M6 drum fill (#815 item 6, owner 2026-11-12 02:29 UTC+10). Links production
 * sequencer.c. Hold: from the 909's next bar a 909 snare on every step, rising
 * 50%..100% over the bar; a step the pattern hits keeps its own hit; release
 * stops at once; stop and panic clear it; mute keeps it silent. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/sequencer.h"

typedef struct { int kit, voice, pos, bar; float vel; } hit_t;
static hit_t hits[4096];
static unsigned nhits, nmidi, checks, failures;
static pattern_t patterns[NPAT];
static seq_t seq;

#define CHECK(c) do { \
    checks++; \
    if (!(c)) { failures++; fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); } \
} while (0)

static int drum(void *ctx, int kit, int voice, float vel)
{
    (void)ctx;
    if (nhits < sizeof hits / sizeof hits[0])
        hits[nhits++] = (hit_t){kit, voice, seq.t[TRK_DRUM].pos, seq.t[TRK_DRUM].bars, vel};
    return 1;
}
static int bass_on(void *c, int p, int n, int a, int s, int v) { (void)c; (void)p; (void)n; (void)a; (void)s; (void)v; return 1; }
static void bass_off(void *c, int p) { (void)c; (void)p; }
static void brk_stop(void *c) { (void)c; }
static void midi_out(void *c, uint8_t st, uint8_t d1, uint8_t d2)
{
    (void)c; (void)d2;
    if ((st & 0xF0u) == 0x90u && (st & 0x0Fu) == seq.ch[TRK_DRUM] && d1 != 0) nmidi++;
}
static const seq_sink_t sink = {
    .drum = drum, .bass_on = bass_on, .bass_off = bass_off, .brk_stop = brk_stop, .midi = midi_out
};

/* run until the 909 has fired n more steps */
static void steps(int n)
{
    seq_advance(&seq, 0, &sink);
    while (n > 0) {
        uint8_t pos = seq.t[TRK_DRUM].pos, bars = seq.t[TRK_DRUM].bars;
        for (int i = 0; i < 100000 && seq.t[TRK_DRUM].pos == pos && seq.t[TRK_DRUM].bars == bars; i++) {
            unsigned k = seq_until_event(&seq, 16u);
            seq_advance(&seq, k ? k : 1u, &sink);
        }
        n--;
    }
}
static void setup(void)
{
    memset(patterns, 0, sizeof patterns);
    for (int i = 0; i < NPAT; i++)
        pattern_init(&patterns[i], 123, 456);
    seq_init(&seq, patterns);
    for (int i = 0; i < NPAT; i++)
        for (int k = 0; k < NKIT; k++) {
            memset(patterns[i].drum[k].hit, 0, sizeof patterns[i].drum[k].hit);
            memset(patterns[i].drum[k].accent, 0, sizeof patterns[i].drum[k].accent);
            patterns[i].drum[k].len = 16;
        }
    for (int i = 0; i < NPAT; i++)
        for (int b = 0; b < NBASS; b++)
            for (int s = 0; s < NSTEPS; s++)
                patterns[i].bass[b].step[s].flags = 0;
    patterns[0].brk.steps = 0;
    for (int s = 0; s < 16; s += 4) patterns[0].drum[0].hit[0][0] |= 1u << s;   /* BD 1, 5, 9, 13 */
    patterns[0].drum[0].hit[SEQ_FILL_VOICE][0] = 1u << 4 | 1u << 12;          /* SD on 2 and 4 */
    seq.bpm = 150.0f;
    seq.send_notes = 1;
    nhits = nmidi = 0;
}
static int sd_at(int bar, int pos, float *vel)
{
    int n = 0;
    for (unsigned i = 0; i < nhits; i++)
        if (hits[i].kit == 0 && hits[i].voice == (int)SEQ_FILL_VOICE && hits[i].bar == bar && hits[i].pos == pos) {
            n++;
            if (vel) *vel = hits[i].vel;
        }
    return n;
}
static int count(int kit, int voice)
{
    int n = 0;
    for (unsigned i = 0; i < nhits; i++) n += hits[i].kit == kit && hits[i].voice == voice;
    return n;
}

static void untouched_without_fill(void)
{
    setup();
    seq_start(&seq);
    steps(32);
    CHECK(count(0, SEQ_FILL_VOICE) == 4);
    CHECK(count(0, 0) == 9);
    CHECK(seq.fill_on == 0);
}

static void starts_on_bar_and_rolls(void)
{
    float v = 0;
    setup();
    seq_start(&seq);
    steps(5);                                  /* into bar 0 */
    seq.fill_req = 1;
    steps(16 - 5);                             /* rest of bar 0: the pattern only */
    for (int p = 0; p < 16; p++) CHECK(sd_at(0, p, 0) == (p == 4 || p == 12));
    CHECK(seq.fill_on == 1);
    steps(15);                                 /* bar 1, all 16 steps */
    for (int p = 0; p < 16; p++) {
        CHECK(sd_at(1, p, &v) == 1);           /* one snare per step, never doubled */
        if (p != 4 && p != 12) {
            float want = 0.5f + 0.5f * (float)p / 15.0f;
            CHECK(v > want - 1e-6f && v < want + 1e-6f);
        }
    }
    CHECK(count(0, 0) == 8);                   /* the kick keeps its pattern */
    for (unsigned i = 0; i < nhits; i++) CHECK(hits[i].kit == 0);   /* 909 only */
    CHECK(nmidi == (unsigned)(count(0, 0) + count(0, SEQ_FILL_VOICE)));   /* each fill hit is echoed */
}

static void release_stops_at_once(void)
{
    setup();
    seq_start(&seq);
    seq.fill_req = 1;
    steps(16);                                 /* bar 1 starts with the fill */
    CHECK(seq.fill_on == 1);
    steps(2);
    seq.fill_req = 0;
    steps(13);
    for (int p = 3; p < 16; p++) CHECK(sd_at(1, p, 0) == (p == 4 || p == 12));
    CHECK(seq.fill_on == 0);
}

static void stop_panic_and_mute(void)
{
    setup();
    seq_start(&seq);
    seq.fill_req = 1;
    steps(17);
    CHECK(seq.fill_on == 1);
    seq_notes_off(&seq, &sink);
    CHECK(seq.fill_on == 0);                   /* panic: waits for the next bar again */
    steps(3);
    CHECK(sd_at(1, 3, 0) == 0);
    steps(16);
    CHECK(seq.fill_on == 1);
    seq_stop(&seq);
    steps(0);
    seq_advance(&seq, 64, &sink);
    CHECK(seq.playing == 0 && seq.fill_on == 0);

    setup();
    seq.vmute = 1u << SEQ_FILL_VOICE;          /* the 909 snare muted */
    seq_start(&seq);
    seq.fill_req = 1;
    steps(32);
    CHECK(count(0, SEQ_FILL_VOICE) == 0);
    setup();
    seq.part_off = 1u << TRK_DRUM;              /* the 909 switched off (M7) */
    seq_start(&seq);
    seq.fill_req = 1;
    steps(32);
    CHECK(count(0, SEQ_FILL_VOICE) == 0);
}

int main(void)
{
    untouched_without_fill();
    starts_on_bar_and_rolls();
    release_stops_at_once();
    stop_panic_and_mute();
    if (failures) {
        fprintf(stderr, "drum fill: %u of %u checks failed\n", failures, checks);
        return 1;
    }
    printf("drum fill: %u checks\n", checks);
    return 0;
}
