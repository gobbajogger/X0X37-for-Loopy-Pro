/* SPDX-License-Identifier: GPL-3.0-only */
/* Transport requests in one audio block: last Start/Continue wins, Stop stays ordered. */
#include <stdio.h>
#include "../../firmware/src/seq/sequencer.h"

static seq_t S;
static pattern_t P[NPAT];
static int checks, failures, starts, continues, stops, note_offs, bass_offs;
#define CHECK(expr) do { checks++; if (!(expr)) { failures++; \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expr); } } while (0)

static int drum(void *ctx, int kit, int voice, float vel) { (void)ctx; (void)kit; (void)voice; (void)vel; return 1; }
static int bass_on(void *ctx, int part, int note, int accent, int slide, int vel)
{ (void)ctx; (void)part; (void)note; (void)accent; (void)slide; (void)vel;  return 1; }
static void bass_off(void *ctx, int part) { (void)ctx; (void)part; bass_offs++; }
static void brk(void *ctx, int st, int bar, float samples, int enabled)
{ (void)ctx; (void)st; (void)bar; (void)samples; (void)enabled; }
static void brk_stop(void *ctx) { (void)ctx; }
static void midi(void *ctx, uint8_t st, uint8_t d1, uint8_t d2)
{
    (void)ctx; (void)d1; (void)d2;
    if (st == 0xFA) starts++;
    if (st == 0xFB) continues++;
    if (st == 0xFC) stops++;
    if ((st & 0xF0u) == 0x80u) note_offs++;
}
static const seq_sink_t sink = {drum, bass_on, bass_off, brk, brk_stop, midi, 0, 0};

static void reset(void)
{
    int i;
    for (i = 0; i < NPAT; i++)
        pattern_init(&P[i], (uint32_t)i + 1u, (uint32_t)i + 2u);
    seq_init(&S, P);
    starts = continues = stops = note_offs = bass_offs = 0;
}
static void consume(void) { seq_advance(&S, 0, &sink); }

