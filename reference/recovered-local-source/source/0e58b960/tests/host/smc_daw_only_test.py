#!/usr/bin/env python3
"""Compare DAW-only routing to frozen DAW and exercise actual queues and hooks.

UI/DSP callbacks are simulated; this is host evidence, not device timing proof.
The reference must be the frozen pad-gate source before DAW-only changes.
"""
import argparse
from pathlib import Path
import os
import re
import subprocess
import tempfile
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--reference-protocol',type=Path,help='smc_protocol.h for the pre-change red control only')
p.add_argument('--reference',type=Path,required=True,help='frozen pre-DAW-only smc_mixer.c')
p.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[2]/'firmware/src/app/smc_mixer.c')
a=p.parse_args();source=a.source.read_text()
reference=a.reference.read_text()
def function(name,source=source):
    m=re.search(r'^[^\n]*\b'+re.escape(name)+r'\([^\n]*\)\s*\{',source,re.M)
    if not m:raise RuntimeError('missing '+name)
    depth,pos=1,m.end()
    while depth:
        depth+=(source[pos]=='{')-(source[pos]=='}');pos+=1
    return source[m.start():pos]
prefix=source[source.index('#define SMC_Q'):source.index('int smc_project_active')]
ui_struct=source[source.index('struct smc_ui_state {'):source.index('static uint32_t smc_ui_context(void)')]
names=['smc_solo_union','smc_project_active','smc_solo_clear','smc_peer_drop','smc_project_drop',
       'smc_ble_midi_slot','smc_ble_link_slot','smc_midi_host','smc_mmc_host','smc_project_reset','smc_fx_clear','smc_ui_cancel','smc_ui_active','smc_ui_begin',
       'smc_guarded_button','smc_message','smc_mmc_drain_stop','smc_poll_peer','smc_poll','smc_frame']
for name in ('smc_button','smc_release','smc_project_reset','smc_lock_tick','smc_fx_main_tick','smc_fx_release_note','smc_frame_peer'):
    assert function(name)==function(name,reference), name+' unexpectedly changed'
functions='\n'.join(function(n) for n in names)
if 'smc_host_accept(' in source:
    assert a.reference_protocol, 'pre-change red control requires --reference-protocol'
    functions=function('smc_host_accept',a.reference_protocol.read_text())+'\n'+functions
