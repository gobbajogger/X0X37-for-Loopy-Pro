#!/usr/bin/env python3
"""Host-only regressions for the fail-closed native nightly build probe."""
from __future__ import annotations

import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import tempfile
from contextlib import redirect_stderr, redirect_stdout
from unittest import mock
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

    def _mock_probe_call(self, output: Path, simulate_cleanup_failure: bool = False,
                         failed_preflight: str | None = None,
                         failed_output_guard: str | None = None):
        """Execute real Git worktree lifecycle with fake build stages only."""
        arguments = ['native_nightly_build_probe.py', '--source', str(self.repo),
                     '--expected-head', self.head, '--expected-tree', self.tree,
                     '--toolchain', self.dir.name, '--sdk', self.dir.name,
                     '--output', str(output)]

        def stage(name, _argv, scratch, _env, logs, _timeout):
            if name.startswith('native-build-'):
                basename = PROBE.PRODUCTS[name.removeprefix('native-build-')]
                dst = scratch / 'build' / basename
                dst.parent.mkdir(parents=True, exist_ok=True)
                dst.write_bytes(b'MOCK FIRMWARE - NOT INSTALLABLE')
            elif name == 'native-controller':
                dst = scratch / 'ble/ctl/out/ctl.img'
                dst.parent.mkdir(parents=True, exist_ok=True)
                dst.write_bytes(b'MOCK CTL - NOT INSTALLABLE')
            (logs / (name + '.log')).write_text('mock stage, not native build\n')
            return {'name': name, 'exit': 19 if name in (failed_preflight, failed_output_guard) else 0,
                    'log': name + '.log'}

        genuine_subprocess_run = subprocess.run

        def intercept_cleanup(argv, *args, **kwargs):
            if (simulate_cleanup_failure and len(argv) >= 6
                    and argv[0] == 'git' and 'worktree' in argv
                    and 'remove' in argv):
                return subprocess.CompletedProcess(argv, 1, '', 'injected cleanup failure')
            return genuine_subprocess_run(argv, *args, **kwargs)

        stdout, stderr = io.StringIO(), io.StringIO()
        with mock.patch.object(PROBE, 'verify_inputs', return_value={'mock': True}), \
             mock.patch.object(PROBE, 'run_check', side_effect=stage), \
             mock.patch.object(PROBE.subprocess, 'run', side_effect=intercept_cleanup), \
             mock.patch.object(PROBE.sys, 'argv', arguments), \
             redirect_stdout(stdout), redirect_stderr(stderr):
            result = PROBE.main()
        return result, stdout.getvalue(), stderr.getvalue()

    def test_failed_postbuild_safety_does_not_erase_compilation(self):
        for safety in ('locked-settings', 'slot-backup', 'installer-slots'):
            with self.subTest(safety=safety):
                output = Path(self.dir.name) / ('fail-' + safety)
                code, stdout, stderr = self._mock_probe_call(output, failed_preflight=safety)
                self.assertEqual(code, 1, stderr)
                manifest = json.loads((output / 'probe.json').read_text())
                self.assertFalse(manifest['native_probe_pass'])
                self.assertFalse(json.loads(stdout)['native_probe_pass'])
                self.assertEqual(manifest['blocked_by'], [safety])
                # Safety checks run AFTER compilation; they cannot erase or
                # prohibit native builds, but do prohibit installable admission.
                self.assertEqual([step['name'] for step in manifest['checks'][:2]],
                                 ['native-build-elite', 'package-integrity-elite'])
                for model, basename in PROBE.PRODUCTS.items():
                    self.assertTrue((output / basename).exists(), model)
                    self.assertIsNotNone(manifest['models'][model]['file'])
                self.assertIsNotNone(manifest['controller'])
                self.assertTrue((output / 'ctl.img').exists())
                self.assertTrue((output / 'logs' / 'native-build-elite.log').exists())
                self.assertTrue((output / 'logs' / 'native-controller.log').exists())
                self.assertEqual(call(self.repo, 'status', '--porcelain', '--untracked-files=normal'), '')

    def test_failed_output_integrity_does_not_claim_native_probe_pass(self):
        for stage in ('package-integrity-elite', 'package-integrity-pro',
                      'controller-integrity'):
            with self.subTest(stage=stage):
                output = Path(self.dir.name) / ('invalid-' + stage)
                code, stdout, stderr = self._mock_probe_call(
                    output, failed_output_guard=stage)
                self.assertEqual(code, 1, stderr)
                result = json.loads((output / 'probe.json').read_text())
                self.assertFalse(result['native_probe_pass'])
                self.assertFalse(json.loads(stdout)['native_probe_pass'])
                matches = [check for check in result['checks'] if check['name'] == stage]
                self.assertEqual(len(matches), 1)
                self.assertEqual(matches[0]['exit'], 19)
                # Compilation was still an actual attempted stage; a downstream
                # validation failure is recorded, never relabelled build failure.
                self.assertEqual(len([check for check in result['checks']
                                      if check['name'].startswith('native-build-')]), 2)
                self.assertEqual(call(self.repo, 'status', '--porcelain'), '')

    def test_cleanup_failure_cannot_publish_positive_manifest_or_stdout(self):
        output = Path(self.dir.name) / 'fresh-output'
        exit_code, stdout, stderr = self._mock_probe_call(output, simulate_cleanup_failure=True)
        self.assertEqual(exit_code, 1)
        self.assertIn('injected cleanup failure', stderr)
        self.assertFalse(json.loads((output / 'probe.json').read_text())['native_probe_pass'])
        self.assertFalse(json.loads(stdout)['native_probe_pass'])
        self.assertEqual(call(self.repo, 'status', '--porcelain', '--untracked-files=normal'), '')

    def test_symlink_output_parent_cannot_alias_source_checkout(self):
        alias = Path(self.dir.name) / 'symlink-to-source'
        alias.symlink_to(self.repo, target_is_directory=True)
        nested = alias / 'new-build-output'
        with self.assertRaisesRegex(ValueError, 'outside the source checkout'):
            self._mock_probe_call(nested)
        self.assertFalse((self.repo / 'new-build-output').exists())
        self.assertEqual(call(self.repo, 'status', '--porcelain', '--untracked-files=normal'), '')

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

    def test_version_spoofed_compiler_rejected_before_sdk_or_build(self):
        fake_toolchain = Path(self.dir.name) / 'spoofed-jieli'
        bin_dir = fake_toolchain / 'pi32v2' / 'bin'
        bin_dir.mkdir(parents=True)
        compiler = bin_dir / 'clang'
        compiler.write_text('#!/bin/sh\necho "clang version 4.0.1 (NOT JIELI)"\n')
        compiler.chmod(0o755)
        # A version string alone is not provenance. The compiler digest must
        # fail before any SDK path or firmware work is attempted.
        with self.assertRaisesRegex(ValueError, 'pinned JieLi compiler SHA-256'):
            PROBE.verify_inputs(fake_toolchain, Path(self.dir.name) / 'missing-sdk')

    def test_pinned_digest_path_allows_further_sdk_identity_checks(self):
        fake_toolchain = Path(self.dir.name) / 'mocked-toolchain'
        bin_dir = fake_toolchain / 'pi32v2' / 'bin'
        bin_dir.mkdir(parents=True)
        compiler = bin_dir / 'clang'
        compiler.write_text('#!/bin/sh\necho "clang version 4.0.1 (UNIT MOCK)"\n')
        compiler.chmod(0o755)
        sdk = Path(self.dir.name) / 'mocked-sdk'
        for relative in PROBE.SDK_PINS:
            file = sdk / relative
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_bytes(b'UNTRUSTED TEST DATA')
        actual_hash = PROBE.sha256(compiler)
        def mock_pin(path):
            if Path(path) == compiler:
                return PROBE.JIELI_COMPILER_SHA256
            return PROBE.SDK_PINS[str(Path(path).relative_to(sdk))]
        # Synthetic digest fixtures test the decision path only. Real positive
        # compiler/SDK bytes must be checked separately in the native build run.
        with mock.patch.object(PROBE, 'sha256', side_effect=mock_pin):
            found = PROBE.verify_inputs(fake_toolchain, sdk)
        self.assertEqual(found['compiler_sha256'], PROBE.JIELI_COMPILER_SHA256)
        self.assertNotEqual(actual_hash, PROBE.JIELI_COMPILER_SHA256)
        self.assertEqual(found['sdk_pins_sha256'], PROBE.SDK_PINS)

    def test_sdk_pin_absence_refused_before_tool_execution(self):
        with self.assertRaisesRegex(ValueError, 'Missing runnable JieLi'):
            PROBE.verify_inputs(Path(self.dir.name) / 'no-toolchain',
                                Path(self.dir.name) / 'no-sdk')


if __name__ == '__main__':
    unittest.main(verbosity=2)
