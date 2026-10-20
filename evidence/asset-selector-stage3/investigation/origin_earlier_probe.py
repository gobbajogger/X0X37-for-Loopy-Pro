#!/usr/bin/env python3
"""Check earlier ORIGIN lookup candidates for a symlink-selected native entry.

Use pinned Linux worker; compile only disposable host fixture shared libraries.
Never mutate the image runtime. A changed selector must miss or safely fall back.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('final_asset_tests', ROOT / 'tests/host/test_asset_cache.py')
tests = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tests)
case = tests.SelectorTests('test_loaded_native_library_alias_retarget_matches_original')
case.setUp()
try:
    native = case.base / 'native'
    native.mkdir()
    real, alias = native / 'real', native / 'alias'
    for folder in (real, alias):
        (folder / 'search').mkdir(parents=True)
        (folder / 'early').mkdir()
    source = native / 'fixture.c'
    for folder, value in ((real, 1), (alias, 2)):
        source.write_text('int token(void) { return ' + str(value) + '; }\n')
        subprocess.run(['/usr/bin/gcc', '-shared', '-fPIC', str(source),
            '-Wl,-soname,libtoken.so', '-o', str(folder / 'search/libtoken.so')], check=True)
    source.write_text('extern int token(void); int entry(void) { return token(); }\n')
    subprocess.run(['/usr/bin/gcc', '-shared', '-fPIC', str(source), '-L' + str(real / 'search'),
        '-ltoken', '-Wl,-rpath,$ORIGIN/early:$ORIGIN/search', '-o', str(real / 'entry.so')], check=True)
    (alias / 'entry.so').symlink_to(real / 'entry.so')
    (case.root / 'tools/fixture_runtime.py').write_text('import ctypes\nTOKEN = str(ctypes.CDLL(' +
        repr(str(alias / 'entry.so')) + ').entry())\n')
    cold = case.worker()
    first = (case.gen / 'x0x_drum_samples.h').read_text()
    warm = case.worker(deny=True)
    inventory = json.loads(next(case.store.glob('discovery/drums/*.json')).read_text())['identity']
    native_lookups = sorted(p for p in inventory['lookups'] if str(native) in p)
    # Add an earlier native candidate under the selected entry alias. The
    # canonical maps entry points to real/entry.so, whose ORIGIN differs.
    source.write_text('int token(void) { return 3; }\n')
    subprocess.run(['/usr/bin/gcc', '-shared', '-fPIC', str(source),
        '-Wl,-soname,libtoken.so', '-o', str(alias / 'early/libtoken.so')], check=True)
    changed = case.worker()
    after = (case.gen / 'x0x_drum_samples.h').read_text()
    try:
        case.assert_original('drums')
        original_match = True
    except AssertionError:
        original_match = False
    print(json.dumps({'cold_hit': cold, 'warm_hit': warm, 'first': first,
        'changed_hit': changed, 'after': after, 'matches_original': original_match,
        'direct': (case.gen / 'x0x_drum_samples.h').read_text(),
        'cached_sha256': hashlib.sha256(after.encode()).hexdigest(),
        'direct_sha256': hashlib.sha256((case.gen / 'x0x_drum_samples.h').read_bytes()).hexdigest(),
        'native_lookups': native_lookups,
        'actual_early_directory_inventoried': str(alias / 'early') in inventory['lookups']}), flush=True)
    assert not cold and warm
    assert not changed and original_match
finally:
    case.tearDown()
