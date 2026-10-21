# SPDX-License-Identifier: GPL-3.0-only
# PAUSED FAILED SIZING EXPERIMENT: whole fm1_cpu1.h import does not link in loader.
# Preserve evidence only. Not an installable or accepted quiescence implementation.
from pathlib import Path
import sys
if len(sys.argv)!=2:raise SystemExit('usage: quiescence_attempt.py DISPOSABLE_PREPARED_EXPORT')
p=Path(sys.argv[1]).resolve()
if (p/'.git').exists():raise SystemExit('refuse worktree')
# Distinct loader capability marker; legacy standalone loaders are rejected by
# the isolated app-stage study. Real activation must be TEST-profile-only.
f=p/'firmware/loader/crt0_ldr.S';s=f.read_text().replace('\t.balign 2','\t.ascii "X0X-BUNDLE-LDR1"\n\t.byte 0\n\t.balign 2',1);f.write_text(s)
f=p/'firmware/src/ota.c';s=f.read_text();a='    if (!official && !own)\n        return -7;';assert a in s
s=s.replace(a,a+'\n    { uint32_t cap=0;\n      for(i=0x48;i+16<=0x48+64;i++) cap |= ota_memeq(b+i,(const uint8_t *)"X0X-BUNDLE-LDR1",16);\n      if(!own || !cap) return -7; /* isolated profile gate, no legacy standalone writes */\n    }');f.write_text(s)
f=p/'firmware/loader/loader.c';s=f.read_text().replace('#include "fm1_sys.h"','#include "fm1_sys.h"\n#include "fm1_cpu1.h"\n#include "fm1_audio.h"');s=s.replace('    uint32_t *p;\n    /* Study:', '    uint32_t *p;\n    fm1_cpu1_halt();\n    fm1_audio_stop();\n    /* Study:');f.write_text(s)
