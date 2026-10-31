#!/usr/bin/env python3
"""Native regression: a saturated MIDI ring must not strand held SMC Solo.
Extracts the actual production queue/poll functions with mocked UI/MIDI side effects.
"""
from pathlib import Path
import os, re, shlex, subprocess, tempfile
ROOT = Path(__file__).resolve().parents[2]
SRC = Path(os.environ.get('SMC_SOURCE', ROOT / 'firmware/src/app/smc_mixer.c'))
s = SRC.read_text()
def extract(name):
 m = re.search(r'^[^\n]*\b'+re.escape(name)+r'\([^\n]*\)\s*\{',s,re.M)
 if not m: raise ValueError('Missing '+name)
 i=m.end(); depth=1
 while depth:
  depth += (s[i]=='{')-(s[i]=='}'); i+=1
 return s[m.start():i]
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from smc_peek_stub import strip as strip_peek
prefix=strip_peek(s[s.index('#define SMC_Q'):s.index('int smc_project_active')])
funcs=['smc_project_active','smc_solo_clear','smc_solo_publish','smc_solo_press','smc_project_drop','smc_project_reset','smc_ble_midi','smc_ble_link','smc_ble_role_get','smc_connected','smc_midi_host','smc_mmc_host','smc_release','smc_mmc_drain_stop','smc_spill_apply']
# Include the MMC STOP FIFO-boundary helper so the regression covers the full current poll path.
if 'smc_poll_peer' in s:
 funcs = ['smc_solo_union', 'smc_peer_drop', 'smc_ble_midi_slot', 'smc_ble_link_slot'] + funcs
body='\n'.join(extract(n) for n in funcs)
hdr=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app/smc_protocol.h"
#include "app/smc_mixer_edges.h"
#include "smc_fx_harness_stubs.h"
#define X0X_COLD
#define PROJ_BT_SMC_CENTRAL 16u
#define PROJ_BT_SMC_USER 32u
#define NKIT 2
#define NPARTS 5
#define NDRUM 11
#define O_NONE 0
#define O_ASK 1
typedef struct { uint8_t kind,a,b,c; } pref_t;
static uint8_t smc_message(const uint8_t *m,uint8_t n,int allow);
static struct { struct { uint8_t bt; } set; } proj;
static struct { int overlay; } ui;
#define SMC_UI_MS 1200u
static struct { uint8_t on, peer; uint32_t until; } smc_ui;
static uint32_t plat_ms(void) { return 0u; }
static int is_busy, is_testing, clear_calls, output_solo;
static int refill_armed, refill_route, refill_cross, refill_calls;
static int perf_testing(void) { return is_testing; }
static int ui_project_busy(void) { return is_busy; }
static void engine_smc_solo(int parts) { output_solo=parts; clear_calls++; }
static void engine_smc_solo_targets(uint8_t parts,uint16_t a,uint16_t b)
{ output_solo=(int)parts | (int)(a!=0)<<8 | (int)(b!=0)<<9; }
static void ui_say(const char *a,const char *b){(void)a;(void)b;}
#define smc_ui_cancel() ((void)0) /* Presentation is exercised by smc_ui_test.c. */
/* This fixture dispatches Solo notes only, never opens an FX view, and already
 * substitutes all FX side effects. Full SELECT/mode ownership is exercised by
 * smc_select_mode_test.c through the production dispatch. */
