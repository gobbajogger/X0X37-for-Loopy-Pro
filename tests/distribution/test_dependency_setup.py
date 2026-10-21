#!/usr/bin/env python3
"""Offline tests of the exact dependency block extracted from Install.command.

No pip operation, network request, MIDI API or native installer is executed.
All Python calls within that block are intercepted by a stateful fake executable.
"""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

BASE = Path(__file__).resolve().parent
text = (BASE.parents[1] / 'tools/distribution/Install.command.in').read_text()
start = text.index('if [ ! -x "$KIT_DIR/.venv/bin/python3" ]; then')
end = text.index('\nBACKUP=', start)
block = text[start:end]

STUB = r'''import json, os, pathlib, shutil, sys
p = pathlib.Path(os.environ['STATE'])
s = json.loads(p.read_text())
a = sys.argv[1:]
with open(os.environ['CALLS'], 'a') as f:
    f.write(json.dumps(a) + '\n')
if a[:2] == ['-m', 'venv']:
    dest = pathlib.Path(a[2]) / 'bin/python3'
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(sys.argv[0], dest)
    dest.chmod(0o755)
elif a[:3] == ['-m', 'pip', 'install']:
    name = a[3]
    if name == 'mido':
        assert a == ['-m', 'pip', 'install', 'mido'], a
        if s.get('fail_mido'): sys.exit(17)
        s['mido'] = not s.get('broken_after_install', False)
    else:
        assert a == ['-m', 'pip', 'install', 'python-rtmidi==1.5.8',
            '--config-settings=setup-args=-Djack=false'], a
        if s.get('fail_rtmidi'): sys.exit(19)
        s['rtmidi'] = not s.get('broken_after_install', False)
    p.write_text(json.dumps(s))
    print('MOCK pip install ' + name)
elif a == ['-c', 'import mido']:
    sys.exit(0 if s['mido'] else 1)
elif a == ['-c', 'import rtmidi']:
    sys.exit(0 if s['rtmidi'] else 1)
elif a[:1] == ['-']:
    code = sys.stdin.read()
    assert 'get_compiled_api()' in code and 'open_port(' not in code
    if not s['mido'] or not s['rtmidi']: sys.exit(21)
    if not s.get('coremidi', True):
        print('CoreMIDI is unavailable in python-rtmidi. Nothing was installed on the keyboard.')
        sys.exit(22)
else:
    raise AssertionError(a)
'''

CASES = [
    ('existing_both_work', {'mido': True, 'rtmidi': True}, 0, []),
    ('missing_mido_only', {'mido': False, 'rtmidi': True}, 0, ['mido']),
    ('missing_rtmidi_only', {'mido': True, 'rtmidi': False}, 0, ['python-rtmidi==1.5.8']),
    ('missing_both', {'mido': False, 'rtmidi': False}, 0, ['mido', 'python-rtmidi==1.5.8']),
    ('fresh_venv', {'mido': False, 'rtmidi': False, 'fresh': True}, 0, ['mido', 'python-rtmidi==1.5.8']),
    ('mido_pip_failure', {'mido': False, 'rtmidi': False, 'fail_mido': True}, 17, ['mido']),
    ('rtmidi_pip_failure', {'mido': True, 'rtmidi': False, 'fail_rtmidi': True}, 19, ['python-rtmidi==1.5.8']),
    ('post_install_import_failure', {'mido': True, 'rtmidi': False, 'broken_after_install': True}, 21, ['python-rtmidi==1.5.8']),
    ('no_coremidi', {'mido': True, 'rtmidi': True, 'coremidi': False}, 22, []),
    ('dependency_log_failure', {'mido': True, 'rtmidi': True, 'tee_failure': True}, 23, []),
]

results = []
for name, state, expected_rc, expected_packages in CASES:
    with tempfile.TemporaryDirectory(prefix=name + '-', dir=None) as tmp:
        root = Path(tmp)
        kit = root / 'kit with spaces'
        run = root / 'logs with spaces'
        kit.mkdir()
        run.mkdir()
        fake = root / 'fake-python'
        fake.write_text('#!' + sys.executable + '\n' + STUB)
        fake.chmod(0o755)
        venv_python = kit / '.venv/bin/python3'
        if not state.get('fresh'):
            venv_python.parent.mkdir(parents=True)
            venv_python.write_bytes(fake.read_bytes())
            venv_python.chmod(0o755)
            (kit / '.venv/preserve-me').write_text('existing environment data\n')
            previous_inode = venv_python.stat().st_ino
        state_file = root / 'state.json'
        state_file.write_text(json.dumps(state))
        calls_file = root / 'calls.jsonl'
        env = {**os.environ, 'KIT_DIR': str(kit), 'RUN_DIR': str(run),
               'FAKE_PYTHON': str(fake), 'STATE': str(state_file), 'CALLS': str(calls_file)}
        harness = 'set -euo pipefail\npython3() { "$FAKE_PYTHON" "$@"; }\n'
        if state.get('tee_failure'):
            harness += 'tee() { cat >/dev/null; return 23; }\n'
        harness += block + '\nprintf "NATIVE_INSTALLER_REACHABLE\\n"\n'
        result = subprocess.run(['bash', '-c', harness], env=env, text=True, capture_output=True)
        assert result.returncode == expected_rc, (name, result.returncode, result.stdout, result.stderr)
        calls = [json.loads(line) for line in calls_file.read_text().splitlines()]
        packages = [a[3] for a in calls if a[:3] == ['-m', 'pip', 'install']]
        assert packages == expected_packages, (name, packages)
        assert ('NATIVE_INSTALLER_REACHABLE' in result.stdout) == (expected_rc == 0), name
        creations = [a for a in calls if a[:2] == ['-m', 'venv']]
        assert len(creations) == int(state.get('fresh', False)), name
        if not state.get('fresh'):
            assert venv_python.stat().st_ino == previous_inode, name
            assert (kit / '.venv/preserve-me').read_text() == 'existing environment data\n', name
        if expected_rc == 0:
            assert (run / 'midi-environment.log').exists(), name
        results.append({'case': name, 'status': 'PASS', 'exit': result.returncode,
                        'pip_requirements': packages, 'venv_created': bool(creations)})
        print(name + ': PASS')

report = {'scope': 'Exact launcher dependency block with offline Python/pip stubs; no real dependency install or MIDI access.',
          'status': 'PASS', 'cases': results}
print(json.dumps(report, indent=2))
