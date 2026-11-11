#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile the actual native pending query against volatile ring indices."""
import os, re, subprocess, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
src=(root/'firmware/src/app/plat_fm1.c').read_text()
matches=[re.search(pattern,src) for pattern in [r'int plat_midi_pending\(void\)\s*\{[^}]*\}',r'uint32_t plat_midi_mark\(void\)\s*\{[^}]*\}',r'int plat_midi_passed\(uint32_t mark\)\s*\{[^}]*\}']]
assert all(matches), 'actual native query/watermark missing'
probe='''#include <stdint.h>
#include <assert.h>
#include <stdio.h>
static volatile uint32_t mi_r,mi_w;
'''+ '\n'.join(m.group() for m in matches)+'''
int main(void){
 const uint32_t cases[][2]={{0,0},{0,1},{63,64},{64,64},{UINT32_MAX,0},{UINT32_MAX,UINT32_MAX}};
 for(unsigned k=0;k<sizeof cases/sizeof cases[0];k++){
  mi_r=cases[k][0];mi_w=cases[k][1];
  assert(plat_midi_pending()==(cases[k][0]!=cases[k][1]));
  assert(plat_midi_mark()==cases[k][1]);
  assert(plat_midi_passed(cases[k][0]));
  assert(mi_r==cases[k][0] && mi_w==cases[k][1]);
 }
 mi_r=0;mi_w=1;assert(!plat_midi_passed(1));assert(plat_midi_passed(UINT32_MAX));
 mi_r=UINT32_MAX;mi_w=0;assert(!plat_midi_passed(0));
 mi_r=1;assert(plat_midi_passed(0));
 puts("PASS actual native non-consuming pending query: empty, pending, ring/writer counter wrap; no native firmware build claim");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'probe.c').write_text(probe)
 subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror',str(p/'probe.c'),'-o',str(p/'probe')],check=True)
 subprocess.run([str(p/'probe')],check=True)
