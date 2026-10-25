#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Real SMC queue/poll/decoder/Solo C: preset switching cannot strand held Solo.

DAW/User switches apply immediately. Because an old-preset NoteOff can be
filtered out by the preset-at-ingress guard, a held temporary Solo must not
survive the preset boundary. Check direct BLE and host MIDI, unchanged preset,
fresh-mode input and idle stability. This is a mocked-host, not device test.
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
    match = re.search(r'^[^\n]*\b' + re.escape(name) + r'\([^\n]*\)\s*\{', source, re.M)
    if not match:
        raise RuntimeError('Missing production function ' + name)
    pos, depth = match.end(), 1
    while depth:
        if pos >= len(source):
            raise RuntimeError('Unterminated production function ' + name)
        depth += (source[pos] == '{') - (source[pos] == '}')
        pos += 1
    return source[match.start():pos]


queue_definitions = source[source.index('#define SMC_Q'):source.index('static int smc_is_solo_release')]
solo_state = source[source.index('static struct {'):source.index('int smc_project_active')]
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
static unsigned solos_on, solos_off, solos_published;
static int busy, testing, noteon, noteoff, faders, knobs, buttons;
static int perf_testing(void) { return testing; }
static int ui_project_busy(void) { return busy; }
static void ui_say(const char *a, const char *b) { (void)a; (void)b; }
static void engine_smc_solo(int x) { (void)x; solos_on = 0; ++solos_off; }
static void engine_smc_solo_targets(uint8_t p, uint16_t a, uint16_t b)
{ solos_on = (unsigned)p | a | b; ++solos_published; }
''' + queue_definitions + '\n' + extract('smc_is_solo_release') + '\n' + solo_state + '\n' + '\n'.join(extract(n) for n in (
    'smc_project_active', 'smc_solo_clear', 'smc_solo_publish', 'smc_solo_press',
    'smc_project_drop', 'smc_ble_midi', 'smc_ble_link', 'smc_midi_host')) + r'''
static void smc_fader(int n, uint8_t v) { (void)n; (void)v; ++faders; }
static void smc_knob(int n, int delta) { (void)n; (void)delta; ++knobs; }
static void smc_button(uint8_t note, int mmc)
{
 ++buttons;
 if (note >= 8u && note <= 15u) { ++noteon; smc_solo_press((int)(note - 8u), 1, mmc); }
}
''' + extract('smc_release') + r'''
static void smc_guarded_button(uint8_t note, int mmc, int allow)
{ if (allow) smc_button(note, mmc); }
''' + extract('smc_message') + r'''
static void smc_mmc_drain_stop(int src, int allow)
{ (void)src; (void)allow; }
''' + extract('smc_poll') + r'''
#define CHECK(ok, why) do { if (!(ok)) { \
 fprintf(stderr,"FAIL %s line %d: %s, mode=%u solo=%u counts=%d/%d q=%u/%u host=%u/%u\n", \
 why,__LINE__,#ok,proj.set.bt,solos_on,noteon,noteoff,smc_qr,smc_qw,smc_host_r,smc_host_w); return 0; \
 } ++checks; } while (0)
