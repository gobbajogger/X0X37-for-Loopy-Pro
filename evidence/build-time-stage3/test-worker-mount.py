#!/usr/bin/env python3
"""Scratch-only reproduction of additional worker input mutation coverage."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from tests.host.test_dev_validate import DevelopmentValidation, RUNNER

fixture = DevelopmentValidation()
fixture.fixture_parent = ROOT/'build/stage3-fixtures'
fixture.fixture_parent.mkdir(parents=True, exist_ok=True)
fixture.setUp()
started = time.monotonic()
try:
    extra = fixture.root/'extra'
    extra.mkdir()
    selected = extra/'selected-input.txt'
    selected.write_text('BEFORE')
    plan = json.loads(fixture.plan.read_text())
    code = ('from pathlib import Path; import time; p=Path('+repr(str(selected))+'); '
            'print("READY "+p.read_text(),flush=True); time.sleep(4); '
            'print("AFTER "+p.read_text(),flush=True)')
    plan['checks'][0]['argv'] = ['python3', '-u', '-c', code]
    fixture.plan.write_text(json.dumps(plan))
    env = dict(fixture.env, CC='cc', CXX='c++')
    argv = [sys.executable, '-B', str(RUNNER), 'run', '--repo', str(fixture.repo),
            '--commit', fixture.commit, '--model', 'elite', '--plan', str(fixture.plan),
            '--output', str(fixture.out), '--timeout', '60', '--no-controller',
            '--worker-image', 'x0x37-validation:local', '--worker-mount', str(extra)]
    image = subprocess.check_output(['docker', 'image', 'inspect', 'x0x37-validation:local',
                                    '--format', '{{.Id}} {{.Os}}/{{.Architecture}}'], text=True).strip()
    process = subprocess.Popen(argv, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    deadline = time.monotonic()+60
    while time.monotonic() < deadline:
        checklog = fixture.out/'check-0.log'
        if checklog.exists() and 'READY BEFORE' in checklog.read_text():
            selected.write_text('CHANGED')
            break
        if process.poll() is not None:
            raise AssertionError('runner stopped before mutation: '+str(process.communicate()))
        time.sleep(0.05)
    else:
        process.terminate()
        raise AssertionError('runner did not reach selected check')
    stdout, stderr = process.communicate(timeout=70)
    (OUT/'test-worker-mount.stdout.log').write_text(stdout)
    (OUT/'test-worker-mount.stderr.log').write_text(stderr)
    logtext = checklog.read_text()
    (OUT/'test-worker-mount.check.log').write_text(logtext)
    issued = (fixture.out/'record.json').exists()
    record = json.loads((fixture.out/'record.json').read_text()) if issued else None
    if issued:
        (OUT/'test-worker-mount.record.json').write_text(json.dumps(record, indent=2)+'\n')
    guard = fixture.guard() if issued else None
    result = {'candidate': subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
              'argv':argv, 'environment_overrides':{'CC':'cc','CXX':'c++'},
              'image':image, 'returncode':process.returncode, 'seconds':time.monotonic()-started,
              'input_before':'BEFORE', 'input_after':'CHANGED', 'check_log':logtext,
              'record_issued':issued,
              'consumer_returncode':guard.returncode if guard else None,
              'mount_captured':record['plan']['worker']['mounts'].get(str(extra)) if record else None,
              'input_hashed_in_dependencies':str(extra) in record['validation_inputs']['inputs']['dependencies'] if record else None,
              'expected':'Input mutation must invalidate evidence or the mount must be rejected as an undeclared dependency.',
              'result':'FAIL' if issued and 'AFTER CHANGED' in logtext else 'PASS'}
    (OUT/'test-worker-mount.result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
finally:
    fixture.doCleanups()
