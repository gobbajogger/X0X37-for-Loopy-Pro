#!/usr/bin/env python3
"""Scratch-only probe: successful driver descendant followed by later failure."""
import importlib.util
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location('fixture', ROOT / 'tests/host/test_ctl_compile.py')
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
f = m.ControllerCompile('test_compile_failure_and_stale_outputs')
f.setUp()
pids = []
result = {}
def process(pid):
    try:
        group = os.getpgid(pid)
    except ProcessLookupError:
        return {'pid': pid, 'exists': False, 'active': False}
    if sys.platform.startswith('linux'):
        stat = Path('/proc') / str(pid) / 'stat'
        state = stat.read_text().split()[2] if stat.exists() else ''
    else:
        state = subprocess.run(['ps', '-o', 'stat=', '-p', str(pid)], capture_output=True, text=True).stdout.strip()
    return {'pid': pid, 'exists': bool(state), 'active': bool(state) and not state.startswith('Z'),
            'state': state, 'process_group': group}
try:
    f.env['SPAWN_CHILD'] = 'src/a.c'
    f.env['FAIL'] = 'src/b.c'
    p = f.run_build(2)
    pids = [int(s) for s in f.children.read_text().splitlines()]
    result = {'platform': sys.platform, 'returncode': p.returncode, 'stdout': p.stdout, 'stderr': p.stderr,
              'successful_driver_children': [process(pid) for pid in pids],
              'events': f.events_list(), 'scratch_removed': not list(f.root.glob('ctlbuild.*')),
              'expected': 'Every owned compiler process group is stopped before scratch cleanup.'}
finally:
    for pid in pids:
        try:
            os.kill(pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    deadline = time.monotonic() + 2
    while any(process(pid)['active'] for pid in pids) and time.monotonic() < deadline:
        time.sleep(.01)
    result['probe_owned_child_cleanup'] = [process(pid) for pid in pids]
    f.doCleanups()
print(json.dumps(result, indent=2))
assert result['returncode'] != 0
assert any(p['active'] for p in result['successful_driver_children'])
assert not any(p['active'] for p in result['probe_owned_child_cleanup'])
