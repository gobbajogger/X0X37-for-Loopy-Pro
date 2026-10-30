#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Native source-window MIDI lifecycle regression: delayed CLIP KEYS on project reset.

Compiles unchanged production functions from main_fm1.c, exercising real queue,
reset, owner bitmap and all-off paths with a deterministic MIDI sink. Does not
claim target compilation, the full main loop, BLE delivery or physical hardware.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

FUNCTIONS = ('ck_send', 'ck_q_run', 'ck_arp_stop_note', 'ck_on', 'ck_clip_on',
             'ck_clip_off', 'ck_key_send', 'ck_voice', 'ck_all_off', 'smk_project_reset')


def source_window(src, name):
    pat = (r'\b(?:CK_FN\s+)?(?:__attribute__\(\(minsize\)\)\s+)?'
           r'(?:static\s+)?void\s+' + re.escape(name) + r'\s*\([^;]*?\)\s*\{')
    m = re.search(pat, src)
    if not m:
        raise ValueError(f'{name}: missing production C function')
    depth = 0
    for i in range(m.end() - 1, len(src)):
        if src[i] == '{':
            depth += 1
        elif src[i] == '}':
            depth -= 1
            if not depth:
                return src[m.start():i + 1]
    raise ValueError(f'{name}: unterminated production function')


PRELUDE = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define CK_FN
#define CK_NKEY 37u
#define CK_MAXN 7u
#define CK_QN 24u
#define CK_M_ARP 1u
#define CK_BIT(map, k) (((map)[(k) >> 5] >> ((k) & 31u)) & 1u)
#define CK_BASE ck_t *cp = &ck
#define SMK_APC40_PAD_COUNT 2u
#define SMK_BTN_KNOB_BANK 5u
typedef struct {
    uint8_t on, ch, arp_note, vel, nk, btn_prev;
    uint16_t last_ms;
    uint32_t snd[2], keys[2];
} ck_t;
typedef struct { uint8_t ck_x1; } settings_t;
typedef struct { settings_t set; } project_t;
typedef struct { uint32_t keys[2], pnotes, buttons; } fm1_input_t;
static ck_t ck;
static project_t proj;
static fm1_input_t fm1_in;
static uint32_t fm1_ms;
static uint16_t ck_qe[CK_QN];
static uint8_t ck_qt[CK_QN], ck_qn;
static uint8_t ck_start_req, clip_pad_active, clip_grid_mode, clip_pad_note[SMK_APC40_PAD_COUNT];
static uint32_t clip_pad_keys;
static uint8_t knob_bank_prev, was_on;
static uint32_t human_delay;
static int on[128], off[128], ch_on[128], ch_off[128], checks, failures;
static void apc_out(uint32_t m) {
    uint32_t st = (m >> 8) & 255u, note = (m >> 16) & 127u;
    if ((st & 0xf0u) == 0x90u) { on[note]++; ch_on[note] = (int)(st & 15u); }
    else if ((st & 0xf0u) == 0x80u) { off[note]++; ch_off[note] = (int)(st & 15u); }
}
static void apc_note(uint32_t n, uint32_t v) { (void)n; (void)v; }
static uint32_t ck_human(uint32_t h, uint32_t seed, uint32_t vel, uint32_t *late) {
    (void)h; (void)seed; *late = human_delay; return vel;
}
static int ck_q_due(uint32_t t, uint32_t now) { return (uint8_t)(now - t) < 128u; }
static uint32_t ck_key_notes(uint32_t key, uint8_t *out) {
    if (key == 0u || key == 1u) { out[0] = 60; out[1] = 64; return 2; }
    out[0] = (uint8_t)(48u + key); return 1;
}
static void ck_q_run(uint32_t only);
'''

TAIL = r'''
#define CHECK(pred, label) do { checks++; if (!(pred)) { failures++; \
    fprintf(stderr, "FAIL %s at line %d\n", label, __LINE__); } } while (0)
