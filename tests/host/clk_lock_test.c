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
static int s_on(void *x, int p, int n, int acc, int sl, int velocity) { (void)x; (void)p; (void)n; (void)acc; (void)sl; (void)velocity; return 1; }
static void s_off(void *x, int p) { (void)x; (void)p; }
static void s_brk(void *x, int s16, int bar, float spb, int en) { (void)x; (void)s16; (void)bar; (void)spb; (void)en; }
static void s_brkstop(void *x) { (void)x; }
static void s_midi(void *x, uint8_t st, uint8_t d1, uint8_t d2) { (void)x; (void)st; (void)d1; (void)d2; }
static const seq_sink_t SINK = {s_drum, s_on, s_off, s_brk, s_brkstop, s_midi, 0, 0};
static seq_t S;
static pattern_t P[NPAT];
static uint32_t ticks;                         /* clocks that the sequencer took */

static double g_t, g_tu, g_tb;                   /* samples run, the next USB and Bluetooth tick (two_clocks) */
static void run(uint32_t samples)               /* blocks of 256, as the engine */
{
    g_t += samples;
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
    g_t = g_tu = 0.0;
    g_tb = 44100.0 * 60.0 / 120.0 / 24.0 / 2.0;  /* Bluetooth half a tick after USB */
}
/* both inputs send clock at bpm (USB and Bluetooth offset by half a tick) for secs. The tick times
 * continue from the last call (reset() starts them again), so short calls give the right tempo. */
