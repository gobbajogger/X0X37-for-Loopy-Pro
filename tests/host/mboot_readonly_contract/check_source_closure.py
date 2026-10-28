#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only Git/source byte check for the pinned contract, not protocol execution."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--firmware', required=True, type=Path)
    ap.add_argument('--upstream', required=True, type=Path)
    args = ap.parse_args()
    manifest = json.loads((args.firmware / 'docs/mboot/readonly-source-closure.json').read_text())
    count = 0
    for name, repo in [('firmware', args.firmware), ('upstream', args.upstream)]:
        pin = manifest[name]
        for path, expected in pin['files'].items():
            raw = subprocess.check_output(['git', '-C', str(repo), 'show', pin['commit'] + ':' + path])
            actual = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
            if actual != expected:
                raise ValueError('pinned blob mismatch: ' + path)
            if name == 'firmware' and (repo / path).read_bytes() != raw:
                raise ValueError('candidate changed protected caller: ' + path)
            count += 1
    print(json.dumps({'result': 'PASS', 'pinned_files': count,
                      'scope': 'exact source closure only; not device or protocol capability proof'}))


if __name__ == '__main__':
    main()
