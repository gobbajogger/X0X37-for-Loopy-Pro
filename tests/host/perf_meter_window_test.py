#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile the actual UI perf_window, proving its shared 24 MHz denominator.

Fake C0 clock mode changes must not change C0+C1 stage percentages.
Source-excerpt host test only; it cannot qualify native TIMER4 timing.
"""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
UI = (ROOT / "firmware/src/app/ui.c").read_text()
HDR = (ROOT / "firmware/src/app/engine.h").read_text()
begin = "static void perf_window(int force)\n{"
end = "/* the test's patterns:"
assert UI.count(begin) == 1
a = UI.index(begin)
b = UI.index(end, a)
window = UI[a:b]
assert "#define ENG_PROF_TICKS_HZ 24000000u" in HDR
assert "ENG_PROF_TICKS_HZ / 44100.0f" in window
assert "plat_cycles_hz()" not in window
assert "plat_cycles_cpu() ? (float)perf.mhz10" not in window

header = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
enum { PR_909, PR_808, PR_303A, PR_303B, PR_BRK, PR_FX, PR_MST, PR_SEQ, ENG_PROF_N };
#define ENG_PROF_TICKS_HZ 24000000u
static volatile uint32_t eng_prof[ENG_PROF_N], eng_prof_frames;
static uint32_t t24, c0_cycles, mock_ms, p_cpu, p_peak;
static int c0_counter_ok;
static uint32_t plat_ms(void) { return mock_ms; }
static uint32_t plat_ticks24(void) { return t24; }
static uint32_t plat_cycles(void) { return c0_cycles; }
static int plat_cycles_cpu(void) { return c0_counter_ok; }
static uint32_t plat_cpu_pct(void) { return p_cpu; }
static uint32_t plat_cpu_peak_pct(void) { return p_peak; }
static int plat_stalls(uint32_t s[3]) { (void)s; return -1; }
static struct {
    uint32_t t_ms, ticks, cyc, frames, prof[ENG_PROF_N], st[3];
    uint16_t mhz10;
    uint8_t load, peak, stalls_on, have;
    uint16_t stage[ENG_PROF_N], cps;
    uint8_t stall[3];
} perf;
'''
footer = r'''
static void reset(int c0_ok) {
    memset(&perf, 0, sizeof perf);
    memset((void *)eng_prof, 0, sizeof eng_prof);
    eng_prof_frames = 0;
    t24 = c0_cycles = mock_ms = p_cpu = p_peak = 0;
    c0_counter_ok = c0_ok;
    perf_window(1); /* establish one synchronized sample window */
}
static void step(uint32_t frames, uint32_t ticks808, uint32_t ticksfx, uint32_t c0delta) {
    mock_ms += 1000;
    t24 += ENG_PROF_TICKS_HZ;
    c0_cycles += c0delta; /* unrelated C0 PMU vs TIMER4; may be 0 */
    eng_prof_frames += frames;
    eng_prof[PR_808] += ticks808;
    eng_prof[PR_FX] += ticksfx;
    perf_window(1);
}
int main(void) {
    /* 50% 808 plus 25% FX at 44.1 kHz, while C0 CKCNTL runs at 360 MHz. */
    reset(1);
    step(44100, 12000000u, 6000000u, 360000000u);
    assert(perf.mhz10 == 3600u);
    assert(perf.stage[PR_808] == 500u && perf.stage[PR_FX] == 250u);
    assert(perf.cps == 408u && perf.stage[PR_BRK] == 0u);

    /* CPU counter disabled/frozen: the same shared 24 MHz timer denominator. */
    reset(0);
    step(44100, 12000000u, 6000000u, 0u);
    assert(perf.stage[PR_808] == 500u && perf.stage[PR_FX] == 250u);
    assert(perf.cps == 408u && perf.mhz10 == 0u);

    /* Counter can change independently between display windows without stage drift. */
    reset(1);
    step(44100, 12000000u, 6000000u, 360000000u);
    c0_counter_ok = 0;
    step(44100, 12000000u, 6000000u, 0u);
    assert(perf.stage[PR_808] == 500u && perf.stage[PR_FX] == 250u);

    /* Two cores each spend 120% of a one-core budget inside overlapping
     * measured spans. Total >100% is intentional, not a 100% utilization claim. */
    reset(1);
    step(44100, 28800000u, 28800000u, 360000000u);
    assert(perf.stage[PR_808] == 1200u && perf.stage[PR_FX] == 1200u);

    /* Guard finite uint16 readouts when counters spike or a timing window fails. */
    reset(0);
    step(44100, 0xffffffffu, 0u, 0u);
    assert(perf.stage[PR_808] == 65535u && perf.cps > 0u);

    /* 32-bit shared stage counter wrapping during a regular one-second window. */
    reset(0);
    eng_prof[PR_808] = 0xfffffff0u;
    perf_window(1); /* resnapshot without advancing frames */
    step(44100, 12000000u, 0u, 0u);
    assert(perf.stage[PR_808] == 500u);

    puts("PASS: real UI perf_window compiled; C0 CKCNTL/fallback independent; 24MHz denominator; overlap; saturation; wrap");
    puts("LIMIT: not a native clock or audio timing qualification");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="dnbox-perf-window-") as tmp:
    test_c = Path(tmp) / "test.c"
    exe = Path(tmp) / "test"
    test_c.write_text(header + window + footer)
    subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall",
                    "-Wextra", "-Werror", str(test_c), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
