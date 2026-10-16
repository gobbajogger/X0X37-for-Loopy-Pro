/* SPDX-License-Identifier: GPL-3.0-only */
/* MID-15: with CLOCK SOURCE AUTO or EXTERNAL, X0X follows the clock of one input (USB, Bluetooth, TRS).
 * The messages go through seq_clk_take, then seq_midi_rt, as engine.c midi_in does. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/sequencer.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("  FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
enum { USB = 0, BLE = 1, TRS = 2 };

static int s_drum(void *x, int k, int v, float vel) { (void)x; (void)k; (void)v; (void)vel; return 1; }
static void s_on(void *x, int p, int n, int acc, int sl) { (void)x; (void)p; (void)n; (void)acc; (void)sl; }
static void s_off(void *x, int p) { (void)x; (void)p; }
static void s_brk(void *x, int s16, int bar, float spb, int en) { (void)x; (void)s16; (void)bar; (void)spb; (void)en; }
static void s_brkstop(void *x) { (void)x; }
static void s_midi(void *x, uint8_t st, uint8_t d1, uint8_t d2) { (void)x; (void)st; (void)d1; (void)d2; }
static const seq_sink_t SINK = {s_drum, s_on, s_off, s_brk, s_brkstop, s_midi, 0, 0};
static seq_t S;
static pattern_t P[NPAT];
static uint32_t ticks;                         /* clocks that the sequencer took */

static void run(uint32_t samples)               /* blocks of 256, as the engine */
{
    while (samples) {
        uint32_t n = samples < 256u ? samples : 256u;
        seq_advance(&S, n, &SINK);
        samples -= n;
    }
}
static void run_ms(uint32_t ms) { run(ms * FS / 1000u); }
static void msg(uint8_t st, uint32_t src)
{
    if (!seq_clk_take(&S, st, src))
        return;
    if (st == 0xF8u && S.clk_src != SEQ_CLK_INT)
        ticks++;
    seq_midi_rt(&S, st);
}
static void reset(uint8_t src)
{
    int i;
    for (i = 0; i < NPAT; i++)
        pattern_init(&P[i], 0x100 + i, 0x200 + i);
    seq_init(&S, P);
    S.send_clock = 0;
    S.clk_src = src;
    ticks = 0;
    run(256);
}
/* both inputs send clock at bpm (USB and Bluetooth offset by half a tick) for secs */
static void two_clocks(double bpm, double secs, int usb_on, int ble_on)
{
    double tick = 44100.0 * 60.0 / bpm / 24.0, tu = 0.0, tb = tick / 2.0, t = 0.0, end = secs * 44100.0;
    while (t < end) {
        while (tu <= t) { if (usb_on) msg(0xF8, USB); tu += tick; }
        while (tb <= t) { if (ble_on) msg(0xF8, BLE); tb += tick; }
        run(256);
        t += 256.0;
    }
}

