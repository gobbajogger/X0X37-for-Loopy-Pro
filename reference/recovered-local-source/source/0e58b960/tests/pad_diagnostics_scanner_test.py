#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Actual scanner/session bodies with simulated inputs, never hardware."""
from pathlib import Path
import re,subprocess,tempfile,os,shlex
R=Path(os.environ.get("X0X_SOURCE_ROOT",Path(__file__).resolve().parents[1]))
h=(R/'firmware/hal/smk_input.h').read_text();p=(R/'firmware/src/app/plat_fm1.c').read_text()
def fn(s,n):
 m=re.search(r'^(?:static )?(?:void|uint32_t) '+n+r'\([^\n]*\).*?\n\{',s,re.M);assert m,n
 i=m.end();d=1
 while d:d+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[m.start():i]
state=re.search(r'static volatile struct \{.*?\} fm1_in;',h,re.S).group()
prefix=r'''
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define BOARD_SMK37 1
#define FM1_NCOL 16u
#define FM1_NBTN 18u
#define FM1_NKEY 37u
#define FM1_NENC 8u
#define SMK_NPAD 16u
#define SMK_KEY_F3 5u
#define SMK_WHITE_OF_PAD(p) (p)
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
static uint32_t smk_diag_epoch,smk_diag_ack;
static uint16_t smk_diag_seen,smk_diag_suppressed,smk_div_held;
static uint8_t smk_diag_enabled,smk_div_owner,smk_div_pending,smk_pad_piano,smk_pad_note[16];
static uint16_t smk_trim_pad[16][5];
static const uint8_t SMK_WHITE_NOTE[16]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
static const int8_t SMK_PIANO_NOTE[16]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,-1};
static unsigned smk_curve(const uint16_t *c,unsigned raw){(void)c;(void)raw;return 100;}
static unsigned smk_vel7(unsigned v){return v;}
'''
constants='\n'.join(re.findall(r'^#define SMK_PAD_(?:ON|OFF|PEAKN).*$',h,re.M))
body=fn(h,'smk__padq')+'\n'+fn(h,'smk__pad')+'\n'+fn(p,'plat_pad_diag_session')
exercise=r'''
static void release(unsigned p){smk__pad(p,0);smk__pad(p,0);}
static void hit(unsigned p){for(int i=0;i<SMK_PAD_PEAKN;i++)smk__pad(p,100);}
int main(void){
 assert(plat_pad_diag_session(1)==1);assert(smk_diag_ack==0);
 hit(0);assert(smk_diag_ack==1 && smk_diag_seen==1);assert(!fm1_in.pnotes && !smk_pad_note[0]);release(0);
 assert(smk_diag_seen==1 && !smk_diag_suppressed);hit(0);release(0);assert(smk_diag_seen==1);
 smk__pad(1,100);plat_pad_diag_session(0);hit(1);assert(!fm1_in.pnotes);release(1);
 hit(1);assert(fm1_in.pnotes==2);release(1);assert(!fm1_in.pnotes);
 hit(2);assert(smk_pad_note[2]);assert(plat_pad_diag_session(1)==2);smk__pad(3,0);
 assert(smk_diag_seen==0 && smk_diag_ack==2);release(2);assert(!fm1_in.pnotes && !smk_pad_note[2]);
 fm1_in.pad_st[4]=0x80;smk__pad(4,100);assert(!(smk_diag_seen&(1u<<4)));
 plat_pad_diag_session(0);smk__pad(5,100);assert(plat_pad_diag_session(1)==3);smk__pad(6,0);plat_pad_diag_session(0);hit(5);assert(!smk_pad_note[5]);release(5);
 /* Last mapped velocity remains unchanged for non-musical division and piano gaps. */
 fm1_in.pad_vel[7]=42;smk_div_owner=1;hit(7);assert(fm1_in.pad_vel[7]==42);release(7);smk_div_owner=0;
 fm1_in.pad_vel[15]=43;smk_pad_piano=1;hit(15);assert(fm1_in.pad_vel[15]==43);release(15);smk_pad_piano=0;
 hit(7);assert(fm1_in.pad_vel[7]==100);release(7);
 smk_diag_epoch=UINT32_MAX;assert(!plat_pad_diag_session(1));assert(!smk_diag_enabled);assert(smk_diag_epoch==UINT32_MAX);
 puts("PASS actual scanner: short/equal hits, ack, held release, stuck ON, peak crossing exit, epoch saturation");
}
'''
with tempfile.TemporaryDirectory() as t:
 src=Path(t)/'test.c';exe=Path(t)/'test';src.write_text(prefix+state+'\n'+constants+'\n'+body+'\n'+exercise)
 subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra',*shlex.split(os.getenv('CFLAGS','')),str(src),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
