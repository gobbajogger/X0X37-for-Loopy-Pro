#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Run actual production frame/sender bodies across a producer-entry reconnect.
Independent regression originally supplied by the session-contract reviewer.
Uses native HAL stubs only; no SDK/HCI replay, package or device access.
"""
from pathlib import Path
import os
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
app = (root/'firmware/src/ble_vendor.c').read_text()
mixer = (root/'firmware/src/app/smc_mixer.c').read_text()
def body(src, sig):
    begin = src.index(sig); end = src.index('{', begin) + 1; depth = 1
    while depth:
        depth += (src[end] == '{') - (src[end] == '}'); end += 1
    return src[begin:end] + '\n'
prefix=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "x0x_smc_host_messages.h"
#define X0X_SMC_VENDOR_TEST 1
static struct { xb_mbox_t mb; xsm_boot_t boot; struct { int connected; } ingress; } ble_r;
static int ble_up=1, ble_host_on=1, ble_want=1;
static volatile uint32_t ble_feedback_epoch=1, smc_link_published=3;
static uint32_t smc_link_seen=3, smc_feedback_epoch_seen=1;
static struct { uint8_t up, known[128], lit[128], pending, value[8]; } smc;
static unsigned calls;
static int interrupt_at_send, interrupt_at_sync;
static void reconnect(void) {
    ble_feedback_epoch=2; ble_r.boot.feedback_epoch=2; smc_link_published=7;
}
static void fm1_ble_sync(void) { if(interrupt_at_sync) { interrupt_at_sync=0; reconnect(); } }
static int ui_project_busy(void) { return 0; }
static int smc_ble_role_get(void) { return 1; }
static void smc_notice(int n) { (void)n; }
static int smc_led(int n) { (void)n; return 1; }
'''
new_sender=''.join(body(app,s) for s in ['uint32_t plat_ble_smc_session(void)','int plat_ble_smc_send_session(uint32_t epoch, const uint8_t *msg, uint8_t len)','int plat_ble_smc_send(const uint8_t *msg, uint8_t len)'])
new_frame=body(mixer,'void smc_frame(void)')
new_wrapper=r'''
static int interrupt_before_sender(uint32_t epoch,const uint8_t *msg,uint8_t len) {
    if (calls++) return 0;
    if(interrupt_at_send) reconnect();
    return plat_ble_smc_send_session(epoch,msg,len);
}
#define plat_ble_smc_send_session interrupt_before_sender
'''
new_main=r'''
#undef plat_ble_smc_send_session
static void reset(void) {
    memset(&smc,0,sizeof smc); xb_ring_init(&ble_r.mb.h2c); calls=0;
    smc.up=1; ble_r.boot.feedback_epoch=ble_feedback_epoch=1;
    ble_r.ingress.connected=1; smc_link_seen=smc_link_published=3; smc_feedback_epoch_seen=1;
    interrupt_at_send=interrupt_at_sync=0;
}
int main(void) {
    uint8_t out[7],type;
    reset(); interrupt_at_send=1; smc_frame();
    assert(!xb_ring_used(&ble_r.mb.h2c) && ble_r.boot.feedback_epoch==2);
    reset(); reconnect(); smc_frame(); assert(!calls && !xb_ring_used(&ble_r.mb.h2c));
    smc_link_seen=smc_link_published; smc_feedback_epoch_seen=2; smc_frame();
    assert(xb_frame_get(&ble_r.mb.h2c,&type,out,sizeof out)==7 && xsm_epoch_read(out)==2);
    reset(); interrupt_at_sync=1; smc_frame(); assert(!calls && !xb_ring_used(&ble_r.mb.h2c));
    const uint8_t msg[3]={0x90,0,127}; assert(!plat_ble_smc_send(msg,3));
    puts("FIXED ACTUAL PRODUCER PASS: mid-construction reconnect is rejected; unacknowledged new link blocks; acknowledged new link sends fresh; snapshot race blocks; untagged send rejects");
}
'''

with tempfile.TemporaryDirectory() as temp:
    out = Path(temp)
    source = out/'producer.c'; binary = out/'producer'
    source.write_text(prefix + new_sender + new_wrapper + new_frame + new_main)
    subprocess.run([os.environ.get('CC','cc'), '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-I'+str(root/'firmware/hal'), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
