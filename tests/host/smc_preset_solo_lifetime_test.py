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
#include <stdlib.h>
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
static int host_link_inject, host_link_reconnect, host_link_injected, host_link_fresh_inside;
/* In the fader callback, emulate a TIMER5 producer refilling the just-freed
 * BLE slot, then trying to enqueue a Solo release into the full SPSC ring. */
static int ble_overflow_inject, ble_overflow_injected, ble_overflow_zero;
static unsigned ble_overflow_channel;
static int perf_testing(void) { return testing; }
static int ui_project_busy(void) { return busy; }
static void ui_say(const char *a, const char *b) { (void)a; (void)b; }
static void engine_smc_solo(int x) { (void)x; solos_on = 0; ++solos_off; }
static void engine_smc_solo_targets(uint8_t p, uint16_t a, uint16_t b)
{ solos_on = (unsigned)p | a | b; ++solos_published; }
''' + queue_definitions + '\n' + extract('smc_is_solo_release') + '\n' + solo_state + '\n' + '\n'.join(extract(n) for n in (
    'smc_project_active', 'smc_solo_clear', 'smc_solo_publish', 'smc_solo_press',
    'smc_project_drop', 'smc_project_reset', 'smc_ble_midi', 'smc_ble_link', 'smc_midi_host')) + r'''
static void smc_fader(int n, uint8_t v)
{
 (void)n; (void)v; ++faders;
 if (host_link_inject && !host_link_injected) {
  host_link_injected = 1;
  /* TIMER5 link ISR interrupts main-loop dispatch of the first, unrelated
   * USB/User CC; a pre-edge host Solo press still waits in the host ring. */
  smc_ble_link(0);
  if (host_link_reconnect) smc_ble_link(1);
  /* The audio MIDI producer can enqueue a fresh Solo during the same
   * preempted fader dispatch. It belongs AFTER the published host cutoff. */
  if (host_link_fresh_inside) {
   uint32_t pkt = (uint32_t)0x9Fu << 8 | (uint32_t)8u << 16 |
                  (uint32_t)127u << 24;
   assert(smc_midi_host(pkt) == 1);
  }
 }
 if (ble_overflow_inject && !ble_overflow_injected) {
  const uint8_t filler[3]={0xB1u,40u,64u};
  const uint8_t release[3]={
   (uint8_t)((ble_overflow_zero ? 0x90u : 0x80u)|ble_overflow_channel),8u,0u};
  ble_overflow_injected=1;
  assert((uint8_t)(smc_qw-smc_qr)==SMC_Q-1u);
  smc_ble_midi(filler,3u); /* ISR refills the slot freed before dispatch */
  assert((uint8_t)(smc_qw-smc_qr)==SMC_Q);
  smc_ble_midi(release,3u); /* this release must be overflow-accounted */
 }
}
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
static void host_cc(void)
{
 const uint32_t pkt=(uint32_t)0xBFu<<8 | (uint32_t)40u<<16 | (uint32_t)64u<<24;
 assert(smc_midi_host(pkt) == 1);
}
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
 host_link_inject=host_link_reconnect=host_link_injected=host_link_fresh_inside=0;
 ble_overflow_inject=ble_overflow_injected=ble_overflow_zero=0;
 ble_overflow_channel=0;
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
/* The BLE ISR may switch links AFTER smc_poll sampled its generation,
 * while a normal host CC is dispatched ahead of an old queued Solo press.
 * The old press must wait for the next poll's cutoff, not transiently mute the
 * instrument under the new link. New-link input must not be lost. */
