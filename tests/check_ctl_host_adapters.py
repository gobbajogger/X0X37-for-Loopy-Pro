#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Native tests for conditional host shim adapters; never builds firmware."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='ctl-host-adapters-') as tmp:
    tmp = Path(tmp)
    flags = ['-std=c11', '-Wall', '-Wextra', '-Werror', '-DCTL_VENDOR_HOST_TEST=1',
             '-I' + str(ROOT / 'ble/ctl/src'), '-I' + str(ROOT / 'firmware/hal')]
    objects = []
    for unit in ['ctl_heap', 'ctl_vendor_crc', 'ctl_timer']:
        obj = tmp / (unit + '.o')
        rename = ['-Dmalloc=study_malloc', '-Dfree=study_free'] if unit == 'ctl_heap' else []
        subprocess.run(['cc', *flags, *rename, '-c', str(ROOT / 'ble/ctl/src' / (unit + '.c')),
                        '-o', str(obj)], check=True)
        objects.append(str(obj))
    # Target ABI uses32-bit addresses. Non-PIE keeps the host test's static heap
    # and synthetic tick below4GiB; no production pointer-width change is made.
    exe = tmp / 'check'
    subprocess.run(['cc', *flags, '-no-pie', str(ROOT / 'tests/ctl_host_heap_test.c'),
                    *objects, '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
