#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Direct BLE SMC preset-at-ingress regression using production queue and poll.

The SMC preset changes immediately. A queued DAW event must not be interpreted
as a fresh User event (or vice versa). Exercise the exact production producer,
consumer, and decoder with modeled UI effects; no device/build claim.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(os.environ.get('SMC_SOURCE', ROOT / 'firmware/src/app/smc_mixer.c'))
source = SOURCE.read_text(encoding='utf-8')


def extract(name):
    pat = r'^[^\n]*\b' + re.escape(name) + r'\([^\n]*\)\s*\{'
    m = re.search(pat, source, re.M)
    if not m:
        raise RuntimeError('Missing production function: ' + name)
    pos, depth = m.end(), 1
    while depth:
        if pos == len(source):
            raise RuntimeError('Unterminated production function: ' + name)
        depth += (source[pos] == '{') - (source[pos] == '}')
        pos += 1
    return source[m.start():pos]


# Capture the exact queue declarations, including the packed event type and
# mode marker (if any). Use real smc_ble_midi, smc_poll and smc_message.
queues = source[source.index('#define SMC_Q'):source.index('static int smc_is_solo_release')]
code = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app/smc_protocol.h"
#define X0X_COLD
#define PROJ_BT_SMC_CENTRAL 16u
#define PROJ_BT_SMC_USER 32u
#define NKIT 2
#define NPARTS 5
#define NDRUM 11
#define O_NONE 0
#define O_ASK 1
static struct { struct { uint8_t bt; } set; } proj;
static struct { int overlay; } ui;
static struct { uint8_t up, fresh, picked, seen, knob_i, pending, known[128]; } smc;
static int busy, testing, calls, knobs, faders, buttons, releases;
static uint8_t last_note;
static int perf_testing(void) { return testing; }
static int ui_project_busy(void) { return busy; }
static int smc_project_active(void) { return 0; }
static void smc_solo_clear(void) { }
static void ui_say(const char *a, const char *b) { (void)a; (void)b; }
static int smc_is_solo_release(uint8_t st, uint8_t note, uint8_t vel)
{ (void)st; (void)note; (void)vel; return 0; }
''' + queues + '\n' + extract('smc_project_drop') + '\n' + extract('smc_ble_midi') + '\n' + extract('smc_ble_link') + r'''
static void smc_fader(int n, uint8_t v) { (void)n; (void)v; ++calls; ++faders; }
static void smc_knob(int n, int d) { (void)n; (void)d; ++calls; ++knobs; }
static void smc_button(uint8_t note, int mmc)
{ (void)mmc; last_note = note; ++calls; ++buttons; }
static void smc_release(uint8_t note)
{ last_note = note; ++calls; ++releases; }
static void smc_guarded_button(uint8_t note, int mmc, int allow)
{ if (allow) smc_button(note, mmc); }
''' + extract('smc_message') + r'''
static void smc_mmc_drain_stop(int src, int allow)
{ (void)src; (void)allow; }
''' + extract('smc_poll') + r'''
static void reset(uint8_t start)
{
    (void)smc_mmc_stop_q; /* Referenced by production queue even when STOP is mocked. */
    memset(&smc, 0, sizeof smc);
    proj.set.bt = PROJ_BT_SMC_CENTRAL;
    busy = testing = calls = knobs = faders = buttons = releases = 0;
    last_note = 0;
    ui.overlay = O_NONE;
    smc_qw = smc_qr = start;
    smc_host_w = smc_host_r = 0;
    memset((void *)smc_mmc_w, 0, sizeof smc_mmc_w);
    memset((void *)smc_mmc_r, 0, sizeof smc_mmc_r);
    memset((void *)smc_mmc_stop_w, 0, sizeof smc_mmc_stop_w);
    memset((void *)smc_mmc_stop_r, 0, sizeof smc_mmc_stop_r);
    smc_ble_solo_release_lost = smc_host_solo_release_lost = 0;
    smc_ble_solo_release_seen = smc_host_solo_release_seen = 0;
    smc_link_loss_seq = smc_link_published = smc_link_seen = 0;
    smc_link_ble_loss_boundary = smc_link_host_loss_boundary = 0;
    smc_ble_link(1);
    smc_poll(); /* sync BLE link before actual event tests */
}
static void set_mode(int user)
{ proj.set.bt = (uint8_t)(PROJ_BT_SMC_CENTRAL | (user ? PROJ_BT_SMC_USER : 0)); }
static void add_knob(void)
{ static const uint8_t k[3] = {0xB0, 0x10, 0x01}; smc_ble_midi(k, 3); }
static void add_button(void)
{ static const uint8_t k[3] = {0x90, 0x5E, 0x7F}; smc_ble_midi(k, 3); }
static void add_daw_fader(void)
{ static const uint8_t k[3] = {0xE0, 0x7F, 0x7F}; smc_ble_midi(k, 3); }
static void add_user_fader(void)
{ static const uint8_t k[3] = {0xB0, 0x28, 0x40}; smc_ble_midi(k, 3); }
static void add_user_mmc(void)
{ static const uint8_t k[6] = {0xF0, 0x7F, 0x7F, 0x06, 0x01, 0xF7}; smc_ble_midi(k, 6); }
static int mmc_case(uint8_t cursor, int before, int after)
{
    reset(cursor);
    set_mode(before);
    if (before) add_user_mmc();
    set_mode(after);
    if (after) add_user_mmc();
    smc_poll();
    int expected = after ? (before ? 2 : 1) : 0;
    if (calls != expected || buttons != expected || knobs || faders ||
        last_note != (expected ? 0x10u : 0u) || smc_qr != smc_qw) {
        fprintf(stderr, "FAIL stock MMC preset pre=%d post=%d cursor=%u "
                "calls=%d expected=%d q=%u/%u\n",
                before, after, cursor, calls, expected, smc_qr, smc_qw);
        return 0;
    }
    return 1;
}
static int scenario(uint8_t cursor, int before, int after, int extra)
{
    reset(cursor);
    set_mode(before);
    add_knob();  /* CC 16 accepted in both presets: detects wrong-epoch replay */
    add_button(); /* even common button should never be delayed across preset */
    if (before) add_user_fader();
    else add_daw_fader();
    set_mode(after);
    if (extra) {
        add_knob();  /* queued after the change, this must survive */
        add_button();
        if (after) add_user_fader();
        else add_daw_fader();
    }
    smc_poll();
    int expected = (before == after ? 3 : 0) + (extra ? 3 : 0);
    if (calls != expected || knobs != expected/3 || buttons != expected/3 ||
        faders != expected/3 || smc_qr != smc_qw) {
        fprintf(stderr, "FAIL queue preset pre=%d post=%d fresh=%d cursor=%u "
                "calls=%d knob=%d buttons=%d faders=%d expected=%d q=%u/%u\n",
                before, after, extra, cursor, calls, knobs, buttons, faders,
                expected, smc_qr, smc_qw);
        return 0;
    }
    smc_poll();
    if (calls != expected) {
        fprintf(stderr, "FAIL duplicate at idle poll\n");
        return 0;
    }
    return 1;
}
int main(void)
{
    unsigned tests = 0;
    static const uint8_t cursors[] = {0, 1, 30, 31, 127, 253, 255};
    for (int before=0; before<=1; before++)
      for (int after=0; after<=1; after++)
        for (int extra=0; extra<=1; extra++)
          for (unsigned n=0; n<sizeof(cursors); n++) {
            if (!scenario(cursors[n], before, after, extra)) return 1;
            ++tests;
          }
    for (int before=0; before<=1; before++)
      for (int after=0; after<=1; after++)
        for (unsigned n=0; n<sizeof(cursors); n++) {
          if (!mmc_case(cursors[n], before, after)) return 1;
          ++tests;
        }
    printf("SMC BLE preset queue/decoder source path: %u/%u PASS\n", tests, tests);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix='smc-ble-preset-queue-') as tmp:
    c = Path(tmp) / 'check.c'
    exe = Path(tmp) / 'check'
    c.write_text(code)
    cmd = [os.environ.get('CC', 'cc'), '-std=c11', '-O1', '-g', '-Wall', '-Wextra',
           '-Werror', *shlex.split(os.environ.get('SAN_FLAGS', '')),
           '-I' + str(ROOT / 'firmware/src'), str(c), '-o', str(exe)]
    subprocess.run(cmd, check=True)
    subprocess.run([str(exe)], check=True)
