#!/usr/bin/env python3
"""Check the loader ORIGIN behavior of a symlink-selected native entry.

Use pinned Linux worker; compile only disposable host fixture shared libraries.
Never mutate the image runtime. A changed selector must miss or safely fall back.
"""
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
    source = native / 'fixture.c'
    for folder, value in ((real, 1), (alias, 2)):
        source.write_text('int token(void) { return ' + str(value) + '; }\n')
        subprocess.run(['/usr/bin/gcc', '-shared', '-fPIC', str(source),
            '-Wl,-soname,libtoken.so', '-o', str(folder / 'search/libtoken.so')], check=True)
    source.write_text('extern int token(void); int entry(void) { return token(); }\n')
    subprocess.run(['/usr/bin/gcc', '-shared', '-fPIC', str(source), '-L' + str(real / 'search'),
        '-ltoken', '-Wl,-rpath,$ORIGIN/search', '-o', str(real / 'entry.so')], check=True)
    (alias / 'entry.so').symlink_to(real / 'entry.so')
    (case.root / 'tools/fixture_runtime.py').write_text('import ctypes\nTOKEN = str(ctypes.CDLL(' +
        repr(str(alias / 'entry.so')) + ').entry())\n')
    cold = case.worker()
    first = (case.gen / 'x0x_drum_samples.h').read_text()
    warm = case.worker(deny=True)
    # Same selected entry and canonical maps file; replace only an existing
    # dependency in the alias ORIGIN folder, with stable names and mode bits.
    source.write_text('int token(void) { return 3; }\n')
    subprocess.run(['/usr/bin/gcc', '-shared', '-fPIC', str(source),
        '-Wl,-soname,libtoken.so', '-o', str(alias / 'search/libtoken.so')], check=True)
    changed = case.worker()
    after = (case.gen / 'x0x_drum_samples.h').read_text()
    try:
        case.assert_original('drums')
        original_match = True
    except AssertionError:
        original_match = False
    print(json.dumps({'cold_hit': cold, 'warm_hit': warm, 'first': first,
        'changed_hit': changed, 'after': after, 'matches_original': original_match}), flush=True)
    assert not cold and warm
    assert not changed and original_match
finally:
    case.tearDown()
