#!/usr/bin/env python3
"""Use the production runtime header and input-lifetime function closure.

Only the DSP note endpoint is stubbed, to count accepted on/off calls. Run the
same lost/delayed-off oracle with shipping and historical Drone availability.

Standalone reproducer: python3 tests/host/test_original_runtime_lost_off.py
"""
from pathlib import Path
import os
import re
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

grace = re.search(r'^#define ORIGINAL_STALE_GRACE_TICKS[^\n]*$', SOURCE, re.MULTILINE)
if grace is None:
    raise AssertionError('Missing production stale-release grace constant')

# Keep the actual transitive closure in dependency order. Missing production
# helpers must fail here or at compilation, never acquire a fake fallback.
selected = [extract('int original_runtime_kind_available('),
            extract('static int runtime_unavailable('),
            extract('static int runtime_result('),
            extract('static int key_held('),
            extract('static void runtime_add_stale('),
            extract('static void runtime_cut_key('),
            grace.group(0),
            extract('static int runtime_stale_tick('),
            extract('int original_runtime_key('),
            extract('static void runtime_clear_source_debt('),
            extract('int original_runtime_release_source('),
            extract('int original_runtime_cut_source(')]

HEAD = r'''
#include "original_runtime.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#define CHECK(x) do { checks++; if(!(x)) { fprintf(stderr, "FAIL:%d %s\n", __LINE__, #x); fails++; } } while (0)
static unsigned checks, fails, ons[128], offs[128], unavailable_notes;
static int runtime_note(original_runtime_part *p, unsigned key, unsigned vel) {
    if (!original_runtime_kind_available(p->kind)) unavailable_notes++;
    if (vel) ons[key]++; else offs[key]++;
    return ORIGINAL_RUNTIME_OK;
}
static original_runtime_part fresh(unsigned kind) {
    original_runtime_part p = {0};
    p.kind = (uint8_t)kind;
    p.status = original_runtime_kind_available((int)kind) ?
        ORIGINAL_RUNTIME_OK : ORIGINAL_RUNTIME_UNAVAILABLE;
    return p;
}
'''