static unsigned checks;
static void set_mode(int user)
{ proj.set.bt = (uint8_t)(PROJ_BT_SMC_CENTRAL | (user ? PROJ_BT_SMC_USER : 0)); }
static void ble_note(int user, int on)
{
 const uint8_t m[3] = {(uint8_t)(on ? (user ? 0x9F : 0x90) : (user ? 0x8F : 0x80)),
                       8u, (uint8_t)(on ? 127u : 0u)};
 smc_ble_midi(m, sizeof m);
}
static void host_note_at(uint8_t note, int on)
{
 uint32_t p = (uint32_t)(on ? 0x9Fu : 0x8Fu) << 8 | (uint32_t)note << 16 |
              (uint32_t)(on ? 127u : 0u) << 24;
 (void)smc_midi_host(p);
}
static void host_note(int on) { host_note_at(8u,on); }
static void reset(uint8_t cursor, int user)
{
 memset(&smc,0,sizeof smc);
 memset(&ui,0,sizeof ui);
 memset((void *)smc_q,0,sizeof smc_q);
 smc_qw=smc_qr=cursor;
 smc_host_w=smc_host_r=cursor;
 smc_mmc_w[0]=smc_mmc_r[0]=0;
 smc_mmc_w[1]=smc_mmc_r[1]=0;
 smc_mmc_stop_w[0]=smc_mmc_stop_r[0]=0;
 smc_mmc_stop_w[1]=smc_mmc_stop_r[1]=0;
 smc_ble_solo_release_lost=smc_host_solo_release_lost=0;
 smc_ble_solo_release_seen=smc_host_solo_release_seen=0;
 smc_link_loss_seq=smc_link_published=smc_link_seen=0;
 smc_link_ble_loss_boundary=smc_link_host_loss_boundary=0;
 solos_on=solos_off=solos_published=0;
 busy=testing=noteon=noteoff=faders=knobs=buttons=0;
 set_mode(user);
 smc_ble_link(1);
 smc_poll();
 solos_off=0;
}
static int scenario(uint8_t cursor, int from, int route, int fresh)
{
 reset(cursor,from);
 if (route) host_note(1); else ble_note(from,1);
 smc_poll();
 CHECK(solos_on && smc_project_active(),"NoteOn must acquire temporary Solo");
 int old_count=noteon;
 smc_poll();
 CHECK(solos_on && noteon==old_count,"steady preset cannot release held Solo");
 set_mode(!from);
 /* A subsequent NoteOff has the *new* preset bit, but the held key belongs
  * to the *previous* map. Main loop must clear it at the preset boundary. */
 if (!fresh && !route) ble_note(from,0);
 if (fresh) ble_note(!from,1);
 smc_poll();
 CHECK(solos_off==1,"preset boundary must end old Solo before new input");
 if (fresh) {
  CHECK(solos_on && noteon==old_count+1,"fresh new-preset press must survive");
  CHECK(smc.solo_target[0]==1u,"new press must own physical Solo key");
 } else {
  CHECK(!solos_on && !smc_project_active(),"old-preset Solo must be cleared");
  CHECK(!smc.solo_target[0],"old physical Solo latch must be released");
 }
 CHECK(smc_qr==smc_qw,"BLE ring drained");
 CHECK(smc_host_r==smc_host_w,"host ring drained");
 smc_poll();
 CHECK(fresh ? (solos_on != 0u) : (solos_on == 0u),"idle poll preserves result");
 return 1;
}
static int idle_switch(uint8_t cursor, int user)
{
 reset(cursor,user);
 smc.known[8]=1u;
 smc.seen=0xFFu;
 smc.picked=0xFFu;
 smc.knob_i=7u;
 set_mode(!user);
 smc_poll();
 CHECK(!smc_project_active() && !solos_on && !solos_off,
       "idle switch must not invent a Solo release");
 CHECK(!smc.seen && !smc.picked && !smc.knob_i && !smc.known[8],
       "preset switch must reset DAW/User pickup and feedback cache");
 return 1;
}
/* Old host NoteOff queued before a BLE link boundary must not clear a
 * new direct-BLE Solo held after reconnect, even when User mode stays on. */
static int stale_host_keyup_link(uint8_t cursor, int fresh_host)
{
 reset(cursor,1);
 host_note(1);
 smc_poll();
 CHECK(solos_on && smc.solo_target[0],"old User-host press holds Solo");
 host_note(0);               /* queued but not yet decoded */
 smc_ble_link(0);
 smc_ble_link(1);             /* both edges before one poll */
 if (fresh_host) host_note(1);
 else ble_note(1,1);          /* fresh BLE press on same strip */
 smc_poll();
 CHECK(solos_on && smc.solo_target[0],"fresh Solo must survive stale pre-edge host NoteOff");
 if (fresh_host) host_note(0); else ble_note(1,0);
 smc_poll();
 CHECK(!solos_on && !smc.solo_target[0],"fresh matched key-up must release Solo");
 return 1;
}
static int ordinary_release(uint8_t cursor, int user, int route)
{
 reset(cursor,user);
 if (route) host_note(1); else ble_note(user,1);
 smc_poll();
 CHECK(solos_on && smc.solo_target[0],"ordinary press");
 if (route) host_note(0); else ble_note(user,0);
 smc_poll();
 CHECK(!solos_on && !smc.solo_target[0],"ordinary release unchanged");
 return 1;
}
/* An old queued host Solo NoteOn must not resurrect Solo after a BLE link
 * reset, particularly when the last old NoteOff overflowed its 32-slot FIFO.
 * Non-Solo host notes (e.g. MUTE) and new-link Solo messages must still work. */
