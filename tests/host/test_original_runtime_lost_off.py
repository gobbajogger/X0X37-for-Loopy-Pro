#!/usr/bin/env python3
"""Compile production original-runtime source functions with DSP note endpoints stubbed.

Standalone reproducer: python3 tests/host/test_original_runtime_lost_off.py
"""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / 'firmware/src/app/original_runtime.c').read_text()

def extract(signature):
    pos = SOURCE.index(signature)
    start = SOURCE.index('{', pos)
    depth = 0
    for end in range(start, len(SOURCE)):
        if SOURCE[end] == '{':
            depth += 1
        elif SOURCE[end] == '}':
            depth -= 1
            if depth == 0:
                return SOURCE[pos:end + 1]
    raise AssertionError('Unclosed C function: ' + signature)

selected = [extract('static int key_held('),
            extract('int original_runtime_key('),
            extract('int original_runtime_release_source('),
            extract('int original_runtime_cut_source(')]
if 'static int runtime_stale_tick(' in SOURCE:
    selected.append(extract('static int runtime_stale_tick('))
else:
    # Pre-fix source must fail the lost-release assertion, not fail to compile.
    selected.append('static int runtime_stale_tick(original_runtime_part *p, unsigned n) { (void)p; (void)n; return ORIGINAL_RUNTIME_OK; }')

HEAD = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
enum { ORIGINAL_INPUT_USB, ORIGINAL_INPUT_BLE, ORIGINAL_INPUT_TRS,
       ORIGINAL_INPUT_KEYS, ORIGINAL_INPUT_SEQUENCE, ORIGINAL_INPUT_COUNT };