MAIN = r'''
static void available_lifetime(unsigned kind) {
    original_runtime_part p = fresh(kind);
    memset(ons,0,sizeof ons); memset(offs,0,sizeof offs);
    CHECK(original_runtime_key(&p, ORIGINAL_INPUT_USB, 48, 100) == ORIGINAL_RUNTIME_OK);
    CHECK(original_runtime_key(&p, ORIGINAL_INPUT_USB, 48, 0) == ORIGINAL_RUNTIME_OK);
    CHECK(!p.keys[0][48] && offs[48] == 1);

    /* Lost old off, then exactly one fresh on/off must recover. */
    p = fresh(kind); memset(ons,0,sizeof ons); memset(offs,0,sizeof offs);
    original_runtime_key(&p, 0, 48, 90);
    original_runtime_cut_source(&p, 0);
    CHECK(!p.keys[0][48] && p.stale[0][48] == 1);
    original_runtime_key(&p, 0, 48, 100);
    original_runtime_key(&p, 0, 48, 0);
    for (int i = 0; i < 17; ++i) CHECK(runtime_stale_tick(&p, 256) == ORIGINAL_RUNTIME_OK);
    CHECK(!p.keys[0][48]); 
    CHECK(offs[48] == 2);

    /* Delayed old off cannot cut a new press. */
    p = fresh(kind); memset(ons,0,sizeof ons); memset(offs,0,sizeof offs);
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
    p = fresh(kind); memset(offs,0,sizeof offs);
    original_runtime_key(&p, 0, 48, 90); original_runtime_cut_source(&p, 0);
    for (int i = 0; i < 128; ++i) CHECK(runtime_stale_tick(&p, 256) == ORIGINAL_RUNTIME_OK);
    original_runtime_key(&p, 0, 48, 90);
    original_runtime_key(&p, 0, 48, 0);
    CHECK(p.keys[0][48] == 1);
    original_runtime_key(&p, 0, 48, 0);
    CHECK(!p.keys[0][48] && offs[48] == 2);

    /* Disconnect drops canceled debt without touching other sources. */
    p = fresh(kind); memset(offs,0,sizeof offs);
    original_runtime_key(&p, 0, 48, 90); original_runtime_cut_source(&p, 0);
    original_runtime_key(&p, 1, 48, 95);
    original_runtime_release_source(&p, 0);
    CHECK(!p.stale[0][48] && p.keys[1][48] == 1);
    original_runtime_key(&p, 0, 48, 0);
    CHECK(p.keys[1][48] == 1);
    original_runtime_key(&p, 1, 48, 0);
    CHECK(!p.keys[1][48] && offs[48] == 2);

    p = fresh(kind); p.keys[0][48] = UINT8_MAX;
    CHECK(original_runtime_key(&p, 0, 48, 100) == ORIGINAL_RUNTIME_FAULT);
    CHECK(p.keys[0][48] == UINT8_MAX);
}

#if !X0X_DRONE_RUNTIME_ENABLED
static void unavailable_lifetime(void) {
    original_runtime_part p = fresh(ORIGINAL_SOUND_DRONE);
    memset(ons,0,sizeof ons); memset(offs,0,sizeof offs);
    CHECK(original_runtime_key(&p, ORIGINAL_INPUT_USB, 48, 90) == ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(!p.keys[ORIGINAL_INPUT_USB][48] && p.stale[ORIGINAL_INPUT_USB][48] == 1);
    CHECK(original_runtime_cut_source(&p, ORIGINAL_INPUT_USB) == ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(p.stale[ORIGINAL_INPUT_USB][48] == 1);
    CHECK(original_runtime_key(&p, ORIGINAL_INPUT_USB, 48, 0) == ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(!p.stale[ORIGINAL_INPUT_USB][48]);
    CHECK(original_runtime_key(&p, ORIGINAL_INPUT_KEYS, 48, 90) == ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(original_runtime_key(&p, ORIGINAL_INPUT_SEQUENCE, 48, 90) == ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(!p.keys[ORIGINAL_INPUT_KEYS][48] && !p.keys[ORIGINAL_INPUT_SEQUENCE][48]);
    CHECK(!ons[48] && !offs[48]);

    /* Off debt survives refusal, saturates, and is isolated by source. */
    p.stale[ORIGINAL_INPUT_USB][48] = UINT8_MAX;
    CHECK(original_runtime_key(&p, ORIGINAL_INPUT_USB, 48, 90) == ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(p.stale[ORIGINAL_INPUT_USB][48] == UINT8_MAX);
    CHECK(original_runtime_key(&p, ORIGINAL_INPUT_BLE, 48, 90) == ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(original_runtime_release_source(&p, ORIGINAL_INPUT_USB) == ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(!p.stale[ORIGINAL_INPUT_USB][48] && p.stale[ORIGINAL_INPUT_BLE][48] == 1);
    CHECK(original_runtime_release_source(&p, ORIGINAL_INPUT_USB) == ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(p.stale[ORIGINAL_INPUT_BLE][48] == 1);

    /* An already-running timer must expire without entering the DSP endpoint.
     * Seed only the backend identity after an ordinary completed key lifetime;
     * this is a carried-clock fixture, not coverage of replace()/adoption. */
    p = fresh(ORIGINAL_SOUND_CHICAGO);
    for (unsigned source = ORIGINAL_INPUT_USB; source <= ORIGINAL_INPUT_BLE; ++source) {
        original_runtime_key(&p, source, 48, 90);
        original_runtime_cut_source(&p, source);
        original_runtime_key(&p, source, 48, 90);
        original_runtime_key(&p, source, 48, 0);
        original_runtime_key(&p, source, 48, 0);
        CHECK(!p.keys[source][48] && p.stale_ttl[source][48]);
    }
    CHECK(p.stale_timers == 2);
    p.kind = ORIGINAL_SOUND_DRONE; p.status = ORIGINAL_RUNTIME_UNAVAILABLE;
    memset(ons,0,sizeof ons); memset(offs,0,sizeof offs);
    CHECK(original_runtime_release_source(&p, ORIGINAL_INPUT_USB) == ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(p.stale_timers == 1 && !p.stale_ttl[ORIGINAL_INPUT_USB][48]);
    CHECK(p.stale_ttl[ORIGINAL_INPUT_BLE][48]);
    CHECK(original_runtime_release_source(&p, ORIGINAL_INPUT_USB) == ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(p.stale_timers == 1);
    CHECK(runtime_stale_tick(&p, 4096) == ORIGINAL_RUNTIME_OK);
    CHECK(!p.stale_timers && !p.stale_ttl[ORIGINAL_INPUT_BLE][48]);
    CHECK(p.status == ORIGINAL_RUNTIME_UNAVAILABLE && !ons[48] && !offs[48]);
    CHECK(original_runtime_release_source(&p, ORIGINAL_INPUT_BLE) == ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(!p.stale_timers);
}
#endif

int main(void) {
    const unsigned kinds[] = {ORIGINAL_SOUND_CHICAGO, ORIGINAL_SOUND_MIAMI,
                              ORIGINAL_SOUND_DRONE, ORIGINAL_SOUND_JUNO};
    CHECK(original_runtime_kind_available(ORIGINAL_SOUND_CHICAGO));
    CHECK(original_runtime_kind_available(ORIGINAL_SOUND_MIAMI));
    CHECK(original_runtime_kind_available(ORIGINAL_SOUND_JUNO));
    CHECK(original_runtime_kind_available(ORIGINAL_SOUND_DRONE) == X0X_DRONE_RUNTIME_ENABLED);
    for (unsigned i = 0; i < sizeof kinds / sizeof kinds[0]; ++i)
        if (original_runtime_kind_available((int)kinds[i])) available_lifetime(kinds[i]);
#if !X0X_DRONE_RUNTIME_ENABLED
    unavailable_lifetime();
#endif
    CHECK(!unavailable_notes);
    printf("original_runtime_lost_off: drone=%d %u checks %u failures\n",
           X0X_DRONE_RUNTIME_ENABLED, checks, fails);
    return fails ? 1 : 0;
}
'''

