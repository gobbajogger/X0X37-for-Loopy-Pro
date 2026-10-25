/* SPDX-License-Identifier: GPL-3.0-only
 * BF-028: a skipped 303 note is a rest on BOTH the audio and MIDI sink.
 * Real sequencer entry points and an adversarial bass callback exercise the
 * same acceptance boundary as engine.c's T_PROB callback.
 */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/sequencer.h"

static int fails, midi_on[128], midi_off[128], on_count, off_count;
static int allow[128], received_slide[128];
#define CHECK(x, msg) do { if (!(x)) { fprintf(stderr, "FAIL: %s\n", msg); fails++; } } while (0)

static void bass_off(void *ctx, int p);
static int drum(void *ctx, int k, int v, float velocity)
{ (void)ctx; (void)k; (void)v; (void)velocity; return 1; }
/* A zero return means the engine's probability rejected the note. */
#ifdef BF028_BASELINE
static void bass_on(void *ctx, int part, int note, int accent, int slide, int velocity)
#else
static int bass_on(void *ctx, int part, int note, int accent, int slide, int velocity)
#endif
{
    (void)ctx; (void)part; (void)accent; (void)velocity;
    received_slide[note] = slide;
    on_count++;
#ifdef BF028_BASELINE
    if (!allow[note]) bass_off(ctx, part);
#else
    return allow[note];
#endif
}
static void bass_off(void *ctx, int p) { (void)ctx; (void)p; off_count++; }
static void brk(void *ctx, int step, int bar, float t, int en)
{ (void)ctx; (void)step; (void)bar; (void)t; (void)en; }
static void brk_stop(void *ctx) { (void)ctx; }
static void midi(void *ctx, uint8_t st, uint8_t d1, uint8_t d2)
{
    (void)ctx; (void)d2;
    if ((st & 15u) != 1u) return; /* the 303 A's default channel 2 */
    if ((st & 0xF0u) == 0x90u) midi_on[d1]++;
    if ((st & 0xF0u) == 0x80u) midi_off[d1]++;
}
static const seq_sink_t sink = {drum, bass_on, bass_off, brk, brk_stop, midi, 0, 0};
static seq_t seq;
static pattern_t patterns[NPAT];
static void reset(void)
{
    memset(midi_on, 0, sizeof midi_on);
    memset(midi_off, 0, sizeof midi_off);
    memset(allow, 0, sizeof allow);
    memset(received_slide, 0x7f, sizeof received_slide);
    on_count = off_count = 0;
    for (int i=0; i<NPAT; i++) pattern_init(&patterns[i], 100u + (uint32_t)i, 200u + (uint32_t)i);
    seq_init(&seq, patterns);
    seq.send_clock = 0;
    seq.send_notes = 1;
    seq.bpm = 120.0f;
    /* Every other part remains empty. */
    patterns[0].bass[0].len = 4;
}
static void advance_samples(uint32_t samples)
{
    while (samples) {
        uint32_t n = samples < 256u ? samples : 256u;
        seq_advance(&seq, 0, &sink);
        while (n) {
            uint32_t k = seq_until_event(&seq, n);
            if (!k || k > n) { fprintf(stderr,"invalid event advance %u/%u\n",k,n); fails++; return; }
            seq_advance(&seq, k, &sink);
            n -= k;
            samples -= k;
        }
    }
}
static void test_skipped_first_note(void)
{
    reset();
    patterns[0].bass[0].step[0] = (bstep_t){40, G_NOTE, 100};
    allow[40] = 0;
    seq_start(&seq);
    advance_samples(3);
    CHECK(on_count == 1, "probability callback reached for first step");
    CHECK(midi_on[40] == 0, "a rejected 303 step must have no external MIDI NoteOn");
    CHECK(!seq.bass_out_valid[0], "skipped note must not have an active MIDI voice");
    CHECK(!seq.t[TRK_BASS0].sounding, "rejected 303 note must not leave a phantom internal voice");
}
static void test_reject_after_audible_note(void)
{
    reset();
    patterns[0].bass[0].step[0] = (bstep_t){40, G_NOTE | BS_SLIDE, 100};
    patterns[0].bass[0].step[1] = (bstep_t){41, G_NOTE, 100};
    allow[40] = 1;
    allow[41] = 0;
    seq_start(&seq);
    advance_samples(3);
    CHECK(midi_on[40] == 1 && seq.bass_out_valid[0], "accepted step retains MIDI NoteOn");
    advance_samples(6000);
    CHECK(on_count == 2, "both bass steps reached the chance callback");
    CHECK(midi_on[41] == 0, "rejected slide target cannot produce MIDI NoteOn");
    CHECK(midi_off[40] == 1, "old external note ended on a skipped slide target");
    CHECK(!seq.bass_out_valid[0] && !seq.bass_slide_valid[0], "skipped slide target clears both tracked MIDI voices");
    CHECK(!seq.t[TRK_BASS0].sounding && !seq.t[TRK_BASS0].held_note && !seq.t[TRK_BASS0].to_off,
          "rejected slide target restores silent 303 sequencer state");
}
static void test_both_accepted(void)
{
    reset();
    patterns[0].bass[0].step[0] = (bstep_t){40, G_NOTE | BS_SLIDE, 100};
    patterns[0].bass[0].step[1] = (bstep_t){41, G_NOTE, 100};
    allow[40] = allow[41] = 1;
    seq_start(&seq);
    advance_samples(6003);
    CHECK(midi_on[40] == 1 && midi_on[41] == 1,
          "accepting both notes preserves existing external MIDI playback");
    CHECK(seq.bass_out_valid[0], "accepted second note is still tracked");
}
static void test_reject_then_clean_attack(void)
{
    reset();
    patterns[0].bass[0].step[0] = (bstep_t){40, G_NOTE, 100};
    patterns[0].bass[0].step[1] = (bstep_t){41, G_NOTE | BS_SLIDE, 100};
    patterns[0].bass[0].step[2] = (bstep_t){42, G_NOTE, 100};
    allow[40] = 1;
    allow[41] = 0;
    allow[42] = 1;
    seq_start(&seq);
    advance_samples(11500);
    CHECK(midi_on[40] == 1 && midi_on[41] == 0 && midi_on[42] == 1,
          "accepted 303 note after skipped slide is the only new MIDI voice");
    CHECK(received_slide[42] == 0,
          "accepted note after skipped slide starts with clean attack, not phantom glide");
}

