#!/usr/bin/env python3
"""Supplemental scratch controls; no fixture/production edits."""
import hashlib
import json
import pathlib
import subprocess

BASE = pathlib.Path(__file__).resolve().parent
original = (BASE / 'capture/plain/smc_boot.c').read_text()
out = BASE / 'scratch-controls'
out.mkdir(exist_ok=True)
records = []
def run(label, argv):
    proc = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (out / (label + '.stdout')).write_bytes(proc.stdout)
    (out / (label + '.stderr')).write_bytes(proc.stderr)
    record = {'label': label, 'argv': argv, 'exit': proc.returncode, 'stdout_sha256': hashlib.sha256(proc.stdout).hexdigest(), 'stderr_sha256': hashlib.sha256(proc.stderr).hexdigest()}
    records.append(record)
    return proc

# Explicit execution receipts for byte-identical captured fixture executables.
for mode in ['plain', 'ubsan']:
    proc = run('captured-' + mode, [str(BASE / 'capture' / mode / ('smc_' + mode))])
    assert proc.returncode == 0 and b'checks=301\n' in proc.stdout and proc.stderr == b''

# Insert diagnostics before the existing assertion; no existing expectations change.
anchor = '    CHECK(project_incompatible == damaged);'
assert original.count(anchor) == 1
matrix = original.replace(anchor, '    printf("MATRIX role=%u sound=%d stream=%d validation=%d expected=%u actual=%u\\n", stored, sound_rc, stream_rc, validation_rc, damaged, (unsigned)project_incompatible);\n' + anchor)
(out / 'matrix.c').write_text(matrix)
flags = ['-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror']
proc = run('matrix-compile', ['/usr/bin/cc', *flags, str(out / 'matrix.c'), '-o', str(out / 'matrix')])
assert proc.returncode == 0
proc = run('matrix-run', [str(out / 'matrix')])
assert proc.returncode == 0 and proc.stderr == b''
rows = [row for row in proc.stdout.decode().splitlines() if row.startswith('MATRIX ')]
assert len(rows) == 20

# Focused deletion control changes only the callee side effect, in scratch C.
assignment = '    if (rc != -9 && present && !transient) project_incompatible = 2;'
assert original.count(assignment) == 1
mutated = original.replace(assignment, '    /* SCRATCH CONTROL ONLY: callee assignment removed. */')
(out / 'no-callee-write.c').write_text(mutated)
proc = run('no-callee-write-compile', ['/usr/bin/cc', *flags, str(out / 'no-callee-write.c'), '-o', str(out / 'no-callee-write')])
assert proc.returncode == 0
proc = run('no-callee-write-run', [str(out / 'no-callee-write')])
assert proc.returncode == 1 and b'project_incompatible == damaged' in proc.stderr

inputs = json.loads((BASE / 'inputs-before.json').read_text())
after = []
for record in inputs:
    data = (BASE / 'closure' / record['path']).read_bytes()
    row = {'path': record['path'], 'size': len(data), 'git_blob': hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest(), 'sha256': hashlib.sha256(data).hexdigest()}
    assert all(row[k] == record[k] for k in row)
    after.append(row)
(BASE / 'inputs-after.json').write_text(json.dumps(after, indent=2) + '\n')
(out / 'execution-records.json').write_text(json.dumps(records, indent=2) + '\n')
print(json.dumps({'supplementary': 'PASS', 'matrix_rows': rows, 'focused_control': 'KILLED exit 1; existing expectations unchanged', 'source_readback': '4/4 unchanged', 'commands': records}, indent=2))
