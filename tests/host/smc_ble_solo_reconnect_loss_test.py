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
prefix=s[s.index('#define SMC_Q'):s.index('int smc_project_active')]
funcs=['smc_project_active','smc_solo_clear','smc_solo_publish','smc_solo_press','smc_project_drop','smc_project_reset','smc_ble_midi','smc_ble_link','smc_ble_role_get','smc_connected','smc_midi_host','smc_mmc_host','smc_release','smc_mmc_drain_stop']
# Include the MMC STOP FIFO-boundary helper so the regression covers the full current poll path.
body='\n'.join(extract(n) for n in funcs)
hdr=r'''
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
#define O_NONE 0
#define O_ASK 1
typedef struct { uint8_t kind,a,b,c; } pref_t;
static struct { struct { uint8_t bt; } set; } proj;
static struct { int overlay; } ui;
static int is_busy, is_testing, clear_calls, output_solo, inject_on_host, injection_count, inject_route, inject_cross;
static int inject_new_ble_on_link, injected_ble, inject_burst;
void smc_ble_midi(const uint8_t *msg, uint8_t len);
static int perf_testing(void) { return is_testing; }
static int ui_project_busy(void) { return is_busy; }
static void engine_smc_solo(int parts) { output_solo=parts; clear_calls++; }
static void engine_smc_solo_targets(uint8_t parts,uint16_t a,uint16_t b)
{ output_solo=(int)parts | (int)(a!=0)<<8 | (int)(b!=0)<<9; }
static void ui_say(const char *a,const char *b){
 (void)b;
 if (inject_new_ble_on_link && strcmp(a,"SMC CONNECTED")==0) {
  inject_new_ble_on_link=0; injected_ble++;
  const uint8_t press[]={0x90u,8u,127u};
  smc_ble_midi(press,3u);
  if (inject_burst) {
   for(unsigned j=1;j<32u;j++) {
    const uint8_t cc[]={0xB0u,16u,(uint8_t)j}; smc_ble_midi(cc,3u);
   }
   const uint8_t release[]={0x80u,8u,0u}; smc_ble_midi(release,3u);
  }
 }
}
static void smc_guarded_button(uint8_t note,int mmc,int allow);
'''
stubs=r'''
static uint8_t smc_message(const uint8_t *m,uint8_t n,int allow)
{
 /* Simulate a producer refill after one ring slot is released, including
  * self-ring and cross-ring ISR/host interleavings. */
 if (inject_on_host && n==3 && (m[0]&0xF0u)==0xB0u) {
  inject_on_host=0; injection_count++;
  if (inject_route==0) {
   assert(smc_midi_host(9u | (uint32_t)0x9Fu<<8 | (uint32_t)8u<<16 | (uint32_t)127u<<24));
   assert(smc_midi_host(9u | (uint32_t)0x8Fu<<8 | (uint32_t)8u<<16));
  } else {
   const uint8_t note_on[]={0x90,8,127};
   smc_ble_midi(note_on,3);
   if (inject_cross) for (unsigned j=1;j<SMC_Q;j++) {
    const uint8_t cc[]={0xB0,16,(uint8_t)j};
    smc_ble_midi(cc,3);
   }
   const uint8_t note_off[]={0x80,8,0};
   smc_ble_midi(note_off,3);
  }
 }
 if (!allow || n!=3) return 0;
 uint8_t type=m[0]&0xf0u,note=m[1],vel=m[2];
 if (note<8u || note>=16u) return 0;
 if (type==0x90u && vel) smc_solo_press((int)(note-8u),1,0);
 else if(type==0x80u||(type==0x90u&&!vel)) smc_release(note);
 return 0;
}
static void smc_guarded_button(uint8_t note,int mmc,int allow)
{(void)note;(void)mmc;(void)allow;}
'''
case=r'''
static uint32_t packet(uint8_t st,uint8_t a,uint8_t b)
{ return 9u | (uint32_t)st<<8 | (uint32_t)a<<16 | (uint32_t)b<<24; }
static void prepare(void)
{
 smc_project_reset(); smc_link_published=smc_link_seen=0; smc.up=1;
 proj.set.bt=PROJ_BT_SMC_CENTRAL|PROJ_BT_SMC_USER;
 is_busy=is_testing=inject_on_host=injection_count=inject_route=inject_cross=0;
 inject_new_ble_on_link=injected_ble=inject_burst=0; ui.overlay=O_NONE; output_solo=clear_calls=0;
 smc_qr=smc_qw=smc_host_r=smc_host_w=0;
 smc_mmc_r[0]=smc_mmc_w[0]=smc_mmc_r[1]=smc_mmc_w[1]=0;
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
static void late_refill(int route, int cross)
{
 prepare();
 if (cross) {
  assert(route==1);
  emit(0,0xB0,40,25);
 } else saturate(route);
 uint32_t before=route==0?smc_host_solo_release_lost:smc_ble_solo_release_lost;
 inject_route=route; inject_cross=cross; inject_on_host=1;
 smc_poll();
 assert(injection_count==1);
 assert((route==0?smc_host_solo_release_lost:smc_ble_solo_release_lost)!=before);
 smc_poll();
 if (smc.solo_target[0] || output_solo) {
  fprintf(stderr,"FAIL delayed release route=%d cross=%d\n",route,cross);
  exit(1);
 }
}
int main(void)
{
 int cases=0;
 for(int r=0;r<2;r++){
  for(int z=0;z<2;z++)for(int full=0;full<2;full++){one(r,z,full,1);cases++;}
  no_false_release(r);cases++;queued_press(r);cases++;
 }
 prepare();press(0);saturate(0);
 assert(!smc_midi_host(packet(0x8f,0x88,0)));
 assert(!smc_midi_host(packet(0x8f,8,0x80)));
 smc_poll();assert(smc.solo_target[0]&&output_solo);cases++;
 prepare();press(0);saturate(0);proj.set.bt&=(uint8_t)~PROJ_BT_SMC_USER;
 assert(!smc_midi_host(packet(0x8f,8,0)));smc_poll();assert(smc.solo_target[0]&&output_solo);cases++;
 prepare();press(1);saturate(1);proj.set.bt&=(uint8_t)~PROJ_BT_SMC_USER;
 emit(1,0x8f,8,0);smc_poll();assert(smc.solo_target[0]&&output_solo);cases++;
 prepare();press(0);saturate(0);saturate(1);
 emit(0,0x80,8,0);emit(1,0x80,8,0);smc_poll();assert(!smc.solo_target[0]&&!output_solo);cases++;
 prepare();press(1);saturate(1);emit(1,0x80,8,0);smc_ble_link(0);smc_poll();
 assert(!smc.solo_target[0]&&!output_solo);cases++;
 prepare();press(0);saturate(0);emit(0,0x80,8,0);is_busy=1;
 smc_poll();is_busy=0;smc_poll();assert(!smc.solo_target[0]&&!output_solo);cases++;
 prepare();press(0);
 smc_host_solo_release_lost=UINT32_MAX;smc_host_solo_release_seen=UINT32_MAX;
 saturate(0);emit(0,0x80,8,0);smc_poll();assert(!smc.solo_target[0]&&!output_solo);cases++;
 /* Late ISR refill: this must remain queued for the NEXT poll and then
  * release Solo, rather than ACKing loss before its press has been drained. */
 late_refill(0,0);cases++;  /* same USB/User ring one-slot refill */
 late_refill(1,0);cases++;  /* same BLE central ring one-slot refill */
 late_refill(1,1);cases++;  /* BLE ring refill during host dispatch */
 /* OLD BLE loss must not clear a genuine new-link press when TIMER5
  * refills the newly emptied ring during the same main-loop poll. */
 prepare(); smc_ble_link(1); smc_poll();
 saturate(1); emit(1,0x80,8,0);  /* reject an old-session release */
 assert(smc_ble_solo_release_lost != smc_ble_solo_release_seen);
 smc_ble_link(0); smc_ble_link(1); inject_new_ble_on_link=1;
 smc_poll();
 assert(injected_ble==1);
 assert(smc.solo_target[0]==1 && output_solo);  /* new press survives */
 smc_poll(); assert(smc.solo_target[0]==1 && output_solo);
 emit(1,0x80,8,0); smc_poll(); assert(!smc.solo_target[0]&&!output_solo);cases++;
 /* A *NEW* lost release already present at the boundary must still
  * release that same connection's press: no blanket loss cancellation. */
 prepare(); smc_ble_link(1); emit(1,0x90,8,127);
 for(unsigned i=1;i<SMC_Q;i++) { const uint8_t cc[]={0xB0,16,(uint8_t)i};smc_ble_midi(cc,3); }
 emit(1,0x80,8,0);smc_poll();assert(!smc.solo_target[0]&&!output_solo);cases++;
 /* A late same-session lost release after link publication cannot be
  * acknowledged in the current snapshot. It must clear on the next poll. */
 prepare(); smc_ble_link(1); inject_new_ble_on_link=1;inject_burst=1;
 smc_poll();assert(injected_ble==1);
 assert(smc_ble_solo_release_lost != smc_ble_solo_release_seen);
 smc_poll();assert(!smc.solo_target[0]&&!output_solo);cases++;
 /* A project reset can already have acknowledged a newer loss than the
  * boundary. Applying an older link watermark must not rewind seen. */
 prepare(); smc_ble_link(1); smc_ble_solo_release_lost++;
 smc_project_drop(); inject_new_ble_on_link=1;
 smc_poll();assert(injected_ble==1);assert(smc.solo_target[0]==1&&output_solo);cases++;
 assert(cases==26);printf("SMC combined source-window Solo+reconnect: %d/26 PASS\n",cases);
 return 0;
}
'''
code=hdr+prefix+body+'\n'+stubs+extract('smc_poll')+'\n'+case
with tempfile.TemporaryDirectory(prefix='smc-r10-combined-') as t:
 f=Path(t)/'test.c';exe=Path(t)/'test';f.write_text(code)
 command=[os.environ.get('CC','gcc'),'-std=c99','-O1','-g','-Wall','-Wextra','-Werror',*shlex.split(os.environ.get('SAN_FLAGS','')),'-I'+str(ROOT/'firmware/src'),str(f),'-o',str(exe)]
 print('COMPILE', ' '.join(command),flush=True)
 subprocess.run(command,check=True)
 subprocess.run([str(exe)],check=True)