static int midpoll_host_link(uint8_t cursor, int up, int fresh)
{
 reset(cursor,1);
 host_cc();                        /* before the link edge; valid User fader */
 host_note(1);                    /* old Solo press queued behind the fader */
 host_link_reconnect=up;
 host_link_inject=1;             /* ISR fires in smc_fader() */
 smc_poll();
 CHECK(host_link_injected && faders==1,
       "midpoll BLE edge exercised after real User-mode host CC dispatch");
 CHECK(!solos_on && !smc.solo_target[0],
       "old queued host Solo must not execute after midpoll link reset");
 CHECK(smc_host_r != smc_host_w,
       "deferred pre-edge Solo stays queued for next poll cutoff");
 if (fresh==1) host_note(1);
 else if (fresh==2 && up) ble_note(1,1);
 smc_poll();
 CHECK(smc_host_r == smc_host_w, "old host input retired exactly once");
 CHECK(solos_on == (unsigned)(fresh==1 || (fresh==2 && up)),
       "new-link press works; old host press cannot be replayed");
 CHECK(buttons==(fresh==1 || (fresh==2 && up)),
       "only the requested fresh Solo press reaches the decoder");
 if(fresh==1 || (fresh==2 && up)) {
  if(fresh==1) host_note(0); else ble_note(1,0);
  smc_poll();
  CHECK(!solos_on && !smc_project_active(),
        "matched new-link NoteOff releases the new Solo");
 }
 smc_poll();
 CHECK(!solos_on && smc_host_r==smc_host_w,
       "idle poll does not replay a discarded old Solo");
 return 1;
}
/* A new host Solo may arrive INSIDE the preempted fader callback, just
 * after TIMER5 publishes the link cutoff and before the old host ring is
 * drained. It must remain distinguishable from the old pending NoteOn,
 * even with an old NoteOff and a coalesced reconnect in the same callback. */
static int midpoll_fresh_during_link(uint8_t cursor, int up, int old_release)
{
 reset(cursor,1);
 host_cc();
 host_note(1);                /* old-session Solo still queued */
 if (old_release) host_note(0);
 host_link_reconnect=up;
 host_link_inject=1;
 host_link_fresh_inside=1;   /* enqueue post-cutoff Solo in real callback */
 smc_poll();
 CHECK(host_link_injected && faders==1,
       "link edge and new host input really occurred during fader dispatch");
 CHECK(!solos_on && !smc.solo_target[0] && buttons==0,
       "old Solo must not run before next poll sees the new link");
 CHECK(smc_host_r!=smc_host_w,
       "old Solo and fresh Solo remain queued after midpoll edge");
 smc_poll();
 CHECK(smc_host_r==smc_host_w,"host ring finishes after reconnection");
 CHECK(solos_on && smc.solo_target[0] && buttons==1,
       "fresh post-edge host Solo must survive cutoff; old Solo skipped");
 host_note(0);
 smc_poll();
 CHECK(!solos_on && !smc.solo_target[0] && !smc_project_active(),
       "new host NoteOff releases just the fresh Solo");
 smc_poll();
 CHECK(!solos_on && smc_host_r==smc_host_w,
       "no stale re-latch after the fresh host release");
 return 1;
}
/* Saturating the BLE central's SPSC ring with unrelated instrument-channel
 * traffic must not turn a rejected foreign NoteOff into a lost *SMC* Solo
 * release. The decoder already rejects channels 2..15 in User mode, so its
 * producer's overflow accounting must obey that exact same channel policy.
 * Accepted channels must still fail safe if their legitimate release is lost.
 */
static int ble_foreign_overflow(uint8_t cursor, int user, unsigned ch, int zero_on)
{
 reset(cursor,user);
 ble_note(user,1);
 smc_poll();
 CHECK(solos_on && smc.solo_target[0],"initial valid Solo is held");
 int old_presses=noteon;
 uint32_t lost_before=smc_ble_solo_release_lost;
 uint8_t dropped_before=smc_dropped;
 const uint8_t filler[3]={0xB1u,40u,64u};
 for(unsigned j=0;j<SMC_Q;j++) smc_ble_midi(filler,3u);
 CHECK((uint8_t)(smc_qw-smc_qr)==SMC_Q,"foreign filler saturates real BLE FIFO");
 const uint8_t release[3] = {(uint8_t)((zero_on ? 0x90u : 0x80u)|ch),8u,0u};
 smc_ble_midi(release,3u);       /* rejected by full ring, not decoder */
 int owned=(ch==0u || (user && ch==15u));
 CHECK(smc_ble_solo_release_lost-lost_before==(uint32_t)owned,
       "only an accepted SMC channel may register an overflowed Solo release");
 CHECK((uint8_t)(smc_dropped-dropped_before)==1u,"overflow counts dropped event");
 smc_poll();
 CHECK(smc_qr==smc_qw && noteon==old_presses,
       "foreign ring traffic drained without phantom Solo commands");
 CHECK(!!solos_on==!owned && !!smc.solo_target[0]==!owned,
       "foreign overflow cannot release Solo; owned lost NoteOff must clear it");
 smc_poll();
 CHECK(!!solos_on==!owned,"idle poll cannot replay lost-release accounting");
 if(owned) { ble_note(user,1); smc_poll(); }
 CHECK(solos_on,"fresh valid Solo remains available after overflow");
 ble_note(user,0);
 smc_poll();
 CHECK(!solos_on && !smc_project_active(),
       "genuine accepted Solo release still works after recovery");
 return 1;
}
/* TIMER5 can refill the BLE FIFO after smc_poll releases one slot but
 * before that fader callback finishes. A concurrently dropped foreign NoteOff
 * must not clear an unrelated held Solo at the next poll; an accepted lost
 * release must still clear it. Repeat at six wraparound cursor positions. */
