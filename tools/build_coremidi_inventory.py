#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build only the read-only CoreMIDI inventory helper on its actual native SDK.

No execution of the produced helper, device access, firmware build or install.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    if sys.platform != 'darwin':
        print('REFUSED: building the CoreMIDI helper requires a native macOS SDK', file=sys.stderr)
        return 2
    root = Path(__file__).resolve().parents[1]
    source = root / 'tools/native/ota_coremidi_inventory.m'
    destination = root / 'build/native/ota_coremidi_inventory'
    destination.parent.mkdir(parents=True, exist_ok=True)
    before = source.read_bytes()
    with tempfile.TemporaryDirectory(prefix='.coremidi-build-', dir=destination.parent) as tmp:
        helper = Path(tmp) / destination.name
        argv = ['xcrun', '--sdk', 'macosx', 'clang', '-fobjc-arc', '-Wall', '-Wextra', '-Werror',
                str(source), '-framework', 'Foundation', '-framework', 'CoreMIDI', '-o', str(helper)]
        try:
            result = subprocess.run(argv, check=False)
        except OSError as exc:
            print(f'BUILD UNAVAILABLE: {exc}', file=sys.stderr)
            return 2
        if result.returncode:
            return result.returncode
        if source.read_bytes() != before:
            print('REFUSED: helper source changed while compiling', file=sys.stderr)
            return 2
        manifest = {'schema': 'x0x37-coremidi-helper-build-v1',
                    'source_sha256': hashlib.sha256(before).hexdigest(),
                    'helper_sha256': hashlib.sha256(helper.read_bytes()).hexdigest()}
        receipt = Path(tmp) / 'manifest.json'
        receipt.write_text(json.dumps(manifest, indent=2) + '\n')
        # A reader racing these two renames may refuse a hash mismatch; it
        # never accepts an unmatched old helper as this source's build.
        os.replace(helper, destination)
        os.replace(receipt, destination.with_suffix('.json'))
        print(json.dumps(manifest, sort_keys=True))
    return 0


if __name__ == '__main__':
    sys.exit(main())
