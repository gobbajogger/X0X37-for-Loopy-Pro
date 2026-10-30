#!/usr/bin/env python3
"""Exercise actual UI safe-persistence admission and retry with native stubs."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[2])
args = parser.parse_args()
root = args.source_root.resolve()
s = (root / 'firmware/src/app/ui.c').read_text()
parts = []
for sig in ('X0X_COLD int ui_boot_flash_busy(void)', 'static void device_bt_save(void)'):
    a = s.index(sig + '\n{'); b = s.index('\n}\n', a) + 3
    parts.append(s[a:b])
prefix = r'''
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define X0X_COLD
#define BOARD_SMK37 1
static struct { unsigned playing, req_start, req_cont, req_stop; } seq;
static struct { unsigned rec; } ui;
static int project_busy, held, stage, notes, perf, ck, pending=1, writes, result, race, hold_calls;
static uint32_t now=100;
static int project_device_bt_pending(void) { return pending; }
static int project_device_bt_save(void) { assert(held); writes++; if(!result) pending=0; return result; }
static int store_stage_held(void) { return stage; }
static int engine_project_notes(void) { return notes; }
static int perf_save_busy(void) { return perf; }
static int perf_testing(void) { return perf; }
uint8_t smk_ck_flash_busy(void) { return ck; }
static void engine_project_hold(int on) {
    if(on) {
        /* Even a hold released before return can zero an intervening audio block.
         * Admission must reject already-busy audio without acquiring that hold. */
        assert(!seq.playing && !seq.req_start && !seq.req_cont && !seq.req_stop &&
               !ui.rec && !notes && !perf && !ck);
        hold_calls++;
    }
    held=on;
    if(on && race) seq.req_start=1; /* Start appears between precheck and held recheck */
}
static uint32_t plat_ms(void) { return now; }
static void say(const char *a, const char *b) { (void)a; (void)b; }
'''
suffix = r'''
int main(void) {
    unsigned *transport[] = {&seq.playing, &seq.req_start, &seq.req_cont, &seq.req_stop, &ui.rec};
    int *busy[] = {&project_busy,&stage,&notes,&perf,&ck};
    for(unsigned k=0;k<5;k++) { *transport[k]=1; device_bt_save(); assert(!writes && !held && pending && !hold_calls); *transport[k]=0; }
    for(unsigned k=0;k<5;k++) { *busy[k]=1; device_bt_save(); assert(!writes && !held && pending && !hold_calls); *busy[k]=0; }
    race=1; device_bt_save(); assert(!writes && !held && pending && hold_calls==1); race=seq.req_start=0;
    result=-1; device_bt_save(); assert(writes==1 && pending && !held);
    device_bt_save(); assert(writes==1); now+=1000; result=0;
    device_bt_save(); assert(writes==2 && !pending && !held);
    pending=1; now++; device_bt_save(); assert(writes==3 && !pending); /* no 20-second gap */
    puts("PASS actual UI device save: no hold while already busy, held race recheck, retry and first safe flush");
}
'''
with tempfile.TemporaryDirectory(prefix='ble-save-') as d:
    code=Path(d)/'test.c'; exe=Path(d)/'test'; code.write_text(prefix+'\n'.join(parts)+suffix)
    subprocess.run([os.getenv('CC','cc'),'-std=c99','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-fno-sanitize-recover=all',str(code),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
