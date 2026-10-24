/* SPDX-License-Identifier: GPL-3.0-only
 * Native link-level regression for MID-13: queued MIDI transport versus
 * an INTERNAL clock-source edit in the same audio block.
 * Compile with production sequencer.c + pattern.c + tb3po.c.
 * Never claim that this is real hardware timing or firmware package QA.
 */
#include <stdio.h>
#include <stdlib.h>
#include "sequencer.h"

static pattern_t patterns[NPAT];
static int checks, failures;
static unsigned frames = 1u;
#define CHECK(expr) do { ++checks; if (!(expr)) { ++failures; fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); } } while (0)

static void start(seq_t *s)
{
    static int initialized;
    if (!initialized) {
        for (int i = 0; i < NPAT; ++i)
            pattern_init(&patterns[i], (unsigned)(123 + i), (unsigned)(789 + i));
        initialized = 1;
    }
    seq_init(s, patterns);
    s->send_clock = 0;
}

static void settle(seq_t *s)
{
    seq_sink_t out = {0};
    seq_advance(s, frames, &out);
}

static void remote_start_cancel(int clock, int msg)
{
    seq_t s;
    start(&s);
    s.clk_src = (unsigned char)clock;
    settle(&s); /* apply EXTERNAL or AUTO before the remote transport */
    CHECK(seq_midi_rt(&s, (unsigned char)msg) == 1);
    CHECK(s.req_start || s.req_cont);
    s.clk_src = SEQ_CLK_INT;
    settle(&s);
    CHECK(s.clk_cur == SEQ_CLK_INT);
    CHECK(!s.playing);
    CHECK(!s.req_start && !s.req_cont && !s.req_rt);
    CHECK(s.ext_pending == 0);
}

static void remote_stop_cancel(int clock)
{
    seq_t s;
    start(&s);
    s.clk_src = (unsigned char)clock;
    settle(&s);
    seq_start(&s); /* local PLAY, should survive the clock-source edit */
    settle(&s);
    CHECK(s.playing);
    CHECK(seq_midi_rt(&s, 0xFCu) == 1); /* incoming remote STOP, still queued */
    CHECK(s.req_stop);
    s.clk_src = SEQ_CLK_INT;
    settle(&s);
    CHECK(s.clk_cur == SEQ_CLK_INT);
    CHECK(s.playing);
    CHECK(!s.req_stop);
}

static void local_transport_survives(int clock)
{
    seq_t s;
    start(&s);
    s.clk_src = (unsigned char)clock;
    settle(&s);
    seq_start(&s);                  /* local PLAY immediately before clock source change */
    s.clk_src = SEQ_CLK_INT;
    settle(&s);
    CHECK(s.playing);
    CHECK(s.start_local);
    CHECK(!s.req_start);
    /* local STOP must still work while INTERNAL */
    seq_stop(&s);
    settle(&s);
    CHECK(!s.playing);
    CHECK(!s.req_stop);
}

static void already_playing_survives(int clock)
{
    seq_t s;
    start(&s);
    s.clk_src = (unsigned char)clock;
    settle(&s);
    seq_start(&s);
    settle(&s);
    CHECK(s.playing);
    s.clk_src = SEQ_CLK_INT;
    settle(&s);
    CHECK(s.playing);
    CHECK(!s.req_start && !s.req_cont && !s.req_stop);
    CHECK(seq_midi_rt(&s, 0x80u) == 0); /* NOTE OFF must not be eaten by transport */
}

static void immediate_internal_incoming_ignored(void)
{
    seq_t s;
    start(&s);
    s.clk_src = SEQ_CLK_INT;
    CHECK(seq_midi_rt(&s, 0xFAu) == 1);
    CHECK(seq_midi_rt(&s, 0xFBu) == 1);
    CHECK(seq_midi_rt(&s, 0xFCu) == 1);
    CHECK(seq_midi_rt(&s, 0xF8u) == 1);
    settle(&s);
    CHECK(!s.playing);
    CHECK(!s.req_start && !s.req_cont && !s.req_stop);
    CHECK(!s.ext_pending);
}

int main(void)
{
    static const unsigned block_frames[] = {1u, 64u, 1024u};
    for (unsigned block = 0; block < sizeof block_frames / sizeof block_frames[0]; ++block) {
        frames = block_frames[block];
        for (unsigned clock = SEQ_CLK_AUTO; clock <= SEQ_CLK_EXT; clock += 2) {
            remote_start_cancel(clock, 0xFAu);
            remote_start_cancel(clock, 0xFBu);
            remote_stop_cancel(clock);
            local_transport_survives(clock);
            already_playing_survives(clock);
        }
        immediate_internal_incoming_ignored();
    }
    fprintf(stderr, "MID-13 linked first-NIGHTLY clock change: %d/%d assertions %s\n",
            checks - failures, checks, failures ? "FAILED" : "PASSED");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
