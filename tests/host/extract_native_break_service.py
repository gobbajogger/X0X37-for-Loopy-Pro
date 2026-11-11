#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile the exact guarded native main-loop completion statement into host proof.
A missing native caller fails extraction (the AS-001 negative), not silently fixed by a test flush."""
from pathlib import Path
import re
r=Path(__file__).resolve().parents[2]
s=(r/'firmware/src/app/main_fm1.c').read_text()
lines=re.findall(r'^\s*if \(!ui_project_busy\(\)\) engine_brk_flush\(\);[^\n]*',s,re.M)
assert len(lines)==1,'AS-001: native guarded completion caller missing or duplicated'
# Pin its actual main-loop neighborhood, rather than accepting a dead helper.
pos=s.index(lines[0]);assert 'seq.ppat[TRK_BRK] != last_pat' in s[pos:pos+400]
(r/'build/gen/adaptive_native_brk_service.h').write_text('static void native_break_service(void) {\n'+lines[0]+'\n}\n')