static int smc_fx_bus_now(uint8_t note) { (void)note; return -1; }
static void smc_guarded_button(uint8_t note,int mmc,int allow);
static int smc_lock_button(int pressed);
'''
stubs=r'''
/* Release-only API in this isolated Solo queue fixture; full lock coverage is separate. */
static int smc_lock_button(int pressed)
{ assert(!pressed); smc.lock_down=0; return 0; }
static uint8_t smc_message(const uint8_t *m,uint8_t n,int allow)
{
 /* Simulated producer ISR runs AFTER the BLE drain, while a host event
  * is dispatched (or after the first CC on its own ring). */
 if (refill_armed && n==3 && (m[0]&0xF0u)==0xB0u) {
  refill_armed=0; refill_calls++;
  if (refill_route==0) {
   assert(smc_midi_host(9u|(uint32_t)0x9Fu<<8|(uint32_t)8u<<16|(uint32_t)127u<<24));
   assert(smc_midi_host(9u|(uint32_t)0x8Fu<<8|(uint32_t)8u<<16));
  } else {
   const uint8_t press[3]={0x90u,8u,127u};
   smc_ble_midi(press,3);
   if (refill_cross) {
    for (unsigned i=1u;i<SMC_Q;i++) {
     const uint8_t cc[3]={0xB0u,16u,(uint8_t)i};
     smc_ble_midi(cc,3);
    }
   }
   const uint8_t release[3]={0x80u,8u,0u};
   smc_ble_midi(release,3);
  }
 }
 if (!allow || n!=3) return 0;
 uint8_t type=m[0]&0xf0u,note=m[1],vel=m[2];
 if (note<8u || note>=16u) return 0;
 if (type==0x90u && vel) smc_solo_press((int)(note-8u),1,0);
 else if(type==0x80u||(type==0x90u&&!vel)) smc_release(note);
 return 0;
}
static __attribute__((unused)) void smc_guarded_button(uint8_t note,int mmc,int allow)
{(void)note;(void)mmc;(void)allow;}
'''
case=r'''
static uint32_t packet(uint8_t st,uint8_t a,uint8_t b)
{ return 9u | (uint32_t)st<<8 | (uint32_t)a<<16 | (uint32_t)b<<24; }
static void prepare(void)
{
 smc_project_reset(); smc_link_published=smc_link_seen=0; smc.up=1;
 proj.set.bt=PROJ_BT_SMC_CENTRAL|PROJ_BT_SMC_USER;
 is_busy=is_testing=0; ui.overlay=O_NONE; output_solo=clear_calls=0;
 smc_qr=smc_qw=smc_host_r=smc_host_w=0;
 smc_mmc_r[0]=smc_mmc_w[0]=smc_mmc_r[1]=smc_mmc_w[1]=0;
 refill_armed=refill_route=refill_cross=refill_calls=0;
 smc_ble_link(1); smc_poll();
}
static void emit(int route,uint8_t st,uint8_t note,uint8_t vel)
{
 if(route==0) assert(smc_midi_host(packet((uint8_t)(st|15u),note,vel)));
 else {const uint8_t m[3]={st,note,vel}; smc_ble_midi(m,3);}
}
static void press(int route)
{
 emit(route,0x90,8,127); smc_poll();
 assert(smc.solo_target[0]==1); assert(output_solo);
}
static void saturate(int route)
{
 for(unsigned i=0;i<SMC_Q;i++){
  if(route==0) assert(smc_midi_host(packet(0xBF,40,(uint8_t)i)));
  else {const uint8_t m[3]={0xB0,16,(uint8_t)i};smc_ble_midi(m,3);}
 }
}
static void delayed_overflow_release(int route,int cross)
{
 prepare();
 if(cross){ assert(route==1); assert(smc_midi_host(packet(0xBFu,40u,25u))); }
 else saturate(route);
 uint32_t before=route==0?smc_host_solo_release_lost:smc_ble_solo_release_lost;
 refill_route=route; refill_cross=cross; refill_armed=1;
 smc_poll(); /* ISR refills just-freed slot; Note Off overflows */
 assert(refill_calls==1);
 assert((route==0?smc_host_solo_release_lost:smc_ble_solo_release_lost)!=before);
 smc_poll(); /* queued Note On then deferred release must not latch */
 if(smc.solo_target[0]||output_solo){
  fprintf(stderr,"FAIL deferred Solo release route=%d cross=%d\n",route,cross);
  exit(1);
 }
}
static void one(int route,int use_noteon_zero,int full,int with_held)
{
 prepare(); if(with_held)press(route); if(full)saturate(route);
 emit(route,use_noteon_zero?0x90:0x80,8,0); smc_poll();
 if(with_held && output_solo){fprintf(stderr,"FAIL route=%d zero=%d full=%d\n",route,use_noteon_zero,full);exit(1);}
 if(with_held && !full)assert(!smc.solo_target[0]);
}
static void no_false_release(int route)
{
 prepare();press(route);saturate(route);emit(route,0x90,9,127);smc_poll();
 assert(smc.solo_target[0]&&output_solo);
}
static void queued_press(int route)
{
 prepare();emit(route,0x90,8,127);
 for(unsigned i=1;i<SMC_Q;i++){
  if(route==0)assert(smc_midi_host(packet(0xBF,40,(uint8_t)i)));
  else {const uint8_t m[3]={0xB0,16,(uint8_t)i};smc_ble_midi(m,3);}
 }
 emit(route,0x80,8,0);smc_poll();assert(!smc.solo_target[0]&&!output_solo);
}
int main(void)
{
 /* DAW only: the retired User host route (0) and cross-route refill are gone. */
 int cases=0;
 for(int z=0;z<2;z++)for(int full=0;full<2;full++){one(1,z,full,1);cases++;}
 no_false_release(1);cases++;queued_press(1);cases++;
 prepare();press(1);saturate(1);emit(1,0x80,8,0);smc_ble_link(0);smc_poll();
 assert(!smc.solo_target[0]&&!output_solo);cases++;
 delayed_overflow_release(1,0); cases++;
 {
  prepare(); smc.home_down=1; saturate(1);
  uint32_t before=smc_ble_solo_release_lost;
  emit(1,0x80,6,0); smc_poll();
  if(smc.home_down || smc_ble_solo_release_lost==before){
   fprintf(stderr,"FAIL HOME release overflow home_down=%u\n",(unsigned)smc.home_down);
   exit(1);
  }
  cases++;
 }
 /* Retired hosted hook: never admits, never publishes a release loss. */
 prepare();
 {
  uint32_t lost=smc_host_solo_release_lost;
  for(unsigned st=0x80;st<0xF0;st+=0x10)for(unsigned ch=0;ch<16;ch++)
   assert(!smc_midi_host(packet((uint8_t)(st|ch),8,0)));
  assert(smc_host_solo_release_lost==lost);
  cases++;
 }
 assert(cases==10);printf("SMC combined source-window Solo overflow (DAW only): %d/10 PASS\n",cases);
 return 0;
}
'''
code=hdr+prefix+body+'\n'+stubs+(extract('smc_poll_peer') + '\n' if 'smc_poll_peer' in s else '') + extract('smc_poll')+'\n'+case
with tempfile.TemporaryDirectory(prefix='smc-r10-combined-') as t:
 f=Path(t)/'test.c';exe=Path(t)/'test';f.write_text(code)
 command=[os.environ.get('CC','gcc'),'-std=c99','-O1','-g','-Wall','-Wextra','-Werror',*shlex.split(os.environ.get('SAN_FLAGS','')),'-I'+str(ROOT/'firmware/src'),'-I'+str(ROOT/'tests/host'),str(f),'-o',str(exe)]
 print('COMPILE', ' '.join(command),flush=True)
 subprocess.run(command,check=True)
 subprocess.run([str(exe)],check=True)
