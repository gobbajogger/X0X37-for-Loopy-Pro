#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build/run only pure capture core tests; optional pinned target object compile.

No device, installer, raw input file or firmware linkage. All bytes synthetic.
Outputs evidence/binaries only to caller-selected temporary/private build directory.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess

ROOT = Path(__file__).resolve().parents[3]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', required=True, type=Path)
    ap.add_argument('--target-compiler', type=Path)
    args = ap.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    src = ROOT / 'tools/mboot/capture_core.c'
    test = Path(__file__).with_name('test_core.c')
    flags = ['-std=c11', '-Wall', '-Wextra', '-Werror', '-pedantic',
             '-I' + str(src.parent)]
    commands = []

    def run(command, **kw):
        command = list(map(str, command))
        commands.append(command)
        r = subprocess.run(command, text=True, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, **kw)
        print(r.stdout, end='')
        with (out / 'commands.log').open('a') as f:
            f.write(json.dumps(command) + '\n' + r.stdout + '\nexit=' + str(r.returncode) + '\n')
        r.check_returncode()
        return r.stdout

    run(['cc', *flags, '-O2', src, test, '-o', out / 'test-core'])
    normal = run([out / 'test-core'])
    run(['cc', *flags, '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
         '-no-pie', src, test, '-o', out / 'test-core-sanitized'])
    sanitizer = run([out / 'test-core-sanitized'], env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
    run(['cc', *flags, '-shared', '-fPIC', src, '-o', out / 'core.so'])
    lib = ctypes.CDLL(str(out / 'core.so'))
    lib.mb_capture_sha256.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p]
    lib.mb_capture_sha256.restype = ctypes.c_int
    rng = random.Random(0x4D424F4F54)
    lengths = list(range(257)) + [511, 512, 513, 4095, 4096, 4097, 16383, 16384, 16385, 1048576]
    for n in lengths:
        raw = rng.randbytes(n)
        data = ctypes.create_string_buffer(raw)
        digest = ctypes.create_string_buffer(32)
        if lib.mb_capture_sha256(data, n, digest) != 0 or digest.raw != hashlib.sha256(raw).digest():
            raise AssertionError('SHA256 differential mismatch at synthetic length ' + str(n))
    target = {'status': 'NOT_RUN', 'reason': 'optional compiler not selected'}
    if args.target_compiler:
        compiler = args.target_compiler.resolve()
        # Same previously qualified compiler pin; reject replacement before execution.
        pin = 'c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f'
        if hashlib.sha256(compiler.read_bytes()).hexdigest() != pin:
            raise ValueError('unrecognized target compiler; do not execute')
        version = run([compiler, '--version'])
        run([compiler, '-target', 'pi32v2', '-mcpu=r3', '-Oz', '-ffreestanding',
             '-fno-builtin', *flags, '-c', src, '-o', out / 'capture-core.o'])
        target = {'status': 'PASS_OBJECT_ONLY', 'compiler_sha256': pin, 'version': version,
                  'object_sha256': hashlib.sha256((out / 'capture-core.o').read_bytes()).hexdigest()}
    receipt = dict(scope='PLATFORM_DISABLED_PURE_CORE', host=normal.strip(),
                   sanitizer=sanitizer.strip(), sha256_differential_cases=len(lengths),
                   target=target, source_sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                   for p in [src, src.with_suffix('.h'), test, Path(__file__)]},
                   limits=['No device/backend/writer or firmware linkage',
                           'Target RAM placement, stack, timing and identity/exclusion NOT_QUALIFIED',
                           'LeakSanitizer disabled in ptrace environment; no core allocations'])
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    main()
