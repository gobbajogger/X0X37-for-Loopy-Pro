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
unit = source.startswith(('src/', 'gen/'))
if unit:
    assert '-Werror' in sys.argv and '-flto' in sys.argv and '-Isrc' in sys.argv
    assert ('-w' in sys.argv) == source.startswith('gen/')
else:
    assert '-flto' not in sys.argv and '-fmerge-functions' not in sys.argv
    assert Path(source).read_text() == ('bitcode' if source == 'obj/ctl.bc' else 'optimized bitcode')
    assert ('-emit-llvm' in sys.argv) == (source == 'obj/ctl.bc')
    assert ('-O2' if source == 'obj/ctl.bc' else '-Oz') in sys.argv
def event(kind):
    with open(os.environ['EVENTS'], 'a') as f:
        fcntl.flock(f, fcntl.LOCK_EX)
        f.write(json.dumps([kind, source, os.getpid(), str(Path.cwd())]) + '\n')
        f.flush()
event('start')
if unit and os.environ.get('BARRIER'):
    units = ['src/a.c', 'src/b.c', 'src/c.S', 'gen/d.c']
    jobs = int(os.environ['X0X_JOBS'])
    required = (units.index(source) // jobs + 1) * jobs
    deadline = time.monotonic() + 5
    while sum(json.loads(l)[0] == 'start' for l in Path(os.environ['EVENTS']).read_text().splitlines()) < required:
        assert time.monotonic() < deadline, 'compile batch did not start'
        time.sleep(.01)
if os.environ.get('AFTER_SUCCESS') == source:
    deadline = time.monotonic() + 5
    while not any(json.loads(l)[:2] == ['end', 'src/a.c'] for l in Path(os.environ['EVENTS']).read_text().splitlines()):
        assert time.monotonic() < deadline, 'earlier driver did not succeed'
        time.sleep(.01)
hang = os.environ.get('HANG') or os.environ.get('HANG_SOURCE') == source
if hang or os.environ.get('SPAWN_CHILD') == source:
    child = subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(60)'])
    with open(os.environ['CHILDREN'], 'a') as f: f.write(str(child.pid) + '\n')
    if hang: time.sleep(60)
else:
    time.sleep(.08 if source == 'src/a.c' else .20)
print('stdout ' + source)
print('stderr ' + source, file=sys.stderr)
if os.environ.get('FAIL') == source:
    event('fail')
    sys.exit(7)
out.write_text(source if unit else 'optimized bitcode' if source == 'obj/ctl.bc' else 'native object')
event('end')
'''
LINKER = r'''
import json, os, sys, subprocess, time
from pathlib import Path
args = sys.argv[1:]
emit = '--plugin-opt=emit-llvm' in args
Path(os.environ['OUT'], 'lto-link.json' if emit else 'link.json').write_text(json.dumps(args))
Path(args[args.index('-Map') + 1]).write_text('map')
if os.environ.get('LINK_HANG') == ('emit' if emit else 'final'):
    child = subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(60)'])
    with open(os.environ['CHILDREN'], 'a') as f: f.write(str(child.pid) + '\n')
    time.sleep(60)
if os.environ.get('LINK_FAIL') == ('emit' if emit else 'final'):
    sys.exit(9)
if not emit:
    assert Path('obj/ctl.o').read_text() == 'native object'
Path(args[args.index('-o') + 1]).write_text(('not bitcode' if os.environ.get('BAD_IR') else 'bitcode') if emit else 'elf')
'''
CLEANUP_MONITOR = r'''
import json, os, subprocess, sys
from pathlib import Path
if any(Path(arg).name.startswith('ctlbuild.') for arg in sys.argv[1:]):
    states = []
    children = Path(os.environ['CHILDREN'])
    for pid in children.read_text().splitlines() if children.exists() else []:
        if sys.platform.startswith('linux'):
            try:
                state = (Path('/proc') / pid / 'stat').read_text().split()[2]
            except FileNotFoundError:
                state = ''
        else:
            state = subprocess.run(['ps', '-o', 'stat=', '-p', pid], capture_output=True, text=True).stdout.strip()
        states.append([int(pid), state])
    Path(os.environ['CLEANUP_STATES']).write_text(json.dumps(states))
os.execv(os.environ['REAL_RM'], [os.environ['REAL_RM'], *sys.argv[1:]])
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

    def monitor_cleanup(self):
        """Observe children at scratch deletion and stop only fixture children on failure."""
        self.addCleanup(self.stop_fixture_children)
        monitor = self.root / 'monitor/rm'
        monitor.parent.mkdir()
        monitor.write_text('#!' + sys.executable + '\n' + CLEANUP_MONITOR)
        monitor.chmod(0o755)
        self.env.update(REAL_RM=shutil.which('rm'), CLEANUP_STATES=str(self.root / 'cleanup-states'),
                        PATH=str(monitor.parent) + ':' + self.env['PATH'])

    def stop_fixture_children(self):
        for pid in self.child_pids():
            try:
                os.kill(pid, signal.SIGKILL)
            except ProcessLookupError:
                pass

    def child_pids(self):
        return [int(n) for n in self.children.read_text().splitlines()] if self.children.exists() else []

    def assert_children_stopped_before_cleanup(self):
        states = json.loads((self.root / 'cleanup-states').read_text())
        self.assertTrue(states, 'no child observed at scratch cleanup')
        for pid, state in states:
            self.assertTrue(not state or state.startswith('Z'), f'owned child {pid} active at scratch cleanup: {state}')
        self.assert_stopped(self.child_pids())
        self.assert_clean()

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
                args = json.loads((self.out / 'lto-link.json').read_text())
                self.assertEqual([v for v in args if v.startswith('obj/') and v.endswith('.o')],
                                 ['obj/' + Path(u).name + '.o' for u in self.units])
                self.assertIn('--plugin-opt=-enable-ipra=true', args)
                self.assertIn('--plugin-opt=emit-llvm', args)
                final = json.loads((self.out / 'link.json').read_text())
                self.assertEqual([v for v in final if v.startswith('obj/') and v.endswith('.o')], ['obj/ctl.o', 'obj/c.S.o'])
                self.assertNotIn('--plugin-opt=emit-llvm', final)
                self.assertIn('ctl.ld', final)
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

    def test_emit_optimization_codegen_and_final_link_fail_closed(self):
        for env in ({'LINK_FAIL': 'emit'}, {'FAIL': 'obj/ctl.bc'},
                    {'FAIL': 'obj/ctl-opt.bc'}, {'LINK_FAIL': 'final'}, {'BAD_IR': '1'}):
            with self.subTest(stage=env):
                for key in ('LINK_FAIL', 'FAIL', 'BAD_IR'): self.env.pop(key, None)
                self.env.update(env)
                for name in ('ctl.img', 'ctl.elf', 'sections.txt', 'symbols.txt'):
                    (self.out / name).write_text('stale success')
                p = self.run_build(2)
                self.assertNotEqual(p.returncode, 0, p.stdout + p.stderr)
                for name in ('ctl.img', 'ctl.elf', 'sections.txt', 'symbols.txt'):
                    self.assertFalse((self.out / name).exists(), name)
                self.assert_clean()

    def test_emit_and_final_link_interruption_joins_children(self):
        self.monitor_cleanup()
        for phase in ('emit', 'final'):
            with self.subTest(phase=phase):
                self.children.unlink(missing_ok=True)
                self.env.update(X0X_JOBS='2', LINK_HANG=phase)
                p = subprocess.Popen(['sh', str(TOP / 'ble/ctl/build_in_container.sh')],
                                     env=self.env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                try:
                    deadline = time.monotonic() + 5
                    while not self.child_pids():
                        self.assertLess(time.monotonic(), deadline)
                        time.sleep(.01)
                    p.send_signal(signal.SIGTERM)
                    stdout, stderr = p.communicate(timeout=5)
                    self.assertEqual(p.returncode, 143, stdout + stderr)
                    self.assertFalse((self.out / 'ctl.elf').exists())
                    self.assert_children_stopped_before_cleanup()
                finally:
                    if p.poll() is None:
                        p.send_signal(signal.SIGTERM); p.communicate(timeout=5)
                    self.stop_fixture_children()
        self.env.pop('LINK_HANG', None)

    def test_codegen_child_is_reaped_on_failure(self):
        self.monitor_cleanup()
        self.env.update(FAIL='obj/ctl-opt.bc', SPAWN_CHILD='obj/ctl-opt.bc')
        p = self.run_build(2)
        self.assertNotEqual(p.returncode, 0)
        self.assertFalse((self.out / 'ctl.elf').exists())
        self.assert_children_stopped_before_cleanup()

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

    def test_successful_driver_child_before_later_failure(self):
        self.monitor_cleanup()
        for jobs, failing in ((2, 'src/b.c'), (1, 'src/b.c'), (2, 'src/c.S')):
            with self.subTest(jobs=jobs, failing=failing):
                self.events.unlink(missing_ok=True)
                self.children.unlink(missing_ok=True)
                self.env.update(SPAWN_CHILD='src/a.c', FAIL=failing, AFTER_SUCCESS=failing)
                try:
                    p = self.run_build(jobs)
                    self.assertEqual(p.returncode, 1, p.stdout + p.stderr)
                    self.assertIn('CC FAIL', p.stdout)
                    self.assertFalse((self.out / 'link.json').exists())
                    self.assert_children_stopped_before_cleanup()
                finally:
                    self.stop_fixture_children()

    def test_successful_driver_child_stopped_on_success(self):
        self.monitor_cleanup()
        self.env['SPAWN_CHILD'] = 'src/a.c'
        p = self.run_build(1)
        self.assertEqual(p.returncode, 0, p.stdout + p.stderr)
        self.assertTrue((self.out / 'ctl.elf').exists())
        self.assert_children_stopped_before_cleanup()

    def test_signal_after_successful_batch_stops_children(self):
        self.monitor_cleanup()
        for jobs, sig in ((jobs, sig) for jobs in (1, 2) for sig in (signal.SIGHUP, signal.SIGINT, signal.SIGTERM)):
            with self.subTest(jobs=jobs, signal=sig):
                self.events.unlink(missing_ok=True)
                self.children.unlink(missing_ok=True)
                self.env.update(X0X_JOBS=str(jobs), SPAWN_CHILD='src/a.c', HANG_SOURCE='src/b.c',
                                AFTER_SUCCESS='src/b.c')
                # An outer asynchronous shell can inherit ignored SIGINT. The
                # tested child needs the normal disposition before its shell exec.
                p = subprocess.Popen(['sh', str(TOP / 'ble/ctl/build_in_container.sh')],
                                     env=self.env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                                     preexec_fn=lambda: signal.signal(signal.SIGINT, signal.SIG_DFL))
                try:
                    deadline = time.monotonic() + 5
                    while len(self.child_pids()) < 2:
                        self.assertLess(time.monotonic(), deadline, 'later compiler did not start')
                        time.sleep(.01)
                    # The previous group must already be stopped at the batch boundary.
                    self.assert_stopped(self.child_pids()[:1])
                    p.send_signal(sig)
                    stdout, stderr = p.communicate(timeout=5)
                    self.assertEqual(p.returncode, 128 + sig, stdout + stderr)
                    self.assert_children_stopped_before_cleanup()
                    self.assertFalse((self.out / 'ctl.elf').exists())
                finally:
                    if p.poll() is None:
                        p.send_signal(signal.SIGTERM)
                        p.communicate(timeout=5)
                    self.stop_fixture_children()

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
