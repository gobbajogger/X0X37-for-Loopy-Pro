"""Resolve the different-handle event-order gap in the saved stop cases.
The mailbox, peer, time and scheduling are simulated. No device access occurs.
"""
from pathlib import Path
import hashlib
import json
import subprocess
import tempfile

E = Path(__file__).resolve().parent
R = E.parents[2]
records = []
inputs = {}
for name in ['firmware/src/ble.c', 'firmware/src/ble/ble_midi_central.c',
             'firmware/src/ble/ble_midi_central.h', 'tests/host/ble_midi_stop_test.c',
             str(E.relative_to(R) / 'different-handle-probe.c'),
             str(Path(__file__).relative_to(R))]:
    inputs[name] = hashlib.sha256((R / name).read_bytes()).hexdigest()
(E / 'probe-inputs.json').write_text(json.dumps(inputs, indent=2) + '\n')

def run(name, argv):
    p = subprocess.run([str(x) for x in argv], cwd=R, capture_output=True, text=True)
    (E / (name + '.log')).write_text(p.stdout + p.stderr)
    records.append(dict(name=name, argv=[str(x) for x in argv], cwd=str(R),
                        returncode=p.returncode, expected=0, log=name + '.log'))
    (E / 'probe-commands.json').write_text(json.dumps(records, indent=2) + '\n')
    assert p.returncode == 0, (name, p.stdout, p.stderr)

source = (R / 'firmware/src/ble.c').read_text()
start = source.index('static void ble_central_service(void)\n{')
end = source.index('\n}\n', start) + 3
service = source[start:end]
(E / 'service-sha256.txt').write_text(hashlib.sha256(service.encode()).hexdigest() + '\n')
with tempfile.TemporaryDirectory(prefix='different-central-handle-') as directory:
    scratch = Path(directory)
    (scratch / 'ble_central_service.inc').write_text(service)
    binary = scratch / 'probe'
    run('probe-compile', ['cc', '-std=c99', '-g', '-O1', '-Wall', '-Wextra', '-Werror',
                         '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                         '-I' + str(scratch), '-I' + str(R / 'firmware/src/ble'),
                         E / 'different-handle-probe.c',
                         R / 'firmware/src/ble/ble_midi_central.c', '-o', binary])
    run('probe', [binary])
print('Different-handle event-order probe: PASS')
