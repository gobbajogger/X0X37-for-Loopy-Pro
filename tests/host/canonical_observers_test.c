/* SPDX-License-Identifier: GPL-3.0-only */
/* Real sequencer/FX observer regression. The runner also builds this fixture
 * against an unmodified source tree and compares complete event/state/PCM files.
 * EXPECT_OBSERVERS=0 omits only new observer assertions and observer fields. */
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "seq/sequencer.h"
#include "dsp/fxbus.h"

#ifndef EXPECT_OBSERVERS
#define EXPECT_OBSERVERS 1
#endif

static unsigned checks, failures, case_id, event_count, midi_count, advances;
static uint32_t now;
static FILE *events, *states, *pcm;
static seq_t seq;
static pattern_t patterns[NPAT];
static fxbus_t fx;
static int16_t ring[79200];
static float zero[256], input[256], left[256], right[256];

#define CHECK(c) do { checks++; if (!(c)) { failures++; \
    if (failures < 30) fprintf(stderr, "FAIL case=%u t=%u line=%d: %s\n", \
        case_id, now, __LINE__, #c); } } while (0)
#if EXPECT_OBSERVERS
#define OBS(c) CHECK(c)
#else
#define OBS(c) ((void)0)
#endif

static void write_bytes(FILE *f, const void *p, size_t n)
{
    if (fwrite(p, 1, n, f) != n) { perror("write"); exit(2); }
}
static void event(int kind, int a, int b, int c, int d, int e)
{
    const uint32_t v[] = {case_id, now, (uint32_t)kind, (uint32_t)a,
        (uint32_t)b, (uint32_t)c, (uint32_t)d, (uint32_t)e};
    write_bytes(events, v, sizeof v);
    event_count++;
}
static int drum(void *ctx, int kit, int voice, float velocity)
{
    union { float f; uint32_t u; } v = {velocity};
    (void)ctx; event(0, kit, voice, (int)v.u, 0, 0); return 1;
}
static int bass_on(void *ctx, int part, int note, int accent, int slide, int velocity)
{ (void)ctx; event(1, part, note, accent, slide, velocity); return 1; }
static void bass_off(void *ctx, int part) { (void)ctx; event(2, part, 0, 0, 0, 0); }
static void brk(void *ctx, int step, int bar, float samples, int enabled)
{
    union { float f; uint32_t u; } v = {samples};
    (void)ctx; event(3, step, bar, (int)v.u, enabled, 0);
}
static void brk_stop(void *ctx) { (void)ctx; event(4, 0, 0, 0, 0, 0); }
static void midi(void *ctx, uint8_t status, uint8_t a, uint8_t b)
{ (void)ctx; midi_count++; event(5, status, a, b, 0, 0); }
static void step(void *ctx, int part, int pos) { (void)ctx; event(6, part, pos, 0, 0, 0); }
static void transport(void *ctx, int running) { (void)ctx; event(7, running, 0, 0, 0, 0); }
static const seq_sink_t sink = {drum, bass_on, bass_off, brk, brk_stop, midi, step, 0, transport};

/* Only pointer identity and the new observer fields are excluded. Everything
 * else, including padding zeroed by seq_init, must match the baseline. */
static void snapshot(void)
{
    size_t first = offsetof(seq_t, playing), end_first, second = offsetof(seq_t, clk_to_next);
    uint32_t header[3];
#if EXPECT_OBSERVERS
    end_first = offsetof(seq_t, metro_phase);
#else
    end_first = second;
#endif
    header[0] = case_id; header[1] = now;
    header[2] = (uint32_t)(end_first - first + sizeof seq - second);
    write_bytes(states, header, sizeof header);
    write_bytes(states, (const unsigned char *)&seq + first, end_first - first);
    write_bytes(states, (const unsigned char *)&seq + second, sizeof seq - second);
}
static void advance(uint32_t n)
{
    now += n;
    seq_advance(&seq, n, &sink);
    advances++;
    snapshot();
}
static void render(uint32_t n)
{
    while (n) {
        uint32_t block = n < 256u ? n : 256u;
        advance(0);
        while (block) {
            uint32_t k = seq_until_event(&seq, block);
            CHECK(k && k <= block);
            if (!k || k > block) exit(2);
            advance(k);
            block -= k; n -= k;
        }
    }
}
static void reset(unsigned clock)
{
    unsigned p, b, i;
    case_id++; now = 0;
    for (p = 0; p < NPAT; p++) pattern_init(&patterns[p], 0x100u + p, 0x200u + p);
    patterns[0].drum[0].hit[0][0] = 0x1111u;
    patterns[0].drum[1].hit[7][0] = 0xaaaau;
    for (b = 0; b < NBASS; b++) for (i = 0; i < 16; i++) {
        patterns[0].bass[b].step[i].flags = (uint8_t)(G_NOTE | (i % 3 ? 0 : BS_ACCENT));
        patterns[0].bass[b].step[i].note = (uint8_t)(36u + b * 12u + i % 7u);
    }
    seq_init(&seq, patterns);
    seq.bpm = 120.0f; seq.clk_src = (uint8_t)clock; seq.send_notes = 1;
    advance(0);
    OBS(seq.metro_view == 0);
}
static int packet(uint8_t status, unsigned source, int consume)
{
    int accepted = seq_clk_take(&seq, status, source);
    if (accepted) CHECK(seq_midi_rt(&seq, status));
    if (consume) advance(0);
    return accepted;
}

static void internal_clock(void)
{
    unsigned i;
    reset(SEQ_CLK_INT);
    seq_start(&seq); advance(0);
    OBS(seq.metro_view == 1);
    for (i = 1; i <= 256; i++) {
        render(173);
        /* 120 BPM is exactly 22050 samples/quarter at 44100 Hz. */
        OBS(seq.metro_view == 1u + ((uint64_t)now * 32u / 22050u) % 32u);
    }
#if EXPECT_OBSERVERS
    {
        uint32_t phase = seq.metro_phase;
        uint8_t view = seq.metro_view;
        seq_stop(&seq); advance(0); OBS(seq.metro_view == 0);
        render(5000); OBS(seq.metro_view == 0 && seq.metro_phase == phase);
        seq_continue(&seq); advance(0);
        OBS(seq.metro_view == view && seq.metro_phase == phase);
    }
#else
    seq_stop(&seq); advance(0); render(5000); seq_continue(&seq); advance(0);
#endif
    seq.bpm = 240.0f;
#if EXPECT_OBSERVERS
    { uint8_t view = seq.metro_view; advance(0); OBS(seq.metro_view == view); }
#else
    advance(0);
#endif
    render(11025); /* one full quarter at the new tempo */
    seq_start(&seq); advance(0); OBS(seq.metro_view == 1);
    render(5513); OBS(seq.metro_view == 17);
    seq_stop(&seq); advance(0); OBS(seq.metro_view == 0);
}

static void track_independence(void)
{
    uint8_t views[64] = {0};
    unsigned pass, i;
    for (pass = 0; pass < 2; pass++) {
        reset(SEQ_CLK_INT);
        if (pass) {
            patterns[0].swing = 89;
            patterns[0].drum[0].rate = RATE_16T;
            patterns[0].drum[1].rate = RATE_8T;
            patterns[0].bass[0].rate = RATE_32;
            patterns[0].bass[0].dir = DIR_RANDOM;
            patterns[0].bass[1].dir = DIR_PINGPONG;
            patterns[0].bass[1].len = 11;
        }
        seq_start(&seq); advance(0);
        for (i = 0; i < 64; i++) {
            render(517);
#if EXPECT_OBSERVERS
            if (!pass) views[i] = seq.metro_view;
            else OBS(seq.metro_view == views[i]);
#endif
        }
        seq_stop(&seq); advance(0);
    }
    (void)views;
}

static void external_clock(void)
{
    unsigned i;
    reset(SEQ_CLK_EXT);
    CHECK(packet(0xfau, 0, 1)); OBS(seq.metro_view == 0);
    render(1000); OBS(seq.metro_view == 0);
    CHECK(packet(0xf8u, 0, 1)); OBS(seq.metro_view == 1);
    for (i = 1; i < 49; i++) {
#if EXPECT_OBSERVERS
        uint8_t previous = seq.metro_view;
#endif
        render(919);
        OBS(seq.metro_view == previous); /* no prediction between accepted clocks */
        CHECK(packet(0xf8u, 0, 1));
        OBS(seq.metro_view >= 1 && seq.metro_view <= 32);
        if (i % 24 == 0) OBS(seq.metro_view == 1);
        CHECK(!packet(0xf8u, 1, 1)); /* rejected owner cannot move the observer */
    }
    render(FS / 2u + 1u); CHECK(seq_clock_waiting(&seq)); OBS(seq.metro_view == 0);
    CHECK(packet(0xf8u, 0, 1)); OBS(seq.metro_view > 0);
    CHECK(packet(0xfcu, 0, 1)); OBS(seq.metro_view == 0);
    render(600); CHECK(packet(0xf8u, 0, 1)); OBS(seq.metro_view == 0);
    CHECK(packet(0xfbu, 0, 1)); OBS(seq.metro_view > 0);
    seq_clk_drop(&seq, 0);
    CHECK(packet(0xf8u, 2, 1)); OBS(seq.metro_view > 0);
    seq.clk_src = SEQ_CLK_INT; advance(0); CHECK(!seq.ext); OBS(seq.metro_view > 0);
    render(2000);
    seq.clk_src = SEQ_CLK_EXT; advance(0); OBS(seq.metro_view == 0);
    CHECK(packet(0xf8u, 2, 1)); OBS(seq.metro_view == 1);
    seq_stop(&seq); advance(0); OBS(seq.metro_view == 0);

    reset(SEQ_CLK_EXT);
    for (i = 0; i < 7; i++) CHECK(packet(0xf8u, 0, 0));
    CHECK(packet(0xfau, 0, 0));
    for (i = 0; i < 3; i++) CHECK(packet(0xf8u, 0, 0));
    advance(0);
    OBS(seq.metro_tick == 2 && seq.metro_view == 3); /* pre-Start epoch discarded */
    seq_stop(&seq); advance(0);

    reset(SEQ_CLK_AUTO);
    seq_start(&seq); advance(0); render(3000);
    CHECK(packet(0xf8u, 0, 1)); OBS(seq.metro_view == 1);
    render(FS / 2u + 1u); CHECK(!seq.ext); OBS(seq.metro_view > 0);
    seq_stop(&seq); advance(0);
}

static void fx_audio(void)
{
    unsigned mode, stereo, ping, feedback, block, i, tail_blocks = 0, occupancy_gaps = 0;
    for (mode = 0; mode < 2; mode++) for (stereo = 0; stereo < 2; stereo++)
    for (ping = 0; ping < 2; ping++) for (feedback = 0; feedback < 3; feedback++) {
        int last_wet = -1, last_signal = -1, settled = -1;
        case_id++; now = 0;
        fxbus_init(&fx, ring, (int)(sizeof ring / sizeof ring[0]));
        fxbus_set(&fx, FX_DL_TYPE, (int)mode);
        fxbus_set(&fx, FX_DL_PING, (int)ping);
        fxbus_set(&fx, FX_DL_FDBK, (int)feedback * 40);
        fxbus_set(&fx, FX_DL_TIME, 0);
        fxbus_set(&fx, FX_DL_LEVEL, 127);
        fxbus_set(&fx, FX_RV_LEVEL, 0);
        fxbus_set(&fx, FX_VOLUME, 127);
        fxbus_set_bpm(&fx, 125.0f);
        for (block = 0; block < 1000; block++) {
            int wet = 0, present = 0;
            memset(input, 0, sizeof input);
            if (block == 0) input[0] = 0.5f;
            if (stereo) fxbus_process_st(&fx, zero, zero, zero, input, left, right, 256);
            else { fxbus_process(&fx, zero, zero, input, left, 256); memset(right, 0, sizeof right); }
            for (i = 0; i < 256; i++) {
                CHECK(isfinite(left[i]) && isfinite(right[i]));
                wet |= left[i] != 0.0f || right[i] != 0.0f;
                present |= input[i] != 0.0f;
            }
            write_bytes(pcm, left, sizeof left); write_bytes(pcm, right, sizeof right);
            OBS(fx.dl_signal == (present || wet));
            if (wet) last_wet = (int)block;
#if EXPECT_OBSERVERS
            if (fx.dl_signal) last_signal = (int)block;
            if (!present && wet) tail_blocks++;
            if (!fx.dl_signal && fx.dl_quiet < fx.dlen) occupancy_gaps++;
#endif
            if (settled < 0 && fx.dl_quiet >= fx.dlen) settled = (int)block;
            now += 256;
        }
        if (!feedback) {
            OBS(last_wet == last_signal);
            OBS(settled > last_signal);
            OBS(fx.dl_signal == 0);
        }
        (void)last_wet; (void)last_signal; (void)settled;
    }
    OBS(tail_blocks > 0 && occupancy_gaps > 0);
    (void)tail_blocks; (void)occupancy_gaps;
    /* The DSP observer measures input/wet taps. Display gating belongs to the
     * engine, so zero level/mute must not bypass clocks or delay processing. */
    fxbus_init(&fx, ring, (int)(sizeof ring / sizeof ring[0]));
    fxbus_set(&fx, FX_DL_TYPE, 1); fxbus_set(&fx, FX_DL_LEVEL, 0);
    fxbus_set(&fx, FX_RV_LEVEL, 0); fx.muted = 2;
    memset(input, 0, sizeof input); input[0] = 0.5f;
    fxbus_process_st(&fx, zero, zero, zero, input, left, right, 256);
    OBS(fx.dl_signal == 1);
    for (i = 0; i < 256; i++) CHECK(left[i] == 0 && right[i] == 0);
    write_bytes(pcm, left, sizeof left); write_bytes(pcm, right, sizeof right);
}

int main(int argc, char **argv)
{
    if (argc != 4) { fprintf(stderr, "usage: %s EVENTS STATES PCM\n", argv[0]); return 2; }
    events = fopen(argv[1], "wb"); states = fopen(argv[2], "wb"); pcm = fopen(argv[3], "wb");
    if (!events || !states || !pcm) { perror("open"); return 2; }
    internal_clock(); track_independence(); external_clock(); fx_audio();
    if (fclose(events) || fclose(states) || fclose(pcm)) { perror("close"); return 2; }
    printf("%s canonical_observers checks=%u failures=%u events=%u midi=%u advances=%u "
        "seq_size=%zu fx_size=%zu observer_assertions=%d\n", failures ? "FAIL" : "PASS",
        checks, failures, event_count, midi_count, advances, sizeof seq, sizeof fx, EXPECT_OBSERVERS);
    return failures ? 1 : 0;
}
