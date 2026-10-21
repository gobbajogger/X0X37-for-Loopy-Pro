#!/usr/bin/env python3
"""Private fixtures only: no SDK, target compile, device, or real cache writes."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import signal
import time
import unittest

ROOT = Path(__file__).resolve().parents[2]
HELPER = ROOT/'tools/legacy_controller_namespace.sh'
RUNNER = ROOT/'tools/dev_validate.py'

class LegacyNamespace(unittest.TestCase):
    def setUp(self):
        (ROOT/'build').mkdir(exist_ok=True)
        self.tmp = tempfile.TemporaryDirectory(dir=ROOT/'build')
        self.addCleanup(self.tmp.cleanup)
        self.work = Path(self.tmp.name)
        (self.work/'ble/ctl').mkdir(parents=True)
        (self.work/'host-home').mkdir()
        self.env = dict(os.environ, HOME=str(self.work/'host-home'), OUT=str(self.work/'outside-output'))
        (self.work/'ble/ctl/build_ctl.sh').write_text('''set -eu
[ "$CTL_NATIVE" = 1 ]
[ "$OUT" = "$PWD/ble/ctl/out" ]
[ "$HOME" = /tmp/legacy-controller-home ]
[ ! -e /tmp/ctlbuild ]
mkdir -p /tmp/ctlbuild "$HOME/.config/x0x" ble/ctl/out
printf private > /tmp/ctlbuild/owned
printf private-cache > "$HOME/.config/x0x/ctl.img"
printf fresh-controller > ble/ctl/out/ctl.img
''')
    def invoke(self, namespace=True, script=None):
        fd = os.open('/proc/self/ns/mnt', os.O_RDONLY)
        try:
            argv = ['sh', str(HELPER), str(self.work), f'/proc/self/fd/{fd}']
            if namespace:
                argv = ['unshare', '--user', '--map-root-user', '--mount', '--pid', '--mount-proc', '--fork', '--kill-child=KILL', *argv]
            return subprocess.run(argv, env=self.env, pass_fds=(fd,), text=True, capture_output=True)
        finally:
            os.close(fd)
    def test_no_namespace_refuses_before_build(self):
        p=self.invoke(False)
        self.assertNotEqual(p.returncode,0)
        self.assertIn('private mount namespace absent',p.stderr)
        self.assertFalse((self.work/'ble/ctl/out').exists())
    def test_private_tmp_home_and_fresh_output(self):
        before=os.readlink('/proc/self/ns/mnt')
        p=self.invoke()
        self.assertEqual(p.returncode,0,p.stdout+p.stderr)
        self.assertEqual((self.work/'ble/ctl/out/ctl.img').read_text(),'fresh-controller')
        self.assertEqual(list((self.work/'host-home').iterdir()),[])
        self.assertFalse((self.work/'outside-output').exists())
        self.assertEqual(os.readlink('/proc/self/ns/mnt'),before)
        # A second invocation gets another empty private /tmp, never reused state.
        p=self.invoke()
        self.assertEqual(p.returncode,0,p.stdout+p.stderr)
    def test_failed_builder_fails(self):
        (self.work/'ble/ctl/build_ctl.sh').write_text('exit 17\n')
        self.assertEqual(self.invoke().returncode,17)
    def test_missing_original_namespace_fd_refuses(self):
        p=subprocess.run(['unshare','--user','--map-root-user','--mount','--pid','--mount-proc','--fork','--kill-child=KILL','sh',str(HELPER),str(self.work),'/proc/self/fd/99999'],capture_output=True)
        self.assertNotEqual(p.returncode,0)
        self.assertFalse((self.work/'ble/ctl/out').exists())
    def test_timeout_kills_descendant_namespace_writer(self):
        marker=self.work/'started'
        late=self.work/'late-write'
        (self.work/'ble/ctl/build_ctl.sh').write_text(
            f'echo started > "{marker}"\n(sleep 1; echo leak > "{late}") &\nsleep 10\n')
        fd=os.open('/proc/self/ns/mnt',os.O_RDONLY)
        try:
            p=subprocess.Popen(['unshare','--user','--map-root-user','--mount','--pid','--mount-proc','--fork','--kill-child=KILL','sh',str(HELPER),str(self.work),f'/proc/self/fd/{fd}'],pass_fds=(fd,),start_new_session=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
            deadline=time.monotonic()+3
            while not marker.exists() and p.poll() is None and time.monotonic()<deadline: time.sleep(.01)
            self.assertTrue(marker.exists(),p.stderr.read() if p.poll() is not None else 'not started')
            os.killpg(p.pid,signal.SIGKILL)
            p.wait(timeout=3)
            time.sleep(1.2)
            self.assertFalse(late.exists())
            p.stderr.close()
        finally: os.close(fd)
    def test_mount_failure_refuses_before_builder(self):
        fake=self.work/'fakebin'; fake.mkdir()
        (fake/'mount').write_text('#!/bin/sh\nexit 19\n'); (fake/'mount').chmod(0o755)
        self.env['PATH']=str(fake)+':'+self.env['PATH']
        self.assertEqual(self.invoke().returncode,19)
        self.assertFalse((self.work/'ble/ctl/out').exists())
    def test_outer_private_ctlbuild_sentinel_preserved(self):
        # Seed only an outer test-owned tmpfs, never the real shared /tmp/ctlbuild.
        probe=self.work/'nested.py'
        probe.write_text("""import os,subprocess
