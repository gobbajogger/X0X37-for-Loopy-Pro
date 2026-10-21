#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Fully resolve current conditional host sources in a selected DIAGNOSTIC layout.

Unlike run.py this does NOT ignore imports or add API roots. It is still NOT
installable: no complete app/update admission and no firmware package generation.
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
p.add_argument('--placement', choices=('oversized', 'data-only-92k', 'data-only-92k-ramload'), default='oversized')
p.add_argument('--high-cpu-bss', action='store_true')
p.add_argument('--cpu-init-pool', action='store_true')
p.add_argument('--tail-merge-probe', action='store_true',
               help='Compiler experiment only: enable disabled-by-default backend tail merging')
p.add_argument('--no-classic-lmp', action='store_true',
               help='Disable Classic LMP name/passkey/eSCO compile-time paths only')
p.add_argument('--no-classic-key-sniff', action='store_true',
               help='Disable Classic-only link-key deletion and auto-sniff paths; LE SM unchanged')
p.add_argument('--no-classic-test-modes', action='store_true',
               help='Disable exact vendor DUT/FCC Classic certification branches only')
p.add_argument('--inlinehint-threshold', type=int, choices=(0,), default=None,
               help='Bounded link-only size discriminator; vendor bitcode unchanged')
a = p.parse_args()
if a.cpu_init_pool and not a.high_cpu_bss: p.error('cpu-init-pool requires high-cpu-bss')
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
(out / 'NON-INSTALLABLE.txt').write_text('Resolved-link diagnostic ONLY. Selected study layout; no complete app/update admission. NEVER INSTALL.\n')
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
    if a.no_classic_test_modes and row['name'] in ('CONFIG_TEST_DUT_CODE', 'CONFIG_TEST_FCC_CODE'):
        value = 0
    if a.no_classic_key_sniff and row['name'] in ('config_delete_link_key', 'config_btstask_auto_exit_sniff'):
        value = 0
    if a.no_classic_lmp and row['name'] in ('CONFIG_LMP_NAME_REQ_ENABLE', 'CONFIG_LMP_PASSKEY_ENABLE', 'CONFIG_LMP_MASTER_ESCO_ENABLE'):
        value = 0
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
if a.placement != 'oversized':
    # Placement-only proof. RAM remains oversized; no executable package exists.
    ld = ld.replace('ORIGIN = 0x02087000, LENGTH = 0x100000',
                    'ORIGIN = 0x02080000, LENGTH = 0x17000')
    ld = ld.replace('    CRAM   (rwx)',
                    '    CDATA (r) : ORIGIN = 0x020A9000, LENGTH = 0x2000\n'
                    '    CLOAD (r) : ORIGIN = 0x020FA000, LENGTH = 0x1000\n'
                    '    CRAM   (rwx)')
    ld = ld.replace('KEEP(*(.ctl_header))', 'KEEP(*(.ctl_header)) . = 128;')
    ld = ld.replace('    } > CFLASH\n\n    /* Code',
                    '    } > CDATA\n\n    /* Code')
    ld = ld.replace('    } > CRAM AT > CFLASH\n    __ctl_data_lma',
                    '    } > CRAM AT > CLOAD\n    __ctl_data_lma')
if a.placement == 'data-only-92k-ramload':
    ld = ld.replace('    } > CRAM AT > CFLASH\n    __ctl_ramtext_lma',
                    '    } > CRAM AT > CLOAD\n    __ctl_ramtext_lma')
    ld = ld.replace('    } > CRAM AT > CLOAD\n    __ctl_data_lma',
                    '    } > CRAM AT > CFLASH\n    __ctl_data_lma')
