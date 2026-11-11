#!/usr/bin/env python3
"""Self-contained negative/positive tests for #115 host evidence identity."""
import json
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

TOOL = Path(__file__).resolve().parents[2] / 'tools/remaining_feature_proof.py'


def invoke(*args, cwd, env=None):
    return subprocess.run(args, cwd=cwd, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, check=False, env=env)


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

# F7: a scoped receipt cannot certify a dirty tracked dependency outside inputs.
# Also pin the launcher that runs in --repo, not a same-named file in our cwd.
with tempfile.TemporaryDirectory() as tmp:
    caller = Path(tmp)
    root = caller / 'repo'
    root.mkdir()
    must_run('git', 'init', '-q', str(root), cwd=root)
    must_run('git', 'config', 'user.name', 'Regression', cwd=root)
    must_run('git', 'config', 'user.email', 'regression@example.invalid', cwd=root)
    (root / 'input.txt').write_text('declared\n')
    (root / 'dependency.txt').write_text('committed\n')
    must_run('git', 'add', 'input.txt', 'dependency.txt', cwd=root)
    must_run('git', 'commit', '-qm', 'source', cwd=root)
    plan = caller / 'plan.json'
    out = caller / 'proof'
    good = {'inputs': ['input.txt'], 'checks': [{'name': 'sample',
            'argv': [sys.executable, '-c', "print('checked')"]}]}
    plan.write_text(json.dumps(good))

    def proof(mode, env=None):
        return invoke(sys.executable, str(TOOL), mode, '--repo', str(root),
                      '--plan', str(plan), '--out', str(out), cwd=caller, env=env)

    def passed(mode, env=None):
        p = proof(mode, env=env)
        assert p.returncode == 0, p.stdout.decode() + p.stderr.decode()

    passed('run')
    (root / 'dependency.txt').write_text('dirty outside input list\n')
    p = proof('verify')
    assert p.returncode != 0 and b'Dirty tracked source' in p.stderr, p.stderr
    p = proof('run')
    assert p.returncode != 0 and b'Dirty tracked source' in p.stderr, p.stderr
    assert not (out / 'receipt.json').exists()
    # A staged edit is still dirty relative to the pinned HEAD.
    must_run('git', 'add', 'dependency.txt', cwd=root)
    p = proof('run')
    assert p.returncode != 0 and b'Dirty tracked source' in p.stderr, p.stderr
    must_run('git', 'reset', '-q', 'HEAD', '--', 'dependency.txt', cwd=root)
    (root / 'dependency.txt').write_text('committed\n')
    # Git can deliberately suppress worktree comparisons. Such an index cannot
    # certify final-source cleanliness, including for an omitted dependency.
    for enable, disable in (('--assume-unchanged', '--no-assume-unchanged'),
                            ('--skip-worktree', '--no-skip-worktree')):
        passed('run')
        must_run('git', 'update-index', enable, 'dependency.txt', cwd=root)
        (root / 'dependency.txt').write_text('hidden tracked edit\n')
        for mode in ('verify', 'run'):
            p = proof(mode)
            assert p.returncode != 0 and b'Hidden tracked source index flags' in p.stderr, p.stderr
        assert not (out / 'receipt.json').exists()
        must_run('git', 'update-index', disable, 'dependency.txt', cwd=root)
        (root / 'dependency.txt').write_text('committed\n')
    # A successful command cannot earn HOST_CHECKED if it dirtied another input.
    changing = {**good, 'checks': [{'name': 'sample', 'argv': [sys.executable, '-c',
                "from pathlib import Path; Path('dependency.txt').write_text('changed')"]}]}
    plan.write_text(json.dumps(changing))
    assert proof('run').returncode != 0
    receipt = json.loads((out / 'receipt.json').read_text())
    assert receipt['checks'][0]['exit_code'] == 0 and receipt['status'] == 'FAIL'
    (root / 'dependency.txt').write_text('committed\n')
    plan.write_text(json.dumps(good))
    passed('run')
    passed('verify')

    # These two launchers have deliberately different bytes. Only --repo's runs.
    target = root / 'helper.sh'
    decoy = caller / 'helper.sh'
    target.write_text('#!/bin/sh\necho actual-repo-launcher\n')
    decoy.write_text('#!/bin/sh\necho wrong-caller-launcher\n')
    target.chmod(0o755)
    decoy.chmod(0o755)
    relative = {**good, 'checks': [{'name': 'sample', 'argv': ['./helper.sh']}]}
    plan.write_text(json.dumps(relative))
    passed('run')
    receipt = json.loads((out / 'receipt.json').read_text())
    pin = receipt['toolchain']['tools']['check:./helper.sh']
    assert pin['path'] == str(target.resolve())
    assert pin['sha256'] == hashlib.sha256(target.read_bytes()).hexdigest()
    assert (out / 'sample.log').read_bytes() == b'actual-repo-launcher\n'
    decoy.write_text('#!/bin/sh\necho changed-decoy\n')
    passed('verify')
    target.write_text('#!/bin/sh\necho changed-target\n')
    p = proof('verify')
    assert p.returncode != 0 and b'Host compiler/runtime/environment changed' in p.stderr, p.stderr
    # A launcher that changes itself while succeeding invalidates this run too.
    target.write_text('#!/bin/sh\nprintf "changed-launcher\\n" > "$0"\n')
    assert proof('run').returncode != 0
    receipt = json.loads((out / 'receipt.json').read_text())
    assert receipt['checks'][0]['exit_code'] == 0 and receipt['status'] == 'FAIL'
    # A caller-only executable is absent in --repo and must be recorded as 127.
    target.unlink()
    assert proof('run').returncode != 0
    receipt = json.loads((out / 'receipt.json').read_text())
    assert receipt['status'] == 'FAIL'
    assert receipt['checks'][0]['exit_code'] == 127
    assert receipt['toolchain']['tools']['check:./helper.sh']['path'] is None
    # Bare argv[0] must also resolve relative PATH entries from --repo, not cwd.
    (root / 'bin').mkdir()
    (caller / 'bin').mkdir()
    target = root / 'bin' / 'proof-helper.sh'
    decoy = caller / 'bin' / 'proof-helper.sh'
    target.write_text('#!/bin/sh\necho actual-path-launcher\n')
    decoy.write_text('#!/bin/sh\necho wrong-path-launcher\n')
    target.chmod(0o755)
    decoy.chmod(0o755)
    path_env = {**os.environ, 'PATH': 'bin' + os.pathsep + os.environ.get('PATH', os.defpath)}
    plan.write_text(json.dumps({**good, 'checks': [
        {'name': 'sample', 'argv': ['proof-helper.sh']}]}))
    passed('run', env=path_env)
    receipt = json.loads((out / 'receipt.json').read_text())
    pin = receipt['toolchain']['tools']['check:proof-helper.sh']
    assert pin['path'] == str(target.resolve())
    assert pin['sha256'] == hashlib.sha256(target.read_bytes()).hexdigest()
    assert (out / 'sample.log').read_bytes() == b'actual-path-launcher\n'
    decoy.write_text('#!/bin/sh\necho changed-path-decoy\n')
    passed('verify', env=path_env)
    target.write_text('#!/bin/sh\necho changed-path-target\n')
    p = proof('verify', env=path_env)
    assert p.returncode != 0 and b'Host compiler/runtime/environment changed' in p.stderr, p.stderr
print('PASS F7 feature proof: dirty/staged/command-mutated dependencies and repo-relative launcher identity')