from pathlib import Path
subprocess.run(['mount','--make-rprivate','/'],check=True)
subprocess.run(['mount','-t','tmpfs','tmpfs','/tmp'],check=True)
p=Path('/tmp/ctlbuild');p.mkdir();(p/'sentinel').write_text('preserve')
fd=os.open('/proc/self/ns/mnt',os.O_RDONLY)
try:
 subprocess.run(['unshare','--user','--map-root-user','--mount','--pid','--mount-proc','--fork','--kill-child=KILL','sh',HELPER,WORK,f'/proc/self/fd/{fd}'],pass_fds=(fd,),check=True)
finally: os.close(fd)
assert (p/'sentinel').read_text()=='preserve'
assert not (p/'owned').exists()
""".replace('HELPER',repr(str(HELPER))).replace('WORK',repr(str(self.work))))
        p=subprocess.run(['unshare','--user','--map-root-user','--mount','--pid','--mount-proc','--fork','--kill-child=KILL','python3',str(probe)],capture_output=True,text=True,env=self.env)
        self.assertEqual(p.returncode,0,p.stdout+p.stderr)
    def test_unsupported_modes_refuse_before_source_or_output(self):
        for flags in (['--no-controller'],['--worker-image','unused-image']):
            p=subprocess.run(['python3',str(RUNNER),'run','--repo',str(self.work/'absent-repo'),'--commit','HEAD','--model','elite','--plan','absent','--output',str(self.work/'result'),'--legacy-controller-isolation',*flags],capture_output=True,text=True)
            self.assertEqual(p.returncode,3,p.stdout+p.stderr)
            self.assertIn('requires native Linux and a controller',p.stderr)
            self.assertFalse((self.work/'result').exists())
    def test_unknown_source_refuses_without_output(self):
        subprocess.run(['git','init','-q',str(self.work)],check=True)
        subprocess.run(['git','-C',str(self.work),'add','.'],check=True)
        subprocess.run(['git','-C',str(self.work),'-c','user.name=test','-c','user.email=test@example.invalid','commit','-qm','fixture'],check=True)
        p=subprocess.run(['python3',str(RUNNER),'run','--repo',str(self.work),'--commit','HEAD','--model','elite','--plan',str(self.work/'absent-plan.json'),'--output',str(self.work/'result'),'--legacy-controller-isolation'],text=True,capture_output=True)
        self.assertEqual(p.returncode,3,p.stdout+p.stderr)
        self.assertIn('only the reviewed 53d5654 baseline',p.stderr)
        self.assertFalse((self.work/'result').exists())

if __name__=='__main__': unittest.main()
