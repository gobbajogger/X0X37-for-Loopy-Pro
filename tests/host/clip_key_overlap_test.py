#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Native host contract for production clip-key MIDI note ownership.

Extract ck_key_send and ck_voice unchanged from firmware/src/app/main_fm1.c,
including the actual ck_press call's status, then exercise the real functions
against deterministic overlapping key/chord outputs and a MIDI sink. The
ck_key_notes mock tests MIDI ownership, not the already-covered scale math.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


def function(src, name):
    match = re.search(r'\b(?:static\s+)?void\s+' + re.escape(name) + r'\s*\([^;]*?\)\s*\{', src)
    if match is None:
        raise RuntimeError(f'{name}: source window missing')
    p, level = match.start(), 0
    for i in range(match.end() - 1, len(src)):
        if src[i] == '{':
            level += 1
        elif src[i] == '}':
            level -= 1
            if not level:
                return src[p:i + 1]
    raise RuntimeError(f'{name}: unclosed source window')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('source', nargs='?', default='firmware/src/app/main_fm1.c')
    ap.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    args = ap.parse_args()
    src = Path(args.source).read_text(encoding='utf-8')
    press = function(src, 'ck_press')
    mark = re.search(r'\bck_key_send\(k,\s*(0x[0-9a-fA-F]+u)\s*\)', press)
    if mark is None:
        raise RuntimeError('ck_press direct note-on call missing')
    press_status = mark.group(1)
    send_src = function(src, 'ck_key_send')
    voice_src = function(src, 'ck_voice')
    live = 'static uint32_t ck_clip_live[4];' in src
    clip_helpers = (function(src, 'ck_clip_on') + '\n' + function(src, 'ck_clip_off')) if live else ''
    host = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CK_NKEY 37u
#define CK_MAXN 7u
#define CK_FN
#define CK_M_ARP 1u
#define CK_BIT(map, k) (((map)[(k) >> 5] >> ((k) & 31u)) & 1u)
typedef struct { uint32_t snd[2]; uint8_t ch, on, nk; } ck_state_t;
static ck_state_t ck;
#define CK_BASE ck_state_t *cp = &ck
static int on[128], off[128], balance[128], errors, checks;
static void ck_on(uint32_t n) {
    if (n >= 128u) abort();
    on[n]++; balance[n]++;
}
static void ck_send(uint32_t status, uint32_t n, uint32_t v) {
    (void)v;
    if ((status & 0xF0u) != 0x80u || n >= 128u) abort();
    off[n]++; balance[n]--;
    if (balance[n] < 0) errors++;
}
/* C/C# snap together to C in MAJOR NEAR; second pair shares two tones
 * between two chords. The upstream scale math is separately tested by
 * smk_clip_keys_test.c; this mock intentionally isolates note ownership. */
static uint32_t ck_key_notes(uint32_t k, uint8_t *out) {
    if (k <= 1u) { out[0] = 60; return 1; }
    if (k == 2u) { out[0] = 60; out[1] = 64; out[2] = 67; return 3; }
    if (k == 3u) { out[0] = 64; out[1] = 67; return 2; }
    out[0] = (uint8_t)(48 + k); return 1;
}
#define CHECK(test, name) do { checks++; if (!(test)) { \
  fprintf(stderr, "FAIL %s line %d\n", name, __LINE__); errors++; } } while(0)
static void reset(void) { memset(&ck, 0, sizeof ck); memset(on, 0, sizeof on); \
 memset(off, 0, sizeof off); memset(balance, 0, sizeof balance); errors = checks = 0; }
static void press(uint32_t k) { ck.snd[k >> 5] |= 1u << (k & 31u); ck_key_send(k, PRESS_STATUS); }
static void release(uint32_t k) { ck.snd[k >> 5] &= ~(1u << (k & 31u)); ck_key_send(k, 0x81u); }
static int suite(void) {
    int failures = 0;
    reset();
    press(0); press(1);
    CHECK(on[60] == 1 && balance[60] == 1, "overlapping snapped keys emit one NoteOn");
    release(0);
    CHECK(off[60] == 0 && balance[60] == 1, "releasing first keeps common voice");
    release(1);
    CHECK(on[60] == 1 && off[60] == 1 && balance[60] == 0, "last release balances snapped note");
    failures += errors;
    reset();
    press(1); press(0); release(1); release(0);
    CHECK(on[60] == 1 && off[60] == 1 && balance[60] == 0, "reverse press/release order");
    failures += errors;
    reset();
    press(0); press(1);
    ck_voice(0x80u); /* old pitch owner before a settings change */
    CHECK(off[60] == 1 && balance[60] == 0, "settings off unique per pitch");
    ck_voice(0x90u); /* new pitch owner after a settings change */
    CHECK(on[60] == 2 && balance[60] == 1, "settings on unique per pitch");
    release(0); release(1);
    CHECK(off[60] == 2 && balance[60] == 0, "setting revoice then key release balances");
    failures += errors;
    reset();
    press(2); press(3);
    CHECK(on[60] == 1 && on[64] == 1 && on[67] == 1, "chords with shared tones deduplicated");
    release(2); release(3);
    CHECK(balance[60] == 0 && balance[64] == 0 && balance[67] == 0, "shared chords all release");
    failures += errors;
    reset();
    press(4); press(5);
    CHECK(on[52] == 1 && on[53] == 1, "different notes still both play");
    release(4); release(5);
    CHECK(balance[52] == 0 && balance[53] == 0, "different notes release");
    failures += errors;
    reset();
    press(0); press(1); ck_voice(0x80u);
    CHECK(off[60] == 1 && balance[60] == 0, "bulk all-off does not duplicate NoteOff");
    failures += errors;
    printf("clip_key_overlap_test: %d failures (12 critical observations)\n", failures);
    return failures ? 1 : 0;
}
int main(void) { return suite(); }
'''
    body = host.replace('#define CK_FN', '#define CK_FN\n#define PRESS_STATUS ' + press_status)
    if live:
        body = body.replace('static ck_state_t ck;', 'static ck_state_t ck;\nstatic uint32_t ck_clip_live[4];')
        body = body.replace('memset(&ck, 0, sizeof ck);',
                            'memset(ck_clip_live, 0, sizeof ck_clip_live); memset(&ck, 0, sizeof ck);')
    body = body.replace('static void reset(void)', clip_helpers + '\n' + send_src + '\n' + voice_src + '\nstatic void reset(void)')
    with tempfile.TemporaryDirectory(prefix='x0x37-clip-overlap-') as td:
        cfile = Path(td, 'clip_overlap.c')
        exe = Path(td, 'clip_overlap')
        cfile.write_text(body, encoding='utf-8')
        opts = os.environ.get('CFLAGS', '-O2 -Wall -Wextra -Werror').split()
        print(f'compiler={args.cc} flags={opts} source={args.source} direct_press_status={press_status}', flush=True)
        comp = subprocess.run([args.cc, '-std=c99', *opts, str(cfile), '-o', str(exe)], check=False)
        if comp.returncode:
            return comp.returncode
        run = subprocess.run([str(exe)], check=False)
        return run.returncode


if __name__ == '__main__':
    raise SystemExit(main())