static int ble_midpoll_foreign_overflow(uint8_t cursor, int user,
                                        unsigned channel, int zero_on)
{
 reset(cursor,user);
 ble_note(user,1);
 smc_poll();
 CHECK(solos_on && smc.solo_target[0],"valid first Solo is held");
 uint32_t lost_before=smc_ble_solo_release_lost;
 uint8_t dropped_before=smc_dropped;
 const uint8_t fader[3]={ (uint8_t)(user ? 0xBFu : 0xE0u),
                          (uint8_t)(user ? 40u : 0u),64u };
 const uint8_t filler[3]={0xB1u,40u,64u};
 smc_ble_midi(fader,3u);
 for(unsigned j=1;j<SMC_Q;j++) smc_ble_midi(filler,3u);
 CHECK((uint8_t)(smc_qw-smc_qr)==SMC_Q,"BLE FIFO starts full");
 ble_overflow_channel=channel;
 ble_overflow_zero=zero_on;
 ble_overflow_inject=1;
 smc_poll();
 int owned=(channel==0u || (user && channel==15u));
 CHECK(ble_overflow_injected && faders==1,
       "fader dispatch actually interleaves with a full-queue BLE producer");
 CHECK(smc_ble_solo_release_lost-lost_before==(uint32_t)owned,
       "midpoll overflow ignores a foreign NoteOff, counts valid releases");
 CHECK((uint8_t)(smc_dropped-dropped_before)==1u,
       "only the overflowing release was dropped");
 CHECK(solos_on && smc.solo_target[0],
       "release lost after poll's watermark cannot clear Solo prematurely");
 CHECK((uint8_t)(smc_qw-smc_qr)==1u,
       "ISR refill remains in ring until next bounded poll");
 smc_poll();
 CHECK(smc_qr==smc_qw && !!solos_on==!owned && !!smc.solo_target[0]==!owned,
       "next poll keeps foreign Solo held but fails safe for owned release");
 smc_poll();
 CHECK(!!solos_on==!owned,"idle poll never replays overflow release");
 if (owned) { ble_note(user,1); smc_poll(); }
 CHECK(solos_on,"ordinary fresh Solo works after concurrent overflow");
 ble_note(user,0);
 smc_poll();
 CHECK(!solos_on && !smc_project_active(),"ordinary release still works");
 return 1;
}
/* Pair the R19 channel-aware overflow producer with R15 reconnect epochs.
 * A queued old-link release must be retired at the link cutoff, while a
 * genuinely lost NEW-link release must still fail-safe after the new Solo
 * press. Unrelated instruments' channels must never clear either lifetime.
 * Use the actual producer, ring and poll/decoder with TIMER5 link callbacks. */
