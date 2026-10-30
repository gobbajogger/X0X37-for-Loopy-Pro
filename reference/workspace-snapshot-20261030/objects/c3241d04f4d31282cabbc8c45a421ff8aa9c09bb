#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile actual CLIP KEYS/STRUM function windows against a controlled MIDI sink.

Tests ownership, late strum pitches, quick key changes, settings revoice and
cleanup; no SDK, full-root, target binary or hardware result is inferred.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

PRELUDE = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define CK_NKEY 37u
#define CK_MAXN 7u
#define CK_FN
#define CK_M_ARP 1u
#define CK_M_CHORD 2u
#define CK_F_LATCH 1u
#define CK_BIT(map, k) (((map)[(k) >> 5] >> ((k) & 31u)) & 1u)
typedef struct {
    uint32_t snd[2], keys[2], acc;
    uint8_t ch, on, vel, fl, nk, korder[37], step, tick;
} ck_t;
typedef struct { uint8_t ck_x2, ck_rate; } settings_t;
typedef struct { settings_t set; } project_t;
static ck_t ck;
static project_t proj;
static const uint8_t CK_RATE_T[8] = {1,2,3,4,5,6,7,8};
#define CK_BASE ck_t *cp = &ck
#define CK_SET settings_t *sp = &proj.set
static int on[128], off[128], checks, failures;
static uint32_t ck_key_notes(uint32_t k, uint8_t *out) {
    if (k == 0 || k == 1) {
        out[0] = 60; out[1] = 64; out[2] = 67; return 3;
    }
    if (k == 2) { out[0] = 60; out[1] = 65; return 2; }
    out[0] = (uint8_t)(48 + k); return 1;
}
static void ck_on(uint32_t n) { on[n]++; }
static void ck_send(uint32_t st, uint32_t n, uint32_t v) {
    (void)v;
    if ((st & 0xF0u) == 0x80u) off[n]++;
    else on[n]++;
}
static void ck_arp_stop_note(void) {}
static void ck_q_run(uint32_t only) { (void)only; }
'''
TAIL = r'''
#define CHECK(c, name) do { checks++; if (!(c)) { failures++; \
    fprintf(stderr, "FAIL %s line %d\n", name, __LINE__); } } while(0)
