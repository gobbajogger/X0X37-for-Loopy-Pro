#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise actual BLE name generation, GAP value and HCI scan response per model."""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='smk-ble-identity-') as tmp:
    for model in ('Pro', 'Elite'):
        binary = str(Path(tmp) / model)
        subprocess.run(['cc', '-O2', '-Wall', '-DBOARD_SMK37=1',
                        f'-DSMK_MODEL="SMK-37 {model}"', '-Ifirmware/hal',
                        '-Ifirmware/src/ble', '-o', binary, 'tests/ble_test.c',
                        '-lpthread'], cwd=ROOT, check=True)
        subprocess.run([binary], cwd=ROOT, check=True)
        print(f'PASS SMK-37 {model}: stable address/suffix, full GAP and scan-response name')