if a.high_cpu_bss:
    ld = ld.replace('    CRAM   (rwx)', '    HIGHCPU (rw) : ORIGIN = 0x01C7D400, LENGTH = 0x1C00\n    CRAM   (rwx)')
    ld = ld.replace('LENGTH = 0x100000\n}', 'LENGTH = 0x6700\n}')
    ld = ld.replace('    .bss (NOLOAD)',
                    '    .highcpu (NOLOAD) : { __ctl_highcpu_start = .; *(.ctl_highcpu_bss) . = ALIGN(4); __ctl_highcpu_end = .; } > HIGHCPU\n    .bss (NOLOAD)')
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
if a.high_cpu_bss: flags.append('-DCTL_VENDOR_HIGHCPU_TEST=1')
if a.cpu_init_pool: flags.append('-DCTL_VENDOR_CPUPOOL_TEST=1')
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
if a.tail_merge_probe:
    command.insert(3, '--plugin-opt=-enable-tail-merge=true')
if a.inlinehint_threshold is not None:
    command.insert(3, '--plugin-opt=-inlinehint-threshold=' + str(a.inlinehint_threshold))
(out / 'link-command.json').write_text(json.dumps(command, indent=2) + '\n')
rc = invoke(command, out, out / 'link.log')
unresolved = sorted(set(re.findall(r"undefined reference to [`']([^']+)'", (out / 'link.log').read_text())))
result = {'tail_merge_probe': a.tail_merge_probe, 'no_classic_lmp': a.no_classic_lmp, 'no_classic_key_sniff': a.no_classic_key_sniff, 'no_classic_test_modes': a.no_classic_test_modes, 'inlinehint_threshold': a.inlinehint_threshold, 'cpu_init_pool': a.cpu_init_pool, 'high_cpu_bss': a.high_cpu_bss, 'placement': a.placement, 'controller_roles': a.controller_roles, 'avctp_extra_commands': a.avctp_extra_commands, 'installable': False, 'resolved': rc == 0, 'link_exit': rc, 'unresolved': unresolved,
          'input_hashes': expected,
          'study_hashes': {name: digest(HERE / name) for name in
                           ('link_runtime.py', 'run.py', 'config-values.json', 'input-hashes.json')},
          'source_hashes': {str(f.relative_to(out)): digest(f) for f in sorted((out / 'src').glob('*')) if f.is_file()}}
if not rc:
    for flag, name in [('-t', 'symbols.txt'), ('-h', 'sections.txt')]:
        with (out / name).open('w') as stream:
            subprocess.run([str(tool / 'objdump'), flag, str(out / 'resolved-NOT-FIRMWARE.elf')], check=True, stdout=stream)
    symbols = (out / 'symbols.txt').read_text()
    def addr(name):
        return int(next(line.split()[0] for line in symbols.splitlines() if line.split() and line.split()[-1] == name), 16)
    result.update(flash_extent_bytes=(addr('__ctl_image_end') - 0x02087000) if a.placement == 'oversized' else None,
                  ram_extent_bytes=addr('__ctl_ram_hi') - 0x01C01800)
    if a.placement != 'oversized':
        sections = {fields[1]: int(fields[2], 16) for line in (out / 'sections.txt').read_text().splitlines()
                    if len(fields := line.split()) >= 5 and fields[1].startswith('.')}
        primary = (addr('__ctl_data_lma') + sections['.data'] if a.placement == 'data-only-92k-ramload' else addr('__ctl_ramtext_lma') + sections['.ramtext']) - 0x02080000
        tertiary = sections['.ramtext'] if a.placement == 'data-only-92k-ramload' else sections['.data']
        result['placed_flash'] = {'primary_bytes': primary, 'primary_capacity': 94208,
                                  'rodata_bytes': sections['.rodata'], 'rodata_capacity': 8192,
                                  'data_load_bytes': tertiary, 'data_load_capacity': 4096,
                                  'total_loaded_bytes': primary + sections['.rodata'] + tertiary,
                                  'ram_still_oversized': not a.high_cpu_bss, 'manifest_placeholder_only': True}

if not rc and a.high_cpu_bss:
    result['high_cpu_bytes'] = addr('__ctl_highcpu_end') - addr('__ctl_highcpu_start')
if not rc:
    result['private_artifact_hashes'] = {name: digest(out / name) for name in
                                       ('resolved-NOT-FIRMWARE.elf', 'resolved.map', 'symbols.txt', 'sections.txt', 'ctl.ld')}
(out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps({k: v for k, v in result.items() if not k.endswith('_hashes')}, indent=2))
raise SystemExit(rc)
