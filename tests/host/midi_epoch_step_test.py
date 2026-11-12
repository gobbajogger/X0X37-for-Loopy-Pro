#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""25.1 plat_midi_cut epoch counter skips zero and increments by exactly one."""
import os, subprocess, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
text=(ROOT/"host/x0x_host.c").read_text()
start=text.index("static uint32_t mi_epoch = 1u;")
i=text.index("uint32_t plat_midi_cut(void)",start)
brace=text.index("{",i); depth=0
for j in range(brace,len(text)):
    if text[j]=="{": depth+=1
    elif text[j]=="}":
        depth-=1
        if depth==0:
            fn=text[i:j+1]
            break
src="#include <stdint.h>\n#include <stdio.h>\n#define MQ 128u\nstatic uint32_t mi_epoch=1u;\nstatic uint8_t mi_fresh[MQ];\n"+fn+r"""
int main(void) {
    uint32_t a,b,c;
    mi_epoch=UINT32_MAX;
    a=plat_midi_cut(); b=plat_midi_cut(); c=plat_midi_cut();
    if(a!=1u || b!=2u || c!=3u) {
        printf("FAIL MIDI epoch sequence: %u %u %u\n",a,b,c); return 1;
    }
    puts("midi epoch step: 0 failures"); return 0;
}
"""
with tempfile.TemporaryDirectory(prefix="x0x-midi-epoch-step-") as d:
    c=Path(d)/"t.c"; exe=Path(d)/"t"; c.write_text(src)
    subprocess.run(["cc","-std=c99","-O2","-Wall","-Wextra","-Werror",str(c),"-o",str(exe)],check=True)
    raise SystemExit(subprocess.call([str(exe)]))
