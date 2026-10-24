#!/usr/bin/env python3
"""SMC BLE link epoch guard: mid-poll ISR reconnect must not eat new-link Solo.

Execute the actual production smc_poll(), ble queue producer, link publication,
project-drop functions from SMC_SOURCE with mocked control/UI effects.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = Path(os.environ.get('SMC_SOURCE', ROOT / 'firmware/src/app/smc_mixer.c')).read_text()


def extract(name):
    pattern = r'^[^\n]*\b' + re.escape(name) + r'\([^\n]*\)\s*\{'
    m = re.search(pattern, source, re.M)
    if not m:
        raise RuntimeError('Missing production function ' + name)
    pos, depth = m.end(), 1
    while depth:
        if pos >= len(source):
            raise RuntimeError('Unterminated production function ' + name)
        depth += (source[pos] == '{') - (source[pos] == '}')
        pos += 1
    return source[m.start():pos]


prefix = source[source.index('#define SMC_Q'):source.index('static int smc_is_solo_release')]
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
#define O_ASK 1
#define O_NONE 0
typedef struct { uint8_t kind, a, b, c; } pref_t;
static struct { struct { uint8_t bt; } set; } proj;
static struct { int overlay; } ui;
static struct { uint8_t up, fresh, picked, seen, knob_i, pending, known[128], preset_user_seen; } smc;
static int busy, testing, injected, inject_stale, trace_count, held_note;
static uint8_t trace[5];
static int perf_testing(void) { return testing; }
static int ui_project_busy(void) { return busy; }
static int smc_project_active(void) { return held_note != 0; }
static void smc_solo_clear(void) { held_note = 0; }
static void ui_say(const char *a, const char *b) { (void)a; (void)b; }
static int smc_is_solo_release(uint8_t st, uint8_t note, uint8_t velocity)
{ (void)st; (void)note; (void)velocity; return 0; }
''' + prefix + '\n' + extract('smc_project_drop') + '\n' + extract('smc_ble_midi') + '\n' + extract('smc_ble_link') + r'''
static uint8_t smc_message(const uint8_t *msg, uint8_t len, int allow)
{
 assert(len == 3 && allow);
 assert(trace_count < (int)sizeof trace);
 trace[trace_count++] = msg[1];
 if (msg[0] == 0x90 && msg[2]) held_note = msg[1];
 if (!injected) {
  injected = 1;
  if (inject_stale) {
   const uint8_t stale[3] = {0x90, 10, 127};
   smc_ble_midi(stale, 3); /* published on OLD BLE session */
  }
  /* TIMER5 disconnect+reconnect happens while main loop dispatches the first
   * old-link event, then receives a NEW-link Solo NoteOn on the same ring. */
  smc_ble_link(0);
  smc_ble_link(1);
  const uint8_t fresh[3] = {0x90, 9, 127};
  smc_ble_midi(fresh, 3);
 }
 return 0;
}
static void smc_guarded_button(uint8_t note, int mmc, int allow)
{ (void)note; (void)mmc; (void)allow; }
static void smc_mmc_drain_stop(int src, int allow)
{ (void)src; (void)allow; }
''' + extract('smc_poll') + r'''
static void prep(uint8_t start, int stale)
{
 (void)smc_mmc_stop_q; /* Keep the extracted production ring declarations compiled. */
 memset(&smc, 0, sizeof smc);
 memset(&ui, 0, sizeof ui);
 proj.set.bt = PROJ_BT_SMC_CENTRAL;
 busy = testing = injected = held_note = trace_count = 0;
 inject_stale = stale;
 memset(trace, 0, sizeof trace);
 smc_qw = smc_qr = start;
 smc_host_w = smc_host_r = 0;
 smc_mmc_w[0] = smc_mmc_r[0] = 0;
 smc_mmc_w[1] = smc_mmc_r[1] = 0;
 smc_mmc_stop_w[0] = smc_mmc_stop_r[0] = 0;
 smc_mmc_stop_w[1] = smc_mmc_stop_r[1] = 0;
 smc_ble_solo_release_lost = smc_host_solo_release_lost = 0;
 smc_ble_solo_release_seen = smc_host_solo_release_seen = 0;
 smc_link_loss_seq = smc_link_published = smc_link_seen = 0;
 smc_link_ble_loss_boundary = smc_link_host_loss_boundary = 0;
 smc_ble_link(1);
 smc_poll(); /* synchronize link epoch without any queued MIDI */
}
static int run(uint8_t start, int stale)
{
 prep(start, stale);
 const uint8_t old[3] = {0x90, 8, 127};
 smc_ble_midi(old, 3);
 smc_poll();
 if (trace_count != 1 || trace[0] != 8 || held_note != 8 ||
     smc_qr == smc_qw) {
  fprintf(stderr,"FAIL midpoll consumed a new-link note or old-link stale: count=%d held=%d qr=%u qw=%u start=%u stale=%d\n",
          trace_count, held_note, smc_qr, smc_qw, start, stale);
  return 0;
 }
 smc_poll(); /* link boundary must clear the old hold, drop stale, deliver new */
 if (trace_count != 2 || trace[1] != 9 || held_note != 9 ||
     smc_qr != smc_qw || !smc.up) {
  fprintf(stderr,"FAIL newly connected Solo was not delivered after epoch: count=%d held=%d qr=%u qw=%u start=%u stale=%d\n",
          trace_count, held_note, smc_qr, smc_qw, start, stale);
  return 0;
 }
 smc_poll(); /* no duplicate replay */
 if (trace_count != 2 || held_note != 9) {
  fprintf(stderr,"FAIL duplicate/dropped new-link Solo after idle poll\n");
  return 0;
 }
 return 1;
}
int main(void)
{
 int count=0;
 for(int stale=0;stale<2;stale++) {
  for(unsigned i=0;i<8;i++) {
   const uint8_t cursors[8] = {0u,1u,15u,31u,64u,127u,240u,255u};
   if (!run(cursors[i], stale)) return 1;
   count++;
  }
 }
 assert(count == 16);
 printf("SMC BLE midpoll link-epoch source-window: %d/16 PASS\n", count);
 return 0;
}
'''

with tempfile.TemporaryDirectory(prefix='smc-ble-midpoll-') as td:
    test_c = Path(td) / 'test.c'
    exe = Path(td) / 'test'
    test_c.write_text(code)
    command = [os.environ.get('CC', 'cc'), '-std=c99', '-O1', '-g', '-Wall',
               '-Wextra', '-Werror', *shlex.split(os.environ.get('SAN_FLAGS', '')),
               '-I' + str(ROOT / 'firmware/src'), str(test_c), '-o', str(exe)]
    subprocess.run(command, check=True)
    subprocess.run([str(exe)], check=True)