static int ble_link_overflow_lifetime(uint8_t cursor, int user,
                                      unsigned channel, int zero_on, int new_link)
{
 reset(cursor,user);
 int owned=(channel==0u || (user && channel==15u));
 uint32_t lost_before=smc_ble_solo_release_lost;
 uint8_t dropped_before=smc_dropped;
 const uint8_t filler[3]={0xB1u,40u,64u};
 const uint8_t release[3]={
  (uint8_t)((zero_on ? 0x90u : 0x80u)|channel),8u,0u};
 if(new_link) {
  smc_ble_link(0);
  smc_ble_link(1); /* both TIMER5 transitions precede consumer's poll */
  ble_note(user,1); /* fresh Solo strictly AFTER the published cutoff */
 } else {
  ble_note(user,1); /* old Solo acquired before the link change */
  smc_poll();
  CHECK(solos_on && smc.solo_target[0],"pre-edge Solo initially held");
 }
 for(unsigned j=(unsigned)new_link;j<SMC_Q;j++) smc_ble_midi(filler,3u);
 CHECK((uint8_t)(smc_qw-smc_qr)==SMC_Q,
       "real BLE ring full at both link/producer phases");
 smc_ble_midi(release,3u);
 CHECK(smc_ble_solo_release_lost-lost_before==(uint32_t)owned,
       "only accepted channel may publish lost release across link boundary");
 CHECK((uint8_t)(smc_dropped-dropped_before)==1u,
       "only overflowing NoteOff drops at link boundary");
 if(!new_link) {
  smc_ble_link(0);
  smc_ble_link(1);
 }
 smc_poll();
 CHECK(smc_qr==smc_qw,"old and new BLE queued events retired without replay");
 CHECK(smc_ble_solo_release_seen==smc_ble_solo_release_lost,
       "consumer retires the exact pre/post link lost-release generation");
 CHECK(!!solos_on==(unsigned)(new_link && !owned),
       "pre-edge Solo clears; post-edge valid release clears, foreign one does not");
 smc_poll();
 CHECK(!!solos_on==(unsigned)(new_link && !owned),
       "idle poll cannot reapply obsolete link loss or duplicate Solo");
 if(!solos_on) {
  ble_note(user,1); smc_poll();
  CHECK(solos_on && smc.solo_target[0],"fresh post-recovery Solo still works");
 }
 ble_note(user,0); smc_poll();
 CHECK(!solos_on && !smc_project_active(),
       "matching legitimate Solo release still ends held state");
 return 1;
}
/* The USB/peripheral User host ring is separate from the direct BLE ring.
 * A project replacement must acknowledge a dropped host Solo NoteOff too:
 * otherwise the previous project's release can unsolo a fresh BLE or USB
 * press in the replacement project. Exercise the real host producer, reset,
 * MIDI decoder and main-loop poll at FIFO wrap boundaries. */
static int project_reset_host_overflow(uint8_t cursor, int zero_noteon,
                                      int link_edge, int foreign, int fresh_ble)
{
 reset(cursor,1);
 host_note(1);
 smc_poll();
 CHECK(solos_on && smc.solo_target[0], "old project host Solo starts held");
 const uint32_t filler=(uint32_t)0xBFu<<8 | (uint32_t)40u<<16 |
                       (uint32_t)64u<<24;
 for(unsigned i=0;i<SMC_Q;i++)
  CHECK(smc_midi_host(filler)==1,"full User host FIFO accepts valid fader frame");
 CHECK((uint8_t)(smc_host_w-smc_host_r)==SMC_Q,
       "host FIFO was saturated by accepted real MIDI packets");
 uint32_t old_lost=smc_host_solo_release_lost;
 uint8_t st=(uint8_t)((zero_noteon ? 0x90u : 0x80u) |
                       (foreign ? 2u : 15u));
 uint32_t release=(uint32_t)st<<8 | (uint32_t)8u<<16;
 CHECK(smc_midi_host(release)==(foreign ? 0 : 1),
       "foreign MIDI release is rejected; owned release is accepted but full");
 CHECK(smc_host_solo_release_lost==old_lost+(unsigned)!foreign,
       "full host FIFO counts only a valid lost SMC release");
 if(link_edge) { smc_ble_link(0); smc_ble_link(1); }
 smc_project_reset();
 CHECK(!solos_on && !smc_project_active(),
       "replacement project begins without stale Solo");
 CHECK(smc_host_r==smc_host_w,
       "replacement project discards old host MIDI FIFO");
 if(fresh_ble) ble_note(1,1); else host_note(1);
 smc_poll();
 CHECK(solos_on && smc.solo_target[0],
       "old host lost-release must not cancel fresh project Solo");
 if(fresh_ble) ble_note(1,0); else host_note(0);
 smc_poll();
 CHECK(!solos_on && !smc_project_active(),
       "fresh project's matching release clears only its own Solo");
 smc_poll();
 CHECK(!solos_on && smc_host_r==smc_host_w,
       "idle poll cannot replay the old project's lost host release");
 return 1;
}
/* A project replacement discards old BLE frames and acknowledges their lost
 * Solo releases. Otherwise a saturated old project's lost NoteOff could
 * release the next project's genuinely fresh held Solo on the next poll.
 * Exercise the actual producer, project reset, queue and main-loop decoder. */