static void test_live_rec_echo_survives(void)
{
    reset();
    patterns[0].bass[0].step[0] = (bstep_t){40, G_REST, 0};
    patterns[0].bass[0].step[1] = (bstep_t){41, G_NOTE, 108};
    allow[41] = 0;     /* must not consult probability: the key already played locally */
    seq_start(&seq);
    advance_samples(3);
    seq_rec_skip(&seq, TRK_BASS0, 1, 0);
    advance_samples(6000);
    CHECK(on_count == 0, "recorded live hit skips the internal replay callback");
    CHECK(midi_on[41] == 1, "recorded live 303 hit still echoes MIDI (REC-06)");
    CHECK(seq.bass_out_valid[0], "recorded live MIDI note remains tracked for its eventual NoteOff");
}

static void test_tie_after_rejection_is_silent(void)
{
    reset();
    patterns[0].bass[0].len = 4;
    patterns[0].bass[0].step[0] = (bstep_t){40, G_NOTE, 80};
    patterns[0].bass[0].step[1] = (bstep_t){45, G_TIE, 90};
    allow[40] = 0;
    allow[45] = 1;  /* acceptance of TIE must not revive a rejected NOTE */
    seq_start(&seq);
    advance_samples(6003);
    CHECK(midi_on[40] == 0 && midi_on[45] == 0,
          "a tie after a skipped 303 note cannot resurrect a MIDI voice");
    CHECK(on_count == 1, "orphan TIE never invokes a second onset callback");
    CHECK(!seq.t[TRK_BASS0].sounding, "orphan TIE preserves silent internal bass state");
    CHECK(!seq.bass_out_valid[0], "rejected note + tie leaves no stranded voice");
}


