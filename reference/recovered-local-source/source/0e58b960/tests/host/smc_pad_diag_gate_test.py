#!/usr/bin/env python3
"""Exercise actual SMC queues/poll/boundary/UI gates with simulated UI and producers.

--source selects the confirmed unmodified input for a red control. No device I/O.
Synchronous producer injections model selected interleavings, not target timing.
"""
import argparse
from pathlib import Path
import os
import re
import subprocess
import tempfile
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[2]/'firmware/src/app/smc_mixer.c')
a=p.parse_args();source=a.source.read_text()
def function(name):
    m=re.search(r'^[^\n]*\b'+re.escape(name)+r'\([^\n]*\)\s*\{',source,re.M)
    if not m:raise RuntimeError('missing '+name)
    depth,pos=1,m.end()
    while depth:
        depth+=(source[pos]=='{')-(source[pos]=='}');pos+=1
    return source[m.start():pos]
prefix=source[source.index('#define SMC_Q'):source.index('int smc_project_active')]
ui_struct=source[source.index('struct smc_ui_state {'):source.index('static uint32_t smc_ui_context(void)')]
names=['smc_solo_union','smc_project_active','smc_solo_clear','smc_peer_drop','smc_project_drop',
       'smc_ble_midi_slot','smc_ble_link_slot','smc_fx_clear','smc_ui_cancel','smc_ui_active','smc_ui_begin',
       'smc_guarded_button','smc_message','smc_mmc_drain_stop','smc_poll_peer','smc_poll','smc_frame']
