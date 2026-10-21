#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Fully resolve current conditional host sources in oversized DIAGNOSTIC regions.

Unlike run.py this does NOT ignore imports or add API roots. It is still NOT
installable: no segmented placement/admission, no image/package generation.
"""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
from run import HERE, digest, invoke

p = argparse.ArgumentParser(description=__doc__)
for name in ('repo', 'toolchain', 'libs', 'rf-tables', 'system-lib', 'out'):
    p.add_argument('--' + name, required=True, type=Path)
p.add_argument('--controller-roles', choices=('all', 'central'), default='all')
p.add_argument('--avctp-extra-commands', choices=(0, 1), type=int, default=1)
a = p.parse_args()
repo, tc, libs, rf, system, out = [x.resolve() for x in
    (a.repo, a.toolchain, a.libs, a.rf_tables, a.system_lib, a.out)]
if out.exists() or out == repo or repo in out.parents:
    p.error('out must be new and outside repo')
pins = json.loads((HERE / 'input-hashes.json').read_text())
inputs = {name: libs / name for name in pins['libraries']}
inputs.update({name: tc / 'pi32v2/bin' / name for name in ('clang', 'ld', 'LLVMgold.so', 'objdump')})
inputs['rf_tables.c'] = rf
expected = {**pins['libraries'], **pins['tools_and_rf']}
expected['system.a'] = 'a71bdc75e5afbcffaf7d79bc2b27f2b65fec63d94d2261b584744d3e9b9098ea'
inputs['system.a'] = system
for name, path in inputs.items():
    if digest(path) != expected[name]:
        p.error('Pinned hash mismatch: ' + name)
out.mkdir(parents=True)
(out / 'NON-INSTALLABLE.txt').write_text('Resolved-link diagnostic ONLY. Oversized regions; no segmented admission or app. NEVER INSTALL.\n')
shutil.copytree(repo / 'ble/ctl/src', out / 'src')
for name in ('x0x_ble_abi.h', 'x0x_smc_host_messages.h'):
    shutil.copy(repo / 'firmware/hal' / name, out / 'src' / name)
(out / 'gen').mkdir(); shutil.copy(rf, out / 'gen/rf_tables.c')
(out / 'lib').mkdir()
for name in (*pins['libraries'], 'system.a'):
    shutil.copy(inputs[name], out / 'lib' / name)
rows = json.loads((HERE / 'config-values.json').read_text())
lines = ['#include <stdint.h>']
for row in rows:
    section = (' __attribute__((section("' + row['section'] + '")))' if 'section' in row else '')
    value = 25 if a.controller_roles == 'central' and row['name'] == 'config_btctler_le_roles' else row['value']
    if row['name'] == 'more_avctp_cmd_support':
        value = a.avctp_extra_commands
    lines.append(f"{row['type']} {row['name']}{section} = {value};")
(out / 'src/config.c').write_text('\n'.join(lines) + '\n')
names = {row['name'] for row in rows}
log = out / 'src/log_tags.c'
log.write_text('\n'.join(line for line in log.read_text().splitlines()
                        if not (set(re.findall(r'\b[A-Za-z_]\w*\b', line)) & names)) + '\n')
ld = (repo / 'ble/ctl/ctl.ld').read_text().replace('LENGTH = 0x10000', 'LENGTH = 0x100000').replace('LENGTH = 0x6700', 'LENGTH = 0x100000')
ld = '\n'.join(line for line in ld.splitlines() if 'ASSERT(__ctl_image_end' not in line)
# LE-only config has no classic codec registrations. These are an empty linker
# collection, not empty runtime callbacks. If roles/config change, re-review.
ld += '\na2dp_sink_media_codec_begin = 0;\na2dp_sink_media_codec_end = 0;\n'
(out / 'ctl.ld').write_text(ld)
tool = tc / 'pi32v2/bin'; (out / 'obj').mkdir()
# The full system archive eagerly selects RTOS/IRQ duplicates before LTO drops
# classic paths. Retain only the exact reused public cbuf implementation.
with (out / 'lib/circular_buf.c.o').open('wb') as stream:
    subprocess.run([str(tool / 'ar'), 'p', str(system), 'circular_buf.c.o'],
                   check=True, stdout=stream)
flags = ('-target pi32v2 -integrated-as -mcpu=r3 -mfprev1 -Oz -flto -fno-common '
         '-fallow-pointer-null -fprefer-gnu-section -mllvm -pi32v2-large-program=true '
         '-ffunction-sections -fdata-sections -fms-extensions -ffreestanding -Wall '
         '-Wextra -Wno-unused-parameter -Werror -DCTL_VENDOR_HOST_TEST=1 -Isrc').split()
objects = []
for source in sorted((out / 'src').glob('*.c')) + sorted((out / 'src').glob('*.S')) + list((out / 'gen').glob('*.c')):
    target = 'obj/' + source.name + '.o'
    extra = ['-Wno-error', '-w'] if source.parent.name == 'gen' else []
    if invoke([str(tool / 'clang'), *flags, *extra, '-c', str(source.relative_to(out)), '-o', target],
              out, out / (target + '.log')):
        raise SystemExit('Compile failed: ' + target + '.log')
    objects.append(target)
command = [str(tool / 'ld'), '--plugin', str(tool / 'LLVMgold.so'),
           '--plugin-opt=mcpu=r3', '--plugin-opt=-mattr=+fprev1',
           '--plugin-opt=-inline-threshold=0', '--plugin-opt=-global-merge-on-const',
           '--plugin-opt=-enable-ipra=true', '--plugin-opt=-pi32v2-large-program=true',
           '--plugin-opt=-dont-used-symbol-list=printf,puts,putchar,put_buf,printf_buf,log_print',
           '--unresolved-symbols=report-all', '--gc-sections', '-T', 'ctl.ld',
           '-Map', 'resolved.map', '-o', 'resolved-NOT-FIRMWARE.elf', *objects,
           '--start-group', *['lib/' + name for name in pins['libraries']], 'lib/circular_buf.c.o', '--end-group']
rc = invoke(command, out, out / 'link.log')
unresolved = sorted(set(re.findall(r"undefined reference to [`']([^']+)'", (out / 'link.log').read_text())))
result = {'controller_roles': a.controller_roles, 'avctp_extra_commands': a.avctp_extra_commands, 'installable': False, 'resolved': rc == 0, 'link_exit': rc, 'unresolved': unresolved,
          'input_hashes': expected,
          'source_hashes': {str(f.relative_to(out)): digest(f) for f in sorted((out / 'src').glob('*')) if f.is_file()}}
if not rc:
    for flag, name in [('-t', 'symbols.txt'), ('-h', 'sections.txt')]:
        with (out / name).open('w') as stream:
            subprocess.run([str(tool / 'objdump'), flag, str(out / 'resolved-NOT-FIRMWARE.elf')], check=True, stdout=stream)
    symbols = (out / 'symbols.txt').read_text()
    def addr(name):
        return int(next(line.split()[0] for line in symbols.splitlines() if line.split() and line.split()[-1] == name), 16)
    result.update(flash_extent_bytes=addr('__ctl_image_end') - 0x02087000,
                  ram_extent_bytes=addr('__ctl_ram_hi') - 0x01C01800)
(out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps({k: v for k, v in result.items() if not k.endswith('_hashes')}, indent=2))
raise SystemExit(rc)
