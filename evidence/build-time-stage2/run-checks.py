#!/usr/bin/env python3
"""Run scratch tool checks. No target build or device access occurs."""
import json
import os
from pathlib import Path
import subprocess
import sys
import time

root = Path(__file__).resolve().parents[2]
out = Path(__file__).resolve().parent
commands = [
    ('native-dev', [sys.executable, '-B', '-m', 'unittest', 'tests.host.test_dev_validate.DevelopmentValidation']),
    ('worker-dev', [sys.executable, '-B', '-m', 'unittest', 'tests.host.test_dev_validate.WorkerValidation']),
    ('validation-inputs', [sys.executable, '-B', 'tests/host/test_validation_inputs.py']),
    ('preferences', [sys.executable, '-B', 'tools/prefs_lint.py', 'docs/USER_PREFERENCES.md']),
    ('diff', ['git', 'diff', '--check']),
]
results = []
for name, argv in commands:
    env = os.environ.copy()
    env.pop('X0X_TEST_WORKER_IMAGE', None)
    if name == 'worker-dev':
        env['X0X_TEST_WORKER_IMAGE'] = 'x0x37-validation:local'
    start = time.monotonic()
    with (out/(name+'.log')).open('w') as log:
        result = subprocess.run(argv, cwd=root, env=env, stdout=log, stderr=subprocess.STDOUT)
    results.append({'name': name, 'argv': argv, 'returncode': result.returncode,
                    'seconds': time.monotonic()-start, 'log': name+'.log'})
    print(name, result.returncode, flush=True)
(out/'checks.json').write_text(json.dumps(results, indent=2)+'\n')
raise SystemExit(1 if any(row['returncode'] for row in results) else 0)
