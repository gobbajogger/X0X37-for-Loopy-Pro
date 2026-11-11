/* SPDX-License-Identifier: GPL-3.0-only */
/* MID-12/MID-13: AUTO/INTERNAL idle clocks cannot wrap ext_since and
 * mistake 27 hours without input for a sub-millisecond MIDI tick. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sequencer.h"

static unsigned checks, failed, mode, frames;
#define CHECK(x) do { ++checks; if (!(x)) { ++failed; if (failed <= 24u) \
    fprintf(stderr, "FAIL mode=%u frames=%u line=%d: %s\n", mode, frames, __LINE__, #x); } } while (0)

static pattern_t patterns[NPAT];
static seq_sink_t sink;
static void setup(seq_t *s, unsigned m)
{
    memset(patterns, 0, sizeof(patterns));
    memset(&sink, 0, sizeof(sink));
    seq_init(s, patterns);
    s->clk_src = (uint8_t)m;
    s->clk_cur = (uint8_t)m;
}
static void advance(seq_t *s, uint32_t n)
{
    while (n) {
        uint32_t k = n < frames ? n : frames;
        CHECK(k > 0u && k <= frames);
        seq_advance(s, k, &sink);
        n -= k;
    }
}
static void idle_saturates(void)
{
    seq_t s;
    setup(&s, mode);
    s.ext_since = 2u * FS - 1u;
    advance(&s, frames);
    uint32_t cap = s.ext_since;
    CHECK(cap >= 2u * FS && cap <= 2u * FS + frames);
    advance(&s, 3u * FS); /* hours later this must still be the last-clock sentinel */
    CHECK(s.ext_since == cap);
    CHECK(s.ext_period == 0u);
    CHECK(s.ext_bpm == 125.0f);
}
static void idle_rollover_then_clock(void)
{
    seq_t s;
    setup(&s, SEQ_CLK_AUTO);
    s.ext_since = UINT32_MAX - 16u; /* legacy live AUTO idle at ~27.05 hours */
    s.ext_now = UINT32_MAX - 16u;
    advance(&s, 64u);
    CHECK(s.ext_since > FS); /* do not turn a 27-hour silence into 47 samples */
    CHECK(s.ext_now == 47u); /* free-running wrap must remain valid */
    CHECK(seq_clk_take(&s, 0xF8u, 0u) == 1);
    CHECK(seq_midi_rt(&s, 0xF8u) == 1);
    advance(&s, 1u);
    CHECK(s.ext_period == 0u); /* one clock cannot measure a whole new tempo */
    CHECK(s.ext_bpm == 125.0f);
    CHECK(s.ext_since == 0u);
    /* A valid subsequent tick interval must still establish a measured tempo. */
    advance(&s, FS / 48u);
    CHECK(seq_clk_take(&s, 0xF8u, 0u) == 1);
    CHECK(seq_midi_rt(&s, 0xF8u) == 1);
    advance(&s, 1u);
    CHECK(s.ext_period != 0u);
    CHECK(s.ext_bpm >= 119.0f && s.ext_bpm <= 121.0f);
}
/* Real user path: an idle INTERNAL session is changed back to AUTO/EXTERNAL
 * immediately before Loopy reconnects. Do not infer tempo from counter wrap. */
static void internal_idle_mode_change_then_clock(unsigned target)
{
    seq_t s;
    setup(&s, SEQ_CLK_INT);
    s.ext_since = UINT32_MAX - 16u;
    s.ext_now = UINT32_MAX - 16u;
    advance(&s, 64u);
    CHECK(s.ext_since > FS);  /* silence is not a 47-sample MIDI interval */
    CHECK(s.ext_now == 47u);  /* the free-running clock still wraps */
    s.clk_src = (uint8_t)target;
    advance(&s, 1u);         /* firmware applies clock-source change in ISR */
    CHECK(s.clk_cur == target);
    CHECK(s.ext_period == 0u);
    CHECK(seq_clk_take(&s, 0xF8u, 0u) == 1);
    CHECK(seq_midi_rt(&s, 0xF8u) == 1);
    advance(&s, 1u);
    CHECK(s.ext_period == 0u);    /* first clock cannot measure a tempo */
    CHECK(s.ext_bpm == 125.0f);
    advance(&s, FS / 48u);        /* genuine 120 BPM quarter-tick interval */
    CHECK(seq_clk_take(&s, 0xF8u, 0u) == 1);
    CHECK(seq_midi_rt(&s, 0xF8u) == 1);
    advance(&s, 1u);
    CHECK(s.ext_period != 0u);
    CHECK(s.ext_bpm >= 119.0f && s.ext_bpm <= 121.0f);
}

/* Early first Clock is not a tempo measurement: boot and INT->AUTO/EXT. */
static void early_first_clock(unsigned target, int from_internal)
{
    seq_t s;
    setup(&s, from_internal ? SEQ_CLK_INT : SEQ_CLK_AUTO);
    advance(&s, FS / 20u); /* 50 ms of silence before changing CLOCK SOURCE */
    s.clk_src = (uint8_t)target;
    advance(&s, FS / 20u); /* 50 ms before first Loopy MIDI F8 */
    CHECK(s.clk_cur == target);
    CHECK(seq_clk_take(&s, 0xF8u, 0u) == 1);
    CHECK(seq_midi_rt(&s, 0xF8u) == 1);
    advance(&s, 1u);
    CHECK(s.ext_q == 1u);
    CHECK(s.ext_period == 0u); /* no previous tick to measure */
    CHECK(s.ext_bpm == 125.0f);
    advance(&s, FS / 48u); /* 120 BPM: 918.75 samples per MIDI clock */
    CHECK(seq_clk_take(&s, 0xF8u, 0u) == 1);
    CHECK(seq_midi_rt(&s, 0xF8u) == 1);
    advance(&s, 1u);
    CHECK(s.ext_period != 0u);
    CHECK(s.ext_bpm >= 119.0f && s.ext_bpm <= 121.0f);
}
/* The 0.5-second dropout also resets the beat ring. Reconnect must not
 * average that missing-clock gap into the next tempo measurement. */
