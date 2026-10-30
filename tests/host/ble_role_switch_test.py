#!/usr/bin/env python3
"""Exercise production role services and orchestrator against public HCI hosts."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'firmware/src/ble.c').read_text()
parts = []
for name in ('ble_central_service', 'ble_device_service'):
    start = source.index(f'static void {name}(void)\n{{')
    end = source.index('\n}\n', start) + 3
    parts.append(source[start:end])
with tempfile.TemporaryDirectory(prefix='ble-role-') as directory:
    scratch = Path(directory)
    (scratch / 'ble_role_services.inc').write_text('\n'.join(parts))
    binary = scratch / 'role'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c99', '-g', '-O1',
                    '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
                    '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    '-I' + str(scratch), '-I' + str(root / 'firmware/hal'),
                    '-I' + str(root / 'firmware/src/ble'),
                    str(Path(__file__).with_suffix('.c')), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