static void test_initial_tie_remains_silent(void)
{
    reset();
    patterns[0].bass[0].step[0] = (bstep_t){45, G_TIE, 100};
    allow[45] = 1;
    seq_start(&seq);
    advance_samples(3);
    CHECK(on_count == 0, "initial orphan TIE cannot create a bass onset");
    CHECK(midi_on[45] == 0, "initial orphan TIE cannot send MIDI NoteOn");
    CHECK(!seq.t[TRK_BASS0].sounding, "initial orphan TIE stays silent internally");
}
static void test_rest_tie_remains_silent(void)
{
    reset();
    patterns[0].bass[0].step[0] = (bstep_t){40, G_REST, 0};
    patterns[0].bass[0].step[1] = (bstep_t){45, G_TIE, 100};
    allow[45] = 1;
    seq_start(&seq);
    advance_samples(6003);
    CHECK(on_count == 0, "REST then TIE must not trigger any bass onset");
    CHECK(midi_on[45] == 0 && !seq.bass_out_valid[0],
          "REST then TIE must not claim a MIDI note");
}
static void test_valid_tie_sustains_only_previous_note(void)
{
    reset();
    patterns[0].bass[0].step[0] = (bstep_t){40, G_NOTE, 100};
    patterns[0].bass[0].step[1] = (bstep_t){45, G_TIE, 100};
    patterns[0].bass[0].step[2] = (bstep_t){42, G_REST, 0};
    allow[40] = allow[45] = 1;
    seq_start(&seq);
    advance_samples(6003);
    CHECK(on_count == 1, "valid NOTE-TIE has exactly one onset");
    CHECK(midi_on[40] == 1 && midi_on[45] == 0,
          "valid TIE sustains original MIDI pitch without retrigger");
    CHECK(seq.t[TRK_BASS0].sounding, "valid TIE keeps original 303 sounding");
    advance_samples(6000);
    CHECK(midi_off[40] == 1 && !seq.bass_out_valid[0],
          "REST after TIE releases held external note exactly once");
}
static void test_rejected_tie_then_ordinary_note(void)
{
    reset();
    patterns[0].bass[0].step[0] = (bstep_t){40, G_NOTE, 100};
    patterns[0].bass[0].step[1] = (bstep_t){45, G_TIE, 100};
    patterns[0].bass[0].step[2] = (bstep_t){42, G_NOTE, 100};
    allow[40] = 0;
    allow[45] = allow[42] = 1;
    seq_start(&seq);
    advance_samples(11500);
    CHECK(on_count == 2, "orphan TIE never invokes callback before later proper NOTE");
    CHECK(midi_on[45] == 0 && midi_on[42] == 1,
          "later accepted NOTE plays without resurrecting rejected TIE");
    CHECK(received_slide[42] == 0, "later accepted NOTE is a clean attack");
    CHECK(seq.t[TRK_BASS0].sounding, "later accepted NOTE remains audible");
}

int main(void)
{
    test_skipped_first_note();
    test_reject_after_audible_note();
    test_both_accepted();
    test_reject_then_clean_attack();
    test_live_rec_echo_survives();
    test_tie_after_rejection_is_silent();
    test_initial_tie_remains_silent();
    test_rest_tie_remains_silent();
    test_valid_tie_sustains_only_previous_note();
    test_rejected_tie_then_ordinary_note();
    if (fails) { printf("BF028: %d failed assertions\n", fails); return 1; }
    puts("BF028: 303 chance matches MIDI and local audio; good notes stay audible");
    return 0;
}
