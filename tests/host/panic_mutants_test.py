#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Mutation runner rejects infrastructure failures and never edits caller sources."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
FILES = ('firmware/src/app/engine.c', 'host/x0x_host.c')
CC = shutil.which('cc')
assert CC, 'cc is required'
with tempfile.TemporaryDirectory(prefix='panic-runner-test-') as directory:
    tmp = Path(directory)
    fixture = tmp / 'source'
    for name in (*FILES, 'tests/host/panic_mutants.sh',
                 'tests/host/live_key_epoch_test.py', 'tests/host/midi_epoch_step_test.py'):
        dst = fixture / name
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / name, dst)
    hashes = {str(fixture / f): hashlib.sha256((fixture / f).read_bytes()).hexdigest() for f in FILES}
    bin_dir = tmp / 'bin'
    bin_dir.mkdir()
    wrapper = bin_dir / 'cc'
    wrapper.write_text('#!/usr/bin/env python3\n' +
        'import hashlib, os, pathlib, subprocess, sys\n' +
        f'expected = {hashes!r}\n' +
        'for name, digest in expected.items():\n' +
        ' if hashlib.sha256(pathlib.Path(name).read_bytes()).hexdigest() != digest: sys.exit(74)\n' +
        'p=pathlib.Path(os.environ["COUNT_FILE"]); n=int(p.read_text())+1; p.write_text(str(n))\n' +
        'if n == int(os.environ["FAIL_AT"]): sys.exit(73)\n' +
        f'sys.exit(subprocess.call([{CC!r}, *sys.argv[1:]]))\n')
    wrapper.chmod(0o755)
    for fail_at in (0, 1, 3, 4):
        counter = tmp / 'count'
        counter.write_text('0')
        env = dict(os.environ, PATH=str(bin_dir)+os.pathsep+os.environ['PATH'],
                   COUNT_FILE=str(counter), FAIL_AT=str(fail_at))
        run = subprocess.run(['sh', 'tests/host/panic_mutants.sh'], cwd=fixture,
                             env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if fail_at == 0:
            assert run.returncode == 0 and 'panic mutants: 2 killed' in run.stdout, run.stdout
            assert counter.read_text() == '4'
        else:
            assert run.returncode != 0 and 'panic mutants: 2 killed' not in run.stdout, run.stdout
            assert ('BASELINE FAILED:' if fail_at == 1 else 'MUTANT ERROR:') in run.stdout, run.stdout
        assert all(hashlib.sha256(Path(f).read_bytes()).hexdigest() == h for f,h in hashes.items())
    # A missing test harness must also fail closed, before mutation.
    (fixture / 'tests/host/live_key_epoch_test.py').unlink()
    run = subprocess.run(['sh', 'tests/host/panic_mutants.sh'], cwd=fixture,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    assert run.returncode != 0 and 'MUTANT KILLED:' not in run.stdout, run.stdout
print('panic mutant runner: 5 cases passed')
