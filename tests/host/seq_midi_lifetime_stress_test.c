/* SPDX-License-Identifier: GPL-3.0-only */
/* Production-source linked MIDI output ownership regression.
 * A live NoteOn must have exactly one NoteOff on the same channel and pitch,
 * including when clock sources, channels and output toggles change while held.
 * This is a host scenario, not a physical MIDI/firmware certification. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sequencer.h"

#define TEST_CASES 180
#define STEPS_PER_CASE 90
#define TEST_ASSERT(cond) do { checks++; if (!(cond)) { if (++failures <= 24) \
  fprintf(stderr, "FAIL case=%u op=%u line=%d: %s\n", case_id, op_id, __LINE__, #cond); } } while (0)

typedef struct {
    uint8_t active[16][128];
    unsigned on, off, realtime, duplicate, orphan;
} midi_log_t;
static unsigned checks, failures, case_id, op_id;
static pattern_t patterns[NPAT];
static uint32_t seed;
static uint32_t rnd(void) { seed = seed * 1664525u + 1013904223u; return seed; }
static void midi_out(void *v, uint8_t status, uint8_t note, uint8_t velocity)
{
    midi_log_t *m = v;
    const unsigned ch = status & 15u;
    const unsigned kind = status & 0xf0u;
    if (kind != 0x90u && kind != 0x80u) {
        if (status >= 0xf8u) m->realtime++;
        return;
    }
    if (note >= 128u) { failures++; return; }
    if (kind == 0x90u && velocity) {
        if (m->active[ch][note]) { m->duplicate++; if (failures++ < 24) fprintf(stderr,"DUP case=%u op=%u channel=%u pitch=%u\n",case_id,op_id,ch,note); }
        m->active[ch][note] = 1;
        m->on++;
    } else {
        if (!m->active[ch][note]) { m->orphan++; if (failures++ < 24) fprintf(stderr,"ORPHAN case=%u op=%u channel=%u pitch=%u\n",case_id,op_id,ch,note); }
        m->active[ch][note] = 0;
        m->off++;
    }
}
static int drum(void *v, int kit, int voice, float vel)
{ (void)v; (void)kit; (void)voice; (void)vel; return 1; }
static int bass_on(void *v, int part, int pitch, int accent, int slide, int vel)
{ (void)v; (void)part; (void)pitch; (void)accent; (void)slide; (void)vel;  return 1; }
static void bass_off(void *v, int part) { (void)v; (void)part; }
static void init(seq_t *s)
{
    for (unsigned p = 0; p < NPAT; ++p) pattern_init(&patterns[p], 17u+p, 41u+p);
    for (unsigned b = 0; b < NBASS; ++b) {
        for (unsigned i = 0; i < 16; ++i) {
            bstep_t *st = &patterns[0].bass[b].step[i];
            *st = (bstep_t){(uint8_t)(60u + b * 14u + (i % 3u) * 2u), G_NOTE, 96};
            if (i % 4 == 2) st->flags |= BS_SLIDE;
        }
        patterns[0].bass[b].len = 16;
    }
    seq_init(s, patterns);
    s->send_notes = 1;
    s->send_clock = 1;
}
static void tick(seq_t *s, seq_sink_t *sink, uint32_t k)
{
    while (k) {
        uint32_t next = seq_until_event(s, k);
        TEST_ASSERT(next > 0 && next <= k);
        if (!next || next > k) return;
        seq_advance(s, next, sink);
        k -= next;
    }
}
static void remote(seq_t *s, uint8_t status, unsigned input)
{
    if (seq_clk_take(s, status, input)) seq_midi_rt(s, status);
}
int main(void)
{
    static const uint32_t bufs[] = {1u, 64u, 1024u};
    unsigned total_on = 0, total_off = 0;
    for (case_id = 0; case_id < TEST_CASES; case_id++) {
        midi_log_t log = {0};
        seq_t s;
        init(&s);
        seq_sink_t output = {0};
        output.ctx = &log;
        output.midi = midi_out;
        output.bass_on = bass_on;
        output.bass_off = bass_off;
        output.drum = drum;
        seed = 0x5918ba5eu + case_id * 0x9e3779b9u;
        op_id = 0;
        seq_start(&s);
        tick(&s, &output, 1);
        for (op_id = 1; op_id <= STEPS_PER_CASE; op_id++) {
            uint32_t r = rnd(), op = (r >> 8) % 13u;
            switch (op) {
                case 0: seq_start(&s); break;
                case 1: seq_stop(&s); break;
                case 2: seq_continue(&s); break;
                case 3: remote(&s, 0xfa, (r >> 17) & 1u); break;
                case 4: remote(&s, 0xfb, (r >> 17) & 1u); break;
                case 5: remote(&s, 0xfc, (r >> 17) & 1u); break;
                case 6: for (unsigned k=0;k<1u+((r>>24)&7u);k++) remote(&s,0xf8,(r>>17)&1u); break;
                case 7: s.clk_src = (uint8_t)((s.clk_src + 1u) % 3u); break;
                case 8: s.send_notes = (uint8_t)!s.send_notes; break;
                case 9: s.ch[TRK_BASS0] = (uint8_t)(((r>>22)&1u)?1:2); break;
                case 10: s.ch[TRK_BASS0] = SEQ_CH_OFF; break;
                case 11: s.ch[TRK_BASS0 + 1] = (uint8_t)(((r>>21)&1u)?2:3); break;
                default: s.ch[TRK_BASS0+1] = SEQ_CH_OFF; break;
            }
            tick(&s, &output, bufs[case_id % 3u]);
        }
        /* Always release output even if NoteOut was just disabled or channels changed. */
        op_id = STEPS_PER_CASE + 1u;
        seq_stop(&s); tick(&s, &output, 1u);
        seq_stop(&s); tick(&s, &output, 1u);
        TEST_ASSERT(!s.playing);
        for (unsigned ch = 0; ch < 16u; ch++)
            for (unsigned n = 0; n < 128u; n++)
                TEST_ASSERT(!log.active[ch][n]);
        TEST_ASSERT(log.on == log.off);
        TEST_ASSERT(log.duplicate == 0);
        TEST_ASSERT(log.orphan == 0);
        total_on += log.on; total_off += log.off;
    }
    TEST_ASSERT(total_on > 5);
    TEST_ASSERT(total_on == total_off);
    fprintf(stderr, "seq MIDI lifetime stress: %u/%u assertions %s, %u note-ons, %u note-offs, %u deterministic scenarios\n", checks - failures, checks, failures ? "FAILED" : "PASSED", total_on, total_off, TEST_CASES);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