static void start(void) {
    memset(&ck, 0, sizeof ck);
    memset(ck_qe, 0, sizeof ck_qe);
    memset(ck_qt, 0, sizeof ck_qt);
    memset(ck_clip_live, 0, sizeof ck_clip_live);
    memset(on, 0, sizeof on);
    memset(off, 0, sizeof off);
    memset(ch_on, 0, sizeof ch_on);
    memset(ch_off, 0, sizeof ch_off);
    ck_qn = 0;
    ck.ch = 7;
    ck.vel = 100;
    proj.set.ck_x1 = 0;
    clip_pad_active = 0;
    clip_grid_mode = 0;
    human_delay = 0;
}
static void press(uint32_t key) {
    ck.snd[key >> 5] |= 1u << (key & 31u);
    ck_key_send(key, 0x91u);
}
static void release(uint32_t key) {
    ck.snd[key >> 5] &= ~(1u << (key & 31u));
    ck_key_send(key, 0x81u);
}
int main(void) {
    start(); human_delay = 15;
    press(0);
    CHECK(ck_qn == 2 && !on[60] && !on[64], "HUMAN pending and nothing transmitted");
    smk_project_reset();
    CHECK(ck_qn == 0 && on[60] == 0 && on[64] == 0, "project reset discards pending NoteOns");
    CHECK(off[60] == 0 && off[64] == 0, "project reset never sends unsolicited NoteOff");
    CHECK(ck_clip_live[60 >> 5] == 0 && ck.snd[0] == 0, "reset clears ownership");

    start(); human_delay = 0;
    ck_clip_on(60);
    human_delay = 15; ck_clip_on(64); ck.snd[0] = 1;
    CHECK(on[60] == 1 && !on[64] && ck_qn == 1, "mix sounding and delayed pitches");
    smk_project_reset();
    CHECK(on[60] == 1 && off[60] == 1, "sounding pitch receives required NoteOff");
    CHECK(on[64] == 0 && off[64] == 0, "cancelled pitch receives no NoteOff");
    CHECK(ch_on[60] == 7 && ch_off[60] == 7, "reset uses original MIDI channel");

    start(); human_delay = 15; press(0);
    ck_q_run(0x100u); /* a legitimate queued NOTE ON has now left the keyboard */
    CHECK(on[60] == 1 && on[64] == 1 && ck_qn == 0, "delayed notes actually transmitted");
    smk_project_reset();
    CHECK(off[60] == 1 && off[64] == 1, "transmitted queued notes get NoteOff");

    start(); human_delay = 15; press(0);
    ck_all_off();  /* normal panic is deliberately flush-then-off */
    CHECK(on[60] == 1 && on[64] == 1 && off[60] == 1 && off[64] == 1,
          "normal all-off preserves matched MIDI events");
    CHECK(ck_qn == 0 && ck_clip_live[60 >> 5] == 0, "panic clears queue and bitmap");

    start(); ck.on = CK_M_ARP; human_delay = 15;
    ck_on(67); ck.arp_note = 68;
    smk_project_reset();
    CHECK(on[67] == 0 && off[67] == 0, "queued ARP note cancelled without unsolicited NoteOff");

    start(); ck.on = CK_M_ARP; human_delay = 0;
    ck_on(67); ck.arp_note = 68;
    smk_project_reset();
    CHECK(on[67] == 1 && off[67] == 1, "audible ARP note properly released");

    start(); human_delay = 0;
    press(0); press(1); release(0);
    CHECK(on[60] == 1 && on[64] == 1 && !off[60] && !off[64], "shared pitches stay on");
    release(1);
    CHECK(off[60] == 1 && off[64] == 1, "shared pitches balance on final key release");

    start(); human_delay = 15; press(0); smk_project_reset();
    human_delay = 0; press(0); release(0);
    CHECK(on[60] == 1 && off[60] == 1 && on[64] == 1 && off[64] == 1,
          "first note after reset can sound and release normally");

    printf("clip_key_project_reset: %d observations, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
'''


def main():
    p = argparse.ArgumentParser()
    p.add_argument('source', nargs='?', default='firmware/src/app/main_fm1.c')
    p.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    args = p.parse_args()
    src = Path(args.source).read_text(encoding='utf8')
    if 'static uint32_t ck_clip_live[4];' not in src:
        raise ValueError('CLIP KEYS live bitmap absent')
    functions = [source_window(src, n) for n in FUNCTIONS]
    body = PRELUDE + '\nstatic uint32_t ck_clip_live[4];\n' + '\n\n'.join(functions) + TAIL
    with tempfile.TemporaryDirectory(prefix='x0x37-ckreset-') as d:
        cpath, binary = Path(d, 'ck_reset.c'), Path(d, 'ck_reset')
        cpath.write_text(body, encoding='utf8')
        flags = os.environ.get('CFLAGS', '-O2 -Wall -Wextra -Werror').split()
        # GCC treats JieLi's minsize attribute as unknown; retain source bytes.
        flags.append('-Wno-attributes')
        print(f'compiler={args.cc} flags={flags} production={args.source}', flush=True)
        cp = subprocess.run([args.cc, '-std=c99', *flags, str(cpath), '-o', str(binary)], check=False)
        if cp.returncode:
            return cp.returncode
        return subprocess.run([str(binary)], check=False).returncode


if __name__ == '__main__':
    raise SystemExit(main())
