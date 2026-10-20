#!/usr/bin/env python3
"""Verify unchanged compiler/link flags, unset jobs, and upper bound with fake tools."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.dont_write_bytecode = True
current = (ROOT / 'ble/ctl/build_in_container.sh').read_text()
old = subprocess.check_output(['git', 'show', '07fa87dc950908d4d720bde9c06c2478cc212b19:ble/ctl/build_in_container.sh'], cwd=ROOT, text=True)
cf = lambda s: s.split('CF=', 1)[1].split('# Work in /tmp:', 1)[0]
link = lambda s: s.split('L=$B/lib\n', 1)[1]
assert cf(old) == cf(current)
assert link(old) == link(current)
spec = importlib.util.spec_from_file_location('fixture', ROOT / 'tests/host/test_ctl_compile.py')
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
rows = []
for jobs in (None, '64'):
    f = m.ControllerCompile('test_parallel_bound_serial_and_ordered_diagnostics_link')
    f.setUp()
    try:
        f.env.pop('X0X_JOBS', None)
        f.env.pop('BARRIER', None)
        f.env.pop('SPAWN_CHILD', None)
        if jobs is not None:
            f.env['X0X_JOBS'] = jobs
        p = subprocess.run(['sh', str(ROOT / 'ble/ctl/build_in_container.sh')], env=f.env, text=True, capture_output=True, timeout=10)
        assert p.returncode == 0, p.stdout + p.stderr
        args = json.loads((f.out / 'link.json').read_text())
        objects = [v for v in args if v.startswith('obj/') and v.endswith('.o')]
        assert objects == ['obj/' + Path(u).name + '.o' for u in f.units]
        f.assert_clean()
        rows.append({'X0X_JOBS': jobs, 'returncode': p.returncode, 'units': f.units, 'objects': objects, 'scratch_removed': True})
    finally:
        f.doCleanups()
print(json.dumps({'compiler_flags_byte_equal': cf(old) == cf(current), 'link_and_post_link_block_byte_equal': link(old) == link(current), 'fake_tool_results': rows}, indent=2))
