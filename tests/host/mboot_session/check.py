#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build synthetic tests and exact production disabled editor case; no device."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[3]

def body(text,signature):
    start=text.index(signature);left=text.index('{',start);end=left+1;depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--target-compiler',type=Path);a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    logs=[]
    def run(cmd,**kwargs):
        cmd=list(map(str,cmd));p=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,**kwargs)
        logs.append({'command':cmd,'exit':p.returncode,'output':p.stdout});print(p.stdout,end='')
        (a.output/'commands.json').write_text(json.dumps(logs,indent=2)+'\n');p.check_returncode()
    inc=ROOT/'tools/mboot';flags=['-std=c11','-Wall','-Wextra','-Werror','-I'+str(inc)]
    src=[inc/'capture_core.c',inc/'session_wire.c',Path(__file__).with_name('fixture.c')]
    run(['cc',*flags,'-DFIXTURE_MAIN',*src,'-o',a.output/'session'])
    run([a.output/'session'])
    run(['cc',*flags,'-DFIXTURE_MAIN','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie',*src,'-o',a.output/'session-sanitized'])
    run([a.output/'session-sanitized'],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
    run([sys.executable,'-B',Path(__file__).with_name('test_session.py')])
    editor=(ROOT/'firmware/src/app/editor.c').read_text()
    # Exact decoder/encoder and production case, not an independently rewritten responder.
    case=body(editor,'    case ED_CAPTURE:')
    funcs='\n'.join(body(editor,x) for x in ('static void ed_b(', 'static void ed_put7(', 'static uint32_t ed_unpack_exact('))
    harness='''#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "session_wire.h"
static uint8_t ed_out[732], ed_smp_buf[512];static uint32_t ed_n;
#define ED_CAPTURE 49
'''+funcs+'''
static void dispatch(const uint8_t *a,uint32_t na){switch(ED_CAPTURE){
'''+case+'''
}}
int main(void){uint8_t q[134]={1,0},packed[160],decoded[32];uint32_t n,i;
 ed_n=0;ed_put7(q,6);n=ed_n;for(i=0;i<n;i++)packed[i]=ed_out[i];
 ed_n=0;dispatch(packed,n);assert(ed_unpack_exact(ed_out,ed_n,decoded,sizeof decoded)==21);
 assert(decoded[6]==0&&mb_wire_u32(decoded+7)==0);
 q[1]=1;ed_n=0;ed_put7(q,70);n=ed_n;for(i=0;i<n;i++)packed[i]=ed_out[i];
 ed_n=0;dispatch(packed,n);assert(ed_unpack_exact(ed_out,ed_n,decoded,sizeof decoded)==7);
 assert(decoded[6]==MB_W_UNSUPPORTED);
 packed[0]=127;ed_n=0;dispatch(packed,2);assert(ed_n==0);
 puts("actual editor disabled CAPS/OPEN and malformed framing PASS");return 0;}
'''
    (a.output/'editor.c').write_text(harness)
    run(['cc',*flags,a.output/'editor.c','-o',a.output/'editor']);run([a.output/'editor'])
    assert 'if (p[3] == ED_CAPTURE ||' in editor
    assert 'mb_wire_dispatch(' not in editor and 'mb_wire_disabled(' in case
    if a.target_compiler:
        cc=a.target_compiler.resolve();pin='c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f'
        assert hashlib.sha256(cc.read_bytes()).hexdigest()==pin,'unrecognized compiler'
        for source in src[:2]:
            run([cc,'-target','pi32v2','-mcpu=r3','-Oz','-ffreestanding','-fno-builtin',*flags,'-c',source,'-o',a.output/(source.stem+'.o')])
    print('PASS: synthetic session/host and production unsupported responder; no platform qualification')
if __name__=='__main__':main()
