#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Regression for motion automation while EXTERNAL clock is explicitly waiting.

Compiles unchanged production motion.c with the actual engine_render motion guard
and seq_clock_waiting function EXTRACTED from the selected source tree.
This is a linked source-window regression, not complete engine/audio/hardware QA.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = (Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]).resolve()
engine = (root / 'firmware/src/app/engine.c').read_text()
seq = (root / 'firmware/src/seq/sequencer.c').read_text()
render = engine.partition('void engine_render(')[2]
if not render:
    raise SystemExit('engine_render missing, cannot validate runtime hook')
# Only accept a guard in the real render function, not a similarly named helper.
guards = re.findall(r'if\s*\(\s*mot_on\s*&&\s*seq\.playing(?:\s*&&\s*!seq_clock_waiting\s*\(\s*&seq\s*\))?\s*\)\s*motion_tick\s*\(\s*&mot\s*,\s*n\s*\)\s*;', render)
if len(guards) != 1:
    raise SystemExit(f'Expected one recognized motion_tick guard inside engine_render; got {len(guards)}')
wait = re.search(r'int\s+seq_clock_waiting\s*\(\s*const\s+seq_t\s*\*\s*s\s*\)\s*\{\s*return\s+[^;]+;\s*\}', seq)
if wait is None:
    raise SystemExit('Cannot extract production seq_clock_waiting definition')
# The actual predicate belongs to the sequencer owner and is compiled verbatim.
source = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "motion.h"
#define FS 44100u
#define SEQ_CLK_EXT 2u
#define SEQ_CLK_INT 1u
#define SEQ_CLK_AUTO 0u
typedef struct { int clk_cur, playing; uint32_t ext_since; } seq_t;
static seq_t seq;
static motion_t mot;
static lane_t lanes[NLANE];
static int mot_on = 1, value = -1, calls, checks, errors;
static int base(void *ctx, int t, int v, int i) {
    (void)ctx; (void)t; (void)v; (void)i; return 0;
}
static void apply(void *ctx, int t, int v, int i, int vout) {
    (void)ctx; (void)t; (void)v; (void)i; value = vout; ++calls;
}
WAIT_DEF
static void block(uint32_t n) { GUARD }
#define CHECK(expr, msg) do { ++checks; if (!(expr)) { ++errors; fprintf(stderr, "FAIL %s: age=%u value=%d calls=%d wait=%d\n", msg, mot.age[0], value, calls, seq_clock_waiting(&seq)); } } while (0)
static void prep(void) {
    memset(lanes, 0, sizeof lanes); memset(&seq, 0, sizeof seq);
    motion_init(&mot, lanes); mot.base = base; mot.apply = apply;
    int slot = motion_alloc(lanes, 0, 0, 1, 0, 0);
    if (slot != 0) { ++errors; fprintf(stderr, "failed to allocate lane\n"); }
    lanes[0].val[0] = 30; lanes[0].val[1] = 100;
    mot.len[0][0] = 100; motion_step(&mot, 0, 0, 0, 1, 16);
    value = 30; calls = 0; mot_on = 1;
}
int main(void) {
    prep(); seq.playing = 1; seq.clk_cur = SEQ_CLK_INT;
    block(50);
    CHECK(mot.age[0] == 50 && value == 65, "INTERNAL running 50-sample glide remains active");
    prep(); seq.playing = 1; seq.clk_cur = SEQ_CLK_EXT; seq.ext_since = FS;
    CHECK(seq_clock_waiting(&seq), "source-matched EXTERNAL WAIT true");
    block(50);
    CHECK(mot.age[0] == 0 && value == 30, "WAIT cannot age glide during 50 silent samples");
    block(100);
    CHECK(mot.age[0] == 0 && value == 30, "WAIT cannot complete stale glide after 150 samples");
    motion_step(&mot, 0, 0, 1, 2, 16);
    CHECK(mot.len[0][0] == 100, "resumed step cannot inherit paused wall-clock samples");
    prep(); seq.playing = 1; seq.clk_cur = SEQ_CLK_EXT; seq.ext_since = FS;
    block(256);
    seq.ext_since = 0; block(50);
    CHECK(mot.age[0] == 50 && value == 65, "first resumed running block counts only live samples");
    prep(); seq.playing = 1; seq.clk_cur = SEQ_CLK_EXT; seq.ext_since = FS / 2u;
    CHECK(!seq_clock_waiting(&seq), "boundary exact 0.5 seconds not yet WAIT");
    block(50);
    CHECK(mot.age[0] == 50 && value == 65, "grace interval interpolates before WAIT");
    prep(); seq.playing = 1; seq.clk_cur = SEQ_CLK_AUTO; seq.ext_since = FS;
    CHECK(!seq_clock_waiting(&seq), "AUTO mode must not use EXTERNAL WAIT");
    block(50);
    CHECK(mot.age[0] == 50 && value == 65, "AUTO fallback can keep advancing");
    prep(); seq.playing = 0; seq.clk_cur = SEQ_CLK_EXT; seq.ext_since = FS;
    block(500);
    CHECK(mot.age[0] == 0 && value == 30, "STOP still freezes automation");
    prep(); seq.playing = 1; seq.clk_cur = SEQ_CLK_INT; mot_on = 0;
    block(500);
    CHECK(mot.age[0] == 0 && value == 30, "disabled automation still freezes");
    fprintf(stderr, "motion EXTERNAL WAIT: %d/%d assertions\n", checks-errors, checks);
    return errors ? 1 : 0;
}
'''.replace('WAIT_DEF', wait.group()).replace('GUARD', guards[0])
cc = os.environ.get('CC', 'gcc')
flags = os.environ.get('CFLAGS', '-O2 -Wall -Wextra -Werror').split()
with tempfile.TemporaryDirectory(prefix='motion-external-wait-') as tmp:
    src = Path(tmp) / 'case.c'; exe = Path(tmp) / 'case'
    src.write_text(source)
    cmd = [cc, '-std=c11', *flags, '-I'+str(root / 'firmware/src/seq'), str(src), str(root / 'firmware/src/seq/motion.c'), '-o', str(exe)]
    build = subprocess.run(cmd, text=True, capture_output=True)
    if build.returncode:
        sys.stderr.write(build.stdout + build.stderr)
        raise SystemExit(f'{cc} compilation failed with exit {build.returncode}')
    run = subprocess.run([str(exe)], text=True, capture_output=True,
                         env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0:halt_on_error=1'})
    sys.stdout.write(run.stdout)
    sys.stderr.write(run.stderr)
    sys.exit(run.returncode)
