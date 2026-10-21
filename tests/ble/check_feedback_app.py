#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile unchanged production app service/sender bodies with native HAL stubs.
No proprietary producer, SDK stack, HCI replay, firmware package or device access.
"""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'firmware/src/ble_vendor.c').read_text()

def function(signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'

prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "x0x_smc_host_messages.h"
#include "ble/smc_host_ingress.h"
static struct { xb_mbox_t mb; xsm_boot_t boot; smc_host_ingress_t ingress; uint8_t fr[128]; } ble_r;
static struct { int trying; } ble_guard;
static uint8_t ble_up=1, ble_want=1, ble_host_on=1, ble_central_link_seen;
static volatile uint32_t ble_feedback_epoch;
static uint32_t ble_alive[2], fm1_ms=1000, ble_ok_at;
static int fm1_c1_on=1, link_up, link_down, accepted, race;
#define BLE_GUARD_OK_MS 100u
static void ble_guard_ok(void *p) { (void)p; }
static void ble_record(void) {}
static void ble_fail(void) { ble_up=0; }
static void fm1_ble_sync(void) {
    if (race) { race=0; ble_feedback_epoch=ble_r.boot.feedback_epoch=3; }
}
static void smc_ble_link(int up) { if(up) ++link_up; else ++link_down; }
static void smk_inlog(uint32_t a,uint32_t b) { (void)a;(void)b; }
static int smc_ble_midi_accept(const uint8_t *p,uint8_t n) {
    assert(n==3 && p[1]==45); ++accepted; return 1;
}
'''
main = r'''
static void link_frame(uint32_t epoch,int up) {
    uint8_t p[6]; xsm_epoch_write(p,epoch); p[4]=(uint8_t)up;p[5]=0;
    assert(!xb_frame_put(&ble_r.mb.c2h,XSM_LINK_SESSION,p,sizeof p));
}
int main(void) {
    ble_r.mb.state=XB_ST_READY;
    ble_r.boot.feedback_epoch=ble_feedback_epoch=1;
    ble_r.ingress.connected=1; ble_central_link_seen=1;
    const uint8_t old[5]={0x80,0x81,0x90,44,127}, fresh[5]={0x80,0x81,0x90,45,0};
    link_frame(1,1); assert(!xb_frame_put(&ble_r.mb.c2h,XSM_NOTIFY,old,5));
    link_frame(1,0); link_frame(2,1);
    assert(!xb_frame_put(&ble_r.mb.c2h,XSM_NOTIFY,fresh,5));
    ble_r.boot.feedback_epoch=2; ble_service();
    assert(accepted==1 && link_down==1 && link_up==1 && ble_feedback_epoch==2);
    ble_r.ingress.decoder.running=0x90; link_frame(2,1); ble_service();
    assert(ble_r.ingress.decoder.running==0x90 && link_up==1);
    uint8_t legacy[2]={1,0}; assert(!xb_frame_put(&ble_r.mb.c2h,XSM_LINK,legacy,2));
    ble_service(); assert(ble_r.ingress.rejected==2); /* stale old notify + legacy link */
    const uint8_t midi[3]={0x90,45,0}; uint8_t frame[7],type;
    assert(plat_ble_smc_send_session(plat_ble_smc_session(),midi,3));
    assert(xb_frame_get(&ble_r.mb.h2c,&type,frame,7)==7 && type==XSM_MIDI_SESSION);
    assert(xsm_epoch_read(frame)==2 && !memcmp(frame+4,midi,3));
    /* TIMER5 changes current identity between snapshot and publication. */
    uint32_t snapshot=plat_ble_smc_session(); race=1; assert(!plat_ble_smc_send_session(snapshot,midi,3));
    assert(!xb_ring_used(&ble_r.mb.h2c));
    assert(ble_feedback_epoch==3);
    assert(!plat_ble_smc_send(midi,3));
    ble_want=0; ble_service(); assert(!ble_feedback_epoch && !ble_r.boot.radio_desired);
    assert(!plat_ble_smc_send_session(plat_ble_smc_session(),midi,3));
    ble_want=1; link_frame(3,1); ble_service(); assert(plat_ble_smc_send_session(plat_ble_smc_session(),midi,3));
    ble_r.mb.state=XB_ST_FAILED; ble_service();
    assert(!ble_feedback_epoch && !plat_ble_smc_send(midi,3));
    puts("actual app service/sender: stale links, duplicate up, late producer, OFF/failure PASS");
}
'''
with tempfile.TemporaryDirectory() as temp:
    p = Path(temp)
    bodies = ''.join(function(s) for s in (
        'static void ble_central_link_notify(int up)',
        'static int ble_vendor_emit(void *ctx, const uint8_t *p, uint8_t n)',
        'static void ble_service(void)',
        'uint32_t plat_ble_smc_session(void)',
        'int plat_ble_smc_send_session(uint32_t epoch, const uint8_t *msg, uint8_t len)',
        'int plat_ble_smc_send(const uint8_t *msg, uint8_t len)'))
    (p / 'test.c').write_text(prefix + bodies + main)
    subprocess.run([os.environ.get('CC','cc'), '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-I'+str(root/'firmware/hal'), '-I'+str(root/'firmware/src'),
                    str(p/'test.c'), '-o', str(p/'test')], check=True)
    subprocess.run([str(p/'test')], check=True)