static int pre_edge_host_cutoff(uint8_t cursor, int overflow, int up, int fresh)
{
 reset(cursor,1);
 host_note_at(24u,1);        /* unrelated User-mode host control, preserved */
 unsigned pre_edge_solos = overflow ? 31u : 1u;
 for(unsigned j=0;j<pre_edge_solos;j++) host_note(1);
 host_note(0);               /* overflows only in saturated scenario */
 CHECK(smc_host_solo_release_lost == (unsigned)overflow,
       "producer records precisely the overflowed old release");
 smc_ble_link(0);
 if(up) smc_ble_link(1);     /* also test a coalesced disconnect/reconnect */
 smc_poll();
 CHECK(!solos_on && !smc.solo_target[0] && !smc_project_active(),
       "pre-edge host Solo presses must not re-latch after BLE link reset");
 CHECK(buttons == 1, "pre-edge non-Solo host control survives the link edge");
 CHECK(smc_host_r == smc_host_w, "old host ring drains at cursor wrap");
 CHECK(smc_host_solo_release_seen == smc_host_solo_release_lost,
       "old overflow release generation retired exactly once");
 smc_poll();
 CHECK(!solos_on && buttons==1, "no stale Solo or duplicate host control on idle poll");
 if(fresh) {
  if(fresh==1) host_note(1);
  else if(up) ble_note(1,1);
  else return 1;             /* direct BLE is disconnected in this case */
  smc_poll();
  CHECK(solos_on && smc.solo_target[0], "fresh host or BLE Solo still works");
  if(fresh==1) host_note(0); else ble_note(1,0);
  smc_poll();
  CHECK(!solos_on && !smc.solo_target[0], "fresh matched Solo key-up works");
 }
 return 1;
}
int main(void)
{
 unsigned cases=0;
 const uint8_t cursors[] = {0u,1u,31u,127u,240u,255u};
 for (unsigned i=0;i<sizeof cursors;i++) {
  for(int overflow=0;overflow<2;overflow++)
   for(int up=0;up<2;up++)
    for(int fresh=0;fresh<3;fresh++) {
     if(!pre_edge_host_cutoff(cursors[i],overflow,up,fresh)) return 1;
     ++cases;
    }
  for (int from=0;from<2;from++) {
   for(int fresh=0;fresh<2;fresh++) {
    if(!scenario(cursors[i],from,0,fresh)) return 1;
    ++cases;
   }
  }
  for(int fresh=0;fresh<2;fresh++) {
   if(!scenario(cursors[i],1,1,fresh)) return 1;
   ++cases;
  }
  for(int user=0;user<2;user++) {
   if(!ordinary_release(cursors[i],user,0)) return 1;
   ++cases;
   if(!idle_switch(cursors[i],user)) return 1;
   ++cases;
  }
  if(!ordinary_release(cursors[i],1,1)) return 1;
  ++cases;
  for(int fresh_host=0;fresh_host<2;fresh_host++) {
   if(!stale_host_keyup_link(cursors[i],fresh_host)) return 1;
   ++cases;
  }
 }
 printf("SMC preset Solo lifetime: %u/%u scenarios PASS, %u checks\n",cases,cases,checks);
 return 0;
}
'''

with tempfile.TemporaryDirectory(prefix='smc-preset-solo-') as tmp:
    test_c = Path(tmp) / 'test.c'
    executable = Path(tmp) / 'test'
    test_c.write_text(code, encoding='utf-8')
    argv = [os.environ.get('CC','cc'),'-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-variable',
            *shlex.split(os.environ.get('SAN_FLAGS','')),
            '-I'+str(ROOT/'firmware/src'),str(test_c),'-o',str(executable)]
    subprocess.run(argv,check=True)
    subprocess.run([str(executable)],check=True)