enum { ORIGINAL_RUNTIME_INVALID = 0, ORIGINAL_RUNTIME_OK = 1, ORIGINAL_RUNTIME_FAULT = -2 };
#define ORIGINAL_STALE_GRACE_TICKS 16u
#define CHECK(x) do { checks++; if(!(x)) { fprintf(stderr, "FAIL:%d %s\n", __LINE__, #x); fails++; } } while (0)
typedef struct {
    uint8_t keys[ORIGINAL_INPUT_COUNT][128];
    uint8_t stale[ORIGINAL_INPUT_KEYS][128];
    uint8_t stale_ttl[ORIGINAL_INPUT_KEYS][128];
    uint32_t stale_pending[ORIGINAL_INPUT_KEYS][4];
    uint16_t stale_tick_frames, stale_timers;
    uint8_t kind;
    int8_t status;
} original_runtime_part;
static unsigned checks, fails, ons[128], offs[128];
static int runtime_result(original_runtime_part *p, int ok) {
    if (!ok) p->status = ORIGINAL_RUNTIME_FAULT;
    return p->status;
}
static int runtime_note(original_runtime_part *p, unsigned key, unsigned vel) {
    (void)p;
    if (vel) ons[key]++; else offs[key]++;
    return ORIGINAL_RUNTIME_OK;
}
static original_runtime_part fresh(void) {
    original_runtime_part p = {0}; p.status = ORIGINAL_RUNTIME_OK; return p;
}
'''

MAIN = r'''
int main(void) {
    original_runtime_part p = fresh();
    CHECK(original_runtime_key(&p, ORIGINAL_INPUT_USB, 48, 100) == ORIGINAL_RUNTIME_OK);
    CHECK(original_runtime_key(&p, ORIGINAL_INPUT_USB, 48, 0) == ORIGINAL_RUNTIME_OK);
    CHECK(!p.keys[0][48] && offs[48] == 1);

    /* Lost old off, then exactly one fresh on/off must recover. */
    p = fresh(); memset(ons,0,sizeof ons); memset(offs,0,sizeof offs);
    original_runtime_key(&p, 0, 48, 90);
    original_runtime_cut_source(&p, 0);
    CHECK(!p.keys[0][48] && p.stale[0][48] == 1);
    original_runtime_key(&p, 0, 48, 100);
    original_runtime_key(&p, 0, 48, 0);
    for (int i = 0; i < 17; ++i) CHECK(runtime_stale_tick(&p, 256) == ORIGINAL_RUNTIME_OK);
    CHECK(!p.keys[0][48]); 
    CHECK(offs[48] == 2);

    /* Delayed old off cannot cut a new press. */
    p = fresh(); memset(ons,0,sizeof ons); memset(offs,0,sizeof offs);
    original_runtime_key(&p, 0, 48, 90);
    original_runtime_cut_source(&p, 0);
    original_runtime_key(&p, 0, 48, 100);
    original_runtime_key(&p, 0, 48, 0);
    CHECK(p.keys[0][48] == 1 && offs[48] == 1);
    CHECK(runtime_stale_tick(&p, 1024) == ORIGINAL_RUNTIME_OK);
    CHECK(p.keys[0][48] == 1);
    original_runtime_key(&p, 0, 48, 0);
    CHECK(!p.keys[0][48] && offs[48] == 2);
    CHECK(runtime_stale_tick(&p, 4096) == ORIGINAL_RUNTIME_OK);
    CHECK(offs[48] == 2 && !p.keys[0][48]);

    /* Grace starts at retrigger, not at recording cancel. */
    p = fresh(); memset(offs,0,sizeof offs);
    original_runtime_key(&p, 0, 48, 90); original_runtime_cut_source(&p, 0);
    for (int i = 0; i < 128; ++i) CHECK(runtime_stale_tick(&p, 256) == ORIGINAL_RUNTIME_OK);
    original_runtime_key(&p, 0, 48, 90);
    original_runtime_key(&p, 0, 48, 0);
    CHECK(p.keys[0][48] == 1);
    original_runtime_key(&p, 0, 48, 0);
    CHECK(!p.keys[0][48] && offs[48] == 2);

    /* Disconnect drops canceled debt without touching other sources. */
    p = fresh(); memset(offs,0,sizeof offs);
    original_runtime_key(&p, 0, 48, 90); original_runtime_cut_source(&p, 0);
    original_runtime_key(&p, 1, 48, 95);
    original_runtime_release_source(&p, 0);
    CHECK(!p.stale[0][48] && p.keys[1][48] == 1);
    original_runtime_key(&p, 0, 48, 0);
    CHECK(p.keys[1][48] == 1);
    original_runtime_key(&p, 1, 48, 0);
    CHECK(!p.keys[1][48] && offs[48] == 2);

    p = fresh(); p.keys[0][48] = UINT8_MAX;
    CHECK(original_runtime_key(&p, 0, 48, 100) == ORIGINAL_RUNTIME_FAULT);
    CHECK(p.keys[0][48] == UINT8_MAX);
    printf("original_runtime_lost_off: %u checks %u failures\n", checks, fails);
    return fails ? 1 : 0;
}
'''

def run_one(compiler, flags):
    with tempfile.TemporaryDirectory(prefix='x0x37-stale-') as d:
        source = Path(d) / 'case.c'
        binary = Path(d) / 'case'
        source.write_text(HEAD + '\n'.join(selected) + MAIN)
        cmd = [compiler, '-std=c11', '-Wall', '-Wextra', '-Werror',
               '-O1', '-g', *flags, str(source), '-o', str(binary)]
        subprocess.run(cmd, check=True)
        subprocess.run([str(binary)], check=True, env=os.environ.copy())

if __name__ == '__main__':
    ran = 0
    for compiler, flags in [
        ('gcc', []), ('clang', []),
        ('gcc', ['-fsanitize=undefined', '-fno-sanitize-recover=all']),
        ('clang', ['-fsanitize=undefined', '-fno-sanitize-recover=all']),
        ('clang', ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                   '-fno-omit-frame-pointer'])]:
        if shutil.which(compiler) is None:
            print(compiler, ' '.join(flags), 'skipped (not installed)', flush=True)
            continue
        print(compiler, ' '.join(flags), flush=True)
        run_one(compiler, flags)
        ran += 1
    if not ran:
        sys.exit('no C compiler found (need gcc or clang)')