static int project_reset_ble_overflow(uint8_t cursor, int user,
                                      int link_edge, int foreign)
{
 reset(cursor,user);
 ble_note(user,1);
 smc_poll();
 CHECK(solos_on && smc.solo_target[0], "old project first Solo is held");
 const uint8_t filler[3]={0xB1u,40u,64u};
 for(unsigned i=0;i<SMC_Q;i++) smc_ble_midi(filler,3u);
 CHECK((uint8_t)(smc_qw-smc_qr)==SMC_Q,
       "old project fills the actual BLE producer ring");
 uint32_t old_lost=smc_ble_solo_release_lost;
 const uint8_t release[3]={
  (uint8_t)(foreign ? 0x82u : (user ? 0x8Fu : 0x80u)),8u,0u};
 smc_ble_midi(release,3u);
 CHECK(smc_ble_solo_release_lost == old_lost + (unsigned)!foreign,
       "only a legitimate lost release increments the producer generation");
 if(link_edge) { smc_ble_link(0); smc_ble_link(1); }
 smc_project_reset();
 CHECK(!solos_on && !smc_project_active(),
       "new project begins with no inherited held Solo");
 CHECK(smc_qr==smc_qw,
       "project reset discards old queued input");
 ble_note(user,1);
 smc_poll();
 CHECK(solos_on && smc.solo_target[0],
       "old project's missing release cannot cancel new project's Solo");
 ble_note(user,0);
 smc_poll();
 CHECK(!solos_on && !smc_project_active(),
       "new project's matching NoteOff still releases Solo");
 smc_poll();
 CHECK(!solos_on,"idle poll cannot resurrect old project state");
 return 1;
}
/* USB/peripheral User SMC MIDI remains usable when the direct BLE central is
 * disconnected. A lost host NoteOff in its separate full FIFO must still
 * fail-safe the held Solo; an unrelated channel's key-up must not. Reconnect
 * must not replay stale release state into a subsequent genuine USB press. */
