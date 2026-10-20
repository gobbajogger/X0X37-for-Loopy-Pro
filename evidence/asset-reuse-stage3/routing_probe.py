#!/usr/bin/env python3
"""Check build.generate scheduling and error propagation without a target build."""
import json
from pathlib import Path
import sys
import threading
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import build


def check(fail):
    barrier = threading.Barrier(2, timeout=5)
    finished = set()
    calls = []
    lock = threading.Lock()

    def generate(kind, root, gen):
        assert root == build.SRC and gen == build.GEN
        with lock:
            calls.append(kind)
        if kind in ('font', 'drums'):
            barrier.wait()
            with lock:
                finished.add(kind)
            if fail and kind == 'font':
                raise RuntimeError('controlled font failure')
        else:
            assert finished == {'font', 'drums'}

    with patch.object(build.asset_cache, 'generate_asset', side_effect=generate):
        if fail:
            try:
                build.generate()
            except RuntimeError as error:
                assert str(error) == 'controlled font failure'
            else:
                raise AssertionError('font failure did not propagate')
            assert set(calls) == {'font', 'drums'}
        else:
            build.generate()
            assert set(calls) == {'font', 'drums', 'breaks'}
            assert calls[-1] == 'breaks'
    assert finished == {'font', 'drums'}
    return {'failure_injected': fail, 'calls': calls, 'finished': sorted(finished), 'result': 'PASS'}


print(json.dumps({'scope': 'build.generate parallel scheduling, dependency order and failure handling',
                  'cases': [check(False), check(True)]}, indent=2))