functions+='\n'+function('smc_message',reference).replace('smc_message(', 'reference_message(',1)
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
static unsigned last_kind,last_a,last_b;
static void record(unsigned k,unsigned a,unsigned b){last_kind=k;last_a=a;last_b=b;}
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
static void smc_button(uint8_t n,int mmc){record(1,n,mmc);buttons++;if(mmc&&n==0x5d)stops++;else if(n==0x5e)plays++;else if(n==0x5f)recs++;else edits++;}
static void smc_release(uint8_t n){record(2,n,0);releases++;}
static void smc_fader(int n,uint8_t v){record(3,n,v);edits++;}
static void smc_knob(int n,int d){record(4,n,(unsigned)d);edits++;}
static int smc_relative(uint8_t v){return (v&64)?-(int)(v&63):(int)v;}
static uint8_t smc_mmc_note(uint8_t cmd){return cmd==34?0x5d:cmd==33?0x5e:cmd==35?0x5f:24;}
void smc_ui_cancel(void);
static void smc_fx_load_page(int page){(void)page;}
static void engine_smc_solo(int mask){(void)mask;}
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
static void clear_trace(void){edits=buttons=stops=plays=recs=releases=0;last_kind=last_a=last_b=0;}
static int differential(void){
 uint8_t m[6]={0};
 /* Complete valid data-byte space for every MIDI status, both legacy-bit states.
  * Compare parser return and callback identity/value with pre-change DAW. */
 for(unsigned st=0x80;st<0xf0;st++)for(unsigned a=0;a<128;a++)for(unsigned b=0;b<128;b++){
  m[0]=st;m[1]=a;m[2]=b;proj.set.bt=16;clear_trace();
  unsigned result=reference_message(m,3,1),k=last_kind,x=last_a,y=last_b,n=actions()+releases;
  for(unsigned bit=0;bit<2;bit++){
   proj.set.bt=16|bit*32;clear_trace();
   CHECK(smc_message(m,3,1)==result&&last_kind==k&&last_a==x&&last_b==y&&actions()+releases==n);
  }
 }
 /* Malformed, short, SysEx/User CC and all out-of-channel cases remain rejected. */
 for(unsigned bit=0;bit<2;bit++){
  proj.set.bt=16|bit*32;
  for(unsigned st=0;st<256;st++)for(unsigned which=1;which<3;which++){
   m[0]=st;m[1]=8;m[2]=0;m[which]|=128;clear_trace();CHECK(smc_message(m,3,1)==4&&!actions()&&!releases);
  }
  for(unsigned len=0;len<3;len++){clear_trace();CHECK(smc_message(m,len,1)==4&&!actions()&&!releases);}
  for(unsigned cmd=1;cmd<=43;cmd++){uint8_t sx[]={0xf0,0x7f,0x7f,6,cmd,0xf7};clear_trace();CHECK(smc_message(sx,6,1)==4&&!actions()&&!releases);}
  performance=1;m[0]=0x90;m[1]=6;m[2]=127;clear_trace();CHECK(smc_message(m,3,0)==1&&buttons==1);
  m[0]=0x9f;clear_trace();CHECK(smc_message(m,3,0)==1&&!buttons);performance=0;
 }
 return 0;
}
static int queues(void){
 const unsigned starts[]={0,240,254,255};
 const unsigned notes[]={6,8,9,10,11,12,13,14,15,24,25,26,27,28,29,30,31,0x5d,0x5e};
 for(unsigned bit=0;bit<2;bit++)for(unsigned k=0;k<4;k++)for(unsigned peer=0;peer<2;peer++){
  unsigned start=starts[k];reset(start);proj.set.bt=16|bit*32;
  /* Legacy bit clear alone must not flush/rearm a live queue or held key. */
  central(peer,0xb0,16,1);smc_states[peer].play_down=1;
  unsigned read=smc_inputs[peer].qr,write=smc_inputs[peer].qw;proj.set.bt&=~32u;
  CHECK(smc_inputs[peer].qr==read&&smc_inputs[peer].qw==write&&smc_states[peer].play_down==1);
  smc_poll();CHECK(edits==1);proj.set.bt=16|bit*32;
  /* Fresh both-peer DAW releases and commands, hosted obsolete data discarded. */
  clear_trace();central(peer,0x80,8,0);central(peer,0x90,24,0);central(peer,0x90,0x5e,127);
  host(0x9f,8,127);mmc(0,33);mmc(1,34);smc_mmc_stop_published[1]++;smc_poll();
  CHECK(releases==2&&plays==1&&!stops&&smc_host_r==smc_host_w&&smc_mmc_r[0]==smc_mmc_w[0]&&smc_mmc_r[1]==smc_mmc_w[1]);
  CHECK(smc_mmc_stop_seen[1]==smc_mmc_stop_published[1]);
  /* Every supported full-ring release class, both NoteOff forms; foreign
   * channel releases must not clear another channel's held state. */
  for(unsigned j=0;j<sizeof(notes)/sizeof(*notes);j++)for(unsigned form=0;form<2;form++){
   reset(start);proj.set.bt=16|bit*32;
   for(unsigned i=0;i<SMC_Q;i++)central(peer,0xb0,16,1);
   central(peer,form?0x90:0x80,notes[j],0);
   CHECK(smc_inputs[peer].release_lost[0]==1);
   smc_states[peer].solo_target[0]=1;smc_states[peer].solo=1;
   smc_states[peer].play_down=smc_states[peer].select_down=smc_states[peer].home_down=255;
   smc_states[peer].lock_down=notes[j]==0x5d;
   smc_states[peer].fx_down=smc_states[peer].fx_owned=255;smc_poll();
   CHECK(!smc_states[peer].solo&&!smc_states[peer].play_down&&!smc_states[peer].select_down&&!smc_states[peer].home_down&&!smc_states[peer].fx_down);
   CHECK(smc_inputs[peer].release_seen[0]==1&&!smc_states[peer].lock_down);
   reset(start);proj.set.bt=16|bit*32;for(unsigned i=0;i<SMC_Q;i++)central(peer,0xb0,16,1);
   for(unsigned ch=1;ch<16;ch++)central(peer,(form?0x90:0x80)|ch,notes[j],0);
   CHECK(!smc_inputs[peer].release_lost[0]&&!smc_inputs[peer].release_lost[2]);
  }
  /* Project reset/load uses its explicit unchanged flush/rearm path. */
  reset(start);proj.set.bt=16|bit*32;central(peer,0x90,0x5e,127);host(0x9f,8,127);mmc(0,33);
  smc_states[0].play_down=smc_states[1].select_down=255;smc_project_reset();
  CHECK(smc_inputs[peer].qr==smc_inputs[peer].qw&&smc_host_r==smc_host_w&&smc_mmc_r[0]==smc_mmc_w[0]);
  CHECK(!smc_states[0].play_down&&!smc_states[1].select_down&&smc_states[0].up&&smc_states[1].up&&smc_states[0].fresh&&smc_states[1].fresh);
  smc_poll();CHECK(!actions());central(peer,0xb0,16,1);smc_poll();CHECK(edits==1);
  /* Down/up coalescence rejects pre-epoch press and admits new packet. */
  reset(start);proj.set.bt=16|bit*32;central(peer,0x90,0x5e,127);smc_ble_link_slot(peer,0);smc_ble_link_slot(peer,1);central(peer,0xb0,16,1);smc_poll();
  CHECK(edits==1&&!plays);smc_ble_link_slot(peer,0);central(peer,0x90,0x5e,127);smc_poll();CHECK(!plays);
  smc_ble_link_slot(peer,1);central(peer,0xb0,16,1);smc_poll();CHECK(edits==2);
  proj.set.bt&=~16u;central(peer,0x90,0x5e,127);smc_poll();CHECK(!plays);
  unsigned w=smc_inputs[peer].qw;central(2,0x90,0x5e,127);smc_ble_link_slot(2,1);CHECK(smc_inputs[peer].qw==w);
 }
 return 0;
}
static int hosted(void){
 reset(254);proj.set.bt=16|32;
 for(unsigned st=0;st<256;st++)for(unsigned a=0;a<256;a++)for(unsigned b=0;b<256;b+=127){
  uint32_t pkt=9u|st<<8|a<<16|b<<24;
  CHECK(!smc_midi_host(pkt)&&smc_host_w==254); /* ordinary instrument/APC route remains eligible */
 }
 for(unsigned src=0;src<256;src++)for(unsigned cmd=0;cmd<256;cmd++){
  smc_mmc_host(cmd,src);CHECK(smc_mmc_w[0]==254&&smc_mmc_w[1]==254&&!smc_mmc_stop_published[0]&&!smc_mmc_stop_published[1]);
 }
 return 0;
}
int main(void){reset(0);if(differential()||queues()||hosted())return 1;
 printf("PASS %u DAW-only actual-body checks: exhaustive parser equivalence, legacy bit, both peers/releases/overflow/reset/reconnect, hosted pass-through\n",checks);}
'''
# A retired User queue has only compatibility cursors, no payload storage.
# Seed cursors to prove discard; the unchanged-source red control keeps payloads.
if 'static uint8_t smc_host_q[' not in source:
    harness=harness.replace('smc_host_q[w%SMC_Q][0]=st;smc_host_q[w%SMC_Q][1]=note;smc_host_q[w%SMC_Q][2]=v;', '(void)st;(void)note;(void)v;')
    harness=harness.replace('smc_mmc_q[src][w%16]=cmd;', '(void)cmd;')
    harness=re.sub(r'smc_mmc_stop_boundary\[[^]]+\]=[^;]+;', '', harness)
with tempfile.TemporaryDirectory(prefix='smc-daw-only-') as d:
    c=Path(d)/'test.c';exe=Path(d)/'test';c.write_text(harness)
    subprocess.run([os.getenv('CC','cc'),'-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
