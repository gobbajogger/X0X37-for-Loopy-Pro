#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only audit of exact historical source, not a synthetic DIAG layout."""
import pathlib, subprocess, sys, tempfile
p=pathlib.Path(sys.argv[1])
expected={'firmware/hal/fm1_irq.h':'fm1_crash_t','firmware/src/ble/ble_glue.h':'ble_guard_t'}
structs=[]
for name,typ in expected.items():
    s=(p/name).read_text();end=s.index('} '+typ+';')+len('} '+typ+';');start=s.rfind('typedef struct {',0,end)
    if start<0:raise ValueError('missing exact struct '+typ)
    structs.append(s[start:end])
editor=(p/'firmware/src/app/editor.c').read_text()
ble=(p/'firmware/src/ble.c').read_text()
for token in ('uint32_t w[71]', 'w[nw++] = bootguard.failed;', 'w[nw++] = bootguard.pending;', 'ed_b(w[k] >> 28);'):
    if token not in editor:raise ValueError('historical editor layout changed')
for token in ('w[n++] = ble_img | (uint32_t)ble_boot_on << 8 | (uint32_t)ble_up << 16 | (uint32_t)ble_dead << 24;', 'w[n++] = ble_want | (uint32_t)ble_host_on << 8', 'w[n++] = fm1_c1_gave_up;', 'w[n++] = (uint32_t)plat_ble_state();'):
    if token not in ble:raise ValueError('historical live layout changed')
plat=(p/'firmware/src/app/plat.h').read_text()
if 'enum { PLAT_BLE_NOIMG, PLAT_BLE_OFF,' not in plat:raise ValueError('OFF enum changed')
with tempfile.TemporaryDirectory() as td:
    c=pathlib.Path(td)/'layout.c';exe=c.with_suffix('')
    c.write_text('#include <stdint.h>\n'+ '\n'.join(structs)+'\n_Static_assert(sizeof(fm1_crash_t)==17*4,"crash");\n_Static_assert(sizeof(ble_guard_t)==29*4,"guard");\n_Static_assert(sizeof(fm1_crash_t)/4+2+sizeof(ble_guard_t)/4==48,"live offset");\nint main(void){return 0;}\n')
    subprocess.run(['cc','-std=c11','-Wall','-Werror',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Historical source layout PASS: crash17 + bootguard2 + guard29 = flags48; want49, state51; OFF1')
