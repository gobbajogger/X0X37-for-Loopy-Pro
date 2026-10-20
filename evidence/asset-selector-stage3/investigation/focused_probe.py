#!/usr/bin/env python3
"""Investigate the saved mocked warm None observation on immutable final source.

Run inside the pinned Linux worker. All fixture writes use TemporaryDirectory.
Trace return values without changing production files. Repeat the exact three-case
sequence and then repeat its first mocked-discovery case in one interpreter.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('final_asset_tests', ROOT / 'tests/host/test_asset_cache.py')
tests = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tests)
original = tests.cache.generate_asset
returns = []

def traced(*args, **kwargs):
    result = original(*args, **kwargs)
    returns.append({'kind': args[0], 'value': result, 'type': type(result).__name__})
    print('RETURN=' + json.dumps(returns[-1]), flush=True)
    assert type(result) is bool, 'generate_asset returned a non-boolean'
    return result

tests.cache.generate_asset = traced
suite = unittest.TestSuite()
suite.addTest(tests.CacheTests('test_discovery_reuse_checks_contents_without_restarting_tools'))
suite.addTest(tests.SelectorTests('test_loader_unknown_injection_and_tokens_disable_reuse'))
suite.addTest(tests.SelectorTests('test_loaded_native_library_alias_retarget_matches_original'))
for _ in range(3):
    suite.addTest(tests.CacheTests('test_discovery_reuse_checks_contents_without_restarting_tools'))
result = unittest.TextTestRunner(verbosity=2).run(suite)
print('SUMMARY=' + json.dumps({'result': 'PASS' if result.wasSuccessful() else 'FAIL',
    'tests': result.testsRun, 'returns': returns,
    'source_sha256': hashlib.sha256((ROOT / 'tools/asset_cache.py').read_bytes()).hexdigest()}), flush=True)
raise SystemExit(not result.wasSuccessful())
