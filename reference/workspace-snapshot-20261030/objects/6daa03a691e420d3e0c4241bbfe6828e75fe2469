#!/usr/bin/env python3
"""Fail-closed, offline native X0X37 nightly BUILD PROBE.

Produces temporary model-specific DEV artifacts and auditable logs. Never
publishes or declares installability. A separate exact-source independent QA,
licensing, immutable hosting, installer and live HTTP gate is always required.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

SDK_PINS = {
    'cpu/wl82/tools/uboot.boot': '4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4',
    'cpu/wl82/tools/cfg_tool.bin': '276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b',
    'cpu/wl82/tools/cfg/eq_cfg_hw.bin': '41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d',
}
# SHA-256 of the independently verified native JieLi pi32v2 Clang 4.0.1 ELF
# from owner-pinned jieli-linux-toolchains-20260730.1.zip. A spoofed
# '--version' banner must never establish executable build provenance.
JIELI_COMPILER_SHA256 = 'c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f'
PRODUCTS = {'elite': 'x0x-smk37-elite.fwsc', 'pro': 'x0x-smk37.fwsc'}
HEX40 = re.compile(r'^[0-9a-f]{40}$')


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1048576), b''):
            h.update(chunk)
    return h.hexdigest()


def git(source: Path, *arguments: str) -> str:
    result = subprocess.run(['git', '-C', str(source), *arguments], text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if result.returncode:
        raise RuntimeError('git ' + ' '.join(arguments) + ': ' + result.stderr.strip())
    return result.stdout.strip()


def verify_source(source: Path, expected_head: str, expected_tree: str) -> dict:
    if not HEX40.fullmatch(expected_head) or not HEX40.fullmatch(expected_tree):
        raise ValueError('Expected source commit and tree must be exact 40-character lowercase SHAs')
    head = git(source, 'rev-parse', 'HEAD')
    tree = git(source, 'rev-parse', 'HEAD^{tree}')
    if head != expected_head or tree != expected_tree:
        raise ValueError(f'STALE SOURCE: local {head} / {tree} does not match expected {expected_head} / {expected_tree}')
    if git(source, 'status', '--porcelain', '--untracked-files=normal'):
        raise ValueError('Source checkout has local modifications/untracked files; refusing a misleading build')
    return {'commit': head, 'tree': tree}


def verify_inputs(toolchain: Path, sdk: Path) -> dict:
    compiler = toolchain / 'pi32v2/bin/clang'
    if not compiler.is_file() or not os.access(compiler, os.X_OK):
        raise ValueError('Missing runnable JieLi pi32v2 clang; toolchain archive and symlinks must be restored')
    compiler_sha256 = sha256(compiler)
    if compiler_sha256 != JIELI_COMPILER_SHA256:
        raise ValueError('pinned JieLi compiler SHA-256 mismatch: '
                         f'expected {JIELI_COMPILER_SHA256}; observed {compiler_sha256}')
    check = subprocess.run([str(compiler), '--version'], text=True, capture_output=True, timeout=15)
    if check.returncode or 'clang version 4.0.1' not in check.stdout:
        raise ValueError('Expected pinned JieLi clang 4.0.1 executable did not run')
    sdk_files = {}
    for relative, want in SDK_PINS.items():
        file = sdk / relative
        if not file.is_file() or sha256(file) != want:
            raise ValueError(f'SDK V1.2.1 pin mismatch: {relative}')
        sdk_files[relative] = want
    return {'compiler_sha256': compiler_sha256, 'compiler_version': check.stdout.splitlines()[0],
            'sdk_pins_sha256': sdk_files}


def run_check(name: str, argv: list[str], cwd: Path, env: dict, log_dir: Path, timeout: int) -> dict:
    start = time.monotonic()
    path = log_dir / (name + '.log')
    with path.open('w', encoding='utf-8') as stream:
        stream.write('Command: ' + json.dumps(argv) + '\n')
        stream.flush()
        try:
            result = subprocess.run(argv, cwd=cwd, env=env, stdout=stream, stderr=subprocess.STDOUT,
                                    timeout=timeout, check=False)
            status = result.returncode
        except subprocess.TimeoutExpired:
            status = 124
            stream.write(f'\nTimed out after {timeout}s\n')
    return {'name': name, 'command': argv, 'exit': status,
            'elapsed_seconds': round(time.monotonic() - start, 2), 'log': path.name}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=Path.cwd(), help='clean, complete local Git checkout')
    parser.add_argument('--expected-head', required=True, help='remote-verified exact commit SHA')
    parser.add_argument('--expected-tree', required=True, help='remote-verified exact source-tree SHA')
    parser.add_argument('--toolchain', required=True, type=Path)
    parser.add_argument('--sdk', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path, help='MUST NOT exist (avoids old build overwrite)')
    parser.add_argument('--timeout', type=int, default=180, help='per-stage timeout seconds')
    args = parser.parse_args()
    if args.timeout < 10 or args.timeout > 1800:
        parser.error('--timeout must be between 10 and 1800 seconds')
    source = args.source.resolve(strict=True)
    tc = args.toolchain.resolve(strict=True)
    sdk = args.sdk.resolve(strict=True)
    # Verify everything before allocating or modifying any build tree/output.
    source_identity = verify_source(source, args.expected_head, args.expected_tree)
    build_inputs = verify_inputs(tc, sdk)
    # Canonicalize every existing parent link before enforcing source isolation.
    # absolute() leaves symlink parents intact and can dirty the verified checkout.
    requested_output = args.output.expanduser()
    if requested_output.exists() or requested_output.is_symlink():
        raise ValueError('Output directory already exists: refuses to overwrite prior candidate/evidence')
    output = requested_output.resolve(strict=False)
    if output.exists() or output.is_symlink():
        raise ValueError('Resolved output directory already exists: refusing overwrite')
    if output == source or source in output.parents:
        raise ValueError('Output must live outside the source checkout')
    if not output.parent.is_dir():
        raise ValueError('Output parent directory does not exist')
    output.mkdir(mode=0o700)
    logs = output / 'logs'
    logs.mkdir()
    scratch = output / '_build_checkout'
    manifest = {'schema': 1, 'source': source_identity, 'inputs': build_inputs,
                'models': {}, 'checks': [], 'controller': None,
                'device_tested': False, 'qa_approved': False, 'installable': False,
                'status': 'NATIVE BUILD PROBE ONLY — NOT AN INSTALLABLE OR DISTRIBUTION APPROVAL'}
    result_code = 1
    try:
        # Worktree has real Git metadata for project scripts, but source caller stays untouched.
        git(source, 'worktree', 'add', '--detach', str(scratch), source_identity['commit'])
        env = os.environ.copy()
        env.update({'JIELI_TOOLCHAIN': str(tc), 'AC79_SDK': str(sdk), 'SDK_LOCAL': str(sdk),
                    'BOARD': 'smk37', 'X0X_DEV_PADCAL': '1', 'X0X_JOBS': '2',
                    'CC': 'cc', 'CXX': 'c++'})
        # Build first. Failure of a noncompiler host/backup/installer check never
        # suppresses a real target compile or discards its immutable artifact.
        # Subsequent safety failures only close the candidate admission gate.
        for model, basename in PRODUCTS.items():
            stage_env = env.copy()
            stage_env['MODEL'] = model
            step = run_check(f'native-build-{model}', ['bash', 'build.sh', '--board', 'smk37'],
                             scratch, stage_env, logs, args.timeout)
            manifest['checks'].append(step)
            artifact = scratch / 'build' / basename
            if step['exit'] == 0 and artifact.is_file() and artifact.stat().st_size > 0:
                dst = output / basename
                shutil.copyfile(artifact, dst)
                guard = run_check(f'package-integrity-{model}',
                                  ['python3', 'tools/check_native_build_output.py',
                                   'package', str(artifact), '--model', model],
                                  scratch, stage_env, logs, args.timeout)
                manifest['checks'].append(guard)
                manifest['models'][model] = {
                    'status': ('BUILT — PACKAGE MODEL/CRC VERIFIED; QA PENDING'
                               if guard['exit'] == 0 else
                               'BUILT — PACKAGE INVALID; NOT INSTALLABLE'),
                    'file': basename, 'bytes': dst.stat().st_size,
                    'sha256': sha256(dst), 'package_check_exit': guard['exit']}
            else:
                manifest['models'][model] = {'status': 'BUILD FAILED/NO ARTIFACT', 'file': None}
        cenv = env.copy()
        cenv.update({'CTL_NATIVE': '1', 'CTL_NO_CACHE': '1'})
        controller = run_check('native-controller', ['bash', 'ble/ctl/build_ctl.sh'],
                               scratch, cenv, logs, args.timeout)
        manifest['checks'].append(controller)
        image = scratch / 'ble/ctl/out/ctl.img'
        if controller['exit'] == 0 and image.is_file() and image.stat().st_size > 0:
            dst = output / 'ctl.img'
            shutil.copyfile(image, dst)
            guard = run_check('controller-integrity',
                              ['python3', 'tools/check_native_build_output.py',
                               'controller', str(image)],
                              scratch, cenv, logs, args.timeout)
            manifest['checks'].append(guard)
            manifest['controller'] = {
                'status': ('BUILT — HEADER/CRC VERIFIED; PRIVATE/DISTRIBUTION NOT CLEARED'
                           if guard['exit'] == 0 else
                           'BUILT — CONTROLLER INVALID; NOT INSTALLABLE'),
                'file': dst.name, 'bytes': dst.stat().st_size,
                'sha256': sha256(dst), 'controller_check_exit': guard['exit']}
        postbuild = [
            ('locked-settings', ['python3', 'tools/check_locked_settings.py']),
            ('slot-backup', ['python3', 'tests/x0x_slots_test.py']),
            ('installer-slots', ['python3', 'tests/install_slots_test.py']),
        ]
        for name, command in postbuild:
            manifest['checks'].append(run_check(name, command, scratch, env, logs, args.timeout))
        failed_safety_checks = [step['name'] for step in manifest['checks']
                                if step['name'] in {name for name, _ in postbuild}
                                and step['exit'] != 0]
        if failed_safety_checks:
            manifest['blocked_by'] = failed_safety_checks
        result_code = 0 if (all(x['exit'] == 0 for x in manifest['checks'])
                             and len(manifest['models']) == len(PRODUCTS)
                             and all(x['file'] for x in manifest['models'].values())
                             and manifest['controller']) else 1
    finally:
        if scratch.exists():
            # A positive result is not authoritative until private cleanup succeeds.
            try:
                remove = subprocess.run(['git', '-C', str(source), 'worktree', 'remove', '--force', str(scratch)],
                                        capture_output=True, text=True)
                if remove.returncode:
                    print('WARNING: private checkout cleanup incomplete: ' + remove.stderr.strip(), file=sys.stderr)
                    result_code = 1
            except OSError as exc:
                print('WARNING: private checkout cleanup incomplete: ' + str(exc), file=sys.stderr)
                result_code = 1
        # Emit the final result only AFTER cleanup; an exit-1 run must never
        # leave either manifest or stdout claiming native_probe_pass=true.
        manifest['native_probe_pass'] = result_code == 0
        (output / 'probe.json').write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
    print(json.dumps({'native_probe_pass': bool(manifest.get('native_probe_pass')),
                      'source': source_identity, 'output': str(output),
                      'release_permission': 'NONE; NOT INSTALLABLE'}, sort_keys=True))
    return result_code


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, RuntimeError, OSError, subprocess.TimeoutExpired) as exc:
        print('PRECHECK FAILED (NO BUILD): ' + str(exc), file=sys.stderr)
        sys.exit(2)