int main(void)
{
    int m;
    for (m = 0; m < 2; m++) {
        uint8_t src = m ? SEQ_CLK_EXT : SEQ_CLK_AUTO;
        const char *nm = m ? "EXTERNAL" : "AUTO";

        reset(src);                             /* two inputs interleaved: the tempo of one input */
        msg(0xFA, USB);
        two_clocks(120.0, 8.0, 1, 1);
        CHECK(S.playing && S.ext, "%s: playing on the clock", nm);
        CHECK(S.ext_bpm > 119.5f && S.ext_bpm < 120.5f, "%s: two inputs at 120 BPM: measured %.2f (not 240)", nm, (double)S.ext_bpm);
        CHECK(S.clk_lock == USB + 1, "%s: USB has the clock (%u)", nm, S.clk_lock);
        CHECK(ticks > 380 && ticks < 390, "%s: 8 s at 120 BPM: %u clocks (384), not two times", nm, ticks);

        msg(0xFA, BLE);                         /* the other input: Start, Stop, Continue are ignored */
        msg(0xFC, BLE);
        run(512);
        CHECK(S.playing, "%s: Bluetooth Stop while USB has the clock: still playing", nm);
        msg(0xFB, BLE);
        CHECK(S.clk_lock == USB + 1, "%s: Bluetooth Continue: USB keeps the clock", nm);

        two_clocks(120.0, 0.3, 0, 1);           /* USB stops (disconnect): Bluetooth waits for 0.4 s */
        CHECK(S.clk_lock == USB + 1 && S.playing, "%s: 0.3 s with no USB clock: USB keeps the clock", nm);
        two_clocks(120.0, 5.0, 0, 1);           /* then Bluetooth gets it (the beat ring holds 8 beats) */
        CHECK(S.clk_lock == BLE + 1, "%s: Bluetooth gets the clock after 0.4 s (%u)", nm, S.clk_lock);
        CHECK(S.playing && S.ext && !seq_clock_waiting(&S), "%s: no stop, no pause at the change", nm);
        CHECK(S.ext_bpm > 119.0f && S.ext_bpm < 121.0f, "%s: tempo after the change %.2f", nm, (double)S.ext_bpm);

        msg(0xFC, BLE);                         /* the input with the clock stops; the other Start waits */
        run(512);
        CHECK(!S.playing, "%s: Bluetooth Stop: stopped", nm);
        run_ms(200);
        msg(0xFA, USB);
        run(512);
        CHECK(!S.playing && S.clk_lock == BLE + 1, "%s: USB Start 0.2 s after the Stop: ignored", nm);
        two_clocks(120.0, 0.5, 0, 1);           /* Bluetooth sends clock while stopped (as Loopy can) */
        msg(0xF8, USB);
        CHECK(S.clk_lock == BLE + 1, "%s: USB clock while the stopped Bluetooth input sends clock: ignored", nm);
        msg(0xFA, USB);                         /* 0.4 s after the Stop: a USB Start gets the clock */
        run(512);
        CHECK(S.playing && S.clk_lock == USB + 1, "%s: USB Start 0.7 s after the Bluetooth Stop: plays (%u)", nm, S.clk_lock);
        msg(0xFC, BLE);
        run(512);
        CHECK(S.playing, "%s: Bluetooth Stop after the change: ignored", nm);

        reset(src);                             /* the Stop, then a Start of the same input at once */
        msg(0xFA, BLE);
        two_clocks(120.0, 1.0, 0, 1);
        msg(0xFC, BLE);
        msg(0xFA, BLE);
        run(512);
        CHECK(S.playing && S.clk_lock == BLE + 1, "%s: Stop, then Start of the input with the clock: plays", nm);

        reset(src);                             /* the same block: the first message wins, each time */
        msg(0xF8, BLE);
        msg(0xF8, USB);
        msg(0xFA, USB);
        CHECK(S.clk_lock == BLE + 1 && !S.req_start, "%s: same block, Bluetooth first: Bluetooth wins", nm);
        reset(src);
        msg(0xF8, USB);
        msg(0xF8, BLE);
        CHECK(S.clk_lock == USB + 1, "%s: same block, USB first: USB wins", nm);
        reset(src);
        msg(0xF8, TRS);
        msg(0xF8, USB);
        CHECK(S.clk_lock == TRS + 1 && ticks == 1, "%s: TRS first: TRS wins", nm);

        reset(src);                             /* a Stop gets no clock; no clock at all: no input has it */
        msg(0xFC, BLE);
        CHECK(S.clk_lock == 0, "%s: a Stop alone: no input has the clock", nm);
        msg(0xF8, USB);
        CHECK(S.clk_lock == USB + 1, "%s: then a USB clock: USB has it", nm);
        run_ms(410);
        CHECK(S.clk_lock == 0, "%s: 0.41 s with nothing: no input has the clock", nm);
    }

    reset(SEQ_CLK_INT);                         /* INTERNAL ignores the clock of both inputs */
    msg(0xFA, USB);
    msg(0xFA, BLE);
    two_clocks(120.0, 2.0, 1, 1);
    CHECK(!S.playing && !S.ext && S.clk_lock == 0 && S.ext_pending == 0, "INTERNAL: both inputs ignored");

    reset(SEQ_CLK_AUTO);                        /* a mode change: INTERNAL clears the lock, AUTO starts again */
    msg(0xFA, USB);
    two_clocks(120.0, 1.0, 1, 0);
    S.clk_src = SEQ_CLK_INT;
    run(512);
    msg(0xF8, USB);
    S.clk_src = SEQ_CLK_AUTO;
    run(512);
    msg(0xF8, BLE);
    CHECK(S.clk_lock == BLE + 1, "AUTO after INTERNAL: the first input gets the clock (%u)", S.clk_lock);

    printf("%s clock lock (MID-15): %d fail\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
