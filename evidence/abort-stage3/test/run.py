"""Run independent stage3 host checks and both saved negative controls.
All transport inputs are simulated. This script has no device access.
"""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import tempfile

R = Path(__file__).resolve().parents[3]
E = Path(__file__).resolve().parent
BASE = '28d0fc230f60bc823c1e5e73d3aed61e68a8e32f'
CANDIDATE = '05eff604cb3f136db71727a2aeeea785cab73b99'
records = []

def run(name, argv, cwd=R, expected=0, text_required=None):
    p = subprocess.run([str(x) for x in argv], cwd=cwd, capture_output=True, text=True)
    output = p.stdout + p.stderr
    (E / (name + '.log')).write_text(output)
    ok = p.returncode == expected and (text_required is None or text_required in output)
    records.append(dict(name=name, argv=[str(x) for x in argv], cwd=str(cwd),
                        returncode=p.returncode, expected=expected,
                        text_required=text_required, result='PASS' if ok else 'FAIL',
                        log=name + '.log'))
    (E / 'commands.json').write_text(json.dumps(records, indent=2) + '\n')
    assert ok, (name, p.returncode, output)
    print(name + ': PASS', flush=True)

assert subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=R, text=True).strip() == CANDIDATE
run('public-receive', ['sh', 'tests/host/ble_midi_receive_test.sh'])
run('diag', ['python3', '-B', 'tests/ble_central_diag_test.py'])
flags = ['cc', '-std=c99', '-g', '-O1', '-Wall', '-Wextra', '-Werror',
         '-fsanitize=address,undefined', '-fno-sanitize-recover=all']
inputs = {}
with tempfile.TemporaryDirectory(prefix='abort-stage3-test-') as scratch:
    for label, revision in [('broken', BASE), ('candidate', CANDIDATE)]:
        folder = Path(scratch) / label
        names = ['firmware/src/ble/ble_midi_central.c', 'firmware/src/ble/ble_midi_central.h',
                 'tests/host/ble_midi_signal_test.c', 'tests/host/ble_midi_abort_test.c',
                 'evidence/session-stage3/investigation-abort-probe.c',
                 'evidence/session-stage3/test-busy-disconnect.c']
        inputs[label] = {}
        for name in names:
            target = folder / name
            target.parent.mkdir(parents=True, exist_ok=True)
            if name.startswith('firmware/'):
                data = subprocess.check_output(['git', 'show', revision + ':' + name], cwd=R)
            else:
                data = (R / name).read_bytes()
            target.write_bytes(data)
            inputs[label][name] = hashlib.sha256(data).hexdigest()
        probes = [('public', 'tests/host/ble_midi_abort_test.c', ['overflow', 'timeout']),
                  ('saved-investigation', 'evidence/session-stage3/investigation-abort-probe.c', ['overflow', 'timeout']),
                  ('saved-test', 'evidence/session-stage3/test-busy-disconnect.c', [''])]
        for probe, source, modes in probes:
            binary = folder / probe
            run(label + '-' + probe + '-compile', flags + [source, 'firmware/src/ble/ble_midi_central.c', '-o', binary], folder)
            for mode in modes:
                expected = 0 if label == 'candidate' else 1 if probe == 'saved-test' else -6
                required = None if label == 'candidate' else ('FAIL: mailbox accepted later polls' if probe == 'saved-test' else 'Assertion failed')
                run(label + '-' + probe + '-' + (mode or 'both'),
                    [binary] + ([mode] if mode else []), folder, expected, required)

files = ['firmware/src/ble/ble_midi_central.c', 'firmware/src/ble/ble_midi_central.h',
         'firmware/src/ble.c', 'tests/host/ble_midi_central_test.c',
         'tests/host/ble_midi_receive_test.c', 'tests/host/ble_midi_signal_test.c',
         'tests/host/ble_midi_abort_test.c', 'tests/host/ble_midi_receive_test.sh',
         'tests/ble_central_diag_test.py', 'tools/ble_autotest.py']
identity = dict(candidate=CANDIDATE, tree=subprocess.check_output(['git', 'rev-parse', 'HEAD^{tree}'], cwd=R, text=True).strip(),
                files={f:hashlib.sha256((R/f).read_bytes()).hexdigest() for f in files},
                probe_inputs=inputs, compiler=subprocess.check_output(['cc', '--version'], text=True))
(E/'identity.json').write_text(json.dumps(identity, indent=2)+'\n')
run('diff-check', ['git', 'diff', '--check'])
print('Independent test scope: PASS')
