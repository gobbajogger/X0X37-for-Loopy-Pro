/* SPDX-License-Identifier: GPL-3.0-only */
/* Native link-level regression: transport origin and Note Off ownership.
 * Link unchanged production sequencer.c, pattern.c and tb3po.c; no mock C handlers.
 * This is NOT target-firmware, queue-pressure, or physical-device validation.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "sequencer.h"

static pattern_t patterns[NPAT];
static int checks, failures;
static unsigned block_frames;
typedef struct { uint8_t st, d1, d2; } event_t;
typedef struct { event_t event[64]; int used, bass_on, bass_off; } observed_t;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); } } while (0)

static void midi_out(void *ctx, uint8_t st, uint8_t d1, uint8_t d2)
{
    observed_t *o = ctx;
    if (o->used < (int)(sizeof o->event / sizeof o->event[0]))
        o->event[o->used++] = (event_t){st, d1, d2};
}
static int bass_on(void *ctx, int part, int note, int accent, int slide, int vel)
{
    observed_t *o = ctx;
    (void)part; (void)note; (void)accent; (void)slide; (void)vel;
    ++o->bass_on;
 return 1; }
static void bass_off(void *ctx, int part)
{
    observed_t *o = ctx;
    (void)part;
    ++o->bass_off;
}
static int count(const observed_t *o, uint8_t st, uint8_t note)
{
    int found = 0;
    for (int i = 0; i < o->used; ++i)
        if (o->event[i].st == st && o->event[i].d1 == note)
            ++found;
    return found;
}
static void setup(seq_t *s, observed_t *o)
{
    for (int i = 0; i < NPAT; ++i)
        pattern_init(&patterns[i], (uint32_t)(100 + i), (uint32_t)(200 + i));
    patterns[0].bass[0].step[0] = (bstep_t){60, G_NOTE | BS_SLIDE, 100};
    patterns[0].bass[0].step[1] = (bstep_t){64, G_NOTE | BS_SLIDE, 90};
    seq_init(s, patterns);
    s->bpm = 120.0f;
    s->send_notes = 1;
    s->send_clock = 0;
    *o = (observed_t){0};
}
static void advance(seq_t *s, observed_t *o, uint32_t frames)
{
    seq_sink_t sink = {.midi=midi_out, .bass_on=bass_on, .bass_off=bass_off, .ctx=o};
    seq_advance(s, frames, &sink);
}
static void local_start(seq_t *s, observed_t *o)
{
    seq_start(s);
    advance(s, o, block_frames);
    CHECK(s->playing);
    CHECK(count(o, 0x91, 60) == 1);
    CHECK(s->bass_out_valid[0]);
}
static void stop_releases_once(void)
{
    seq_t s; observed_t o;
    setup(&s, &o);
    local_start(&s, &o);
    CHECK(seq_midi_rt(&s, 0x80) == 0); /* MIDI Note Off is NOT transport. */
    CHECK(count(&o, 0x81, 60) == 0);
    seq_stop(&s);
    advance(&s, &o, block_frames);
    CHECK(!s.playing);
    CHECK(count(&o, 0x81, 60) == 1);
    CHECK(!s.bass_out_valid[0]);
    seq_stop(&s);
    advance(&s, &o, block_frames);
    CHECK(count(&o, 0x81, 60) == 1);
    CHECK(o.used == 2); /* One On and one Off. */
}
static void midi_echo_must_not_retrigger(void)
{
    seq_t s; observed_t o;
    setup(&s, &o);
    local_start(&s, &o);
    CHECK(seq_midi_rt(&s, 0xFA) == 1);
    advance(&s, &o, block_frames);
    CHECK(s.playing);
    CHECK(count(&o, 0x91, 60) == 1);
    CHECK(count(&o, 0x81, 60) == 0);
    seq_stop(&s);
    advance(&s, &o, block_frames);
    CHECK(count(&o, 0x81, 60) == 1);
}
static void internal_cancels_queued_remote_stop(void)
{
    seq_t s; observed_t o;
    setup(&s, &o);
    local_start(&s, &o);
    CHECK(seq_midi_rt(&s, 0xFC) == 1);
    CHECK(s.req_stop && s.req_stop_rt);
    s.clk_src = SEQ_CLK_INT;
    advance(&s, &o, block_frames);
    CHECK(s.clk_cur == SEQ_CLK_INT);
    CHECK(s.playing && !s.req_stop);
    CHECK(count(&o, 0x81, 60) == 0); /* Old remote Stop must NOT cut a local PLAY. */
    seq_stop(&s);
    advance(&s, &o, block_frames);
    CHECK(!s.playing);
    CHECK(count(&o, 0x81, 60) == 1);
}
static void remote_stop_releases_note_once(void)
{
    seq_t s; observed_t o;
    setup(&s, &o);
    local_start(&s, &o);
    CHECK(seq_midi_rt(&s, 0xFC) == 1);
    advance(&s, &o, block_frames);
    CHECK(!s.playing);
    CHECK(count(&o, 0x81, 60) == 1);
    CHECK(seq_midi_rt(&s, 0xFC) == 1);
    advance(&s, &o, block_frames);
    CHECK(count(&o, 0x81, 60) == 1);
}
static void external_clock_stop_sends_single_noteoff(void)
{
    seq_t s; observed_t o;
    setup(&s, &o);
    s.clk_src = SEQ_CLK_EXT;
    advance(&s, &o, block_frames); /* Clock-source application waits for F8. */
    CHECK(s.clk_cur == SEQ_CLK_EXT && !s.playing);
    CHECK(seq_midi_rt(&s, 0xFA) == 1);
    CHECK(seq_midi_rt(&s, 0xF8) == 1);
    advance(&s, &o, block_frames);
    CHECK(s.playing && s.ext);
    CHECK(count(&o, 0x91, 60) == 1);
    CHECK(count(&o, 0xFA, 0) == 0); /* No outgoing MIDI Start echo. */
    CHECK(seq_midi_rt(&s, 0xFC) == 1);
    advance(&s, &o, block_frames);
    CHECK(!s.playing);
    CHECK(count(&o, 0x81, 60) == 1);
    CHECK(count(&o, 0xFC, 0) == 0); /* No outgoing MIDI Stop echo. */
    CHECK(seq_midi_rt(&s, 0xF8) == 1);
    advance(&s, &o, block_frames);
    CHECK(!s.playing && count(&o, 0x81, 60) == 1);
}
static void changed_channel_clears_old_note(void)
{
    seq_t s; observed_t o;
    setup(&s, &o);
    local_start(&s, &o);
    s.ch[TRK_BASS0] = 4;
    advance(&s, &o, block_frames);
    CHECK(count(&o, 0x81, 60) == 1); /* Off MUST use originally sounding channel. */
    CHECK(count(&o, 0x84, 60) == 0);
    CHECK(!s.bass_out_valid[0]);
    seq_stop(&s);
    advance(&s, &o, block_frames);
    CHECK(count(&o, 0x81, 60) == 1);
}
static void slide_stop_releases_both_notes(void)
{
    seq_t s; observed_t o;
    setup(&s, &o);
    local_start(&s, &o);
    advance(&s, &o, 6000u); /* Next live sixteenth step, with slide overlap. */
    CHECK(count(&o, 0x91, 64) == 1);
    CHECK(s.bass_slide_valid[0] && s.bass_out_valid[0]);
    CHECK(count(&o, 0x81, 60) == 0);
    seq_stop(&s);
    advance(&s, &o, block_frames);
    CHECK(count(&o, 0x81, 60) == 1);
    CHECK(count(&o, 0x81, 64) == 1);
    CHECK(!s.bass_slide_valid[0] && !s.bass_out_valid[0]);
    seq_stop(&s);
    advance(&s, &o, block_frames);
    CHECK(count(&o, 0x81, 60) == 1 && count(&o, 0x81, 64) == 1);
}
int main(void)
{
    static const unsigned frames[] = {1u, 64u, 1024u};
    for (unsigned i = 0; i < sizeof frames / sizeof frames[0]; ++i) {
        block_frames = frames[i];
        stop_releases_once();
        midi_echo_must_not_retrigger();
        internal_cancels_queued_remote_stop();
        remote_stop_releases_note_once();
        external_clock_stop_sends_single_noteoff();
        changed_channel_clears_old_note();
        slide_stop_releases_both_notes();
    }
    fprintf(stderr, "seq transport/NoteOff linked: %d/%d checks %s\n", checks - failures, checks,
            failures ? "FAILED" : "PASSED");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
