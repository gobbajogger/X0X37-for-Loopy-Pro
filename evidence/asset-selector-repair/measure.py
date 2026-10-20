#!/usr/bin/env python3
"""Measure real generator bytes and separate-process reuse in disposable Linux sources."""
from pathlib import Path
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
NAMES = ('felucca_font.h', 'x0x_drum_samples.h', 'x0x_drum_tables.h', 'x0x_break_bank.h')


def worker():
    root, store, mode = Path(sys.argv[2]), Path(sys.argv[3]), sys.argv[4]
    os.chdir(root)
    sys.path.insert(0, str(root / 'tools'))
    import asset_cache
    import build
    asset_cache.cache_directory = lambda root: store
    if mode == 'warm':
        def forbidden(*args, **kwargs):
            raise AssertionError('persistent warm inventory restarted discovery')
        asset_cache.run_checked = forbidden
    build.generate()
    print('HASHES=' + json.dumps({n: asset_cache.digest(root / 'build/gen' / n) for n in NAMES}))


def main():
    report = {'image': os.environ['MEASURE_IMAGE_ID'], 'uid': os.getuid(), 'runs': {}}
    with tempfile.TemporaryDirectory(prefix='asset-selector-measure-') as tmp:
        base = Path(tmp)
        root, store = base / 'source', base / 'cache'
        for name in ('tools', 'assets', 'firmware/src'):
            shutil.copytree(ROOT / name, root / name)
        for mode in ('disabled', 'cold', 'warm', 'fresh'):
            if mode == 'fresh':
                other = base / 'fresh'
                shutil.copytree(root, other, ignore=shutil.ignore_patterns('build', '__pycache__'))
                root = other
            env = dict(os.environ, X0X_ASSET_CACHE='0' if mode == 'disabled' else '1')
            command = [sys.executable, '-u', '-B', str(Path(__file__).resolve()), '--worker',
                       str(root), str(store), 'warm' if mode in ('warm', 'fresh') else mode]
            start = time.monotonic()
            result = subprocess.run(command, env=env, capture_output=True, text=True)
            seconds = time.monotonic() - start
            print(result.stdout, end='', flush=True)
            print(result.stderr, end='', flush=True)
            assert result.returncode == 0, result.returncode
            hashes = json.loads(next(s for s in result.stdout.splitlines() if s.startswith('HASHES='))[7:])
            hits = [s for s in result.stdout.splitlines() if s.startswith('asset cache:')]
            report['runs'][mode] = {'seconds': seconds, 'hashes': hashes, 'hits': hits}
            if mode == 'disabled':
                expected = hashes
            assert hashes == expected, mode
            assert len(hits) == 3
            assert all(s.endswith(' hit' if mode in ('warm', 'fresh') else ' miss') for s in hits), hits
    report['warm_speedup'] = report['runs']['disabled']['seconds'] / report['runs']['warm']['seconds']
    report['result'] = 'PASS'
    report['limits'] = 'Asset generation only. Fresh uses a disposable source copy; real clean Git worktree checks belong to independent stage3.'
    (OUT / 'timing.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == '--worker':
        worker()
    else:
        main()
