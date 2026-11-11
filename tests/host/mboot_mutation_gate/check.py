#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host proof of pure gate + exact extracted common wrappers; no hardware calls."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[3]


def function(source,signature):
    start=source.index(signature)
    left=source.index('{',start);depth=1;end=left+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    a.output.mkdir(parents=True,exist_ok=True)
    flags=['-std=c11','-Wall','-Wextra','-Werror','-pedantic','-I'+str(ROOT/'firmware/hal')]
    def run(args,**kw):
        r=subprocess.run(list(map(str,args)),text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,**kw)
        print(r.stdout,end='');r.check_returncode();return r.stdout
    test=Path(__file__).with_name('test_gate.c')
    run(['cc',*flags,test,'-o',a.output/'gate'])
    pure=run([a.output/'gate'])
    run(['cc',*flags,'-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie',test,'-o',a.output/'gate-sanitized'])
    run([a.output/'gate-sanitized'],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
    flash=(ROOT/'firmware/hal/fm1_flash.h').read_text()
    erase=function(flash,'static int fl_erase4k(')
    write=function(flash,'static int fl_write(')
    assert erase.index('fm1_capture_mutation_allowed')<erase.index('FL_PARK')<erase.index('fl_erase4k_ram')
    assert write.index('fm1_capture_mutation_allowed')<write.index('FL_PARK')<write.index('fl_prog_ram')
    # Compile the actual changed wrapper bodies, replacing only low-level HAL calls
    # with counters. Never compile/execute SPI/MMIO/asm source on the host.
    harness='''#include "fm1_capture_gate.h"
#include <assert.h>
#include <stdio.h>
static unsigned calls;
#define FL_PARK() 0
#define FL_UNPARK() ((void)0)
#define FL_FAR(x) (x)
static uint32_t irq_save(void){return 0;}
static void irq_restore(uint32_t f){(void)f;}
static int fl_erase4k_ram(uint32_t off,uint32_t *t){(void)off;(void)t;calls++;return 0;}
static int fl_prog_ram(uint32_t off,const uint8_t *p,uint32_t n,uint32_t *t)
{(void)off;(void)p;(void)n;(void)t;calls++;return 0;}
'''+erase+'\n'+write+'''
int main(void){uint32_t took=0;uint8_t bytes[300]={0};
assert(!fl_erase4k(0,&took));assert(!fl_write(0,bytes,300));assert(calls==3);
assert(!fm1_capture_gate_acquire(&fm1_capture_barrier,1,1,1));
assert(fl_erase4k(0,&took)==FM1_CAPTURE_BUSY);assert(calls==3);
assert(fl_write(0,bytes,300)==FM1_CAPTURE_BUSY);assert(calls==3);
assert(!fm1_capture_gate_current(&fm1_capture_barrier,1));
assert(!fm1_capture_gate_release(&fm1_capture_barrier,1));
assert(!fl_write(0,bytes,300));assert(calls==5);
assert(!fm1_capture_gate_acquire(&fm1_capture_barrier,2,1,1));
assert(!fl_write(0,bytes,0));assert(calls==5);
assert(fm1_capture_gate_current(&fm1_capture_barrier,2));
fm1_capture_handoff();assert(!fm1_capture_gate_current(&fm1_capture_barrier,2));
puts("PASS: exact common wrappers block raw calls; reset hook revokes; zero-length no mutation");return 0;}
'''
    with tempfile.TemporaryDirectory(prefix='mboot-wrapper-proof-') as tmp:
        p=Path(tmp)/'wrappers.c';p.write_text(harness)
        run(['cc',*flags,'-fsanitize=address,undefined','-no-pie',p,'-o',a.output/'wrappers'])
        wrappers=run([a.output/'wrappers'],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
    system=(ROOT/'firmware/hal/fm1_sys.h').read_text()
    for sig in ['static void fm1_reboot(', 'static void fm1_enter_uboot(',
                'FM1_INLINE void fm1_core_reset(', 'FM1_INLINE void fm1_enter_update(']:
        body=function(system,sig)
        assert body.count('fm1_capture_handoff();')==1
        first=body.index('fm1_capture_handoff();')
        side=min(body.find(x) for x in ('__asm__','*(volatile') if x in body)
        assert first<side
    # No actual firmware acquisition call may sneak in under a disabled claim.
    for p in (ROOT/'firmware').rglob('*'):
        if p.suffix in ('.c','.h') and p.name!='fm1_capture_gate.h':
            assert 'fm1_capture_gate_acquire(' not in p.read_text(),str(p)
    result={'pure_gate':pure.strip(),'actual_wrappers':wrappers.strip(),'reset_boundaries':4,
            'platform':'OFF_NO_PRODUCTION_ACQUISITION','limits':['Host mocks never execute MMIO','Not complete multi-core exclusion or target qualification']}
    (a.output/'host-receipt.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__=='__main__':main()