functions='\n'.join(function(n) for n in names)
if 'void smc_pad_diag_boundary(void)\n{' in source:functions+='\n'+function('smc_pad_diag_boundary')
else:functions+='\nvoid smc_pad_diag_boundary(void) { } /* prior SMC interface has no boundary handler */\n'
harness=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define X0X_COLD
#define BOARD_SMK37 1
#define SMCFX_N 8
#define PROJ_BT_SMC_CENTRAL 16u
#define PROJ_BT_SMC_USER 32u
#define SMC_UI_MS 1200u
enum { NKIT=2, NPARTS=5, NDRUM=16, O_NONE, O_ASK, B_HOME };
typedef struct { uint8_t kind,a,b,c; } pref_t;
static struct { struct { uint8_t bt; } set; } proj;
static struct { uint8_t overlay,help; } ui;
static uint32_t blit_hash[4];
static int diagnostic,busy,performance;
static unsigned edits,buttons,stops,plays,recs,releases,feedback,ticks,unlocks,notices;
static unsigned solo_parts,solo_a,solo_b,fx_mask,checks,injection;
static uint32_t now=100;
int ui_pad_diag_active(void){return diagnostic;}
static int ui_project_busy(void){return busy;}
static int perf_testing(void){return performance;}
static uint32_t plat_ms(void){return now;}
static uint32_t smc_ui_context(void){return 42;}
void smk_clip_lcd_force(void) { }
static void ui_say(const char*s,int n){(void)s;(void)n;notices++;}
static void button_tap(unsigned n){(void)n;buttons++;}
static void inject_if_needed(void);
static void engine_smc_solo_targets(uint8_t p,uint16_t a,uint16_t b){solo_parts=p;solo_a=a;solo_b=b;inject_if_needed();}
static void smc_fx_publish(void);
static void smc_fx_main_tick(void){ticks++;}
static void smc_lock_tick(void){unlocks++;}
static void smc_frame_peer(void){feedback++;}
static void smc_button(uint8_t n,int mmc){buttons++;if(mmc&&n==0x5d)stops++;else if(n==0x5e)plays++;else if(n==0x5f)recs++;else edits++;}
static void smc_release(uint8_t n){(void)n;releases++;}
static void smc_fader(int n,uint8_t v){(void)n;(void)v;edits++;}
static void smc_knob(int n,int d){(void)n;(void)d;edits++;}
static int smc_relative(uint8_t v){return (v&64)?-(int)(v&63):(int)v;}
static uint8_t smc_mmc_note(uint8_t cmd){return cmd==34?0x5d:cmd==33?0x5e:cmd==35?0x5f:24;}
void smc_pad_diag_boundary(void);
''' + prefix+ui_struct+r'''
static void smc_fx_publish(void){fx_mask=smc_states[0].fx_down|smc_states[1].fx_down;}
''' + functions+r'''
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void central(unsigned slot,unsigned st,unsigned note,unsigned v){uint8_t m[]={(uint8_t)st,(uint8_t)note,(uint8_t)v};smc_ble_midi_slot(slot,m,3);}
static void host(unsigned st,unsigned note,unsigned v){unsigned w=smc_host_w;smc_host_q[w%SMC_Q][0]=st;smc_host_q[w%SMC_Q][1]=note;smc_host_q[w%SMC_Q][2]=v;smc_host_w=(uint8_t)(w+1);}
static void mmc(unsigned src,unsigned cmd){unsigned w=smc_mmc_w[src];smc_mmc_q[src][w%16]=cmd;smc_mmc_w[src]=(uint8_t)(w+1);}
static void inject_if_needed(void){if(injection){injection=0;central(0,0xb0,16,1);host(0xbf,16,1);mmc(1,33);}}
static void reset(unsigned start){
 memset(smc_inputs,0,sizeof smc_inputs);memset(smc_states,0,sizeof smc_states);memset(smc_ui_raw,0,sizeof smc_ui_raw);
 diagnostic=busy=performance=0;proj.set.bt=PROJ_BT_SMC_CENTRAL|PROJ_BT_SMC_USER;
 edits=buttons=stops=plays=recs=releases=feedback=ticks=unlocks=notices=injection=0;
 smc_slot=0;ui.overlay=O_NONE;ui.help=0;solo_parts=solo_a=solo_b=fx_mask=0;
 smc_host_r=smc_host_w=start;
 for(unsigned i=0;i<2;i++){smc_inputs[i].qr=smc_inputs[i].qw=start;smc_inputs[i].link_seen=smc_inputs[i].link_published=1;smc_states[i].up=1;smc_mmc_r[i]=smc_mmc_w[i]=start;smc_mmc_stop_published[i]=smc_mmc_stop_seen[i]=0;smc_mmc_stop_boundary[i]=start;}
}
static unsigned actions(void){return edits+buttons+stops+plays+recs;}
static void all_inputs(unsigned slot){
 central(slot,0xb0,16,1);central(slot,0x90,24,127);central(slot,0x90,0x5e,127);central(slot,0x90,0x5f,127);central(slot,0x90,0x60,127);
 host(0xbf,40,77);host(0x9f,0x5e,127);mmc(0,33);mmc(1,34);
}
int main(void){
 const unsigned starts[]={0,240,254,255};
 for(unsigned k=0;k<4;k++){
  unsigned start=starts[k];reset(start);diagnostic=1;all_inputs(0);all_inputs(1);
  smc_poll();smc_frame();
  CHECK(!actions()&&!releases&&!feedback&&!ticks&&!unlocks); /* red on ungated actual poll */
  for(unsigned peer=0;peer<2;peer++)CHECK(smc_inputs[peer].qr==smc_inputs[peer].qw);
  CHECK(smc_host_r==smc_host_w&&smc_mmc_r[0]==smc_mmc_w[0]&&smc_mmc_r[1]==smc_mmc_w[1]);
  /* Held cleanup bypasses normal SELECT/Pause release and never changes the lock identity. */
  for(unsigned peer=0;peer<2;peer++){smc_states[peer].fx_down=1<<peer;smc_states[peer].fx_owned=255;smc_states[peer].fx_main_wait=7;smc_states[peer].solo_target[0]=1;smc_states[peer].solo=1;smc_states[peer].drum_solo[0]=2;smc_states[peer].locked=1;smc_states[peer].lock_down=2;smc_states[peer].select_down=smc_states[peer].play_down=smc_states[peer].home_down=255;smc_states[peer].picked=smc_states[peer].seen=smc_states[peer].input_seen=255;}
  smc_ui.on=1;smc_ui.context=42;smc_ui.until=200;
  smc_pad_diag_boundary();CHECK(!solo_parts&&!solo_a&&!solo_b&&!fx_mask&&!smc_ui.on&&!actions()&&!releases);
  for(unsigned peer=0;peer<2;peer++){CHECK(!smc_states[peer].fx_down&&!smc_states[peer].fx_owned&&!smc_states[peer].fx_main_wait);CHECK(!smc_states[peer].lock_down&&!smc_states[peer].select_down&&!smc_states[peer].play_down&&!smc_states[peer].home_down);CHECK(smc_states[peer].locked&&!smc_states[peer].picked&&!smc_states[peer].seen&&!smc_states[peer].input_seen);}
  /* Overlay callbacks cannot take ownership while diagnosing. */
  smc_ui_begin(2,1);CHECK(!smc_ui.on);smc_ui.on=1;CHECK(!smc_ui_active());
  /* Exit discards input since the last active poll, even with no poll during the session. */
  all_inputs(0);all_inputs(1);diagnostic=0;smc_pad_diag_boundary();CHECK(!actions());
  smc_poll();CHECK(!actions());
  central(0,0xb0,16,1);central(1,0x90,24,127);smc_poll();CHECK(edits==2);
  smc_frame();CHECK(feedback==2);smc_ui_begin(2,1);CHECK(smc_ui.on);
  /* Outside diagnostics original release/transport continue through actual parser. */
  central(0,0x80,24,0);central(1,0x90,8,0);central(0,0x90,0x5e,127);central(1,0x90,0x5f,127);smc_poll();CHECK(releases==2&&!stops&&plays==1&&recs==1);
  /* Full queues, lost STOP and lost release generations are consumed, not replayed. */
  reset(start);diagnostic=1;
  for(unsigned i=0;i<SMC_Q;i++){central(0,0xb0,16,1);central(1,0x90,0x5e,127);host(0xbf,40,70);}
  for(unsigned i=0;i<16;i++){mmc(0,33);mmc(1,34);}
  smc_mmc_stop_published[0]=1;smc_mmc_stop_boundary[0]=smc_mmc_w[0];
  central(0,0x80,24,0); /* actual full-ring release loss */
  CHECK(smc_inputs[0].release_lost[0]);
  smc_pad_diag_boundary();CHECK(!actions()&&smc_mmc_stop_seen[0]==1&&smc_inputs[0].release_seen[0]==smc_inputs[0].release_lost[0]);
  diagnostic=0;smc_pad_diag_boundary();smc_poll();CHECK(!actions());
  /* Both peer epochs remain producer-owned while active, then reconcile normally. */
  reset(start);diagnostic=1;smc_ble_link_slot(0,0);smc_ble_link_slot(0,1);smc_ble_link_slot(1,0);
  uint32_t pub0=smc_inputs[0].link_published,pub1=smc_inputs[1].link_published;
  central(0,0xb0,16,1);smc_poll();CHECK(!actions()&&smc_inputs[0].link_published==pub0&&smc_inputs[1].link_published==pub1&&smc_inputs[0].link_seen==1);
  diagnostic=0;smc_pad_diag_boundary();central(0,0xb0,16,1);smc_poll();CHECK(edits==1&&smc_states[0].up&&!smc_states[1].up);CHECK(smc_inputs[0].link_seen==pub0&&smc_inputs[1].link_seen==pub1);
  /* Simulate an ISR publication during cleanup; a subsequent active poll still discards it. */
  reset(start);diagnostic=1;injection=1;smc_pad_diag_boundary();smc_poll();diagnostic=0;smc_pad_diag_boundary();smc_poll();CHECK(!actions());
  /* Session open/close without an active poll still cuts both boundaries. */
  reset(start);all_inputs(0);diagnostic=1;smc_pad_diag_boundary();all_inputs(1);diagnostic=0;smc_pad_diag_boundary();smc_poll();CHECK(!actions());
 }
 printf("PASS %u actual-body diagnostic gate checks: two peers, wrap/full queues, boundaries, no commands/feedback, held cleanup, epochs and normal exit\n",checks);
}
'''
# A retired User queue has only compatibility cursors, no payload storage.
# Seed cursors to prove discard; the unchanged-source red control keeps payloads.
if 'static uint8_t smc_host_q[' not in source:
    harness=harness.replace('smc_host_q[w%SMC_Q][0]=st;smc_host_q[w%SMC_Q][1]=note;smc_host_q[w%SMC_Q][2]=v;', '(void)st;(void)note;(void)v;')
    harness=harness.replace('smc_mmc_q[src][w%16]=cmd;', '(void)cmd;')
    harness=re.sub(r'smc_mmc_stop_boundary\[[^]]+\]=[^;]+;', '', harness)
with tempfile.TemporaryDirectory(prefix='smc-diag-') as d:
    c=Path(d)/'test.c';exe=Path(d)/'test';c.write_text(harness)
    subprocess.run([os.getenv('CC','cc'),'-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
