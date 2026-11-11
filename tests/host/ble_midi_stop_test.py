#!/usr/bin/env python3
"""Run the production service body with simulated mailbox, time and radio state.

The source root option permits negative controls in a scratch tree.
No device access occurs. Target interrupt timing is outside this test.
"""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[2])
parser.add_argument('case', nargs='?', choices=['cancel', 'scan', 'failed-status', 'edges'])
args = parser.parse_args()
root = args.source_root.resolve()
source = (root / 'firmware/src/ble.c').read_text()
start = source.index('static void ble_central_service(void)\n{')
end = source.index('\n}\n', start) + 3
with tempfile.TemporaryDirectory(prefix='ble-stop-') as directory:
    scratch = Path(directory)
    (scratch / 'ble_central_service.inc').write_text(source[start:end])
    binary = scratch / 'stop'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c99', '-g', '-O1', '-Wall',
                    '-Wextra', '-Werror', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', '-I' + str(scratch), '-I' + str(root / 'firmware/src/ble'),
                    str(Path(__file__).with_suffix('.c')),
                    str(root / 'firmware/src/ble/ble_midi_central.c'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)] + ([args.case] if args.case else []), check=True)
