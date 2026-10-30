/* SPDX-License-Identifier: GPL-3.0-only */
/* engine.c loop_bars: the bar-count guess of a user loop uses the loop's own rate.
 * run_loop_bars.sh cuts loop_bars out of firmware/src/app/engine.c (the real function, not a
 * copy) into LOOP_BARS_SRC and includes it here.
 *   loop_bars_test */
#include <stdint.h>
#include <stdio.h>
/* engine.c may mark loop_bars X0X_COLD; host extract has no x0x_cold.h — ignore the attr. */
#ifndef X0X_COLD
#define X0X_COLD
#endif
#include LOOP_BARS_SRC

static int bad;
static void check(uint32_t ns, uint32_t rate, int want, const char *what)
{
    int got = loop_bars(ns, rate);
    printf("  %s %-46s %7u samples at %5u Hz: %d bar(s)\n", got == want ? "ok  " : "FAIL", what,
           (unsigned)ns, (unsigned)rate, got);
    bad += got != want;
}

int main(void)
{
    /* one bar at 110 BPM is 2.18 s; two bars at 96 BPM (the built-in loops) are 5 s */
    check(96218u, 44100u, 1, "one bar at 110 BPM, 44.1 kHz");
    check(48109u, 22050u, 1, "one bar at 110 BPM, 22.05 kHz");
    check(220500u, 44100u, 2, "two bars at 96 BPM, 44.1 kHz");
    check(110250u, 22050u, 2, "two bars at 96 BPM, 22.05 kHz");
    check(352800u, 44100u, 4, "four bars at 120 BPM, 44.1 kHz");
    /* the guard: had the rate been ignored (22.05 kHz assumed), the 44.1 kHz bar would read as 2 bars */
    check(96218u, 22050u, 2, "the same samples read as 22.05 kHz");
    check(48109u, 0u, 1, "rate 0: 22.05 kHz assumed (old headers)");
    puts(bad ? "LOOP BARS FAILED" : "loop bars: the guess follows the loop's rate");
    return bad != 0;
}
