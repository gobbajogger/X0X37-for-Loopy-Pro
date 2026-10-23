#!/usr/bin/env python3
"""Host-only regressions for the fail-closed native nightly build probe."""
from __future__ import annotations

import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / 'tools' / 'native_nightly_build_probe.py'
SPEC = importlib.util.spec_from_file_location('native_nightly_build_probe', SCRIPT)
PROBE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PROBE)


def call(repo, *args):
    return subprocess.run(['git', '-C', str(repo), *args], capture_output=True,
                          text=True, check=True).stdout.strip()


class NativeProbeTests(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.dir.cleanup)
        self.repo = Path(self.dir.name) / 'checkout'
        self.repo.mkdir()
        call(self.repo, 'init', '-q')
        call(self.repo, 'config', 'user.name', 'X0X37 regression')
        call(self.repo, 'config', 'user.email', 'nightly-probe@example.invalid')
        (self.repo / 'sample.txt').write_text('snapshot\n')
        call(self.repo, 'add', 'sample.txt')
        call(self.repo, 'commit', '-qm', 'test source snapshot')
        self.head = call(self.repo, 'rev-parse', 'HEAD')
        self.tree = call(self.repo, 'rev-parse', 'HEAD^{tree}')

    def test_exact_commit_and_tree_permitted(self):
        self.assertEqual(PROBE.verify_source(self.repo, self.head, self.tree),
                         {'commit': self.head, 'tree': self.tree})

    def test_stale_commit_and_wrong_tree_refused(self):
        with self.assertRaisesRegex(ValueError, 'STALE SOURCE'):
            PROBE.verify_source(self.repo, '0' * 40, self.tree)
        with self.assertRaisesRegex(ValueError, 'STALE SOURCE'):
            PROBE.verify_source(self.repo, self.head, '0' * 40)

    def test_dirty_tracked_and_untracked_refused(self):
        (self.repo / 'sample.txt').write_text('modified\n')
        with self.assertRaisesRegex(ValueError, 'untracked|modifications'):
            PROBE.verify_source(self.repo, self.head, self.tree)
        call(self.repo, 'checkout', '--', 'sample.txt')
        (self.repo / 'untracked.txt').write_text('untracked\n')
        with self.assertRaisesRegex(ValueError, 'untracked|modifications'):
            PROBE.verify_source(self.repo, self.head, self.tree)

    def test_malformed_sha_refused(self):
        with self.assertRaisesRegex(ValueError, '40-character'):
            PROBE.verify_source(self.repo, 'short', self.tree)

    def test_check_exit_and_logs_not_trusted_as_pass(self):
        logs = Path(self.dir.name) / 'logs'
        logs.mkdir()
        failed = PROBE.run_check('failed', ['python3', '-c', 'raise SystemExit(13)'],
                                 self.repo, os.environ.copy(), logs, 10)
        self.assertEqual(failed['exit'], 13)
        self.assertIn('python3', (logs / 'failed.log').read_text())
        passed = PROBE.run_check('ok', ['python3', '-c', 'print("pass")'],
                                 self.repo, os.environ.copy(), logs, 10)
        self.assertEqual(passed['exit'], 0)

    def test_sdk_pin_absence_refused_before_tool_execution(self):
        with self.assertRaisesRegex(ValueError, 'Missing runnable JieLi'):
            PROBE.verify_inputs(Path(self.dir.name) / 'no-toolchain',
                                Path(self.dir.name) / 'no-sdk')


if __name__ == '__main__':
    unittest.main(verbosity=2)
