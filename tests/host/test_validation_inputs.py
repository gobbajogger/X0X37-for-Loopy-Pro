#!/usr/bin/env python3
"""Scratch identity/reuse tests. No real builds, gate validation or live evidence."""
import copy
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

TOP = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(TOP/'tools'))
import validation_inputs as identity

class Inputs(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.repo = self.root/'repo'; self.repo.mkdir()
        self.git(self.repo, 'init', '-q', '-b', 'integration')
        (self.repo/'code.c').write_text('source')
        self.commit(self.repo)
        self.tc = self.root/'tc'; self.tc.mkdir(); (self.tc/'compiler').write_text('compiler v1')
        self.sdk = self.root/'sdk'; self.sdk.mkdir(); (self.sdk/'library').write_text('sdk v1')
        self.refs = self.root/'refs'; self.ref = self.refs/'reference'; self.ref.mkdir(parents=True)
        self.git(self.ref, 'init', '-q')
        (self.ref/'dsp.c').write_text('reference v1'); self.commit(self.ref)
        self.pins = 'reference='+self.git(self.ref, 'rev-parse', 'HEAD').strip()
        self.runner = self.root/'runner'; self.runner.mkdir()
        for name in ('gate.sh', 'gate_flashmap.py', 'gate_sigwrap.py', 'validation_inputs.py', 'pre-push.hook'):
            shutil.copy2(TOP/'tools'/name, self.runner/name)
        # The gate clears caller build selectors before capturing its inputs.
        # Keep baseline capture equivalent when dev_validate sets MODEL/BOARD.
        self.env = {k:v for k,v in os.environ.items()
                    if k not in ('MODEL', 'BOARD')
                    and not k.startswith(('X0X_', 'JIELI_', 'AC79_', 'GIT_'))}
        self.config = {'models': ['elite', 'pro'], 'configs': {'dev': 1, 'rel': 0}, 'time_scale': 1}
        self.env['X0X_TEST_TIME_SCALE'] = '1'

    def git(self, repo, *args):
        return subprocess.check_output(['git', '-C', str(repo), *args], text=True)

    def commit(self, repo):
        self.git(repo, 'add', '.')
        self.git(repo, '-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'fixture')

    def capture(self):
        return identity.capture(self.repo, 'HEAD', 'gate', self.config, self.tc, self.sdk,
                                self.refs, self.pins, env=self.env, runner_dir=self.runner)

    def test_identical_and_changes(self):
        baseline = copy.deepcopy(self.capture())
        identity.validate(baseline, self.capture())
        for path in (self.tc/'compiler', self.sdk/'library', self.ref/'dsp.c', self.runner/'gate.sh',
                     self.runner/'gate_flashmap.py', self.runner/'validation_inputs.py', self.runner/'pre-push.hook'):
            with self.subTest(path=path):
                old = path.read_bytes(); path.write_bytes(old+b'\nchanged')
                with self.assertRaisesRegex(ValueError, 'changed'):
                    identity.validate(baseline, self.capture())
                path.write_bytes(old)
        # Pre-migration FM-1-inclusive evidence cannot qualify for the SMK-only gate.
        self.config['models'].append('fm1')
        with self.assertRaisesRegex(ValueError, 'changed'):
            identity.validate(baseline, self.capture())
        self.config['models'].pop()
        self.config['configs']['rel'] = 1
        self.assertNotEqual(baseline['sha256'], self.capture()['sha256'])
        self.config['configs']['rel'] = 0
        self.env['CFLAGS'] = '-DCHANGED'
        self.assertNotEqual(baseline['sha256'], self.capture()['sha256'])
        self.env.pop('CFLAGS')
        (self.repo/'README.md').write_text('docs'); self.commit(self.repo)
        self.assertEqual(baseline['sha256'], self.capture()['sha256'])
        (self.repo/'code.c').write_text('new source'); self.commit(self.repo)
        self.assertNotEqual(baseline['sha256'], self.capture()['sha256'])
        with self.assertRaises(ValueError): identity.validate({})

    def test_installed_runner_name_preserves_identity(self):
        baseline = self.capture()
        installed = self.runner/'x0x37_gate.sh'
        (self.runner/'gate.sh').rename(installed)
        actual = identity.capture(self.repo, 'HEAD', 'gate', self.config, self.tc, self.sdk,
                                  self.refs, self.pins, env=self.env, runner_dir=self.runner,
                                  runner_path=installed)
        self.assertEqual(baseline['sha256'], actual['sha256'])

    def test_optional_emcc_changes_coverage_identity(self):
        baseline = self.capture()
        sdk = self.root/'emsdk'; sdk.mkdir()
        (sdk/'emcc').write_text('#!/bin/sh\necho fixture\n'); (sdk/'emcc').chmod(0o755)
        (sdk/'backend').write_text('llvm v1')
        self.env.update(PATH=str(sdk)+os.pathsep+self.env['PATH'], EMSDK=str(sdk))
        (sdk/'upstream/emscripten').mkdir(parents=True)
        changed = self.capture()
        self.assertNotEqual(baseline['sha256'], changed['sha256'])
        cache = sdk/'upstream/emscripten/cache'
        cache.mkdir(parents=True); (cache/'generated.a').write_text('derived output')
        # Creating the cache parent directories is also an output-only change.
        # Keep the existing SDK directory structure present before comparison.
        stable = self.capture()
        self.assertEqual(changed['sha256'], stable['sha256'])
        (cache/'generated.a').write_text('rebuilt output')
        self.assertEqual(stable['sha256'], self.capture()['sha256'])
        (sdk/'backend').write_text('llvm v2')
        self.assertNotEqual(changed['sha256'], self.capture()['sha256'])

    def test_real_gate_reuse_routing_with_scratch_inputs(self):
        # Override uname only to reach the portable admission/reuse path on macOS.
        # The missing target compiler stops every non-reuse route before a build.
        fakebin = self.root/'bin'; fakebin.mkdir()
        (fakebin/'uname').write_text('#!/bin/sh\necho Linux\n'); (fakebin/'uname').chmod(0o755)
        self.env.update(PATH=str(fakebin)+os.pathsep+self.env['PATH'], X0X_REPO=str(self.repo),
            X0X_RESULTS=str(self.root/'results'), X0X_GATE_WORK=str(self.root/'work'),
            X0X_TOOLCHAIN=str(self.tc), AC79_SDK=str(self.sdk), X0X_REFS=str(self.refs),
            X0X_REF_PINS=self.pins, X0X_GATE_INNER='1')
        baseline = self.capture()
        tree = __import__('hashlib').sha1(self.git(self.repo, 'ls-tree', '-r', '--full-tree', 'HEAD').encode()).hexdigest()
        results = self.root/'results'; results.mkdir()
        record = results/(tree+'.md')
        def gate(*extra):
            return subprocess.run(['bash', str(self.runner/'gate.sh'), 'HEAD', '--purpose',
                'owner-validation', '--reason', 'Scratch routing regression', *extra], env=self.env,
                capture_output=True, text=True)
        for suffix in ('', '\n- Validation inputs: wrong'):
            record.write_text('Result: PASS\n- Config CRCs: fixture'+suffix+'\n')
            result = gate()
            self.assertNotEqual(result.returncode, 0, result.stdout+result.stderr)
            self.assertNotIn('result used again', result.stdout)
        record.write_text('Result: PASS\n- Config CRCs: fixture\n- Validation inputs: '+baseline['sha256']+'\n')
        logs = self.root/'logs'; logs.mkdir()
        with record.open('a') as stream:
            stream.write(f'- Worktree: {self.root}/source. Logs: {logs}.\n')
        self.assertNotEqual(gate().returncode, 0)  # identity line alone is insufficient
        (logs/'validation-inputs.json').write_text(json.dumps(baseline))
        result = gate()
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        self.assertIn('result used again', result.stdout)
        for extra in (('--force', 'Verify force routing'), ('--trial',)):
            result = gate(*extra)
            self.assertNotEqual(result.returncode, 0)
            self.assertNotIn('result used again', result.stdout)
        (self.sdk/'library').write_text('changed SDK')
        self.assertNotEqual(gate().returncode, 0)
        self.assertFalse((self.root/'work').exists())

if __name__ == '__main__':
    unittest.main()
