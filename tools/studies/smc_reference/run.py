#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""NON-INSTALLABLE exact-library root/size probe. Never emits an image/package."""
import argparse
import hashlib
import json
import pathlib
import re
import shutil
import subprocess
import sys

HERE = pathlib.Path(__file__).resolve().parent
BASE = '816e0d3987bcb69be972cff2cd4df4469969f196'
VARIANTS = {
    'donor': ('build_in_container.sh', 2, 1, False),
    'one-link': ('build_in_container.sh', 1, 0, False),
    'central': ('core_measure.sh', 1, 0, False),
    'both': ('core_both_measure.sh', 1, 1, False),
    'central-private': ('core_measure.sh', 1, 0, True),
    'both-private': ('core_both_measure.sh', 1, 1, True),
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def invoke(command, cwd, log):
    with log.open('w') as stream:
        return subprocess.run(command, cwd=cwd, stdout=stream,
                              stderr=subprocess.STDOUT, check=False).returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=pathlib.Path, required=True)
    parser.add_argument('--toolchain', type=pathlib.Path, required=True)
    parser.add_argument('--libs', type=pathlib.Path, required=True,
                        help='Privately prepared exact archives; see README')
    parser.add_argument('--rf-tables', type=pathlib.Path, required=True)
    parser.add_argument('--out', type=pathlib.Path, required=True,
                        help='New scratch directory outside the repository')
    parser.add_argument('--variant', choices=VARIANTS, required=True)
    args = parser.parse_args()
    repo, tc, libs, rf, out = [p.resolve() for p in
                             (args.repo, args.toolchain, args.libs, args.rf_tables, args.out)]
    if out.exists() or out == repo or repo in out.parents:
        parser.error('--out must not exist and must be outside the repository')
    pins = json.loads((HERE / 'input-hashes.json').read_text())
    inputs = {name: libs / name for name in pins['libraries']}
    inputs.update({'clang': tc / 'pi32v2/bin/clang',
                   'ld': tc / 'pi32v2/bin/ld',
                   'LLVMgold.so': tc / 'pi32v2/bin/LLVMgold.so',
                   'objdump': tc / 'pi32v2/bin/objdump', 'rf_tables.c': rf})
    expected = {**pins['libraries'], **pins['tools_and_rf']}
    for name, path in inputs.items():
        if digest(path) != expected[name]:
            parser.error(f'Pinned hash mismatch: {name}')
    out.mkdir(parents=True)
    (out / 'NON-INSTALLABLE.txt').write_text(
        'SIZE PROBE ONLY. Missing runtime imports/startup; oversized regions. '
        'Never run, package, install, or interpret relaxed-link success as firmware.\n')
    # Export only public baseline sources, never alter the supplied checkout.
    for prefix in ('ble/ctl/src', 'ble/ctl/ctl.ld', 'firmware/hal/x0x_ble_abi.h'):
        names = subprocess.check_output(['git', '-C', str(repo), 'ls-tree', '-r',
                                         '--name-only', BASE, '--', prefix], text=True)
        for name in names.splitlines():
            target = out / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(subprocess.check_output(
                ['git', '-C', str(repo), 'show', f'{BASE}:{name}']))
    root_key, hw, server, private = VARIANTS[args.variant]
    work = out / 'ble/ctl'
    shutil.copy(out / 'firmware/hal/x0x_ble_abi.h', work / 'src/x0x_ble_abi.h')
    (work / 'gen').mkdir()
    shutil.copy(rf, work / 'gen/rf_tables.c')
    (work / 'lib').mkdir()
    for name in pins['libraries']:
        shutil.copy(libs / name, work / 'lib' / name)
    rows = json.loads((HERE / 'config-values.json').read_text())
    overrides = {'config_btctler_le_hw_nums': hw,
                 'config_btctler_le_rx_nums': hw * 5,
                 'config_btctler_le_acl_total_nums': hw * 5,
                 'config_le_gatt_server_num': server}
    lines = ['#include <stdint.h>']
    for row in rows:
        section = (' __attribute__((section("' + row['section'] + '")))'
                   if 'section' in row else '')
        lines.append(f"{row['type']} {row['name']}{section} = "
                     f"{overrides.get(row['name'], row['value'])};")
    (work / 'src/config.c').write_text('\n'.join(lines) + '\n')
    # Avoid duplicate definitions already in the donor numeric configuration.
    names = {row['name'] for row in rows}
    log_path = work / 'src/log_tags.c'
    log_path.write_text('\n'.join(line for line in log_path.read_text().splitlines()
                                if not (set(re.findall(r'\b[A-Za-z_]\w*\b', line)) & names)) + '\n')
    main_path = work / 'src/ctl_main.c'
    text = main_path.read_text().replace('hci_packet_handler',
                                        'measurement_raw_hci_packet_handler')
    if private:
        text = text.replace('static const hci_transport_t ctl_transport =',
                            'static const hci_transport_t ctl_transport __attribute__((unused)) =')
        text = text.replace('extern int btctrler_task_init',
                            'extern const void *hci_transport_h4_controller_instance(void);\n'
                            'extern int btctrler_task_init')
        text = text.replace('btctrler_task_init(&ctl_transport, NULL)',
                            'btctrler_task_init(hci_transport_h4_controller_instance(), NULL)')
    main_path.write_text(text)
    ld_path = work / 'ctl.ld'
    text = ld_path.read_text().replace('LENGTH = 0x10000', 'LENGTH = 0x100000')
    text = text.replace('LENGTH = 0x6700', 'LENGTH = 0x100000')
    text = '\n'.join(line for line in text.splitlines() if 'ASSERT(__ctl_image_end' not in line)
    ld_path.write_text(text + '\n')
    tool = tc / 'pi32v2/bin'
    obj = work / 'obj'
    obj.mkdir()
    cflags = ('-target pi32v2 -integrated-as -mcpu=r3 -mfprev1 -Oz -flto '
              '-fno-common -fallow-pointer-null -fprefer-gnu-section '
              '-mllvm -pi32v2-large-program=true -ffunction-sections -fdata-sections '
              '-fms-extensions -ffreestanding -Wall -Wextra -Wno-unused-parameter -Werror').split()
    objects = []
    for source in sorted((work / 'src').glob('*.c')) + sorted((work / 'src').glob('*.S')) + list((work / 'gen').glob('*.c')):
        target = obj / (source.name + '.o')
        flags = ['-Wno-error', '-w'] if source.parent.name == 'gen' else []
        command = [str(tool / 'clang'), *cflags, *flags, '-Isrc', '-c',
                   str(source.relative_to(work)), '-o', str(target.relative_to(work))]
        if invoke(command, work, target.with_suffix('.log')):
            raise RuntimeError(f'Compilation failed; see {target.with_suffix(".log")}')
        objects.append(str(target.relative_to(work)))
    roots = json.loads((HERE / 'roots.json').read_text())[root_key]
    common = [str(tool / 'ld'), '--plugin', str(tool / 'LLVMgold.so'),
              '--plugin-opt=mcpu=r3', '--plugin-opt=-mattr=+fprev1',
              '--plugin-opt=-inline-threshold=0', '--plugin-opt=-global-merge-on-const',
              '--plugin-opt=-enable-ipra=true', '--plugin-opt=-pi32v2-large-program=true',
              *['--undefined=' + root for root in roots],
              '--plugin-opt=-dont-used-symbol-list=printf,puts,putchar,put_buf,printf_buf,log_print',
              '--gc-sections', '-T', 'ctl.ld', *objects, '--start-group',
              *['lib/' + name for name in pins['libraries']], '--end-group']
    # Report-all is authoritative for unresolved imports. Its failure is preserved.
    strict = common + ['--unresolved-symbols=report-all', '-o', 'obj/strict-NOT-FIRMWARE.elf']
    strict_rc = invoke(strict, work, out / 'report-all.log')
    unresolved = sorted(set(re.findall(r"undefined reference to [`']([^']+)'",
                                       (out / 'report-all.log').read_text())))
    (out / 'unresolved.txt').write_text('\n'.join(unresolved) + '\n')
    if strict_rc == 0 or not unresolved:
        raise RuntimeError('Unexpected report-all outcome; inspect before trusting size comparison')
    relaxed = common + ['--unresolved-symbols=ignore-all', '-Map', 'obj/size.map',
                        '-o', 'obj/size-NOT-FIRMWARE.elf']
    if invoke(relaxed, work, out / 'relaxed-size.log'):
        raise RuntimeError('Relaxed size link failed')
    image = obj / 'size-NOT-FIRMWARE.elf'
    for flag, name in [('-t', 'symbols.txt'), ('-h', 'sections.txt')]:
        subprocess.run([str(tool / 'objdump'), flag, str(image)], check=True,
                       stdout=(out / name).open('w'))
    symbols = (out / 'symbols.txt').read_text()
    def addr(name):
        return int(next(line.split()[0] for line in symbols.splitlines()
                        if line.split() and line.split()[-1] == name), 16)
    result = {'variant': args.variant, 'baseline_commit': BASE, 'installable': False,
              'report_all_exit': strict_rc, 'unresolved': unresolved,
              'flash_extent_bytes': addr('__ctl_image_end') - 0x02087000,
              'ram_extent_bytes': addr('__ctl_ram_hi') - 0x01C01800,
              'roots': roots, 'input_hashes': expected,
              'recipe_hashes': {name: digest(HERE / name) for name in
                               ('run.py', 'config-values.json', 'roots.json', 'input-hashes.json')}}
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({key: result[key] for key in ('variant', 'installable',
                      'report_all_exit', 'flash_extent_bytes', 'ram_extent_bytes')}))
    print('Unresolved imports:', ', '.join(unresolved))


if __name__ == '__main__':
    main()