def run_one(compiler, flags, drone):
    with tempfile.TemporaryDirectory(prefix='x0x37-stale-') as d:
        source = Path(d) / 'case.c'
        binary = Path(d) / 'case'
        source.write_text(HEAD + '\n'.join(selected) + MAIN)
        cmd = [compiler, '-std=c11', '-Wall', '-Wextra', '-Werror',
               '-O1', '-g', *flags, '-DX0X_DRONE_RUNTIME_ENABLED=' + str(drone),
               '-I' + str(ROOT / 'firmware/src/app'),
               '-I' + str(ROOT / 'experiments/original_synth'),
               '-I' + str(ROOT / 'experiments/wobble'),
               str(source), '-o', str(binary)]
        subprocess.run(cmd, check=True)
        subprocess.run([str(binary)], check=True, env=os.environ.copy())

if __name__ == '__main__':
    ran = 0
    for compiler, flags in [
        ('gcc', []), ('clang', []),
        ('gcc', ['-fsanitize=undefined', '-fno-sanitize-recover=all']),
        ('gcc', ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                 '-fno-omit-frame-pointer']),
        ('clang', ['-fsanitize=undefined', '-fno-sanitize-recover=all']),
        ('clang', ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                   '-fno-omit-frame-pointer'])]:
        if shutil.which(compiler) is None:
            print(compiler, ' '.join(flags), 'NOT_RUN (not installed)', flush=True)
            continue
        for drone in (0, 1):
            print(compiler, ' '.join(flags), 'X0X_DRONE_RUNTIME_ENABLED=' + str(drone), flush=True)
            run_one(compiler, flags, drone)
            ran += 1
    if not ran:
        sys.exit('no C compiler found (need gcc or clang)')
