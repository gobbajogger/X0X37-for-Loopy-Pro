#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exact high-level admission tests; synthetic host counters, no MMIO/device."""
import argparse
import json
import os
from pathlib import Path
import resource
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[3]
BASE='8f277ca86c88659f5bf7034aed33edaec0866371'


def body(source,signature):
    start=source.index(signature);left=source.index('{',start);end=left+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',required=True,type=Path);a=ap.parse_args()
    a.output.mkdir(parents=True,exist_ok=True)
    flags=['-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-sign-compare',
           '-I'+str(ROOT/'firmware/hal'),'-I'+str(ROOT/'firmware/src')]
    log=[]
    def run(cmd,expected_failure=False,**kw):
        def nocore():resource.setrlimit(resource.RLIMIT_CORE,(0,0))
        r=subprocess.run(list(map(str,cmd)),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,preexec_fn=nocore,**kw)
        log.append({'command':list(map(str,cmd)),'exit':r.returncode,'output':r.stdout})
        print(r.stdout,end='')
        if expected_failure:
            if r.returncode==0 or 'st_save' not in r.stdout:raise AssertionError('baseline must compile then fail admission assertion')
        else:r.check_returncode()
    source=Path(__file__).with_name('test_storage_ota.c')
    run(['cc',*flags,source,'-o',a.output/'storage-ota'])
    run([a.output/'storage-ota'])
    run(['cc',*flags,'-fsanitize=address,undefined','-no-pie',source,'-o',a.output/'storage-ota-sanitized'])
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')
    run([a.output/'storage-ota-sanitized'],env=env)
    with tempfile.TemporaryDirectory(prefix='mboot-admission-proof-') as tmp:
        tmp=Path(tmp)
        for name in ('storage.c','ota.c'):
            (tmp/name).write_bytes(subprocess.check_output(['git','-C',str(ROOT),'show',BASE+':firmware/src/'+name]))
        # Same harness/new pure gate, actual old caller sources: compile succeeds,
        # held st_save performs old work and the refusal assertion must fail.
        run(['cc','-I'+str(tmp),*flags,source,'-o',a.output/'baseline-negative'])
        run([a.output/'baseline-negative'],expected_failure=True)
        editor=(ROOT/'firmware/src/app/editor.c').read_text()
        service=body(editor,'static int ed_service(void)')
        handle=body(editor,'static void ed_handle(')
        handle=handle[:handle.index('    switch (cmd)')]+'''    (void)i;(void)a;(void)na;dispatch++;
}\n'''
        poll=body((ROOT/'firmware/src/app/main_fm1.c').read_text(),'static int capture_maintenance_poll(void)')
        enum=editor[editor.index('enum { ED_INFO'):editor.index(';',editor.index('enum { ED_INFO'))+1]
        harness='''#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "fm1_capture_gate.h"
#define ED_MAINTENANCE_ADMIT() fm1_capture_pre_admit()
#define ED_MAINTENANCE_HELD() fm1_capture_maintenance_held()
#define ED_HDR0 0x7d
#define ED_HDR1 0x46
#define ED_HDR2 0x4c
static unsigned begins,dispatch,housekeep,ended,consumed;
static uint32_t fm1_ms=20000,ed_reboot_at;
static struct {uint32_t mode,recovery,at;} ed_project;
static struct {uint32_t ota_req,uboot_req;} usb;
static uint8_t packet[4]={ED_HDR0,ED_HDR1,ED_HDR2,9};
static int pending;
static void ed_begin(uint32_t cmd){(void)cmd;begins++;}
static void ed_project_end(int apply){(void)apply;ended++;ed_project.mode=0;}
static void boot_housekeep(void){housekeep++;}
static int boot_cmd(uint32_t cmd){(void)cmd;return 0;}
static int ota_frame_get(const uint8_t **p,uint32_t *n){*p=packet;*n=4;return pending;}
static void ota_frame_done(void){pending=0;consumed++;}
'''+enum+'\n'+handle+'\n'+service+'\n'+poll+'''
int main(void){
assert(!capture_maintenance_poll());
assert(!fm1_capture_gate_acquire(&fm1_capture_barrier,1,1,1));
assert(capture_maintenance_poll());assert(fm1_capture_gate_current(&fm1_capture_barrier,1));
ed_project.mode=1;pending=1;usb.ota_req=1;usb.uboot_req=1;ed_reboot_at=5;
assert(capture_maintenance_poll());
assert(!pending && consumed==1 && !usb.ota_req && !usb.uboot_req && !ed_reboot_at);
assert(ed_project.mode==1 && !housekeep && !ended && !begins && !dispatch);
assert(!fm1_capture_gate_current(&fm1_capture_barrier,1));
ed_handle(packet,4);assert(!begins && !dispatch);
assert(!fm1_capture_gate_release(&fm1_capture_barrier,1));
assert(!capture_maintenance_poll());assert(!pending);
pending=1;assert(!ed_service());
assert(housekeep==1 && ended==1 && begins==1 && dispatch==1 && consumed==2);
puts("PASS: actual editor pre-dispatch/service and main poll; idle held lease not revoked; queued work discarded; inactive dispatch unchanged");return 0;}
'''
        p=tmp/'editor.c';p.write_text(harness)
        run(['cc',*flags,'-fsanitize=address,undefined','-no-pie',p,'-o',a.output/'editor-main'])
        run([a.output/'editor-main'],env=env)
    main_source=(ROOT/'firmware/src/app/main_fm1.c').read_text()
    for sig in ('static void safe_main(void)','static void fm1_main(void)'):
        code=body(main_source,sig);assert code.count('capture_maintenance_poll()')==1
        assert code.index('usb_retry(fm1_ms)')<code.index('capture_maintenance_poll()')
        assert code.index('capture_maintenance_poll()')<code.index('if (usb.ota_req)' if sig.startswith('static void safe') else 'ota_pending = ed_service()')
    wiring=(ROOT/'firmware/src/x0x.c').read_text()
    for macro in ('ST_MAINTENANCE_ADMIT','OTA_MAINTENANCE_ADMIT','ED_MAINTENANCE_ADMIT'):
        assert '#define '+macro+'() fm1_capture_pre_admit()' in wiring
        assert wiring.index('#define '+macro)<wiring.index('#include \"storage.c\"')
    result={'result':'PASS','baseline':'COMPILED_THEN_EXPECTED_REFUSAL_ASSERTION_FAILURE',
        'candidate':'ACTUAL_STORAGE_OTA_AND_EDITOR_MAIN_PREFIXES_PASS','normal_safe_main_admission_points':2,
        'platform':'OFF','limits':['No live capture dispatch or device','Editor prefix executes pre-switch admission, not all command bodies']}
    (a.output/'commands.json').write_text(json.dumps(log,indent=2)+'\n')
    (a.output/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))


if __name__=='__main__':main()
