#!/usr/bin/env python3
"""Development evidence integration tests; only scratch repos and fake builds.
No target toolchain, live queue, gate index, or device is used.
"""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT/'tools/dev_validate.py'
GUARD = ROOT/'tools/require_gate_pass.sh'


class DevelopmentValidation(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.repo = self.root/'repo'
        self.repo.mkdir()
        self.git('init', '-q')
        (self.repo/'build.sh').write_text('''#!/bin/sh
set -eu
[ "$MODEL" = elite ]
[ "$X0X_DEV_PADCAL" = 1 ]
mkdir -p build
printf 'development package' > build/x0x-smk37-elite.fwsc
echo 'app code area 100 B, 40 B free'
echo 'RAM .data+.bss 40 B of 100; pool 100 B of 10000'
''')
        (self.repo/'.gitignore').write_text('build/\nble/ctl/out/\n')
        (self.repo/'test.py').write_text('print("affected test passed")\n')
        self.git('add', '.')
        self.git('-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'fixture')
        self.commit = self.git('rev-parse', 'HEAD')
        self.plan = self.root/'plan.json'
        self.plan.write_text(json.dumps({'base': self.commit, 'scope': 'fixture behavior',
                                        'checks': [{'reason': 'changed fixture', 'argv': [sys.executable, 'test.py']}]}))
        self.out = self.root/'result'
        self.queue = self.root/'QUEUE.md'
        self.queue.write_text('')
        self.env = os.environ.copy()
        self.env.update(X0X_REPO=str(self.repo), X0X_QUEUE_FILE=str(self.queue),
                        X0X_RESULTS_DIR=str(self.root/'no-gate-results'))

    def git(self, *args):
        return subprocess.check_output(['git', '-C', str(self.repo), *args], text=True).strip()

    def run_validation(self, controller=False, timeout=1800):
        launcher = 'import runpy,sys; from unittest.mock import patch; sys.argv=sys.argv[1:]; a=patch("platform.system",return_value="Linux"); b=patch("platform.machine",return_value="x86_64"); a.start(); b.start(); runpy.run_path(sys.argv[0],run_name="__main__")'
        return subprocess.run([sys.executable, '-c', launcher, str(RUNNER), 'run', '--repo', str(self.repo),
                               '--commit', self.commit, '--model', 'elite', '--plan', str(self.plan),
                               '--output', str(self.out), '--timeout', str(timeout)] + ([] if controller else ['--no-controller']), capture_output=True, text=True)

    def guard(self, *args):
        return subprocess.run(['bash', str(GUARD), self.commit, '--fwsc', str(self.out/'x0x-smk37-elite.fwsc'),
                               '--dev-record', str(self.out/'record.json'), *args], env=self.env,
                              capture_output=True, text=True)

    def ready(self):
        r = self.run_validation()
        self.assertEqual(r.returncode, 0, r.stdout+r.stderr)

    def test_success_without_gate_and_queue_enforced(self):
        self.ready()
        self.assertEqual(self.guard().returncode, 0)
        self.assertEqual(self.guard('--config', 'rel').returncode, 3)
        self.queue.write_text('- [ ] missing pending change\n')
        self.assertEqual(self.guard().returncode, 3)
        self.assertFalse((self.root/'no-gate-results').exists())

    def test_artifact_log_source_and_controller_mismatch(self):
        self.ready()
        pkg = self.out/'x0x-smk37-elite.fwsc'
        original = pkg.read_bytes()
        pkg.write_bytes(b'other')
        self.assertEqual(self.guard().returncode, 3)
        pkg.write_bytes(original)
        log = self.out/'check-0.log'
        old_log = log.read_bytes()
        log.write_text('changed')
        self.assertEqual(self.guard().returncode, 3)
        log.write_bytes(old_log)
        ctl = self.root/'ctl.img'
        ctl.write_bytes(b'unknown controller')
        self.assertEqual(self.guard('--ctl', str(ctl)).returncode, 3)
        (self.repo/'new.c').write_text('changed source')
        self.git('add', '.')
        self.git('-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'changed')
        self.commit = self.git('rev-parse', 'HEAD')
        self.assertEqual(self.guard().returncode, 3)

    def test_docs_only_commit_reuses_evidence(self):
        self.ready()
        (self.repo/'README.md').write_text('documentation only')
        self.git('add', '.')
        self.git('-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'docs')
        self.commit = self.git('rev-parse', 'HEAD')
        self.assertEqual(self.guard().returncode, 0)

    def test_controller_selection_and_cache_isolation_request(self):
        ctl = self.repo/'ble/ctl'
        ctl.mkdir(parents=True)
        (ctl/'build_ctl.sh').write_text('printf "controller build\\n"\n[ "$CTL_NO_CACHE" = 1 ] || exit 1\nmkdir -p ble/ctl/out\nprintf controller > ble/ctl/out/ctl.img\n')
        self.git('add', '.')
        (ctl/'build_in_container.sh').write_text('# fixture for B=$(mktemp -d isolation contract\n')
        self.git('add', '.')
        self.git('-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'controller')
        self.commit = self.git('rev-parse', 'HEAD')
        result = self.run_validation(controller=True)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        self.assertEqual(self.guard().returncode, 3)
        image = self.out/'ctl.img'
        self.assertEqual(self.guard('--ctl', str(image)).returncode, 0)
        image.write_bytes(b'changed controller')
        self.assertEqual(self.guard('--ctl', str(image)).returncode, 3)

    def test_failed_check_issues_no_record(self):
        plan = json.loads(self.plan.read_text())
        plan['checks'][0]['argv'] = [sys.executable, '-c', 'raise SystemExit(1)']
        self.plan.write_text(json.dumps(plan))
        self.assertEqual(self.run_validation().returncode, 3)
        self.assertFalse((self.out/'record.json').exists())
        self.assertEqual(len(self.git('worktree', 'list').splitlines()), 1)

    def test_old_controller_scripts_refused_before_execution(self):
        ctl = self.repo/'ble/ctl'
        ctl.mkdir(parents=True)
        (ctl/'build_ctl.sh').write_text('exit 0\n')
        (ctl/'build_in_container.sh').write_text('exit 0\n')
        self.git('add', '.')
        self.git('-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'old controller')
        self.commit = self.git('rev-parse', 'HEAD')
        self.assertEqual(self.run_validation(controller=True).returncode, 3)
        self.assertFalse((self.out/'check-0.log').exists())
        self.assertFalse((self.out/'record.json').exists())

    def test_timeout_kills_owned_child(self):
        marker = self.root/'child-survived'
        child = 'import time,pathlib; time.sleep(3); pathlib.Path(' + repr(str(marker)) + ').touch()'
        parent = 'import subprocess,sys,time; subprocess.Popen([sys.executable,"-c",' + repr(child) + ']); time.sleep(30)'
        plan = json.loads(self.plan.read_text())
        plan['checks'][0]['argv'] = [sys.executable, '-c', parent]
        self.plan.write_text(json.dumps(plan))
        self.assertEqual(self.run_validation(timeout=1).returncode, 3)
        time.sleep(3)
        self.assertFalse(marker.exists())
        self.assertFalse((self.out/'record.json').exists())
        self.assertEqual(len(self.git('worktree', 'list').splitlines()), 1)

    def test_check_cannot_change_source_commit(self):
        plan = json.loads(self.plan.read_text())
        plan['checks'][0]['argv'] = ['sh', '-c', 'echo injected > injected.c; git add injected.c; git -c user.name=t -c user.email=t@t commit -qm injected']
        self.plan.write_text(json.dumps(plan))
        self.assertEqual(self.run_validation().returncode, 3)
        self.assertFalse((self.out/'record.json').exists())

    def test_missing_plan_and_budget_evidence_refused(self):
        self.plan.write_text(json.dumps({'base': self.commit, 'scope': 'fixture', 'checks': []}))
        self.assertEqual(self.run_validation().returncode, 3)
        self.assertFalse((self.out/'record.json').exists())
        self.plan.write_text(json.dumps({'base': self.commit, 'scope': 'fixture', 'no_tests_reason': 'no host test'}))
        path = self.repo/'build.sh'
        path.write_text(path.read_text().replace('pool 100 B of 10000', 'pool 9999 B of 10000'))
        self.git('add', '.')
        self.git('-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'bad budget')
        self.commit = self.git('rev-parse', 'HEAD')
        self.assertEqual(self.run_validation().returncode, 3)
        self.assertFalse((self.out/'record.json').exists())

    def test_installer_selects_dev_evidence_without_cached_controller(self):
        self.ready()
        sys.path.insert(0, str(ROOT/'tools'))
        import fm1_install
        from types import SimpleNamespace
        from unittest.mock import patch
        args = SimpleNamespace(commit=self.commit, package=str(self.out/'x0x-smk37-elite.fwsc'),
                               config='dev', ctl=None, no_ctl=True, dev_record=str(self.out/'record.json'))
        with patch.dict(os.environ, self.env):
            self.assertEqual(fm1_install.gate_guard(args), 0)
            args.no_ctl = False
            self.assertEqual(fm1_install.gate_guard(args), 9)
            args.no_ctl = True
            args.config = 'rel'
            self.assertEqual(fm1_install.gate_guard(args), 9)


if __name__ == '__main__':
    unittest.main()
