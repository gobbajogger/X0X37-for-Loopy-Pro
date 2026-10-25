/* SPDX-License-Identifier: GPL-3.0-only */
/* BF-005: Queued USB clocks must not advance a new Bluetooth transport owner.
 * Host regression: owner-matching seq_clk_drop clears ext_pending with clk_lock. */
#include <stdio.h>
#include "../../firmware/src/seq/sequencer.h"

static int drum(void *x, int k, int v, float vel) { (void)x; (void)k; (void)v; (void)vel; return 1; }
static void bass_on(void *x, int p, int n, int a, int slide, int vel) { (void)x; (void)p; (void)n; (void)a; (void)slide; (void)vel; }
static void bass_off(void *x, int p) { (void)x; (void)p; }
static void brk(void *x, int p, int b, float n, int en) { (void)x; (void)p; (void)b; (void)n; (void)en; }
static void brk_stop(void *x) { (void)x; }
static void midi_out(void *x, unsigned char s, unsigned char n, unsigned char v) { (void)x; (void)s; (void)n; (void)v; }
static void stepped(void *x, int t, int p) { (void)p; if (t == 0) ++*(int *)x; }
static void feed(seq_t *s, unsigned char msg, unsigned src) {
    if (seq_clk_take(s, msg, src)) seq_midi_rt(s, msg);
}
static int trial(int old_count) {
    pattern_t patterns[NPAT];
    seq_t seq;
    int steps = 0;
    const seq_sink_t sink = {drum, bass_on, bass_off, brk, brk_stop, midi_out, stepped, &steps};
    for (int i = 0; i < NPAT; i++) pattern_init(&patterns[i], (unsigned)(i + 1), (unsigned)(i + 11));
    seq_init(&seq, patterns);
    seq.clk_src = SEQ_CLK_EXT;
    seq.send_clock = 0;
    seq_advance(&seq, 0, &sink);
    for (int i = 0; i < old_count; i++) feed(&seq, 0xF8, 0); /* USB queued */
    seq_clk_drop(&seq, 0);                                /* USB disconnect */
    feed(&seq, 0xFA, 1);                                   /* Bluetooth Start */
    for (int fresh_tick = 1; fresh_tick <= 7; fresh_tick++) {
        feed(&seq, 0xF8, 1);
        seq_advance(&seq, 1, &sink);
        int expected_steps = fresh_tick < 7 ? 1 : 2;
        if (steps != expected_steps) {
            printf("FAIL old_pending=%d fresh_ble_tick=%d expected_909_steps=%d observed=%d ext_ticks=%u\n",
                   old_count, fresh_tick, expected_steps, steps, seq.t[0].ext_ticks);
            return 1;
        }
    }
    printf("PASS old_pending=%d: step 2 followed the 7th fresh Bluetooth clock\n", old_count);
    return 0;
}
int main(void) { int control = trial(0); int regression = trial(3); return control || regression; }
