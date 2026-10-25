/* SPDX-License-Identifier: GPL-3.0-only */
/* External-clock dropout: a held 303 MIDI note must be released, and
 * a new input may resume on the next step without an early/doubled note.
 * Native test links the production sequencer, pattern and TB-3PO sources. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sequencer.h"

#define CHECK(e) do { checks++; if (!(e)) { failures++; if (failures < 25) \
    fprintf(stderr, "FAIL block=%u line=%d: %s\n", block, __LINE__, #e); } } while (0)
typedef struct {
    unsigned ons, offs, steps, clock_out, transport_out, duplicate, orphan;
    uint8_t held[16][128];
    int last_step;
} output_t;
static pattern_t patterns[NPAT];
static unsigned checks, failures, block;

static void midi_out(void *ctx, uint8_t st, uint8_t note, uint8_t vel)
{
    output_t *o = ctx;
    unsigned kind = st & 0xF0u, ch = st & 15u;
    if (st >= 0xF8u) {
        if (st == 0xF8u) o->clock_out++;
        else o->transport_out++;
    } else if (kind == 0x90u && vel) {
        if (o->held[ch][note]) o->duplicate++;
        o->held[ch][note] = 1;
        o->ons++;
    } else if (kind == 0x80u || (kind == 0x90u && !vel)) {
        if (!o->held[ch][note]) o->orphan++;
        o->held[ch][note] = 0;
        o->offs++;
    }
}
static void step_out(void *ctx, int part, int step)
{
    output_t *o = ctx;
    if (part == TRK_BASS0) {o->steps++; o->last_step = step;}
}
static int bass_on(void *ctx, int part, int note, int accent, int slide, int vel)
{ (void)ctx; (void)part; (void)note; (void)accent; (void)slide; (void)vel;  return 1; }
static void bass_off(void *ctx, int part) { (void)ctx; (void)part; }
static int drum(void *ctx, int kit, int voice, float vel)
{ (void)ctx; (void)kit; (void)voice; (void)vel; return 1; }

static void render(seq_t *s, const seq_sink_t *out, uint32_t n)
{
    while (n) {
        uint32_t k = seq_until_event(s, n);
        CHECK(k > 0 && k <= n);
        if (!k || k > n) return;
        seq_advance(s, k, out);
        n -= k;
    }
}
static void render_blocks(seq_t *s, const seq_sink_t *out, uint32_t frames)
{
    while (frames) {
        uint32_t n = frames < block ? frames : block;
        render(s, out, n);
        frames -= n;
    }
}
static int clock_packet(seq_t *s, const seq_sink_t *out, unsigned input)
{
    int accepted = seq_clk_take(s, 0xF8u, input);
    if (accepted) seq_midi_rt(s, 0xF8u);
    render(s, out, 1);
    return accepted;
}
static void check_note_state(const output_t *o, unsigned expected)
{
    unsigned held = 0;
    for (unsigned ch = 0; ch < 16u; ch++)
        for (unsigned note = 0; note < 128u; note++) held += o->held[ch][note];
    CHECK(held == expected);
}

int main(void)
{
    const uint32_t blocks[] = {1u, 64u, 1024u};
    for (unsigned i = 0; i < sizeof blocks / sizeof *blocks; i++) {
        block = blocks[i];
        for (unsigned p = 0; p < NPAT; p++) pattern_init(&patterns[p], p + 17u, p + 39u);
        for (unsigned p = 0; p < 16u; p++)
            patterns[0].bass[0].step[p] = (bstep_t){(uint8_t)(60u + (p % 3u)), G_NOTE | BS_SLIDE, 100};
        patterns[0].bass[0].len = 16;
        seq_t s;
        seq_init(&s, patterns);
        s.clk_src = SEQ_CLK_EXT;
        s.send_notes = 1;
        output_t o = {0};
        const seq_sink_t sink = {.ctx=&o, .midi=midi_out, .step=step_out,
                                 .bass_on=bass_on, .bass_off=bass_off, .drum=drum};
        render(&s, &sink, 1);                  /* select EXTERNAL */
        seq_start(&s);
        render(&s, &sink, 1);                  /* local PLAY arms, no clock */
        CHECK(s.playing && s.ext);
        CHECK(o.steps == 0 && o.ons == 0);
        CHECK(clock_packet(&s, &sink, 0) == 1); /* USB: first tick plays step 1 */
        CHECK(o.steps == 1 && o.ons == 1 && o.offs == 0);
        check_note_state(&o, 1);
        CHECK(clock_packet(&s, &sink, 1) == 0); /* Bluetooth cannot steal live USB lock */
        CHECK(o.steps == 1);

        render_blocks(&s, &sink, FS / 2u + 2u); /* no clock > 0.5 s */
        CHECK(seq_clock_waiting(&s));
        CHECK(s.playing && s.ext && s.clk_lost);
        CHECK(o.steps == 1);
        CHECK(o.ons == 1 && o.offs == 1);     /* timeout releases held slide */
        check_note_state(&o, 0);
        CHECK(s.clk_lock == 0);               /* timed-out owner relinquishes input */

        for (unsigned tick = 1; tick <= 5u; tick++) {
            CHECK(clock_packet(&s, &sink, 1) == 1); /* Bluetooth takes over */
            CHECK(o.steps == 1 && o.ons == 1); /* no phantom first step */
        }
        CHECK(!seq_clock_waiting(&s));
        CHECK(clock_packet(&s, &sink, 0) == 0); /* USB no longer owns */
        CHECK(clock_packet(&s, &sink, 1) == 1); /* sixth new tick: next step */
        CHECK(o.steps == 2 && o.ons == 2 && o.offs == 1);
        CHECK(o.last_step == 1);              /* zero-based step 2 */
        check_note_state(&o, 1);
        seq_stop(&s);
        render(&s, &sink, 1);
        seq_stop(&s);                         /* idempotent STOP and NoteOff */
        render(&s, &sink, 1);
        CHECK(!s.playing);
        CHECK(o.ons == 2 && o.offs == 2);
        CHECK(o.duplicate == 0 && o.orphan == 0);
        CHECK(o.transport_out == 0 && o.clock_out == 0); /* EXTERNAL echoes nothing */
        check_note_state(&o, 0);
    }
    fprintf(stderr, "seq external dropout/resume: %u/%u checks %s\n",
            checks - failures, checks, failures ? "FAILED" : "PASSED");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
