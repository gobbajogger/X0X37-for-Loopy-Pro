# SPDX-License-Identifier: GPL-3.0-only
# NON-INSTALLABLE: isolated complete-helper budget, not updater adoption.
from pathlib import Path
import sys,shutil,subprocess
if len(sys.argv)!=4: raise SystemExit('usage: prepare.py DISPOSABLE_BASELINE_EXPORT FINAL_APP_ELF SEALED_APP_BIN')
p=Path(sys.argv[1]).resolve()
if (p/'.git').exists():raise SystemExit('refuse worktree')
# Disable package output BEFORE pin validation; even caller shell mistakes fail safe.
f=p/'tools/build.py';s=f.read_text();a=s.index('    pkg = fm1pkg_make.ufw');s=s[:a]+'    print("NON-INSTALLABLE bundle-loader budget; no package")\n    return 0\n'+s[a:];f.write_text(s)
subprocess.run([sys.executable,str(Path(__file__).with_name('pin_app.py')),sys.argv[2],sys.argv[3],str(p/'firmware/loader/bp_pin.h')],check=True)
# Exact existing semantics, excluding only the payload loop that assumes XIP.
s=(p/'firmware/hal/x0x_ble_segment.h').read_text()
a=s.index('static inline int xb4_validate('); b=s.index('    for (i = 0; i < XB4_SEGMENTS; ++i) {',s.index('return XB4_E_IRQ;',a))
v=s[a:b].replace('xb4_validate(', 'bp_manifest_structure(').replace(', xb4_read_fn read, void *ctx','').replace(' || !read','')+'    return XB4_OK;\n}\n'
(p/'firmware/loader/bp_manifest.h').write_text('/* SPDX-License-Identifier: MPL-2.0 */\n#include "x0x_ble_segment.h"\n'+v)
for name in ('bundle_probe.h','record_owner.h','rollback_pins.h'):
    shutil.copyfile(Path(__file__).with_name(name),p/'firmware/loader'/name)
f=p/'firmware/loader/ldr_core.c';s=f.read_text();a=s.index('static int ldr_session(void)');s=s[:a]+'#include "bp_pin.h"\n#include "bp_manifest.h"\n#include "bundle_probe.h"\n#include "record_owner.h"\n'+s[a:]
s=s.replace('static uint8_t hdr[0x400],', 'static uint8_t hdr[0x400] __attribute__((aligned(4))),')
s=s.replace('i, k, fl_off','fl_off').replace('key, s, done = 0','key')
a=s.index('    /* 3. app area, sector by sector */');s=s[:a]+'''    rc = bp_record_admit(ota_off, ota_len, hdr+384, sec, cur);
    if (rc) return rc;
    rc = bp_install(fl_off, fl_size, key, hdr, sec, cur);
    if (rc) return rc;
    return bp_record_finish(hdr+384, sec);
}
''';f.write_text(s)
f=p/'firmware/loader/loader.c';s=f.read_text().replace('    ldr_record_clear();                             /* first thing: a crash below cannot loop through a\n                                                     * warm reset; resume after power loss uses the flash record */','    /* Study: retain RAM recovery record until successful complete install. */')
s=s.replace('#define FL_RANGE_OK(off, n) FL_LDR_RANGE_OK(off, n)', '''#define FL_RANGE_OK(off, n) (FL_IN(off,n,0x4000u,0x9D000u) || FL_IN(off,n,0xAB000u,0xAF000u) || FL_IN(off,n,0xFE000u,0xFF000u) || FL_IN(off,n,0xBF000u,0xC0000u))''');f.write_text(s)



f=p/'firmware/loader/crt0_ldr.S';s=f.read_text().replace('\t.balign 2','\t.ascii "X0X-BUNDLE-LDR1"\n\t.byte 0\n\t.balign 2',1);f.write_text(s)
shutil.copyfile(Path(__file__).with_name('loader_quiesce.h'),p/'firmware/hal/bp_loader_quiesce.h')
f=p/'firmware/loader/loader.c';s=f.read_text().replace('#include "fm1_sys.h"','#include "fm1_sys.h"\n#include "bp_loader_quiesce.h"').replace('    uint32_t *p;','    uint32_t *p;\n    bp_loader_quiesce();');f.write_text(s)
# TEST-only stage gate covers normal and safe callers of ota_session. Default
# exports retain existing behavior. Owner must define this macro from the
# never-started recovery latch; absent wiring refuses every TEST update.
f=p/'firmware/src/ota.c';s=f.read_text();a='    ota_show(1, 0);';assert a in s
s=s.replace(a,'''#if X0X_SMC_VENDOR_TEST && !FELUCCA_LOADER
#ifndef X0X_BUNDLE_UPDATE_ALLOWED
#define X0X_BUNDLE_UPDATE_ALLOWED() 0
#endif
    if (!X0X_BUNDLE_UPDATE_ALLOWED()) return -74;
#endif
'''+a,1)
a='    if (!official && !own)\n        return -7;';assert a in s
s=s.replace(a,a+'''
#if X0X_SMC_VENDOR_TEST && !FELUCCA_LOADER
    { uint32_t cap=0;
      for(i=0x48;i+16<=0x48+64;i++)
        cap |= ota_memeq(b+i,(const uint8_t *)"X0X-BUNDLE-LDR1",16);
      if(!own || !cap) return -7;
    }
#endif
''');f.write_text(s)
# Reject before ota_stage, because ota_session historically unstages on errors.
# Generic cleanup must never destroy an owned/uncertain persistent record in
# this TEST profile. The admitted loader owns successful record retirement.
f=p/'firmware/src/ota.c';s=f.read_text();needle='    rc = ota_stage();';assert needle in s
s=s.replace(needle,'''#if X0X_SMC_VENDOR_TEST && !FELUCCA_LOADER
    if (!X0X_BUNDLE_UPDATE_ALLOWED()) return -74;
#endif
'''+needle)
needle='static void ota_unstage(void)\n{';assert needle in s
s=s.replace(needle,needle+'''
#if X0X_SMC_VENDOR_TEST && !FELUCCA_LOADER
    return; /* only bundle loader may retire the exact admitted record */
#endif
''')
# Refuse staging atop ANY non-erased record, even invalid/foreign: don't erase
# uncertain ownership. Existing persistent loader must resume/recover instead.
needle='    /* 4. stage the package\'s loader: erase, body, verify, head last */';assert needle in s
s=s.replace(needle,'''#if X0X_SMC_VENDOR_TEST && !FELUCCA_LOADER
    if (ota_fread(OTA_RES, mine, 112)) return -75;
    for (i=0;i<112;i++) if (mine[i]!=255) return -75;
#endif
'''+needle);f.write_text(s)