static void first_clock_after_dropout(unsigned target)
{
    seq_t s;
    setup(&s, SEQ_CLK_AUTO);
    s.clk_src = (uint8_t)target;
    advance(&s, FS); /* first clock occurs well after power-up */
    CHECK(seq_clk_take(&s, 0xF8u, 0u) == 1);
    CHECK(seq_midi_rt(&s, 0xF8u) == 1);
    advance(&s, 1u);
    advance(&s, FS / 48u);
    CHECK(seq_clk_take(&s, 0xF8u, 0u) == 1);
    CHECK(seq_midi_rt(&s, 0xF8u) == 1);
    advance(&s, 1u);
    CHECK(s.ext_period != 0u);
    CHECK(s.ext_bpm >= 119.0f && s.ext_bpm <= 121.0f);
    advance(&s, FS / 2u + 64u); /* trigger AUTO fallback or EXTERNAL wait */
    CHECK(s.ext_q == 0u && s.ext_nb == 0u);
    float before = s.ext_bpm;
    CHECK(seq_clk_take(&s, 0xF8u, 0u) == 1);
    CHECK(seq_midi_rt(&s, 0xF8u) == 1);
    advance(&s, 1u);
    CHECK(s.ext_q == 1u);
    CHECK(s.ext_bpm == before); /* first returning F8 measures no tempo */
    advance(&s, FS / 48u);
    CHECK(seq_clk_take(&s, 0xF8u, 0u) == 1);
    CHECK(seq_midi_rt(&s, 0xF8u) == 1);
    advance(&s, 1u);
    CHECK(s.ext_bpm >= 119.0f && s.ext_bpm <= 121.0f);
}


/* Bluetooth may deliver several 0xF8 ticks in one audio callback. The first
 * packet after silence establishes phase, not an artificial tempo; all ticks
 * must remain counted, and the next *separately timed* tick can measure BPM. */
static void first_clock_burst(unsigned target, unsigned input, unsigned burst)
{
    seq_t s;
    setup(&s, target);
    advance(&s, FS / 10u);  /* early Loopy connection, 100 ms after boot */
    for (unsigned i = 0; i < burst; i++) {
        CHECK(seq_clk_take(&s, 0xF8u, input) == 1);
        CHECK(seq_midi_rt(&s, 0xF8u) == 1);
    }
    advance(&s, 1u);         /* one audio callback, multiple F8 inputs */
    CHECK(s.ext_q == burst);
    CHECK(s.ext_nb == 0u);
    CHECK(s.ext_period == 0u);   /* no independently measured interval */
    CHECK(s.ext_bpm == 125.0f);
    CHECK(s.clk_lock == input + 1u);
    CHECK(seq_clk_take(&s, 0xF8u, input ^ 1u) == 0); /* no double-rate input */
    advance(&s, FS / 48u); /* next true 120-BPM clock tick */
    CHECK(seq_clk_take(&s, 0xF8u, input) == 1);
    CHECK(seq_midi_rt(&s, 0xF8u) == 1);
    advance(&s, 1u);
    CHECK(s.ext_q == burst + 1u);
    CHECK(s.ext_period != 0u);
    CHECK(s.ext_bpm >= 119.0f && s.ext_bpm <= 121.0f);
}

int main(void)
{
    const unsigned partitions[] = {1u, 64u, 1024u};
    for (unsigned i = 0; i < 3u; ++i) {
        frames = partitions[i];
        for (mode = SEQ_CLK_AUTO; mode <= SEQ_CLK_EXT; ++mode)
            idle_saturates();
        mode = SEQ_CLK_AUTO;
        idle_rollover_then_clock();
        internal_idle_mode_change_then_clock(SEQ_CLK_AUTO);
        internal_idle_mode_change_then_clock(SEQ_CLK_EXT);
        early_first_clock(SEQ_CLK_AUTO, 0);
        early_first_clock(SEQ_CLK_EXT, 0);
        early_first_clock(SEQ_CLK_AUTO, 1);
        early_first_clock(SEQ_CLK_EXT, 1);
        first_clock_after_dropout(SEQ_CLK_AUTO);
        first_clock_after_dropout(SEQ_CLK_EXT);
        for (unsigned burst = 2u; burst <= 5u; burst++) {
            first_clock_burst(SEQ_CLK_AUTO, 0u, burst); /* USB */
            first_clock_burst(SEQ_CLK_AUTO, 1u, burst); /* Bluetooth */
            first_clock_burst(SEQ_CLK_EXT, 0u, burst);
            first_clock_burst(SEQ_CLK_EXT, 1u, burst);
        }
    }
    fprintf(stderr, "seq AUTO idle wrap: %u/%u checks %s\n", checks-failed, checks, failed?"FAILED":"PASSED");
    return failed ? 1 : 0;
}
