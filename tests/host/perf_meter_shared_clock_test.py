#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile live PERF timing / C1 FX / fold slices under independent fake clocks.

This tests clock selection, attribution, overlap, wrap and fallback logic only.
A fake TIMER4 is not proof of FM1 hardware elapsed time or native C1 throughput.
No private SDK, original synth, firmware image or device is used.
"""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
ENG = (ROOT / "firmware/src/app/engine.c").read_text()
HDR = (ROOT / "firmware/src/app/engine.h").read_text()
UI = (ROOT / "firmware/src/app/ui.c").read_text()
HOST = (ROOT / "host/x0x_host.c").read_text()
HAL_TIME = (ROOT / "firmware/hal/fm1_time.h").read_text()
HAL_C1 = (ROOT / "firmware/hal/fm1_cpu1.h").read_text()
PLATFORM = (ROOT / "firmware/src/app/plat_fm1.c").read_text()
HAL_PERF = (ROOT / "firmware/hal/fm1_perf.h").read_text()


def between(source, begin, end):
    assert source.count(begin) == 1, f"missing/nonunique start: {begin!r}"
    a = source.index(begin)
    b = source.find(end, a + len(begin))
    assert b >= a, f"missing stop after {begin!r}"
    return source[a:b]


# A 24 MHz TIMER4 peripheral, not a core-local PMU; both cores use 0x10804.
assert "#define FM1_T4_CNT (*(volatile uint32_t *)0x10804u)" in HAL_TIME
assert "#define FM1_C1_T4 (*(volatile uint32_t *)0x10804u)" in HAL_C1
assert "return fm1_ticks();" in PLATFORM
assert "uint32_t plat_ticks24(void) { return fm1_ticks(); }" in PLATFORM
assert "FM1_C0_TL_CKCNTL" in HAL_PERF and "return FM1_C0_TL_CKCNTL;" in HAL_PERF
assert "#define ENG_PROF_TICKS_HZ 24000000u" in HDR
assert "const float ticks_per_sample = (float)ENG_PROF_TICKS_HZ / 44100.0f;" in UI
assert "perf.cps = sum / df > 65535u ? 65535u : (uint16_t)(sum / df);" in UI
assert '"STAGE ELAPSED %  TICKS / SAMPLE"' in UI
assert "ENG_PROF_TICKS_HZ" in HOST and "1e9 / (double)ENG_PROF_TICKS_HZ" in HOST

prof = between(ENG, "#define PROF_T(v)", "/* ---------------------------------------------------------------- state --- */")
assert "plat_ticks24()" in prof and "plat_cycles()" not in prof, "C0 cycles must not time C1"
assert prof.count("plat_ticks24()") == 3, "PROF_T, PROF_ADD, PROF_ADD_CORE"
fx = between(ENG, "static void core2_fx(uint32_t arg)\n{", "static void tape_after_fx(void)")
assert "PROF_ADD_CORE(1, PR_FX, px);" in fx
assert "plat_cycles()" not in fx
fold = between(ENG, "static void prof_c1_fold(int keep)\n{", "#ifdef X0X_HOST")
assert "if (keep)" in fold and "eng_prof_c1[i] = 0;" in fold
assert "eng_prof_wait += plat_ticks24() - pt;" in ENG

# Compile the production macros and complete C1-FX / fold implementations.
# Two fake clocks deliberately disagree: the C0 CKCNTL register can run fast,
# freeze, or be absent while shared TIMER4 advances independently.
source = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
enum { PR_909, PR_808, PR_303A, PR_303B, PR_BRK, PR_FX, PR_MST, PR_SEQ, ENG_PROF_N };
#define ENG_PROF_TICKS_HZ 24000000u
#define ENG_TAIL_CS 32u
static volatile uint32_t eng_prof[ENG_PROF_N], eng_prof_c1[ENG_PROF_N];
static uint32_t wall_ticks, c0_ckcntl;
static unsigned c0_reads, c1_fx_calls, c0_busy, perf_cycles_available;
static uint32_t plat_ticks24(void) { return wall_ticks; }
static uint32_t plat_cycles(void) {
    ++c0_reads;
    return perf_cycles_available ? c0_ckcntl : wall_ticks;
}
#define eng_sync() ((void)0)
static uint32_t eng_fx_n;
static uint32_t eng_fx_done;
static float buffer_l[256], buffer_r[256], dry_l[256], dry_r[256], rev[256], dly[256];
static float *eng_fx_ol = buffer_l, *eng_fx_or = buffer_r;
static struct { unsigned unused; } fx;
static void fxbus_process_st(void *ignored, const float *a, const float *b,
     const float *c, const float *d, float *e, float *f, int n) {
    (void)ignored; (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    ++c1_fx_calls;
    wall_ticks += 3u * (uint32_t)n;  /* 24 MHz elapsed ticks */
    if (c0_busy) c0_ckcntl += 900u * (uint32_t)n; /* C0 is concurrently busy */
}
'''
source += prof + "\n" + fold + "\n" + fx
source += r'''
static void reset(void) {
    memset((void *)eng_prof, 0, sizeof eng_prof);
    memset((void *)eng_prof_c1, 0, sizeof eng_prof_c1);
    wall_ticks = c0_ckcntl = c0_reads = c1_fx_calls = c0_busy = 0;
    perf_cycles_available = 1;
    eng_fx_done = 0; eng_fx_n = 0;
}
static void c1_fx(unsigned n) {
    eng_fx_n = n;
    core2_fx(0);
    assert(eng_fx_done == n);
}
static uint32_t pct10(uint32_t stage_ticks, uint32_t frames) {
    return (uint32_t)((float)stage_ticks * 1000.0f /
                      ((float)frames * ((float)ENG_PROF_TICKS_HZ / 44100.0f)) + 0.5f);
}
int main(void) {
    /* The old C0 counter would have credited tens of thousands of C1 ticks.
     * Real C1 elapsed 80*3=240 ticks while C0 executed concurrently. */
    reset(); c0_busy = 1;
    c1_fx(80);
    assert(c1_fx_calls == 3 && c0_ckcntl == 72000u);
    assert(eng_prof_c1[PR_FX] == 240u && eng_prof[PR_FX] == 0);
    assert(c0_reads == 0); /* stage macros never touch core-0 CKCNTL */
    prof_c1_fold(1);
    assert(eng_prof[PR_FX] == 240u && eng_prof_c1[PR_FX] == 0);
    prof_c1_fold(1);
    assert(eng_prof[PR_FX] == 240u); /* no double fold */

    /* C0 cycle counter frozen: C1 still accumulates its elapsed TIMER4 time. */
    reset(); c0_busy = 0; c0_ckcntl = 123456;
    c1_fx(32);
    assert(c0_ckcntl == 123456u && eng_prof_c1[PR_FX] == 96u);

    /* C0 CKCNTL unusable: platform would fall back to TIMER4 for plat_cycles.
     * The PERF stage must have exactly the same 24MHz unit and denominator. */
    reset(); perf_cycles_available = 0;
    (void)plat_cycles(); /* exercise the simulated fallback, not the stage */
    c0_reads = 0;
    c1_fx(32);
    assert(eng_prof_c1[PR_FX] == 96u && c0_reads == 0);
    assert(pct10(eng_prof_c1[PR_FX], 32) == pct10(96u, 32));

    /* Unsigned modulo deltas survive the ~179s TIMER4 rollover. */
    reset(); wall_ticks = 0xfffffff0u;
    c1_fx(32);
    assert(eng_prof_c1[PR_FX] == 96u && wall_ticks == 80u);

    /* C1 successfully finished 32 frames; C0 finishes remaining 48 after
     * C1 failure. Fold completed C1 credit once and count only C0's work. */
    reset(); c1_fx(32);
    PROF_T(start_fallback);
    fxbus_process_st(&fx, dry_l, dry_r, rev, dly, buffer_l, buffer_r, 48);
    PROF_ADD(PR_FX, start_fallback);
    prof_c1_fold(1);
    assert(eng_prof[PR_FX] == 240u && eng_prof_c1[PR_FX] == 0);
    prof_c1_fold(1);
    assert(eng_prof[PR_FX] == 240u);

    /* C1 failure while a job was incomplete: discard suspect private credit
     * before C0 replays it; do not double-bill the replayed work. */
    reset(); c1_fx(32);
    prof_c1_fold(0);
    assert(eng_prof[PR_FX] == 0u && eng_prof_c1[PR_FX] == 0);
    PROF_T(start_replay);
    fxbus_process_st(&fx, dry_l, dry_r, rev, dly, buffer_l, buffer_r, 32);
    PROF_ADD(PR_FX, start_replay);
    assert(eng_prof[PR_FX] == 96u);

    /* 2 physically overlapping C0/C1 timed spans may exceed a one-core
     * budget when summed. This is elapsed overlap, NOT CPU utilization. */
    reset();
    PROF_T(core0_start);
    PROF_T(core1_start);
    wall_ticks += 150000u;
    PROF_ADD_CORE(0, PR_909, core0_start);
    PROF_ADD_CORE(1, PR_FX, core1_start);
    prof_c1_fold(1);
    assert(eng_prof[PR_909] == 150000u && eng_prof[PR_FX] == 150000u);
    assert(pct10(eng_prof[PR_909], 256) > 1000u);
    assert(pct10(eng_prof[PR_FX], 256) > 1000u);
    assert(pct10(eng_prof[PR_FX], 256) + pct10(eng_prof[PR_909], 256) > 2000u);

    puts("PASS: shared-clock fake concurrency, C1 FX, C0 stall/fallback, TIMER4 wrap, fold/replay, denominator");
    puts("LIMIT: only source/host-model checks; native TIMER4 C1 accuracy and CPU usage NOT MEASURED");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="dnbox-perf-shared-clock-") as temp:
    cpath = Path(temp) / "shared_clock.c"
    exe = Path(temp) / "shared_clock"
    cpath.write_text(source)
    subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall",
                    "-Wextra", "-Werror", str(cpath), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Source: exact production macros, core2_fx and prof_c1_fold slices")
