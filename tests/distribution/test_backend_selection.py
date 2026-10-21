#!/usr/bin/env python3
"""Offline launcher CoreMIDI-selection tests; no hardware or dependency install."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

BASE = Path(__file__).resolve().parent
launcher = (BASE.parents[1] / 'tools/distribution/Install.command.in').read_text()
start = launcher.index('# Use CoreMIDI for this launcher and its child processes')
end = launcher.index('\nBACKUP=', start)
block = launcher[start:end]
embedded = block.split("<<'MIDI_PYTHON' 2>&1 | tee \"$RUN_DIR/midi-environment.log\"\n", 1)[1].split('\nMIDI_PYTHON', 1)[0]
compile(embedded, 'launcher CoreMIDI preflight', 'exec')

RTMIDI = '''import json, os
API_UNSPECIFIED = 0
API_MACOSX_CORE = 1
API_UNIX_JACK = 3
state = json.loads(os.environ['STATE'])
def record(event):
    with open(os.environ['CALLS'], 'a') as f: f.write(json.dumps(event) + '\\n')
def get_compiled_api(): return [3] if state.get('no_core') else [1, 3]
class Client:
    def __init__(self, direction, rtapi):
        assert rtapi == API_MACOSX_CORE, rtapi
        self.direction = direction
        record(['create', direction, rtapi])
    def get_current_api(self): return 3 if state.get('fallback') else 1
    def get_ports(self):
        if state.get('enumeration_error'): raise RuntimeError('injected CoreMIDI enumeration error')
        return state.get(self.direction, ['SMK-37 Elite'])
    def delete(self): record(['delete', self.direction])
    def open_port(self, *a, **k): raise AssertionError('MIDI port open is forbidden in preflight')
    def open_virtual_port(self, *a, **k): raise AssertionError('Virtual port open is forbidden')
    def send_message(self, *a, **k): raise AssertionError('MIDI send is forbidden')
def MidiIn(rtapi): return Client('input', rtapi)
def MidiOut(rtapi): return Client('output', rtapi)
'''
MIDO = '''import json, os, types
state = json.loads(os.environ['STATE'])
name, api = os.environ['MIDO_BACKEND'].split('/')
backend = types.SimpleNamespace(name=name, api=('UNIX_JACK' if state.get('wrong_mido') else api))
def get_input_names():
    assert backend.name == 'mido.backends.rtmidi' and backend.api == 'MACOSX_CORE'
    return list(dict.fromkeys(state.get('input', ['SMK-37 Elite'])))
def get_output_names():
    assert backend.name == 'mido.backends.rtmidi' and backend.api == 'MACOSX_CORE'
    return list(dict.fromkeys(state.get('output', ['SMK-37 Elite'])))
'''

cases = [
    ('unset_backend', {}, None, 0),
    ('inherited_jack', {}, 'mido.backends.rtmidi/UNIX_JACK', 0),
    ('inherited_other_backend', {}, 'mido.backends.portmidi', 0),
    ('inherited_coremidi', {}, 'mido.backends.rtmidi/MACOSX_CORE', 0),
    ('inherited_rtmidi_api', {'rtmidi_env': 'UNIX_JACK'}, None, 0),
    ('empty_coremidi_endpoints', {'input': [], 'output': []}, None, 0),
    ('duplicate_raw_port_names', {'input': ['SMK-37 Elite', 'SMK-37 Elite']}, None, 0),
    ('missing_compiled_coremidi', {'no_core': True}, None, 2),
    ('unexpected_actual_api', {'fallback': True}, None, 2),
    ('enumeration_failure', {'enumeration_error': True}, None, 2),
    ('mido_selection_mismatch', {'wrong_mido': True}, None, 2),
    ('log_write_failure', {'tee_failure': True}, None, 24),
]
results = []
for name, state, inherited, rc in cases:
    with tempfile.TemporaryDirectory(prefix='midi-test-', dir=None) as td:
        p = Path(td)
        (p / 'rtmidi.py').write_text(RTMIDI)
        (p / 'mido.py').write_text(MIDO)
        env = {**os.environ, 'VENV_PYTHON': sys.executable, 'RUN_DIR': str(p),
               'PYTHONPATH': str(p), 'PYTHONDONTWRITEBYTECODE': '1',
               'STATE': json.dumps(state), 'CALLS': str(p / 'calls.jsonl')}
        env.pop('MIDO_BACKEND', None)
        env.pop('RTMIDI_API', None)
        if inherited is not None: env['MIDO_BACKEND'] = inherited
        if state.get('rtmidi_env'): env['RTMIDI_API'] = state['rtmidi_env']
        script = 'set -euo pipefail\n'
        if state.get('tee_failure'):
            script += 'tee() { cat >/dev/null; return 24; }\n'
        script += block + '\necho NATIVE_INSTALLER_REACHABLE\n'
        script += '''"$VENV_PYTHON" -c 'import os; print("Child backend:", os.environ["MIDO_BACKEND"])'\n'''
        result = subprocess.run(['bash', '-c', script], env=env, text=True, capture_output=True)
        assert result.returncode == rc, (name, result.returncode, result.stdout, result.stderr)
        assert ('NATIVE_INSTALLER_REACHABLE' in result.stdout) == (rc == 0), name
        if not state.get('tee_failure'):
            assert (p / 'midi-environment.log').read_text() == result.stdout.split('NATIVE_INSTALLER_REACHABLE')[0], name
        if rc == 0:
            assert 'Child backend: mido.backends.rtmidi/MACOSX_CORE' in result.stdout, name
            assert "Inherited MIDO_BACKEND: " + repr(inherited or '') in result.stdout, name
        if name == 'empty_coremidi_endpoints':
            assert 'CoreMIDI input ports: 0' in result.stdout and 'Mido input names: []' in result.stdout
        if name == 'duplicate_raw_port_names':
            assert "[0] 'SMK-37 Elite'" in result.stdout and "[1] 'SMK-37 Elite'" in result.stdout
        events = [json.loads(s) for s in (p / 'calls.jsonl').read_text().splitlines()] if (p / 'calls.jsonl').exists() else []
        created = [e[1] for e in events if e[0] == 'create']
        deleted = [e[1] for e in events if e[0] == 'delete']
        assert created == deleted, (name, events)
        assert all(e[0] in ('create', 'delete') for e in events), name
        # The child export must not rewrite the caller's environment mapping.
        assert env.get('MIDO_BACKEND') == inherited, name
        results.append({'case': name, 'result': 'PASS', 'exit': rc, 'clients_deleted': deleted})
        print(name + ': PASS')

report = {'status': 'PASS', 'scope': 'Exact launcher block, mocked Mido/RtMidi, no actual MIDI/device access.',
          'cases': results, 'limitations': ['No actual CoreMIDI runtime available in cloud.',
          'This removes a possible backend mismatch, not proof of the cause of the user\'s empty input list.']}
print(json.dumps(report, indent=2))
