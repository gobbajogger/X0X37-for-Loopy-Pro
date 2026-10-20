#!/usr/bin/env python3
"""Compare unchanged original generators with cold and warm cached generation."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import asset_cache
import build

OUT = ROOT / 'evidence/asset-reuse-stage2'
GEN = ROOT / 'build/gen'
BASELINE = ROOT / 'build/asset-baseline'
BASELINE.mkdir(parents=True, exist_ok=True)
names = [n for group in asset_cache.OUTPUTS.values() for n in group]

def hashes(folder):
    return {n: asset_cache.digest(folder / n) for n in names}

def run(command):
    subprocess.run(command, cwd=ROOT, check=True)

start = time.monotonic()
commands = [[sys.executable, str(ROOT / 'tools/gen_font.py'), str(BASELINE / 'felucca_font.h')],
            [sys.executable, str(ROOT / 'tools/gen_drum_samples.py'), str(BASELINE / 'x0x_drum_samples.h')]]
procs = [subprocess.Popen(c, cwd=ROOT) for c in commands]
for proc in procs:
    if proc.wait():
        raise RuntimeError('baseline generator failed')
run(['sh', str(ROOT / 'tools/gen_builtin_break.sh'), str(BASELINE)])
baseline_seconds = time.monotonic() - start
expected = hashes(BASELINE)
# Use a private cache under Git metadata. Do not disturb another run's cache.
store = asset_cache.cache_directory(ROOT).parent / ('stage2-measure-' + str(os.getpid()))
real_directory = asset_cache.cache_directory
asset_cache.cache_directory = lambda root: store
if GEN.exists():
    shutil.rmtree(GEN)
start = time.monotonic()
build.generate()
cold_seconds = time.monotonic() - start
assert hashes(GEN) == expected, 'cold output differs from original generators'
shutil.rmtree(GEN)
start = time.monotonic()
build.generate()
warm_seconds = time.monotonic() - start
assert hashes(GEN) == expected, 'warm output differs from original generators'
fresh = ROOT / 'build' / ('asset-fresh-' + str(os.getpid()))
run(['git', '-C', str(ROOT), 'worktree', 'add', '--detach', str(fresh), 'HEAD'])
try:
    for name in ('build.py', 'asset_cache.py'):
        shutil.copyfile(ROOT / 'tools' / name, fresh / 'tools' / name)
    code = ('import sys;from pathlib import Path;sys.path.insert(0,"tools");'
            'import asset_cache,build;asset_cache.cache_directory=lambda root:Path(sys.argv[1]);'
            'build.generate()')
    start = time.monotonic()
    subprocess.run([sys.executable, '-u', '-B', '-c', code, str(store)], cwd=fresh, check=True)
    fresh_seconds = time.monotonic() - start
    assert hashes(fresh / 'build/gen') == expected, 'fresh-worktree output differs'
    fresh_status = subprocess.check_output(['git', '-C', str(fresh), 'status', '--porcelain'], text=True)
    assert all(line.endswith(('tools/build.py', 'tools/asset_cache.py'))
               for line in fresh_status.splitlines()), fresh_status
finally:
    run(['git', '-C', str(ROOT), 'worktree', 'remove', '--force', str(fresh)])
result = {'baseline_seconds': baseline_seconds, 'cold_seconds': cold_seconds,
          'warm_seconds': warm_seconds, 'saved_seconds': baseline_seconds - warm_seconds,
          'warm_speedup': baseline_seconds / warm_seconds, 'outputs': expected,
          'fresh_worktree_seconds': fresh_seconds, 'fresh_worktree_outputs_equal': True,
          'fresh_worktree_status': fresh_status.splitlines(),
          'cache': str(store), 'image': os.environ['MEASURE_IMAGE_ID'],
          'source_files': {str(p.relative_to(ROOT)): asset_cache.digest(p)
                           for p in [ROOT / 'tools/build.py', ROOT / 'tools/asset_cache.py',
                                     ROOT / 'tests/host/test_asset_cache.py']}}
(OUT / 'timing.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2), flush=True)