int main(void)
{
    reset();
    seq_start(&S);
    seq_continue(&S);
    CHECK(!S.req_start && S.req_cont);             /* later Continue supersedes Start */
    consume();
    CHECK(starts == 0 && continues == 1);          /* no duplicate MIDI transport */
    CHECK(S.playing && !S.req_start && !S.req_cont);

    reset();
    seq_continue(&S);
    seq_start(&S);
    CHECK(S.req_start && !S.req_cont);             /* later Start supersedes Continue */
    consume();
    CHECK(starts == 1 && continues == 0);
    CHECK(S.playing);

    reset();
    S.playing = 1;
    S.t[TRK_DRUM].started = 1;
    S.t[TRK_DRUM].pos = 5;
    S.t[TRK_DRUM].to_next = 512u << 8;
    seq_start(&S);
    seq_continue(&S);
    consume();
    CHECK(S.t[TRK_DRUM].pos == 6);                  /* Continue preserves playback phase */
    CHECK(starts == 0 && continues == 1);

    reset();
    seq_start(&S);
    consume();
    seq_stop(&S);
    seq_start(&S);                                 /* Stop followed by Start is still sequential */
    consume();
    CHECK(starts == 2 && stops == 1 && continues == 0);
    CHECK(S.playing);

    reset();
    seq_start(&S);
    seq_stop(&S);                                  /* Stop after Start cancels it */
    consume();
    CHECK(!S.playing && starts == 0 && continues == 0);

    reset();
    seq_start(&S);
    consume();
    S.send_notes = 1;
    S.bass_out_valid[0] = 1;
    S.bass_notes_out[0] = 36;
    S.bass_out_ch[0] = 1;
    S.t[TRK_BASS0].sounding = 1;
    seq_start(&S);
    seq_continue(&S);
    consume();
    CHECK(note_offs == 1);                         /* exactly one Note Off for a held 303 */
    CHECK(bass_offs == 1 && !S.bass_out_valid[0]);
    CHECK(starts == 1 && continues == 1);

    reset();
    S.clk_src = S.clk_cur = SEQ_CLK_EXT;
    S.ext = 1;
    CHECK(seq_clk_take(&S, 0xFA, 0) == 1);
    CHECK(seq_midi_rt(&S, 0xFA) == 1);
    CHECK(seq_clk_take(&S, 0xFB, 0) == 1);
    CHECK(seq_midi_rt(&S, 0xFB) == 1);
    CHECK(!S.req_start && S.req_cont);             /* MIDI packet burst also coalesces */
    consume();
    CHECK(S.playing && starts == 0 && continues == 0 && stops == 0); /* no echo */

    reset();
    S.clk_src = S.clk_cur = SEQ_CLK_EXT;
    S.ext = 1;
    CHECK(seq_clk_take(&S, 0xFB, 1) == 1);
    CHECK(seq_midi_rt(&S, 0xFB) == 1);
    CHECK(seq_clk_take(&S, 0xFA, 1) == 1);
    CHECK(seq_midi_rt(&S, 0xFA) == 1);
    CHECK(S.req_start && !S.req_cont);
    consume();
    CHECK(S.playing && starts == 0 && continues == 0);

    reset();
    seq_continue(&S);
    seq_start(&S);
    seq_continue(&S);
    seq_start(&S);
    consume();
    CHECK(starts == 1 && continues == 0);          /* arbitrary burst, last request wins */


    /* MID-13: setting INTERNAL before the audio block applies cancels a
     * previously queued MIDI Start/Continue/Stop, not later local gestures. */
    reset();
    CHECK(seq_midi_rt(&S, 0xFA) == 1);
    S.clk_src = SEQ_CLK_INT;
    consume();
    CHECK(!S.playing && starts == 0 && !S.req_start && !S.req_rt);

    reset();
    CHECK(seq_midi_rt(&S, 0xFB) == 1);
    S.clk_src = SEQ_CLK_INT;
    consume();
    CHECK(!S.playing && continues == 0 && !S.req_cont);

    reset();
    S.playing = 1;
    CHECK(seq_midi_rt(&S, 0xFC) == 1);
    S.clk_src = SEQ_CLK_INT;
    consume();
    CHECK(S.playing && stops == 0 && !S.req_stop && !S.req_stop_rt);

    reset();
    seq_start(&S);
    S.clk_src = SEQ_CLK_INT;
    consume();
    CHECK(S.playing && starts == 1 && S.start_local);

    reset();
    seq_continue(&S);
    S.clk_src = SEQ_CLK_INT;
    consume();
    CHECK(S.playing && continues == 1 && S.start_local);

    reset();
    S.playing = 1;
    seq_stop(&S);
    S.clk_src = SEQ_CLK_INT;
    consume();
    CHECK(!S.playing && stops == 1);

    reset();
    CHECK(seq_midi_rt(&S, 0xFA) == 1);
    seq_continue(&S); /* local Continue supersedes remote Start */
    S.clk_src = SEQ_CLK_INT;
    consume();
    CHECK(S.playing && starts == 0 && continues == 1);

    reset();
    S.playing = 1;
    CHECK(seq_midi_rt(&S, 0xFC) == 1);
    seq_start(&S); /* local Start must survive canceled remote Stop */
    S.clk_src = SEQ_CLK_INT;
    consume();
    CHECK(S.playing && starts == 1 && stops == 0);

    reset();
    S.clk_src = SEQ_CLK_INT;
    consume();
    S.clk_src = SEQ_CLK_AUTO;
    consume();
    CHECK(seq_midi_rt(&S, 0xFA) == 1);
    consume();
    CHECK(S.playing); /* MIDI returns when AUTO is selected again */

    reset();
    seq_stop(&S); /* earlier local Stop superseded by remote Stop */
    CHECK(seq_midi_rt(&S, 0xFC) == 1);
    S.playing = 1;
    S.clk_src = SEQ_CLK_INT;
    consume();
    CHECK(S.playing && stops == 0);

    reset();
    CHECK(seq_midi_rt(&S, 0x80) == 0); /* Note Off is never clock traffic */

    fprintf(stderr, "transport-coalesce: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
