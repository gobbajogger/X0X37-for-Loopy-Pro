#!/usr/bin/env python3
"""Self-contained negative/positive tests for #115 host evidence identity."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

TOOL = Path(__file__).resolve().parents[2] / 'tools/remaining_feature_proof.py'


def invoke(*args, cwd):
    return subprocess.run(args, cwd=cwd, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, check=False)


def must_run(*args, cwd):
    p = invoke(*args, cwd=cwd)
    assert p.returncode == 0, p.stdout.decode() + p.stderr.decode()
    return p


with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp) / 'src'
    root.mkdir()
    must_run('git', 'init', '-q', str(root), cwd=root)
    must_run('git', 'config', 'user.name', 'Regression', cwd=root)
    must_run('git', 'config', 'user.email', 'regression@example.invalid', cwd=root)
    (root / 'fixture.txt').write_text('original source\n')
    must_run('git', 'add', 'fixture.txt', cwd=root)
    must_run('git', 'commit', '-qm', 'source', cwd=root)
    ok = {'inputs': ['fixture.txt'], 'checks': [{'name': 'sample',
            'argv': [sys.executable, '-c', "print('PASS unit test')"]}]}
    plan = Path(tmp) / 'plan.json'
    plan.write_text(json.dumps(ok))
    out = Path(tmp) / 'evidence'
    must_run(sys.executable, str(TOOL), 'run', '--plan', str(plan), '--repo', str(root),
             '--out', str(out), cwd=root)
    must_run(sys.executable, str(TOOL), 'verify', '--repo', str(root), '--out', str(out), cwd=root)
    first = json.loads((out / 'receipt.json').read_text())
    assert first['status'] == 'HOST_CHECKED'
    assert 'check:' + sys.executable in first['toolchain']['tools']
    (out / 'sample.log').write_bytes(b'forged PASS\n')
    assert invoke(sys.executable, str(TOOL), 'verify', '--repo', str(root), '--out', str(out), cwd=root).returncode
    must_run(sys.executable, str(TOOL), 'run', '--plan', str(plan), '--repo', str(root),
             '--out', str(out), cwd=root)
    (root / 'fixture.txt').write_text('modified source\n')
    assert invoke(sys.executable, str(TOOL), 'verify', '--repo', str(root), '--out', str(out), cwd=root).returncode
    assert invoke(sys.executable, str(TOOL), 'run', '--plan', str(plan), '--repo', str(root),
                  '--out', str(out), cwd=root).returncode
    (root / 'fixture.txt').write_text('original source\n')
    # A failed rerun invalidates earlier passing evidence, even if the source
    # is restored later. Only a successful new run can establish a PASS.
    q = invoke(sys.executable, str(TOOL), 'verify', '--repo', str(root), '--out', str(out), cwd=root)
    assert q.returncode != 0
    must_run(sys.executable, str(TOOL), 'run', '--plan', str(plan), '--repo', str(root), '--out', str(out), cwd=root)
    must_run(sys.executable, str(TOOL), 'verify', '--repo', str(root), '--out', str(out), cwd=root)
    # A working external launcher whose bytes change must invalidate proof,
    # even when the committed input files and command arguments stay fixed.
    helper = Path(tmp) / 'host-helper.sh'
    helper.write_text('#!/bin/sh\necho original\n')
    helper.chmod(0o755)
    custom = {**ok, 'checks': [{'name': 'sample', 'argv': [str(helper)]}]}
    plan.write_text(json.dumps(custom))
    must_run(sys.executable, str(TOOL), 'run', '--plan', str(plan), '--repo', str(root), '--out', str(out), cwd=root)
    must_run(sys.executable, str(TOOL), 'verify', '--repo', str(root), '--out', str(out), cwd=root)
    helper.write_text('#!/bin/sh\necho changed\n')
    assert invoke(sys.executable, str(TOOL), 'verify', '--repo', str(root), '--out', str(out), cwd=root).returncode
    helper.unlink()
    plan.write_text(json.dumps(ok))
    # A command that cannot be launched is recorded as FAIL (127) with no
    # reusable passing receipt from this or any earlier run in that folder.
    missing = {**ok, 'checks': [{'name': 'sample', 'argv': ['missing-x0x37-executable-that-does-not-exist']}]}
    plan.write_text(json.dumps(missing))
    assert invoke(sys.executable, str(TOOL), 'run', '--plan', str(plan), '--repo', str(root),
                  '--out', str(out), cwd=root).returncode
    failed = json.loads((out / 'receipt.json').read_text())
    assert failed['status'] == 'FAIL' and failed['checks'][0]['exit_code'] == 127
    assert b'FAILED TO LAUNCH' in (out / 'sample.log').read_bytes()
    assert invoke(sys.executable, str(TOOL), 'verify', '--repo', str(root), '--out', str(out), cwd=root).returncode
    plan.write_text(json.dumps(ok))
    must_run(sys.executable, str(TOOL), 'run', '--plan', str(plan), '--repo', str(root), '--out', str(out), cwd=root)
    must_run(sys.executable, str(TOOL), 'verify', '--repo', str(root), '--out', str(out), cwd=root)
    # Invalid plan cannot preserve a previously passed receipt either.
    plan.write_text('{ not valid JSON')
    assert invoke(sys.executable, str(TOOL), 'run', '--plan', str(plan), '--repo', str(root),
                  '--out', str(out), cwd=root).returncode
    assert not (out / 'receipt.json').exists()
    plan.write_text(json.dumps(ok))
    must_run(sys.executable, str(TOOL), 'run', '--plan', str(plan), '--repo', str(root), '--out', str(out), cwd=root)
    bad = {**ok, 'checks': [{'name': 'sample', 'argv': [sys.executable, '-c', 'raise SystemExit(3)']}]}
    plan.write_text(json.dumps(bad))
    assert invoke(sys.executable, str(TOOL), 'run', '--plan', str(plan), '--repo', str(root),
                  '--out', str(out), cwd=root).returncode
    assert json.loads((out / 'receipt.json').read_text())['status'] == 'FAIL'
    assert invoke(sys.executable, str(TOOL), 'verify', '--repo', str(root), '--out', str(out), cwd=root).returncode
    plan.write_text(json.dumps({**ok, 'inputs': ['../escape.txt']}))
    assert invoke(sys.executable, str(TOOL), 'run', '--plan', str(plan), '--repo', str(root),
                  '--out', str(out), cwd=root).returncode
print('PASS remaining_feature_proof: source/log/runner mutations, failed/absent command, stale receipt and invalid plan rejected')