static void reset(void) {
    memset(&ck, 0, sizeof ck);
    memset(on, 0, sizeof on);
    memset(off, 0, sizeof off);
    proj.set.ck_x2 = 0x20u;
    ck.on = CK_M_CHORD;
}
static void balanced(const char *name) {
    int k;
    for (k = 0; k < 128; k++) if (on[k] || off[k]) {
        checks++;
        if (on[k] != off[k]) {
            failures++;
            fprintf(stderr, "FAIL %s note %d on %d off %d\n",
                    name, k, on[k], off[k]);
        }
    }
}
int main(void) {
    reset();
    ck_press(0, 100);
    CHECK(on[60] == 1 && !on[64] && !on[67], "first strum note only");
    ck_strum(1000);
    CHECK(on[60] == 1 && on[64] == 1 && on[67] == 1, "strum delay preserves other tones");
    ck_release(0);
    balanced("solo strum");

    reset();
    ck_press(0, 100); ck_strum(1000);
    ck_press(1, 100); ck_strum(1000);
    CHECK(on[60] == 1 && on[64] == 1 && on[67] == 1,
          "two overlapping strums each pitch once");
    ck_release(0); ck_release(1);
    balanced("held shared chord");

    reset();
    ck_press(0, 100); ck_strum(1000);
    ck_press(1, 100); ck_release(0);
    ck_strum(1000); ck_release(1);
    CHECK(on[60] == 1 && on[64] == 1 && on[67] == 1,
          "old owner release during new strum");
    balanced("release during strum");

    reset();
    ck_press(0, 100); ck_press(1, 100); ck_strum(1000);
    ck_release(0); ck_release(1);
    CHECK(on[60] == 1 && on[64] == 1 && on[67] == 1,
          "old strum finishes before new snapshot");
    balanced("rapid second key");

    reset();
    ck_press(0, 100); ck_strum(1000);
    ck_press(2, 100); ck_strum(1000);
    ck_release(0); ck_release(2);
    CHECK(on[60] == 1 && on[64] == 1 && on[67] == 1 && on[65] == 1,
          "partially shared chords retain unique pitch");
    balanced("partial share");

    reset();
    ck_press(0, 100);
    ck_voice(0x80u); ck_voice(0x90u);
    CHECK(ck.nk == 0, "settings change cancels unsounded strum remainder");
    ck_strum(1000);
    CHECK(on[60] == 2 && on[64] == 1 && on[67] == 1,
          "no late duplicate after revoice");
    ck_release(0);
    CHECK(on[60] == off[60], "original strum note balanced");

    reset();
    ck_press(0, 100); ck_release(0);
    CHECK(on[60] == 1 && off[60] == 1, "short strum sounding root balanced");
    CHECK(!on[64] && !off[64] && !on[67] && !off[67],
          "short strum no unplayed tone NoteOff");

    reset();
    ck_press(0, 100); ck_voice(0x80u); ck_voice(0x90u); ck_release(0);
    CHECK(on[60] == 2 && off[60] == 2, "revoice strum sounding root balanced");
    CHECK(on[64] == 1 && off[64] == 1 && on[67] == 1 && off[67] == 1,
          "revoice strum delayed tones balanced");

    reset();
    ck_press(0, 100); ck_all_off();
    CHECK(on[60] == 1 && off[60] == 1, "panic stops sounding root");
    CHECK(!on[64] && !off[64] && !on[67] && !off[67],
          "panic does not release unsounded tones");
    ck_press(0, 100); ck_strum(1000); ck_release(0);
    CHECK(on[60] == 2 && off[60] == 2 &&
          on[64] == 1 && off[64] == 1 && on[67] == 1 && off[67] == 1,
          "reentry after panic plays once");
    printf("clip_key_strum_overlap: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
'''

def window(source, name):
    pat = r'\b(?:CK_FN\s+)?static\s+void\s+' + re.escape(name) + r'\s*\([^;]*?\)\s*\{'
    match = re.search(pat, source)
    if match is None:
        return None
    depth = 0
    for i in range(match.end() - 1, len(source)):
        if source[i] == '{':
            depth += 1
        elif source[i] == '}':
            depth -= 1
            if not depth:
                return source[match.start():i + 1]
    raise RuntimeError(name + ": unclosed function")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("source", nargs="?", default="firmware/src/app/main_fm1.c")
    ap.add_argument("--cc", default=os.environ.get("CC", "cc"))
    args = ap.parse_args()
    source = Path(args.source).read_text(encoding="utf-8")
    names = ("ck_key_send", "ck_voice", "ck_all_off", "ck_strum", "ck_press", "ck_release")
    functions = [window(source, n) for n in names]
    if any(f is None for f in functions):
        raise RuntimeError("production function missing")
    snapshot = window(source, "ck_strum_snapshot")
    prelude = PRELUDE
    if snapshot:
        if not re.search(r'\bstatic\s+uint32_t\s+ck_strum_keep\s*\[\s*4\s*\]\s*;', source):
            raise RuntimeError("production strum ownership bitmap missing")
        prelude += "\nstatic uint32_t ck_strum_keep[4];\n" + snapshot + "\n"
    live = 'static uint32_t ck_clip_live[4];' in source
    if live:
        helpers = [window(source, name) for name in ('ck_clip_on', 'ck_clip_off')]
        if any(h is None for h in helpers):
            raise RuntimeError("clip MIDI ownership helper missing")
        prelude += "\nstatic uint32_t ck_clip_live[4];\n" + "\n".join(helpers) + "\n"
    tail = TAIL
    if live:
        tail = tail.replace('    memset(&ck, 0, sizeof ck);',
                            '    memset(ck_clip_live, 0, sizeof ck_clip_live);\n    memset(&ck, 0, sizeof ck);')
    c_source = prelude + "\n".join(functions) + tail
    with tempfile.TemporaryDirectory(prefix="x0x37-strum-") as d:
        cfile = Path(d, "strum.c")
        exe = Path(d, "strum")
        cfile.write_text(c_source, encoding="utf-8")
        flags = os.environ.get("CFLAGS", "-O2 -Wall -Wextra -Werror").split()
        print("source=", args.source, "cc=", args.cc, "flags=", flags,
              "snapshot=", bool(snapshot), flush=True)
        comp = subprocess.run([args.cc, "-std=c99", *flags, str(cfile), "-o", str(exe)],
                              check=False)
        if comp.returncode:
            return comp.returncode
        return subprocess.run([str(exe)], check=False).returncode

if __name__ == "__main__":
    raise SystemExit(main())
