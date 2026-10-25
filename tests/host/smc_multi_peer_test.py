#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Production peer ingress/poll/Solo/frame fault injection. No hardware claim."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
def source_text():
    return Path(os.environ.get('SMC_SOURCE', root / 'firmware/src/app/smc_mixer.c')).read_text()
def extract(source, name):
    m = re.search(r'^[^\n]*\b' + name + r'\([^\n]*\)\s*\{', source, re.M)
    if not m:
        raise ValueError(name)
    i, depth = m.end(), 1
    while depth:
        depth += (source[i] == '{') - (source[i] == '}')
        i += 1
    return source[m.start():i]
def make_code(s):
    prefix = s[s.index('#define SMC_Q'):s.index('int smc_project_active')]
    prefix = prefix.replace(extract(s, 'smc_clip_on'), '')
    return r'''
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
typedef struct { uint8_t kind,a,b,c; } pref_t;
static struct { struct { uint8_t bt; } set; } proj;
static struct { uint8_t overlay; } ui;
static struct { uint8_t on,peer; } smc_ui;
static int busy,testing,same_target,reset_at,inject_edge,inject_loss;
static int actions[3], sent[2], tx_fail_peer=-1,tx_edge_peer=-1;
static uint8_t published;
static uint16_t drums[2];
static int perf_testing(void) { return testing; }
static int ui_project_busy(void) { return busy; }
static void engine_fx_mute(int n) { (void)n; }
static void engine_smc_solo(int n) { published=(uint8_t)n; drums[0]=drums[1]=0; }
static void engine_smc_solo_targets(uint8_t n,uint16_t a,uint16_t b)
{ published=n; drums[0]=a; drums[1]=b; }
static void ui_say(const char *a,const char *b) { (void)a;(void)b; }
static void smc_ui_cancel(void) { smc_ui.on=0; }
static void smc_solo_publish(void);
static int smc_lock_button(int pressed);
static void smc_guarded_button(uint8_t,int,int);
''' + prefix + '\n' + '\n'.join(extract(s, n) for n in (
    'smc_project_active','smc_solo_clear','smc_solo_publish','smc_solo_press',
    'smc_project_drop','smc_project_reset','smc_ble_midi_peer','smc_ble_midi',
    'smc_ble_link_peer','smc_ble_link','smc_ble_role_get','smc_connected',
    'smc_midi_host','smc_mmc_host','smc_release')) + r'''
static int smc_lock_button(int pressed) { smc.lock_down=!!pressed; return 1; }
static uint8_t smc_message(const uint8_t *m,uint8_t len,int allow)
{
    if (!allow || len!=3) return 0;
    actions[smc_peer]++;
    if (reset_at && m[1]==20) {reset_at=0;smc_project_reset();return 0;}
    if ((m[0]&0xf0)==0x90 && m[1]>=8 && m[1]<16 && m[2])
        smc_solo_press(m[1]-8,same_target?1:smc_peer+1,0);
    if ((m[0]&0xf0)==0x80 || ((m[0]&0xf0)==0x90 && !m[2])) smc_release(m[1]);
    if(inject_edge && smc_peer==0) {
        inject_edge=0;smc_ble_link_peer(0,0);smc_ble_link_peer(0,1);
        const uint8_t press[]={0x90,8,127};smc_ble_midi_peer(0,press,3);
    }
    if(inject_loss && smc_peer==0) {
        inject_loss=0;
        const uint8_t press[]={0x90,8,127},off[]={0x80,8,0};
        smc_ble_midi_peer(0,press,3);
        for(int i=1;i<32;i++) {const uint8_t cc[]={0xB0,16,1};smc_ble_midi_peer(0,cc,3);}
        smc_ble_midi_peer(0,off,3);
    }
    return 0;
}
static void smc_guarded_button(uint8_t note,int mmc,int allow)
{ (void)mmc;if(allow && note==0x5d && reset_at){reset_at=0;smc_project_reset();} }
''' + '\n'.join(extract(s, n) for n in ('smc_mmc_drain_stop','smc_context_prepare','smc_poll_peer','smc_poll')) + r'''
static void smc_notice(int n) {(void)n;}
static int smc_led(int n) {(void)n;return 1;}
int plat_ble_smc_send_peer(uint8_t peer,const uint8_t *msg,uint8_t n)
{
    assert(peer<2&&msg&&n==3);sent[peer]++;
    if(tx_edge_peer==peer){tx_edge_peer=-1;smc_ble_link_peer(peer,0);smc_ble_link_peer(peer,1);}
    return tx_fail_peer!=peer;
}
''' + '\n'.join(extract(s, n) for n in ('smc_frame_peer','smc_frame')) + r'''
static void emit(int p,uint8_t st,uint8_t a,uint8_t b)
{const uint8_t m[]={st,a,b};smc_ble_midi_peer((uint8_t)p,m,3);}
static void press(int p) {emit(p,0x90,8,127);}
static void off(int p) {emit(p,0x80,8,0);}
static void fill(int p) {for(int i=0;i<32;i++)emit(p,0xB0,16,1);}
static void reset(int cursor)
{
    memset(smc_states,0,sizeof smc_states);memset(smc_inputs,0,sizeof smc_inputs);
    memset(smc_links_seen,0,sizeof smc_links_seen);memset(smc_losses_seen,0,sizeof smc_losses_seen);
    memset((void*)smc_link_seq,0,sizeof smc_link_seq);smc_select(0);
    memset((void*)smc_pause_lost,0,sizeof smc_pause_lost);
    memset(smc_pause_seen,0,sizeof smc_pause_seen);
    memset((void*)smc_pause_link_boundary,0,sizeof smc_pause_link_boundary);
    memset((void*)smc_mmc_w,0,sizeof smc_mmc_w);memset((void*)smc_mmc_r,0,sizeof smc_mmc_r);
    memset((void*)smc_mmc_stop_w,0,sizeof smc_mmc_stop_w);memset((void*)smc_mmc_stop_r,0,sizeof smc_mmc_stop_r);
    smc_host_r=smc_host_w=0;smc_host_solo_release_lost=smc_host_solo_release_seen=0;
    memset(actions,0,sizeof actions);memset(sent,0,sizeof sent);
    busy=testing=reset_at=inject_edge=inject_loss=same_target=0;tx_fail_peer=tx_edge_peer=-1;
    smc_solo_all=published=0;smc_drum_solo_all[0]=smc_drum_solo_all[1]=0;
    proj.set.bt=PROJ_BT_SMC_CENTRAL;ui.overlay=O_NONE;
    for(int p=0;p<2;p++){smc_inputs[p].r=smc_inputs[p].w=(uint8_t)cursor;smc_ble_link_peer(p,1);}
    smc_poll();
}
static void hosted_press(void)
{
    proj.set.bt|=PROJ_BT_SMC_USER;smc_poll();
    assert(smc_midi_host(9u|0x9fu<<8|8u<<16|127u<<24));smc_poll();
}
int main(void)
{
    for(int cursor=0;cursor<256;cursor++) {
        reset(cursor);press(0);press(1);smc_poll();assert(published==3);
        off(0);smc_poll();assert(published==2);off(1);smc_poll();assert(!published);
        reset(cursor);same_target=1;press(0);press(1);smc_poll();assert(published==1);
        off(0);smc_poll();assert(published==1);off(1);smc_poll();assert(!published);
        reset(cursor);press(0);press(1);smc_poll();fill(0);off(0);off(1);smc_poll();assert(!published);
        reset(cursor);press(0);press(1);smc_poll();fill(0);off(0);smc_poll();assert(published==2);
        assert(smc_losses_seen[0]==1&&smc_losses_seen[1]==0);
        reset(cursor);press(0);press(1);smc_poll();fill(0);off(0);
        smc_ble_link_peer(0,0);smc_ble_link_peer(0,1);press(0);smc_poll();assert(published==2);
        press(0);smc_poll();assert(published==3);
        assert(smc_states[1].solo_target[0]==2);
        reset(cursor);press(0);press(1);smc_poll();fill(0);smc_ble_link_peer(0,0);smc_poll();assert(published==2);
        reset(cursor);press(0);press(1);smc_poll();inject_edge=1;emit(0,0xb0,16,1);smc_poll();
        assert(published==3);smc_poll();assert(published==3);off(0);smc_poll();assert(published==2);
        reset(cursor);press(1);smc_poll();inject_loss=1;emit(0,0xb0,16,1);smc_poll();
        assert(smc_losses_seen[0]==0);smc_poll();assert(smc_losses_seen[0]==1&&published==2);
        reset(cursor);hosted_press();assert(published==4);press(0);press(1);smc_poll();
        smc_ble_link_peer(0,0);smc_poll();assert(published==6);off(1);smc_poll();assert(published==4);
        reset(cursor);press(0);press(1);smc_poll();reset_at=1;emit(0,0x90,20,127);press(1);smc_poll();
        assert(!published&&!smc_project_active());assert(smc_inputs[1].r==smc_inputs[1].w);
        reset(cursor);emit(0,0xb0,16,1);proj.set.bt|=PROJ_BT_SMC_USER;
        emit(1,0xb0,16,1);smc_poll();assert(actions[0]==0&&actions[1]==1);
        reset(cursor);fill(0);smc_states[0].lock_down=smc_states[0].play_down=smc_states[0].select_down=1;
        off(0);smc_poll();assert(smc_states[0].lock_down&&!smc_states[0].play_down&&!smc_states[0].select_down);
        fill(0);emit(0,0x80,0x5d,0);smc_poll();assert(!smc_states[0].lock_down);
        reset(cursor);smc_select(0);smc_solo_press(0,NPARTS+1+NDRUM,0);
        smc_select(1);smc_solo_press(0,NPARTS+1+NDRUM,0);smc_release(8);assert(drums[1]==1);
        smc_select(0);smc_release(8);assert(!drums[1]);
    }
    assert(!smc_resuming);
    reset(0);smc_state_t before[3];smc_ble_input_t inputs[2];
    memcpy(before,smc_states,sizeof before);memcpy(inputs,smc_inputs,sizeof inputs);
    smc_ble_link_peer(255,1);emit(255,0x90,8,127);
    assert(!memcmp(before,smc_states,sizeof before)&&!memcmp(inputs,smc_inputs,sizeof inputs));
    reset(0);tx_fail_peer=0;smc_frame();assert(sent[0]==1&&sent[1]==51);
    assert(!smc_states[0].known[0]&&smc_states[1].known[0]);
    tx_fail_peer=-1;smc_frame();assert(sent[0]==52&&sent[1]==51);
    reset(0);tx_edge_peer=0;smc_frame();assert(!smc_states[0].known[0]&&smc_states[1].known[0]);
    smc_poll();smc_frame();assert(smc_states[0].known[0]);
    printf("END");
    for (int p = 0; p < 2; p++)
        printf(" p%d u=%u f=%u r=%u w=%u link=%u rel=%u known=%u solo=%u lock=%u",
               p, smc_states[p].up, smc_states[p].fresh, smc_inputs[p].r, smc_inputs[p].w,
               smc_inputs[p].link, smc_inputs[p].release_lost, smc_states[p].known[0],
               smc_states[p].solo, smc_states[p].lock_down);
    printf(" h=%u sent=%d,%d act=%d,%d pub=%u bytes=%u\n",
           smc_states[2].up, sent[0], sent[1], actions[0], actions[1], published,
           (unsigned)sizeof(smc_ble_input_t));
    puts("PASS: production multi-peer ingress/poll/Solo: all 256 cursors; feedback isolation/failure/session ack");
}
'''
def compile_and_run(source):
    code = make_code(source)
    with tempfile.TemporaryDirectory(prefix='smc-multi-peer-') as tmp:
        tmp = Path(tmp)
        c = tmp / 'test.c'
        exe = tmp / 'test'
        c.write_text(code)
        subprocess.run([os.environ.get('CC','cc'), '-std=c99', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
                        *shlex.split(os.environ.get('SAN_FLAGS','')), '-I' + str(root / 'firmware/src'),
                        str(c), '-o', str(exe)], check=True)
        return subprocess.check_output([str(exe)], text=True)
if __name__ == '__main__':
    print(compile_and_run(source_text()), end='')
