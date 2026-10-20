#!/usr/bin/env python3
"""Check partial PASS reporting and unreadable index errors in private scratch Git repositories."""
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import runpy
import subprocess
import sys
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location('routing_fixture', ROOT / 'tests/host/test_quickcheck_plan.py')
FIXTURE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(FIXTURE)
rows = []
q = FIXTURE.QuickcheckPlan()
try:
    q.setUp()
    q.put('web/page.js', 'changed\n')
    q.put('tests/run_tests.sh', '''#!/bin/sh
# X0X_TESTS
run() { [ "$1" != breaks-web ] || return 0; echo "  ok   $1"; }
run fastmath sh tests/host/probe.c
run breaks-web sh web/page.js
''')
    p = q.qc(False)
    assert 'missing PASS: breaks-web' in p.stdout and '(runner rc 0)' in p.stdout, p.stdout + p.stderr
    assert '  ok   fastmath' in (q.repo / 'build/qc/tests.log').read_text()
    rows.append({'case': 'partial_selected_PASS', 'result': 'PASS', 'quickcheck_returncode': p.returncode, 'stdout': p.stdout})
finally:
    q.doCleanups()
q = FIXTURE.QuickcheckPlan()
try:
    q.setUp()
    q.put('web/page.js', 'changed\n')
    original = Path.read_text
    def unreadable(path, *args, **kwargs):
        if path == q.repo / 'tests/host/probe.c':
            raise PermissionError('stage3 injected unreadable test source')
        return original(path, *args, **kwargs)
    stderr = io.StringIO()
    stdout = io.StringIO()
    with patch.object(sys, 'argv', [str(ROOT / 'tools/quickcheck_plan.py'), str(q.repo), q.base, 'HEAD', '--folder']), patch.object(Path, 'read_text', unreadable), contextlib.redirect_stderr(stderr), contextlib.redirect_stdout(stdout):
        try:
            runpy.run_path(str(ROOT / 'tools/quickcheck_plan.py'), run_name='__main__')
        except SystemExit as e:
            rc = e.code
        else:
            raise AssertionError('unreadable index did not fail')
    assert rc == 1
    assert 'Quickcheck planning failed:' in stderr.getvalue()
    assert stdout.getvalue() == ''
    rows.append({'case': 'unreadable_test_source', 'result': 'PASS', 'planner_returncode': rc, 'stderr': stderr.getvalue()})
finally:
    q.doCleanups()
print(json.dumps(rows, indent=2))
