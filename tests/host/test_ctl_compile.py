#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Exercise Linux controller orchestration with fake compilers and private outputs."""
import json
import os
from pathlib import Path
import signal
import shutil
import subprocess
import sys
import tempfile
import time
import unittest

TOP = Path(__file__).resolve().parents[2]
COMPILER = r'''
import fcntl, json, os
from pathlib import Path
import subprocess, sys, time
source = sys.argv[sys.argv.index('-c') + 1]
out = Path(sys.argv[sys.argv.index('-o') + 1])
assert '-Werror' in sys.argv and '-flto' in sys.argv and '-Isrc' in sys.argv
assert ('-w' in sys.argv) == source.startswith('gen/')
def event(kind):
    with open(os.environ['EVENTS'], 'a') as f:
        fcntl.flock(f, fcntl.LOCK_EX)
        f.write(json.dumps([kind, source, os.getpid(), str(Path.cwd())]) + '\n')
        f.flush()
event('start')
if os.environ.get('BARRIER'):
    units = ['src/a.c', 'src/b.c', 'src/c.S', 'gen/d.c']
    jobs = int(os.environ['X0X_JOBS'])
    required = (units.index(source) // jobs + 1) * jobs
    deadline = time.monotonic() + 5
    while sum(json.loads(l)[0] == 'start' for l in Path(os.environ['EVENTS']).read_text().splitlines()) < required:
        assert time.monotonic() < deadline, 'compile batch did not start'
        time.sleep(.01)
if os.environ.get('HANG') or os.environ.get('SPAWN_CHILD') == source:
    child = subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(60)'])
    with open(os.environ['CHILDREN'], 'a') as f: f.write(str(child.pid) + '\n')
    if os.environ.get('HANG'): time.sleep(60)
else:
    time.sleep(.08 if source == 'src/a.c' else .20)
print('stdout ' + source)
print('stderr ' + source, file=sys.stderr)
if os.environ.get('FAIL') == source:
    event('fail')
    sys.exit(7)
out.write_text(source)
event('end')
'''
LINKER = r'''
import json, os, sys
from pathlib import Path
args = sys.argv[1:]
Path(os.environ['OUT'], 'link.json').write_text(json.dumps(args))
Path(args[args.index('-Map') + 1]).write_text('map')
Path(args[args.index('-o') + 1]).write_text('elf')
'''


