#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile production 808 accounting/control slices with deterministic fake clocks.

This is a host meter/control-path test, NOT audio identity or device CPU evidence.
It needs only a C compiler and the exact checkout's engine.c, no private SDK.
"""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SRC = (ROOT / "firmware/src/app/engine.c").read_text()


def slice_between(start, stop):
    assert SRC.count(start) == 1, f"missing/nonunique start: {start}"
    a = SRC.index(start) + len(start)
    b = SRC.find(stop, a)
    assert b >= a, f"missing end after start: {stop}"
    return SRC[a:b]


body = slice_between("static void d808_body_run(int core)\n{", "\n/* 0 = body visible")
fold = slice_between("static void prof_c1_fold(int keep)\n{", "\n#ifdef X0X_HOST")
wait = slice_between("static int body_wait(int core)\n{", "\n/* one job, render and mix")
job = slice_between("} else if (j == JB_808) {", "} else if (j == JB_A || j == JB_B) {")
assert "PROF_ADD_CORE(core, PR_808, p8body);" in body
assert "PROF_ADD_CORE(core, PR_808, p8);" in job
assert job.count("PROF_ADD_CORE(core, PR_808, p8);") == 3
assert "d808_body_run(0);" in wait
assert "d808_body_run(1);" in SRC

prof_start = SRC.index("#define PROF_T(v)")
prof_end = SRC.index("/* ---------------------------------------------------------------- state --- */", prof_start)
PROF_MACROS = SRC[prof_start:prof_end]
assert PROF_MACROS.count("plat_ticks24()") == 3
assert "plat_cycles()" not in PROF_MACROS  # a C0-only counter cannot time C1

# Cross-check the other two active-engine attribution boundaries, without
# trying to substitute a fake synth or BREAK renderer for their real DSP.
bass = slice_between("} else if (j == JB_A || j == JB_B) {", "} else {\n        PROF_T(pk);")
credit = "PROF_ADD_CORE(core, 2 + bv, pb);"
assert bass.count(credit) == 3, "bass DSP, post-DSP, and bus mix must be accounted separately"
assert bass.find(credit) < bass.find("bass_render_ok[bv]")
assert bass.find(credit, bass.find(credit) + 1) < bass.find("stage_wait((uint32_t)(2 + bv), core)")
assert bass.rfind(credit) > bass.find("add_glide(dly, bb[bv],")
assert "pb = plat_ticks24();" in bass, "the waits must be excluded from bass's next interval"
assert job.count("p8 = plat_ticks24();") == 2, "808 restarted spans must share the 24 MHz clock"

mix_anchor = "    brk_midi_drain();\n    for (j = 0; j < NBASS; j++)"
assert SRC.count(mix_anchor) == 1
mix_tail = SRC[SRC.index(mix_anchor):]
brk_credit = "PROF_ADD(PR_BRK, pbrkmix);"
assert brk_credit in mix_tail
assert mix_tail.index("PROF_T(pbrkmix)") < mix_tail.index("mix_targets(PART_BRK, g, solo_parts)")
assert mix_tail.index(brk_credit) > mix_tail.index("meter(PART_BRK, kbuf, g, n)")
assert mix_tail.index(brk_credit) < mix_tail.index("bfx_send_shape(0, rev, (int)n)")

prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
enum { PR_909, PR_808, PR_303A, PR_303B, PR_BRK, PR_FX, PR_MST, PR_SEQ, ENG_PROF_N };
static volatile uint32_t eng_prof[ENG_PROF_N], eng_prof_c1[ENG_PROF_N];
/* EXACT_PRODUCTION_PROF_MACROS */
#define eng_sync() ((void)0)
static uint32_t fake_cycles, eng_job_arg, eng_c1_waited;
static uint8_t eng_808_body, eng_808_metal_done, eng_808_split, d808_was_active;
static uint32_t eng_stage;
static float t0[256], tr[256], t1[256], t2[256];
static float original_workspace[2048];
static struct { int active; } d808;
static unsigned n_pan, n_render, n_fx, n_body;
static uint32_t plat_cycles(void) { return 0u; } /* C0 counter frozen; TIMER4 advances */
static uint32_t plat_ticks24(void) { return fake_cycles; }
static int plat_cpu2_busy(void) { return 0; }
static int plat_cpu2_wait(void) { fake_cycles += 200; return 0; }
static int drum808_active(const void *d) { (void)d; fake_cycles += 5; return d808.active; }
static void drum808_pan_settle(void *d) { (void)d; ++n_pan; fake_cycles += 11; }
static void drum808_metal_prepare(void *d, float raw[4][256], int n, uint16_t a) {
    (void)d; (void)raw; (void)n; (void)a; fake_cycles += 30;
}
static void drum808_finish_rest(void *d, const float raw[4][256], float *l, float *r,
    float *rev, float *dly, int n, uint16_t a) {
    (void)d; (void)raw; (void)l; (void)r; (void)rev; (void)dly; (void)n; (void)a;
    fake_cycles += 20;
}
static void drum808_render_st_lanes(void *d, float *l, float *r, float *rev, float *dly,
    int n, uint16_t a, uint32_t lanes) {
    (void)d; (void)l; (void)r; (void)rev; (void)dly; (void)n; (void)a; (void)lanes;
    ++n_body; fake_cycles += 70;
}
static void drum808_render_st_masked(void *d, float *l, float *r, float *rev,
    float *dly, int n, uint16_t a) {
    (void)d; (void)l; (void)r; (void)rev; (void)dly; (void)n; (void)a;
    ++n_render; fake_cycles += 300;
}
static void bfx_shape(int part, float *l, float *r, float *rev, float *dly, int ch, int n) {
    (void)part; (void)l; (void)r; (void)rev; (void)dly; (void)ch; (void)n;
    ++n_fx; fake_cycles += 23;
}
static void mix_808(float *l, float *r, uint32_t n, uint8_t solo) {
    (void)l; (void)r; (void)n; (void)solo;
    if (d808_was_active) fake_cycles += 37;
}
static int stage_wait(uint32_t s, int core) {
    (void)s; (void)core;
    fake_cycles += 4000; /* a long 909 dependency must never appear as 808 work */
    return 0;
}
#define D8_LANES_BODY 3u
#define PART_808 1
static uint8_t eng_job_solo;
'''
fold_start = '''static void prof_c1_fold(int keep)\n{'''
body_start = '''static void d808_body_run(int core)\n{'''
wait_start = '''static int body_wait(int core)\n{'''
job_start = '''static int run_808(int core)\n{
    uint32_t arg = eng_job_arg, n = arg & 0xFFu ? arg & 0xFFu : 256u, i;
    uint32_t offset = (arg >> 20) & 0xFFu, wt = 0, w0;
    uint8_t solo_parts = eng_job_solo;
    int active = 1;
    float *left808 = t0 + offset, *right808 = tr + offset;
    {
'''
suffix = r'''
    }
    (void)wt; (void)active;
    return 1;
}
static void reset(void) {
    memset((void *)eng_prof, 0, sizeof eng_prof);
    memset((void *)eng_prof_c1, 0, sizeof eng_prof_c1);
    fake_cycles = eng_c1_waited = eng_job_arg = eng_808_body = 0;
    eng_808_split = eng_808_metal_done = d808_was_active = 0;
    eng_stage = 1; d808.active = 0;
    n_pan = n_render = n_fx = n_body = 0;
}
int main(void) {
    reset();
    assert(plat_cycles() == 0u);  /* this old C0 clock is useless for C1 */
    assert(run_808(0) == 1);
    assert(eng_prof[PR_808] == 16); /* 5 active-scan + 11 pan settle; no 4000 wait */
    assert(n_pan == 1 && n_render == 0 && n_fx == 0 && n_body == 0);

    reset(); d808.active = 1;
    assert(run_808(0) == 1);
    assert(eng_prof[PR_808] == 365); /* scan 5 + synth 300 + FX 23 + mix 37 */
    assert(n_pan == 0 && n_render == 1 && n_fx == 1);

    reset(); d808.active = 1; eng_808_split = 1;
    d808_body_run(1); /* C1 owns the split-body DSP */
    assert(eng_prof_c1[PR_808] == 70 && eng_prof[PR_808] == 0);
    assert(run_808(0) == 1);
    assert(eng_prof[PR_808] == 115); /* 5 scan + 30 metal + 20 rest + 23 FX + 37 mix */
    assert(n_body == 1 && n_render == 0 && n_fx == 1);
    prof_c1_fold(1);
    assert(eng_prof[PR_808] == 185 && eng_prof_c1[PR_808] == 0);
    prof_c1_fold(1);
    assert(eng_prof[PR_808] == 185); /* repeated join never counts twice */

    reset(); d808.active = 1; eng_808_split = 1;
    assert(run_808(0) == 1); /* C1 unavailable, core 0 does the body too */
    assert(n_body == 1 && eng_prof_c1[PR_808] == 0);
    assert(eng_prof[PR_808] == 185); /* same work as split plus 70 actual body */
    eng_prof_c1[PR_808] = 70; /* discarded failed C1 credit */
    prof_c1_fold(0);
    assert(eng_prof_c1[PR_808] == 0 && eng_prof[PR_808] == 185);
    /* Its 200-cycle join and 4000-cycle stage wait are not 808 work. */
    puts("PASS perf-meter-808-source: silent, active, split C1, C0 takeover");
    return 0;
}
'''
prefix = prefix.replace("/* EXACT_PRODUCTION_PROF_MACROS */", PROF_MACROS)
code = (prefix + fold_start + fold + "\n" + body_start + body + "\n" + wait_start + wait + "\n" + job_start + job + suffix)
with tempfile.TemporaryDirectory(prefix="dnbox-perf-meter-") as tmp:
    src = Path(tmp) / "test.c"
    exe = Path(tmp) / "test"
    src.write_text(code)
    subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", str(src), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Source: 808 path slices compiled from", ROOT / "firmware/src/app/engine.c")
