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
int main(void)
{
    const unsigned partitions[] = {1u, 64u, 1024u};
    for (unsigned i = 0; i < 3u; ++i) {
        frames = partitions[i];
        for (mode = SEQ_CLK_AUTO; mode <= SEQ_CLK_EXT; ++mode)
            idle_saturates();
        mode = SEQ_CLK_AUTO;
        idle_rollover_then_clock();
    }
    fprintf(stderr, "seq AUTO idle wrap: %u/%u checks %s\n", checks-failed, checks, failed?"FAILED":"PASSED");
    return failed ? 1 : 0;
}