class ControllerCompile(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.repo = self.root / 'repo'
        for d in ('ble/ctl/src', 'ble/ctl/gen', 'ble/ctl/lib', 'firmware/hal'):
            (self.repo / d).mkdir(parents=True)
        self.units = ['src/a.c', 'src/b.c', 'src/c.S', 'gen/d.c']
        for name in self.units:
            (self.repo / 'ble/ctl' / name).write_text('source')
        (self.repo / 'ble/ctl/ctl.ld').write_text('linker')
        (self.repo / 'firmware/hal/x0x_ble_abi.h').write_text('ABI')
        self.tc = self.root / 'tc/pi32v2/bin'
        self.tc.mkdir(parents=True)
        for name, src in [('clang', COMPILER), ('ld', LINKER), ('objdump', 'print("section")')]:
            f = self.tc / name
            f.write_text('#!' + sys.executable + '\n' + src)
            f.chmod(0o755)
        self.out = self.root / 'out'
        self.out.mkdir()
        self.events = self.root / 'events'
        self.children = self.root / 'children'
        self.env = {**os.environ, 'TC': str(self.root / 'tc'), 'REPO': str(self.repo),
                    'OUT': str(self.out), 'TMPDIR': str(self.root),
                    'EVENTS': str(self.events), 'CHILDREN': str(self.children)}
        if not shutil.which('setsid'):
            launcher = self.root / 'bin/setsid'
            launcher.parent.mkdir()
            launcher.write_text('#!' + sys.executable + '\nimport os, sys\nos.setsid()\nos.execv(sys.argv[1], sys.argv[1:])\n')
            launcher.chmod(0o755)
            self.env['PATH'] = str(launcher.parent) + ':' + self.env['PATH']
        self.env.pop('FAIL', None)
        self.env.pop('HANG', None)

    def run_build(self, jobs):
        self.env['X0X_JOBS'] = str(jobs)
        return subprocess.run(['sh', str(TOP / 'ble/ctl/build_in_container.sh')],
                              env=self.env, text=True, capture_output=True, timeout=10)

    def assert_clean(self):
        self.assertEqual(list(self.root.glob('ctlbuild.*')), [])

    def events_list(self):
        return [json.loads(l) for l in self.events.read_text().splitlines()]

    def test_parallel_bound_serial_and_ordered_diagnostics_link(self):
        for jobs in (1, 2, 4):
            with self.subTest(jobs=jobs):
                self.events.unlink(missing_ok=True)
                self.env['BARRIER'] = '1'
                p = self.run_build(jobs)
                self.assertEqual(p.returncode, 0, p.stdout + p.stderr)
                active = peak = 0
                for event in self.events_list():
                    active += 1 if event[0] == 'start' else -1
                    peak = max(peak, active)
                self.assertEqual(active, 0)
                self.assertEqual(peak, jobs)
                args = json.loads((self.out / 'link.json').read_text())
                self.assertEqual([v for v in args if v.startswith('obj/') and v.endswith('.o')],
                                 ['obj/' + Path(u).name + '.o' for u in self.units])
                self.assertIn('--plugin-opt=-enable-ipra=true', args)
                self.assertEqual((self.out / 'cc_warn.txt').read_text().splitlines(),
                                 [stream + ' ' + unit for unit in self.units for stream in ('stderr', 'stdout')])
                self.assert_clean()

    def test_compile_failure_and_stale_outputs(self):
        for name in ('ctl.img', 'ctl.elf', 'sections.txt'):
            (self.out / name).write_text('stale success')
        self.env['FAIL'] = 'src/a.c'
        self.env['SPAWN_CHILD'] = 'src/a.c'
        p = self.run_build(2)
        self.assertNotEqual(p.returncode, 0)
        self.assertIn('CC FAIL', p.stdout)
        self.assertFalse((self.out / 'link.json').exists())
        for name in ('ctl.img', 'ctl.elf', 'sections.txt'):
            self.assertFalse((self.out / name).exists())
        self.assertIn('stderr src/a.c', (self.out / 'cc_warn.txt').read_text())
        self.assert_stopped([int(n) for n in self.children.read_text().splitlines()])
        self.assert_clean()

    def test_invalid_job_counts_fail_without_stale_success(self):
        for jobs in ('', '0', '-1', 'abc', '65', '99999999999999999999', '01', '2.5'):
            with self.subTest(jobs=jobs):
                (self.out / 'ctl.elf').write_text('stale')
                self.env['BARRIER'] = '1'
                p = self.run_build(jobs)
                self.assertNotEqual(p.returncode, 0)
                self.assertIn('Invalid X0X_JOBS', p.stderr)
                self.assertFalse((self.out / 'ctl.elf').exists())
                self.assert_clean()

    def test_interruption_joins_compilers_and_descendants_before_cleanup(self):
        self.env.update(X0X_JOBS='2', HANG='1')
        p = subprocess.Popen(['sh', str(TOP / 'ble/ctl/build_in_container.sh')],
                             env=self.env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.addCleanup(lambda: p.poll() is None and p.kill())
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if self.children.exists() and len(self.children.read_text().splitlines()) == 2:
                break
            time.sleep(.01)
        else:
            self.fail('compilers did not start')
        p.send_signal(signal.SIGTERM)
        stdout, stderr = p.communicate(timeout=5)
        self.assertEqual(p.returncode, 143, stdout + stderr)
        pids = [event[2] for event in self.events_list()] + [int(n) for n in self.children.read_text().splitlines()]
        self.assert_stopped(pids)
        self.assert_clean()
        self.assertFalse((self.out / 'ctl.elf').exists())
    def assert_stopped(self, pids):
        # Delivery of SIGKILL to a descendant can finish just after its driver is reaped.
        deadline = time.monotonic() + 1
        for pid in pids:
            while True:
                if sys.platform.startswith('linux'):
                    proc = Path('/proc') / str(pid) / 'stat'
                    state = proc.read_text().split()[2] if proc.exists() else ''
                else:
                    state = subprocess.run(['ps', '-o', 'stat=', '-p', str(pid)], capture_output=True, text=True).stdout.strip()
                if not state or state.startswith('Z'):
                    break
                self.assertLess(time.monotonic(), deadline, f'owned process {pid} still active: {state}')
                time.sleep(.01)


if __name__ == '__main__':
    unittest.main()
