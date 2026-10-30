#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build-only SMK MBOOT qualification. Never opens a device or builds an installer."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

PIN = '64b74d009e6d21fe85ca60754b7c01d438214804'
TREE = '105c8347f2b8a1f1f4e3993bec31735fde1e5828'
ROOT = Path(__file__).resolve().parents[2]


def run(command, cwd, env=None):
    print('+ ' + ' '.join(map(str, command)), flush=True)
    subprocess.run(list(map(str, command)), cwd=cwd, env=env, check=True)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_upstream(up):
    git = lambda *a: subprocess.check_output(['git', '-C', str(up), *a], text=True).strip()
    if git('rev-parse', 'HEAD') != PIN or git('rev-parse', 'HEAD^{tree}') != TREE:
        raise ValueError('wrong upstream source pin')
    run(['git', 'diff', '--exit-code', 'HEAD', '--'], up)
    for line in git('ls-tree', '-r', 'HEAD').splitlines():
        metadata, name = line.split('\t', 1)
        mode, kind, blob = metadata.split()
        raw = (up / name).read_bytes()
        actual = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
        if kind != 'blob' or mode != '100644' or actual != blob:
            raise ValueError('upstream source bytes differ: ' + name)
    # Reject shadow headers/scripts or other unknown source inputs.
    unknown = git('ls-files', '--others', '--exclude-standard').splitlines()
    if unknown and not (unknown == ['toolchain'] and (up / 'toolchain').is_symlink()):
        raise ValueError('untracked upstream input; build/ and toolchain/ must be ignored')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--upstream', required=True, type=Path)
    parser.add_argument('--receipt', required=True, type=Path)
    parser.add_argument('--toolchain-id', required=True,
                        help='Recorded revision; not a hardware compatibility claim')
    parser.add_argument('--qualify-alternative-toolchain', action='store_true')
    args = parser.parse_args()
    up = args.upstream.resolve()
    verify_upstream(up)
    if args.toolchain_id != '20250324.1' and not args.qualify_alternative_toolchain:
        raise ValueError('alternative toolchain requires explicit build-only qualification')
    tool_names = ('common/bin/clang', 'common/bin/objcopy', 'common/bin/objdump',
                  'pi32v2/bin/lto-wrapper')
    tools = {name: sha(up / 'toolchain' / name) for name in tool_names}
    run([up / 'toolchain/common/bin/clang', '--version'], up)
    boot = up / 'build/smk37/uboot.boot'
    run(['make', '-B', '-j2', 'build/smk37/uboot.boot', 'build/core-test',
         'build/contract-test', 'build/boot-test-smk37'], up)
    first = sha(boot)
    run(['make', '-B', '-j2', 'build/smk37/uboot.boot'], up)
    if sha(boot) != first:
        raise ValueError('native SMK boot container is not reproducible')
    for target in ('core-test', 'contract-test', 'boot-test-smk37'):
        run([up / 'build' / target], up)
    # Tests use synthetic 1 MiB images and the actual native container. All
    # transformations stay in process memory; no output image is serialized.
    source = ROOT / 'tools/mboot/adoption.c'
    test = ROOT / 'tests/host/mboot_adoption/test_adoption.c'
    with tempfile.TemporaryDirectory(prefix='mboot-adoption-') as directory:
        temp = Path(directory)
        flags = ['-std=c11', '-g', '-Wall', '-Wextra', '-Werror',
                 '-I' + str(source.parent), '-I' + str(up / 'include')]
        for sanitize in (False, True):
            executable = temp / ('test-sanitize' if sanitize else 'test')
            extra = ['-O1', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if sanitize else ['-O2']
            run(['cc', *flags, *extra, source, up / 'src/core.c', test, '-o', executable], ROOT)
            env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
            run([executable, boot], ROOT, env)
        # Compile the adoption API for the actual target too; this is a source
        # compile check, not linkage into the bootloader or shipping firmware.
        run([up / 'toolchain/common/bin/clang', '-target', 'pi32v2', '-mcpu=r3',
             '-Oz', '-ffreestanding', '-fno-builtin', *flags, '-c', source,
             '-o', temp / 'adoption.o'], ROOT)
    receipt = {
        'status': 'SOURCE_ONLY_BUILD_ONLY', 'upstream_commit': PIN, 'upstream_tree': TREE,
        'toolchain_id': args.toolchain_id, 'toolchain_sha256': tools,
        'alternative_toolchain': args.toolchain_id != '20250324.1',
        'container_sha256': first, 'container_bytes': boot.stat().st_size,
        'boot_slot_spare_bytes': 0x3830 - boot.stat().st_size,
        'native_bounds': json.loads((up / 'build/smk37/size.json').read_text()),
        'reproducible': True, 'host_adoption': 'PASS', 'asan_ubsan': 'PASS',
        'target_adoption_object': 'PASS',
        'source_sha256': {str(p.relative_to(ROOT)): sha(p) for p in
                          (source, source.with_name('adoption.h'), Path(__file__), test)},
        'limits': ['No device access, installation, firmware package, or controller build.',
                   'Default application updater and installer paths are unchanged.',
                   'Private upstream hardware-image tests are SKIP; synthetic tests ran.',
                   'LeakSanitizer NOT_RUN: ptrace environment; detect_leaks=0.',
                   'Alternative compiler qualification is structural/build-only, not hardware proof.']}
    args.receipt.parent.mkdir(parents=True, exist_ok=True)
    args.receipt.write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    main()