static void two_clocks(double bpm, double secs, int usb_on, int ble_on)
{
    double tick = 44100.0 * 60.0 / bpm / 24.0, end = g_t + secs * 44100.0;
    if (g_tu < g_t - tick)                      /* (a time with no clock: no burst of late ticks) */
        g_tu = g_t;
    if (g_tb < g_t - tick)
        g_tb = g_t + tick / 2.0;
    while (g_t < end) {
        while (g_tu <= g_t) { if (usb_on) msg(0xF8, USB); g_tu += tick; }
        while (g_tb <= g_t) { if (ble_on) msg(0xF8, BLE); g_tb += tick; }
        run(256);
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

    for (m = 0; m < 2; m++) {                   /* a change of input restarts the tempo measure: no dip */
        uint8_t src = m ? SEQ_CLK_EXT : SEQ_CLK_AUTO;
        const char *nm = m ? "EXTERNAL" : "AUTO";
        int k;
        float lo = 999.0f, hi = 0.0f;
        reset(src);
        msg(0xFA, USB);
        two_clocks(120.0, 6.0, 1, 1);
        two_clocks(120.0, 0.45, 0, 1);          /* USB ends: after 0.4 s Bluetooth gets the clock */
        CHECK(S.clk_lock == BLE + 1, "%s: Bluetooth has the clock after the change", nm);
        two_clocks(120.0, 1.0, 0, 1);           /* 2 beats at 120 BPM */
        for (k = 0; k < 40; k++) {              /* then 4 s: the tempo stays in 119-121 */
            two_clocks(120.0, 0.1, 0, 1);
            lo = S.ext_bpm < lo ? S.ext_bpm : lo;
            hi = S.ext_bpm > hi ? S.ext_bpm : hi;
        }
        CHECK(lo >= 119.0f && hi <= 121.0f, "%s: tempo after the change %.2f-%.2f (want 120 +-1 after 2 beats)", nm, (double)lo, (double)hi);

        reset(src);                             /* the clock stops just after a change: 120 BPM stays */
        msg(0xFA, USB);
        two_clocks(120.0, 6.0, 1, 1);
        two_clocks(120.0, 0.45, 0, 1);
        two_clocks(120.0, 0.1, 0, 1);
        CHECK(S.ext_bpm > 119.0f && S.ext_bpm < 121.0f, "%s: tempo just after the change %.2f", nm, (double)S.ext_bpm);
        run(44100);                             /* no clock for 1 s */
        if (m)
            CHECK(seq_clock_waiting(&S) && S.ext_bpm > 119.5f && S.ext_bpm < 120.5f, "EXTERNAL: waits, tempo %.2f", (double)S.ext_bpm);
        else
            CHECK(!S.ext && S.bpm == 120.0f, "AUTO: internal tempo %.2f after the stop (want 120)", (double)S.bpm);
    }

    for (m = 0; m < 4; m++) {                   /* the link ends at 97 BPM: the tempo of the clock stays, not 120 */
        uint8_t src = (m & 1) ? SEQ_CLK_EXT : SEQ_CLK_AUTO;
        uint32_t in = (m & 2) ? USB : BLE;
        const char *nm = (m & 1) ? "EXTERNAL" : "AUTO", *inm = (m & 2) ? "USB" : "Bluetooth";
        reset(src);                             /* internal tempo 120 (the default) */
        S.bpm = 120.0f;                         /* internal tempo 120, not the clock tempo */
        msg(0xFA, in);
        two_clocks(97.0, 6.0, in == USB, in == BLE);
        seq_clk_drop(&S, in);                   /* the link of that input ends, no other input */
        CHECK(S.clk_lock == 0, "%s %s: link end frees the clock", nm, inm);
        run(44100);                             /* 1 s */
        if (m & 1)
            CHECK(seq_clock_waiting(&S) && S.ext_bpm >= 96.5f && S.ext_bpm <= 97.5f, "EXTERNAL %s: waits, tempo %.2f (want 97)", inm, (double)S.ext_bpm);
        else
            CHECK(!S.ext && S.bpm == 97.0f, "AUTO %s: internal tempo %.2f after the link end (want 97)", inm, (double)S.bpm);
    }
    for (m = 0; m < 2; m++) {                   /* Bluetooth to USB at 97 BPM, then the clock stops: 97 stays */
        uint8_t src = m ? SEQ_CLK_EXT : SEQ_CLK_AUTO;
        const char *nm = m ? "EXTERNAL" : "AUTO";
        reset(src);
        msg(0xFA, BLE);
        two_clocks(97.0, 6.0, 0, 1);
        two_clocks(97.0, 0.45, 1, 0);           /* Bluetooth stops: after 0.4 s USB gets the clock */
        CHECK(S.clk_lock == USB + 1, "%s: USB has the clock after the change (%u)", nm, S.clk_lock);
        two_clocks(97.0, 3.0, 1, 0);            /* about 5 beats: after 2 beats only, the measure is 97.5 (Q1 +-1) */
        run(44100);                             /* no clock for 1 s */
        if (m)
            CHECK(seq_clock_waiting(&S) && S.ext_bpm >= 96.5f && S.ext_bpm <= 97.5f, "EXTERNAL: waits after the change, tempo %.2f (want 97)", (double)S.ext_bpm);
        else
            CHECK(!S.ext && S.bpm == 97.0f, "AUTO: internal tempo %.2f after the change and stop (want 97)", (double)S.bpm);
    }

    for (m = 0; m < 2; m++) {                   /* the link of the input with the clock ends: at once free */
        uint8_t src = m ? SEQ_CLK_EXT : SEQ_CLK_AUTO;
        const char *nm = m ? "EXTERNAL" : "AUTO";
        float lo = 999.0f, hi = 0.0f;
        int k;
        reset(src);
        msg(0xFA, BLE);
        two_clocks(120.0, 6.0, 1, 1);
        CHECK(S.clk_lock == BLE + 1, "%s: Bluetooth has the clock", nm);
        seq_clk_drop(&S, USB);                  /* another input's link ends: no change */
        CHECK(S.clk_lock == BLE + 1, "%s: USB link end: Bluetooth keeps the clock", nm);
        seq_clk_drop(&S, BLE);                  /* (engine.c midi_in at PLAT_MIDI_LINK_END(1)) */
        CHECK(S.clk_lock == 0, "%s: Bluetooth link end: no input has the clock", nm);
        two_clocks(120.0, 0.03, 1, 0);          /* the next USB tick gets it, no 0.4 s wait */
        CHECK(S.clk_lock == USB + 1 && S.playing && S.ext, "%s: USB gets the clock at once (%u)", nm, S.clk_lock);
        for (k = 0; k < 30; k++) {              /* no dip at the change (the measure starts again) */
            two_clocks(120.0, 0.1, 1, 0);
            if (k >= 10) {
                lo = S.ext_bpm < lo ? S.ext_bpm : lo;
                hi = S.ext_bpm > hi ? S.ext_bpm : hi;
            }
        }
        CHECK(lo >= 119.0f && hi <= 121.0f && S.playing && !seq_clock_waiting(&S), "%s: after the link end %.2f-%.2f", nm, (double)lo, (double)hi);
        msg(0xFC, USB);                         /* the USB Stop acts: USB has the clock */
        run(512);
        CHECK(!S.playing, "%s: USB Stop after the change: stopped", nm);
        reset(src);                             /* USB unplug while USB has the clock */
        msg(0xF8, USB);
        seq_clk_drop(&S, USB);
        msg(0xFA, BLE);
        run(512);
        CHECK(S.clk_lock == BLE + 1 && S.playing, "%s: USB unplug: a Bluetooth Start acts at once", nm);
    }

    for (m = 0; m < 2; m++) {                   /* Song Position (F2) follows the input with the clock */
        uint8_t src = m ? SEQ_CLK_EXT : SEQ_CLK_AUTO;
        const char *nm = m ? "EXTERNAL" : "AUTO";
        reset(src);
        CHECK(seq_clk_take(&S, 0xF2, BLE) == 1 && S.clk_lock == 0, "%s: F2 with no clock input: passes, gets no clock", nm);
        msg(0xF8, USB);
        CHECK(seq_clk_take(&S, 0xF2, BLE) == 0, "%s: F2 from Bluetooth while USB has the clock: ignored", nm);
        CHECK(seq_clk_take(&S, 0xF2, USB) == 1 && S.clk_lock == USB + 1, "%s: F2 from USB: passes", nm);
        run_ms(300);
        CHECK(seq_clk_take(&S, 0xF2, USB) == 1, "%s: F2 from USB after 0.3 s: passes", nm);
        run_ms(150);                            /* F2 does not keep the clock: 0.45 s with no Clock */
        CHECK(S.clk_lock == 0 && seq_clk_take(&S, 0xF2, BLE) == 1, "%s: F2 does not keep the clock alive", nm);
        msg(0xF8, BLE);
        msg(0xFC, BLE);                         /* Bluetooth stops; after 0.4 s USB F2 is still ignored */
        run_ms(450);
        msg(0xF8, BLE);
        CHECK(seq_clk_take(&S, 0xF2, USB) == 0, "%s: F2 from USB after the Bluetooth Stop: ignored (only Start or Continue takes over)", nm);
        seq_clk_drop(&S, BLE);
        CHECK(seq_clk_take(&S, 0xF2, USB) == 1, "%s: F2 from USB after the Bluetooth link end: passes", nm);
    }
    reset(SEQ_CLK_INT);
    msg(0xF8, USB);
    CHECK(seq_clk_take(&S, 0xF2, BLE) == 1 && seq_clk_take(&S, 0xF2, USB) == 1, "INTERNAL: F2 passes from all inputs, as before");

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

    /* A stopped input may keep sending clocks after Stop. At the grace expiry a
     * second input can take over within the same audio block as an old burst.
     * Those queued old-owner ticks cannot start or advance the new transport. */
    for (m = 0; m < 2; m++) {
        uint8_t src = m ? SEQ_CLK_EXT : SEQ_CLK_AUTO;
        const char *nm = m ? "EXTERNAL" : "AUTO";
        for (int reverse = 0; reverse < 2; reverse++) {
            unsigned old_src = reverse ? BLE : USB;
            unsigned new_src = reverse ? USB : BLE;
            for (int new_tick = 0; new_tick <= 1; new_tick++) {
                reset(src);
                msg(0xFA, old_src);
                msg(0xF8, old_src);
                run(256);                           /* old input starts step 1 */
                msg(0xFC, old_src);
                run(256);                           /* old input stops */
                for (int i = 0; i < 9; i++) {       /* clocks continue after Stop: 450ms */
                    msg(0xF8, old_src);
                    run_ms(50);
                }
                CHECK(!S.playing && S.clk_stop && S.clk_lock == old_src + 1,
                      "%s old=%u: stopped input still owns the clock", nm, old_src);
                for (int i = 0; i < 6; i++)
                    msg(0xF8, old_src);             /* six old-owner ticks queued in this block */
                CHECK(S.ext_pending == 6, "%s old=%u: six old ticks pending (%u)", nm, old_src, S.ext_pending);
                msg(0xFA, new_src);                 /* grace elapsed: new source takes over */
                CHECK(S.clk_lock == new_src + 1, "%s old=%u: new Start gets the lock", nm, old_src);
                if (new_tick)
                    msg(0xF8, new_src);             /* first real new-owner downbeat */
                run(256);
                CHECK(S.playing && S.t[TRK_DRUM].started == (unsigned)new_tick &&
                          S.t[TRK_DRUM].pos == 0,
                      "%s old=%u new_tick=%d: stale clocks advanced new transport (started=%u pos=%u)",
                      nm, old_src, new_tick, S.t[TRK_DRUM].started, S.t[TRK_DRUM].pos);
            }
        }
    }

    printf("%s clock lock (MID-15): %d fail\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