static int offline_host_solo_overflow(uint8_t cursor, int zero_noteon,
                                      int foreign, int reconnect)
{
 reset(cursor,1);
 smc_ble_link(0);
 smc_poll();
 CHECK(!smc.up && !solos_on, "central link is physically down");
 host_note(1);
 smc_poll();
 CHECK(solos_on && smc.solo_target[0],
       "USB User Solo press works with BLE central disconnected");
 for(unsigned i=0;i<SMC_Q;i++) host_cc();
 CHECK((uint8_t)(smc_host_w-smc_host_r)==SMC_Q,
       "offline host producer FIFO is genuinely full");
 uint32_t before=smc_host_solo_release_lost;
 uint8_t st=(uint8_t)((zero_noteon ? 0x90u : 0x80u) |
                       (foreign ? 2u : 15u));
 uint32_t release=(uint32_t)st<<8 | (uint32_t)8u<<16;
 CHECK(smc_midi_host(release)==(foreign ? 0 : 1),
       "only User channel 16 belongs to USB SMC");
 CHECK(smc_host_solo_release_lost==before+(unsigned)!foreign,
       "only an owned dropped USB release advances the lost-generation");
 smc_poll();
 CHECK((uint8_t)(smc_host_w-smc_host_r)==0,
       "USB ring drains while direct BLE central is offline");
 CHECK(!!solos_on==(unsigned)foreign,
       "owned lost USB NoteOff fails safe offline; foreign key-up does not");
 if(foreign) {
  host_note(0);
  smc_poll();
  CHECK(!solos_on && !smc_project_active(),
        "valid USB key-up still releases after foreign MIDI rejection");
 }
 host_note(1);
 smc_poll();
 CHECK(solos_on && smc.solo_target[0],
       "new USB Solo remains usable after overflow without central");
 if(reconnect) {
  smc_ble_link(1);
  smc_poll();
  CHECK(!solos_on && !smc_project_active(),
        "link return clears old hold without reusing stale release");
  host_note(1);
  smc_poll();
  CHECK(solos_on && smc.solo_target[0],
        "fresh USB Solo after BLE reconnect is not cancelled by old loss");
 }
 host_note(0);
 smc_poll();
 CHECK(!solos_on && !smc_project_active(),
       "matched USB release ends final fresh Solo");
 smc_poll();
 CHECK(!solos_on && smc_host_r==smc_host_w,
       "idle after disconnection/reconnect never replays Solo");
 return 1;
}
int main(void)
{
 unsigned cases=0;
 const uint8_t cursors[] = {0u,1u,31u,127u,240u,255u};
 /* Focused USB/User lost-release behavior while BLE central is OFFLINE.
  * 6 wrapping ring cursors x NoteOff/zero-on x owned/foreign x reconnect. */
 if (getenv("SMC_ONLY_OFFLINE_HOST_OVERFLOW")) {
  for(unsigned i=0;i<sizeof cursors;i++)
   for(int zero_noteon=0;zero_noteon<2;zero_noteon++)
    for(int foreign=0;foreign<2;foreign++)
     for(int reconnect=0;reconnect<2;reconnect++) {
      if(!offline_host_solo_overflow(cursors[i],zero_noteon,
                                     foreign,reconnect)) return 1;
      ++cases;
     }
  printf("SMC offline USB host lost release: %u/%u scenarios PASS, %u checks\n",
         cases,cases,checks);
  return 0;
 }
 /* Focused host ring/project reset regression, distinct from BLE producer.
  * 6 FIFO cursors x 2 release encodings x 2 link epochs x accepted/foreign
  * release x new BLE/host input = 96 real queue and decoder scenarios. */
 if (getenv("SMC_ONLY_HOST_RESET_OVERFLOW")) {
  for(unsigned i=0;i<sizeof cursors;i++)
   for(int zero_noteon=0;zero_noteon<2;zero_noteon++)
    for(int link_edge=0;link_edge<2;link_edge++)
     for(int foreign=0;foreign<2;foreign++)
      for(int fresh_ble=0;fresh_ble<2;fresh_ble++) {
       if(!project_reset_host_overflow(cursors[i],zero_noteon,
                                       link_edge,foreign,fresh_ble)) return 1;
       ++cases;
      }
  printf("SMC User host reset overflow: %u/%u scenarios PASS, %u checks\n",
         cases,cases,checks);
  return 0;
 }
 /* Focused real producer/project-reset lifetime check for rapid triage. */
 if (getenv("SMC_ONLY_RESET_OVERFLOW")) {
  for(unsigned i=0;i<sizeof cursors;i++)
   for(int user=0;user<2;user++)
    for(int link_edge=0;link_edge<2;link_edge++)
     for(int foreign=0;foreign<2;foreign++) {
      if(!project_reset_ble_overflow(cursors[i],user,link_edge,foreign)) return 1;
      ++cases;
     }
  printf("SMC BLE project reset overflow: %u/%u scenarios PASS, %u checks\n",
         cases,cases,checks);
  return 0;
 }
 /* Focused paired BLE link/overflow phase regression; not a hardware test. */
 if (getenv("SMC_ONLY_LINK_OVERFLOW")) {
  for(unsigned user=0;user<2;user++)
   for(unsigned ch=0;ch<16;ch++)
    for(int zero_on=0;zero_on<2;zero_on++)
     for(int new_link=0;new_link<2;new_link++)
      for(unsigned i=0;i<sizeof cursors;i++) {
       if(!ble_link_overflow_lifetime(cursors[i],user,ch,zero_on,new_link)) return 1;
       ++cases;
      }
  printf("SMC BLE link overflow lifetime: %u/%u scenarios PASS, %u checks\n",
         cases,cases,checks);
  return 0;
 }
 /* Focused overflow suite, used for negative controls and CI triage. */
 if (getenv("SMC_ONLY_FOREIGN_OVERFLOW")) {
  for(unsigned user=0;user<2;user++)
   for(unsigned ch=0;ch<16;ch++)
    for(int zero_on=0;zero_on<2;zero_on++)
     for(unsigned i=0;i<sizeof cursors;i++) {
      if(!ble_foreign_overflow(cursors[i],user,ch,zero_on)) return 1;
      ++cases;
     }
  printf("SMC BLE foreign overflow: %u/%u scenarios PASS, %u checks\n",
         cases,cases,checks);
  return 0;
 }
 /* Focused midpoll ISR refill regression, including a deliberately broken
  * prior-source negative control in the native regression runner. */
 if (getenv("SMC_ONLY_MIDPOLL_FOREIGN_OVERFLOW")) {
  for(unsigned user=0;user<2;user++)
   for(unsigned ch=0;ch<16;ch++)
    for(int zero_on=0;zero_on<2;zero_on++)
     for(unsigned i=0;i<sizeof cursors;i++) {
      if(!ble_midpoll_foreign_overflow(cursors[i],user,ch,zero_on)) return 1;
      ++cases;
     }
  printf("SMC BLE midpoll foreign overflow: %u/%u scenarios PASS, %u checks\n",
         cases,cases,checks);
  return 0;
 }
 /* Only the newly added interleaved-post-edge regression, suitable for
  * targeted fault injection; ordinary runs execute the complete suite. */
 if (getenv("SMC_ONLY_FRESH_CUTOFF")) {
  for (unsigned i=0;i<sizeof cursors;i++)
   for(int up=0;up<2;up++)
    for(int old_release=0;old_release<2;old_release++) {
     if(!midpoll_fresh_during_link(cursors[i],up,old_release)) return 1;
     ++cases;
    }
  printf("SMC midpoll fresh host cutoff: %u/%u scenarios PASS, %u checks\n",
         cases,cases,checks);
  return 0;
 }
 /* Standard run includes the BLE-disconnected USB/User release-loss phase. */
 for(unsigned i=0;i<sizeof cursors;i++)
  for(int zero_noteon=0;zero_noteon<2;zero_noteon++)
   for(int foreign=0;foreign<2;foreign++)
    for(int reconnect=0;reconnect<2;reconnect++) {
     if(!offline_host_solo_overflow(cursors[i],zero_noteon,
                                    foreign,reconnect)) return 1;
     ++cases;
    }
 for(unsigned i=0;i<sizeof cursors;i++)
  for(int zero_noteon=0;zero_noteon<2;zero_noteon++)
   for(int link_edge=0;link_edge<2;link_edge++)
    for(int foreign=0;foreign<2;foreign++)
     for(int fresh_ble=0;fresh_ble<2;fresh_ble++) {
      if(!project_reset_host_overflow(cursors[i],zero_noteon,
                                      link_edge,foreign,fresh_ble)) return 1;
      ++cases;
     }
 for(unsigned i=0;i<sizeof cursors;i++)
  for(int user=0;user<2;user++)
   for(int link_edge=0;link_edge<2;link_edge++)
    for(int foreign=0;foreign<2;foreign++) {
     if(!project_reset_ble_overflow(cursors[i],user,link_edge,foreign)) return 1;
     ++cases;
    }
 for(unsigned user=0;user<2;user++)
  for(unsigned ch=0;ch<16;ch++)
   for(int zero_on=0;zero_on<2;zero_on++)
    for(int new_link=0;new_link<2;new_link++)
     for(unsigned i=0;i<sizeof cursors;i++) {
      if(!ble_link_overflow_lifetime(cursors[i],user,ch,zero_on,new_link)) return 1;
      ++cases;
     }
 for(unsigned user=0;user<2;user++)
  for(unsigned ch=0;ch<16;ch++)
   for(int zero_on=0;zero_on<2;zero_on++)
    for(unsigned i=0;i<sizeof cursors;i++) {
     if(!ble_foreign_overflow(cursors[i],user,ch,zero_on)) return 1;
     ++cases;
    }
 for(unsigned user=0;user<2;user++)
  for(unsigned ch=0;ch<16;ch++)
   for(int zero_on=0;zero_on<2;zero_on++)
    for(unsigned i=0;i<sizeof cursors;i++) {
     if(!ble_midpoll_foreign_overflow(cursors[i],user,ch,zero_on)) return 1;
     ++cases;
    }
 for (unsigned i=0;i<sizeof cursors;i++) {
  for(int up=0;up<2;up++)
   for(int fresh=0;fresh<3;fresh++) {
    if(!midpoll_host_link(cursors[i],up,fresh)) return 1;
    ++cases;
   }
  for(int up=0;up<2;up++)
   for(int old_release=0;old_release<2;old_release++) {
    if(!midpoll_fresh_during_link(cursors[i],up,old_release)) return 1;
    ++cases;
   }
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
