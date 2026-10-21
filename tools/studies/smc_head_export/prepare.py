# SPDX-License-Identifier: GPL-3.0-only
"""Prepare disposable head-export loader source, NEVER create a package.
Usage: prepare.py DISPOSABLE_BASELINE_EXPORT
Pre-SPL transition admission remains a separate required review/hardware gate.
"""
from pathlib import Path
import shutil
import sys
p=Path(sys.argv[1]).resolve();here=Path(__file__).resolve().parent
if (p/'.git').exists():raise SystemExit('refuse worktree')
f=p/'tools/build.py';s=f.read_text();a=s.index('    pkg = fm1pkg_make.ufw')
s=s[:a]+'    print("HEAD EXPORT SOURCE STUDY: NO PACKAGE")\n    return 0\n'+s[a:];f.write_text(s)
for name in ('head_export.h','loader_core.h'):shutil.copyfile(here/name,p/'firmware/loader'/name)
s=(here.parent/'smc_bundle_loader/record_owner.h').read_text().replace('X0X-BUNDLE-LDR1','X0X-HEAD-EXPORT')
(p/'firmware/loader/head_record_owner.h').write_text(s)
q=(here.parent/'smc_bundle_loader/loader_quiesce.h').read_text()
q=q.replace('    *(volatile uint32_t *)0x1EEE004u |= 2u;', '    *(volatile uint16_t *)0x12E00u &= ~0x800u; /* first C action: stop TX DMA */\n    *(volatile uint32_t *)0x1EEE004u |= 2u;').replace('    *(volatile uint16_t *)0x12E00u &= ~0x800u;\n}', '}')
(p/'firmware/hal/head_loader_quiesce.h').write_text(q)
f=p/'firmware/loader/crt0_ldr.S';s=f.read_text().replace('\t.balign 2','\t.ascii "X0X-HEAD-EXPORT"\n\t.byte 0\n\t.balign 2',1);f.write_text(s)
f=p/'firmware/loader/loader.c';s=f.read_text()
s=s.replace('#define FL_RANGE_OK(off, n) FL_LDR_RANGE_OK(off, n)','#define FL_RANGE_OK(off, n) FL_IN(off,n,0xBF000u,0xC0000u)')
s=s.replace('#include "fm1_sys.h"','#include "fm1_sys.h"\n#include "head_loader_quiesce.h"')
a=s.index('static int ldr_prog(');b=s.index('static void ldr_record_clear',a);s=s[:a]+s[b:]
s=s.replace('#include "ldr_core.c"','#include "loader_core.h"')
a=s.index('        ota_service();');b=s.index('\n    }\n}',a)
s=s[:a]+'''        hx_service();
        if (usb.ota_req) { usb.ota_req=0; (void)hx_admit(); }
        if (hx_reset_at && (int32_t)(ldr_ms()-hx_reset_at)>=0) {
            usb_detach();fm1_delay_ms(30);fm1_core_reset();
        }
'''+s[b:]
s=s.replace('    ldr_record_clear();                             /* first thing: a crash below cannot loop through a\n                                                     * warm reset; resume after power loss uses the flash record */','    bp_loader_quiesce(); /* earliest C entry; does not claim pre-SPL DMA shutdown */')
f.write_text(s)
print('Prepared read-only exporter source; only owned record-sector erase, no package')
