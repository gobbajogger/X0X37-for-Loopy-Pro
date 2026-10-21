# SPDX-License-Identifier: GPL-3.0-only
# NON-INSTALLABLE: isolated complete-helper budget, not updater adoption.
from pathlib import Path
import sys,shutil
if len(sys.argv)!=2: raise SystemExit('usage: prepare.py DISPOSABLE_BASELINE_EXPORT')
p=Path(sys.argv[1]).resolve()
if (p/'.git').exists():raise SystemExit('refuse worktree')
# Exact existing semantics, excluding only the payload loop that assumes XIP.
s=(p/'firmware/hal/x0x_ble_segment.h').read_text()
a=s.index('static inline int xb4_validate('); b=s.index('    for (i = 0; i < XB4_SEGMENTS; ++i) {',s.index('return XB4_E_IRQ;',a))
v=s[a:b].replace('xb4_validate(', 'bp_manifest_structure(').replace(', xb4_read_fn read, void *ctx','').replace(' || !read','')+'    return XB4_OK;\n}\n'
(p/'firmware/loader/bp_manifest.h').write_text('/* SPDX-License-Identifier: MPL-2.0 */\n#include "x0x_ble_segment.h"\n'+v)
shutil.copyfile(Path(__file__).with_name('bundle_probe.h'),p/'firmware/loader/bundle_probe.h')
f=p/'firmware/loader/ldr_core.c';s=f.read_text();a=s.index('static int ldr_session(void)');s=s[:a]+'#include "bp_manifest.h"\n#include "bundle_probe.h"\n'+s[a:]
s=s.replace('static uint8_t hdr[0x400],', 'static uint8_t hdr[0x400] __attribute__((aligned(4))),')
s=s.replace('i, k, fl_off','fl_off').replace('key, s, done = 0','key')
a=s.index('    /* 3. app area, sector by sector */');s=s[:a]+'''    rc = bp_install(fl_off, fl_size, key, hdr, sec, cur);
    if (rc) return rc;
    /* Exact active staging record only; never scan data/sample sectors. Read
     * failure or unsuccessful finish must not clear either record. */
    if (ldr_fread(OTA_RES, sec, 112)) return -71;
    if (ota_rd16(sec + 6) != 0x5441u || ota_rd16(sec) != ota_crc16(sec + 2,78,0)) return -72;
    if (ldr_erase(OTA_RES & ~4095u)) return -73;
    ldr_record_clear();
    return 0;
}
''';f.write_text(s)
f=p/'firmware/loader/loader.c';s=f.read_text().replace('    ldr_record_clear();                             /* first thing: a crash below cannot loop through a\n                                                     * warm reset; resume after power loss uses the flash record */','    /* Study: retain RAM recovery record until successful complete install. */')
s=s.replace('#define FL_RANGE_OK(off, n) FL_LDR_RANGE_OK(off, n)', '''#define FL_RANGE_OK(off, n) (FL_IN(off,n,0x4000u,0x9D000u) || FL_IN(off,n,0xAB000u,0xAF000u) || FL_IN(off,n,0xFE000u,0xFF000u) || FL_IN(off,n,0xBF000u,0xC0000u))''');f.write_text(s)
f=p/'tools/build.py';s=f.read_text();a=s.index('    pkg = fm1pkg_make.ufw');s=s[:a]+'    print("NON-INSTALLABLE bundle-loader budget; no package")\n    return 0\n'+s[a:];f.write_text(s)

