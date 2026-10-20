#!/usr/bin/env python3
"""Gate admission regression: scratch inputs only; never builds or runs a full gate.
Run with python3 tests/host/test_gate_reason.py. Linux also covers PASS reuse.
"""
import hashlib
import os
from pathlib import Path
import platform
import subprocess
import tempfile
import unittest

TOP = Path(__file__).resolve().parents[2]


class GateReason(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.env = {k: v for k, v in os.environ.items()
                    if not k.startswith(('X0X_', 'GIT_', 'JIELI_', 'AC79_'))}
        self.env.update(X0X_REPO=str(self.root / 'repo'),
                        X0X_RESULTS=str(self.root / 'results'),
                        X0X_GATE_WORK=str(self.root / 'work'),
                        X0X_GATE_LOCK=str(self.root / 'lock'),
                        X0X_TOOLCHAIN=str(self.root / 'missing-toolchain'),
                        AC79_SDK=str(self.root / 'missing-sdk'), X0X_GATE_INNER='1')

    def gate(self, *args):
        return subprocess.run(['bash', str(TOP / 'tools/gate.sh'), *args],
                              env=self.env, text=True, capture_output=True)

    def test_refuses_unjustified_calls_without_side_effects(self):
        for args in [[], ['HEAD'], ['HEAD', '--trial'], ['HEAD', '--force', 'retry'],
                     ['HEAD', '--purpose', 'development', '--reason', 'new commit'],
                     ['HEAD', '--purpose', 'release-install'],
                     ['HEAD', '--purpose', 'owner-validation', '--reason', ' \n\t'],
                     ['HEAD', '--purpose', 'owner-validation', '--reason'],
                     ['HEAD', '--purpose', 'owner-validation', '--reason', '--trial'],
                     ['HEAD', '--purpose', 'release-package', '--reason', 'release v1', '--trial'],
                     ['HEAD', '--purpose', 'owner-validation', '--reason', 'request', '--force', '   '],
                     ['HEAD', 'other', '--purpose', 'owner-validation', '--reason', 'request']]:
            with self.subTest(args=args):
                p = self.gate(*args)
                self.assertEqual(p.returncode, 2, p.stdout + p.stderr)
                self.assertFalse(list(self.root.iterdir()), p.stdout + p.stderr)
        self.env['X0X_GATE_SETUP_ONLY'] = '1'
        self.assertEqual(self.gate('HEAD').returncode, 2)
        self.assertFalse(list(self.root.iterdir()))

    def test_valid_purposes_reach_platform_or_commit_check(self):
        for purpose in ['release-install', 'release-package', 'owner-validation']:
            p = self.gate('HEAD', '--purpose', purpose, '--reason', 'Owner request: validate candidate v1')
            self.assertNotIn('REFUSED', p.stdout + p.stderr)
            self.assertIn('unknown commit' if platform.system() == 'Linux' else 'use Linux', p.stdout)

    @unittest.skipUnless(platform.system() == 'Linux', 'Linux gate reuse')
    def test_reuse_and_force_keep_admission_contract(self):
        repo = self.root / 'repo'
        repo.mkdir()
        def git(*args):
            return subprocess.check_output(['git', '-C', str(repo), *args], env=self.env)
        git('init', '-q', '-b', 'integration')
        (repo / 'source.c').write_text('fixture\n')
        git('add', '.')
        git('-c', 'user.name=Test', '-c', 'user.email=test@example.invalid', 'commit', '-qm', 'fixture')
        tree = hashlib.sha1(git('ls-tree', '-r', '--full-tree', 'HEAD')).hexdigest()
        results = self.root / 'results'
        results.mkdir()
        (results / (tree + '.md')).write_text('Result: PASS\n- Config CRCs: fixture\n')
        args = ['HEAD', '--purpose', 'release-install', '--reason', 'Install candidate v1 | owner request']
        p = self.gate(*args)
        self.assertEqual(p.returncode, 0, p.stdout + p.stderr)
        self.assertIn('result used again', p.stdout)
        self.assertIn('purpose: release-install; reason: Install candidate v1 / owner request',
                      (results / 'index.md').read_text())
        self.assertFalse((self.root / 'work').exists())
        p = self.gate(*args, '--force', 'Toolchain changed since recorded PASS')
        self.assertEqual(p.returncode, 2, p.stdout + p.stderr)
        self.assertIn('no toolchain', p.stdout)
        p = self.gate('HEAD', '--trial', '--purpose', 'owner-validation', '--reason', 'Owner requested lane validation')
        self.assertIn('no toolchain', p.stdout)
        self.assertNotIn('result used again', p.stdout)


if __name__ == '__main__':
    unittest.main()
