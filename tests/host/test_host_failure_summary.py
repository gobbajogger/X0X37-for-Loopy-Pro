#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Regression tests for truthful host status, comparisons and source identity."""
import hashlib
import json
import subprocess
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import host_failure_summary as mod

HEAD_A = 'a' * 40
HEAD_B = 'b' * 40
REAL_SOURCE_CHECK = mod.assert_clean_source


class FailureSummaryTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='ui-host-proof-')
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.git = patch.object(mod, 'git_head', return_value=HEAD_A)
        self.git.start()
        self.addCleanup(self.git.stop)
        self.clean = patch.object(mod, 'assert_clean_source')
        self.clean.start()
        self.addCleanup(self.clean.stop)

    def transcript(self, statuses):
        path = self.root / 'full.log'
        path.write_text(statuses)
        return path

    def record(self, transcript, states, head=HEAD_A, rc=0):
        return {'schema': mod.RUN_SCHEMA, 'head': head, 'root': str(self.root),
                'source_clean_before_after': True,
                'command': ['sh', 'tests/run_tests.sh'],
                'exit_code': rc,
                'selected_tests': sorted(states),
                'plan_sha256': mod.plan_hash(states),
                'transcript_sha256': hashlib.sha256(transcript.read_bytes()).hexdigest()}

    def baseline(self, statuses, current_head=HEAD_A):
        t = self.transcript(statuses)
        states = mod.parse_status(mod.read_transcript(t))
        proof = self.record(t, states, current_head, int('FAIL' in states.values()))
        return mod.build_summary(self.root, t, run_record=proof)

    def test_parallel_results(self):
        d = self.root / '.results'; d.mkdir()
        (d / '0001').write_text('  ok fastmath\n')
        (d / '0002').write_text('  FAIL smc-ui (see build/host/smc-ui.log)\n')
        (d / '0002.fail').touch()
        (self.root / 'smc-ui.log').write_text('clang: error: missing symbol\n')
        result = mod.build_summary(self.root)
        self.assertEqual(result['counts'], {'ok': 1, 'FAIL': 1, 'not-run': 0})
        self.assertFalse(result['source_verified'])
        self.assertIsNone(result['head'])
        self.assertIn('error:', result['failures']['smc-ui']['error'])

    def test_serial_ignores_stale_parallel_results(self):
        d = self.root / '.results'; d.mkdir()
        (d / '0002').write_text('  FAIL stale\n')
        t = self.transcript('  ok fastmath\n  FAIL sound-matrix\n')
        (self.root / 'sound-matrix.log').write_text('AssertionError: bad colour\n')
        result = mod.build_summary(self.root, t)
        self.assertEqual(list(result['failures']), ['sound-matrix'])

    def test_plain_compiler_error(self):
        t = self.transcript('  FAIL compiler\n')
        (self.root / 'compiler.log').write_text('clang: error: unknown\n')
        self.assertIn('error:', mod.build_summary(self.root, t)['failures']['compiler']['error'])

    def test_missing_logs_stay_failed(self):
        t = self.transcript('  FAIL test-a\n')
        result = mod.build_summary(self.root, t)
        self.assertEqual(result['counts']['FAIL'], 1)
        self.assertEqual(result['failures']['test-a']['error'], 'LOG MISSING')

    def test_missing_status_is_error(self):
        (self.root / '.results').mkdir()
        with self.assertRaisesRegex(ValueError, 'No suite results'):
            mod.build_summary(self.root)

    def test_deeply_indented_errors_not_results(self):
        t = self.transcript('  ok first\n  FAIL actual\n       FAIL fake\n       ok fake-ok\n')
        self.assertEqual(mod.build_summary(self.root, t)['counts']['FAIL'], 1)

    def test_duplicate_can_not_erase_fail(self):
        t = self.transcript('  FAIL smc-ui\n  ok smc-ui\n')
        with self.assertRaisesRegex(ValueError, 'Duplicate host test'):
            mod.build_summary(self.root, t)

    def test_duplicate_across_parallel_files_rejected(self):
        d = self.root / '.results'; d.mkdir()
        (d / '0001').write_text('  FAIL smc-ui\n')
        (d / '0002').write_text('  ok smc-ui\n')
        with self.assertRaisesRegex(ValueError, 'Duplicate host test'):
            mod.build_summary(self.root)

    def test_skip_existing_failure_never_cleared(self):
        old = self.baseline('  FAIL ui-shared-glyph\n  ok fastmath\n')
        b = self.root / 'baseline.json'; b.write_text(json.dumps(old))
        t = self.transcript('  not-run ui-shared-glyph (ASan)\n  ok fastmath\n')
        proof = self.record(t, {'ui-shared-glyph': 'not-run', 'fastmath': 'ok'})
        current = mod.build_summary(self.root, t, b, proof)
        self.assertEqual(current['comparison']['status'], 'COMPARABLE')
        self.assertEqual(current['comparison']['cleared_failures'], [])
        self.assertEqual(current['comparison']['unresolved_not_run'], ['ui-shared-glyph'])

    def test_subset_baseline_not_comparable(self):
        old = self.baseline('  FAIL smc-ui\n  ok fastmath\n')
        b = self.root / 'baseline.json'; b.write_text(json.dumps(old))
        t = self.transcript('  ok fastmath\n')
        current = mod.build_summary(self.root, t, b, self.record(t, {'fastmath': 'ok'}))
        self.assertEqual(current['comparison']['status'], 'INCOMPARABLE_TEST_SELECTION')
        self.assertEqual(current['comparison']['cleared_failures'], [])
        self.assertEqual(current['comparison']['unresolved_not_selected'], ['smc-ui'])

    def test_verified_same_selection_clears_only_executed_pass(self):
        old = self.baseline('  FAIL smc-ui\n  ok fastmath\n')
        b = self.root / 'baseline.json'; b.write_text(json.dumps(old))
        t = self.transcript('  ok smc-ui\n  ok fastmath\n')
        result = mod.build_summary(self.root, t, b,
                                   self.record(t, {'smc-ui': 'ok', 'fastmath': 'ok'}))
        self.assertEqual(result['comparison']['status'], 'COMPARABLE')
        self.assertEqual(result['comparison']['cleared_failures'], ['smc-ui'])

    def test_baseline_only_fail_map_is_unverified(self):
        b = self.root / 'baseline.json'; b.write_text('{"failures":{"smc-ui":{}}}')
        t = self.transcript('  ok smc-ui\n')
        result = mod.build_summary(self.root, t, b,
                                   self.record(t, {'smc-ui': 'ok'}))
        self.assertEqual(result['comparison']['status'], 'UNVERIFIED_BASELINE')
        self.assertEqual(result['comparison']['cleared_failures'], [])

    def test_unbound_transcript_never_confirms_old_failure_cleared(self):
        old = self.baseline('  FAIL smc-ui\n')
        b = self.root / 'baseline.json'; b.write_text(json.dumps(old))
        t = self.transcript('  ok smc-ui\n')
        report = mod.build_summary(self.root, t, b)
        self.assertEqual(report['comparison']['status'], 'UNVERIFIED_PROVENANCE')
        self.assertFalse(report['source_verified'])
        self.assertEqual(report['comparison']['cleared_failures'], [])

    def test_stale_transcript_checked_out_at_other_head_rejected(self):
        t = self.transcript('  ok smc-ui\n')
        proof = self.record(t, {'smc-ui': 'ok'}, head=HEAD_B)
        with self.assertRaisesRegex(ValueError, 'different Git HEAD'):
            mod.build_summary(self.root, t, run_record=proof)

    def test_missing_clean_source_attestation_rejected(self):
        t = self.transcript('  ok smc-ui\n')
        record = self.record(t, {'smc-ui': 'ok'})
        del record['source_clean_before_after']
        with self.assertRaisesRegex(ValueError, 'clean-source pre/post-run'):
            mod.build_summary(self.root, t, run_record=record)

    def test_dirty_source_report_rejected(self):
        t = self.transcript('  ok smc-ui\n')
        record = self.record(t, {'smc-ui': 'ok'})
        with patch.object(mod, 'assert_clean_source',
                          side_effect=ValueError('Source worktree/index is dirty')):
            with self.assertRaisesRegex(ValueError, 'dirty'):
                mod.build_summary(self.root, t, run_record=record)

    def test_changed_tracked_host_test_rejected_by_real_git_status(self):
        repo = self.root / 'fixture'
        repo.mkdir()
        tests = repo / 'tests'
        tests.mkdir()
        tracked = tests / 'host_test.py'
        tracked.write_text('print("committed")\n')
        subprocess.run(['git', '-C', str(repo), 'init', '-q'], check=True)
        subprocess.run(['git', '-C', str(repo), 'add', '.'], check=True)
        subprocess.run(['git', '-C', str(repo), '-c', 'user.name=Fixture',
                        '-c', 'user.email=fixture@example.invalid',
                        'commit', '-qm', 'Create fixture'], check=True)
        REAL_SOURCE_CHECK(repo)
        tracked.write_text('print("uncommitted change")\n')
        with self.assertRaisesRegex(ValueError, 'Source worktree/index is dirty'):
            REAL_SOURCE_CHECK(repo)
        t = self.transcript('  ok smc-ui\n')
        record = self.record(t, {'smc-ui': 'ok'})
        with patch.object(mod, 'assert_clean_source',
                          side_effect=lambda root=None: REAL_SOURCE_CHECK(repo)):
            with self.assertRaisesRegex(ValueError, 'Source worktree/index is dirty'):
                mod.build_summary(self.root, t, run_record=record)

    def test_missing_source_root_rejected(self):
        t = self.transcript('  ok smc-ui\n')
        record = self.record(t, {'smc-ui': 'ok'})
        del record['root']
        with self.assertRaisesRegex(ValueError, 'valid source root'):
            mod.build_summary(self.root, t, run_record=record)

    def test_dirty_record_root_rejected_from_clean_cwd(self):
        # The check must use the recorded root, not the current directory.
        repo = self.root / 'fixture-root'
        (repo / 'tests').mkdir(parents=True)
        tracked = repo / 'tests' / 'host_test.py'
        tracked.write_text('print("committed")\n')
        subprocess.run(['git', '-C', str(repo), 'init', '-q'], check=True)
        subprocess.run(['git', '-C', str(repo), 'add', '.'], check=True)
        subprocess.run(['git', '-C', str(repo), '-c', 'user.name=Fixture',
                        '-c', 'user.email=fixture@example.invalid',
                        'commit', '-qm', 'Create fixture'], check=True)
        head = subprocess.check_output(['git', '-C', str(repo), 'rev-parse', 'HEAD'],
                                       text=True).strip()
        tracked.write_text('print("uncommitted change")\n')
        t = self.transcript('  ok smc-ui\n')
        record = self.record(t, {'smc-ui': 'ok'}, head=head)
        record['root'] = str(repo)
        self.git.stop()
        self.clean.stop()
        try:
            with self.assertRaisesRegex(ValueError, 'Source worktree/index is dirty'):
                mod.build_summary(self.root, t, run_record=record)
        finally:
            self.git.start()
            self.clean.start()

    def test_untracked_host_test_rejected_by_real_git_status(self):
        repo = self.root / 'fixture-untracked'
        repo.mkdir()
        subprocess.run(['git', '-C', str(repo), 'init', '-q'], check=True)
        (repo / 'unexpected_test.py').write_text('print("untracked")\n')
        with self.assertRaisesRegex(ValueError, 'Source worktree/index is dirty'):
            REAL_SOURCE_CHECK(repo)

    def test_modified_transcript_rejected(self):
        t = self.transcript('  ok smc-ui\n')
        proof = self.record(t, {'smc-ui': 'ok'})
        t.write_text('  FAIL smc-ui\n')
        with self.assertRaisesRegex(ValueError, 'transcript SHA-256 mismatch'):
            mod.build_summary(self.root, t, run_record=proof)

    def test_tampered_plan_rejected(self):
        t = self.transcript('  ok smc-ui\n')
        proof = self.record(t, {'smc-ui': 'ok'})
        proof['selected_tests'] = ['smc-ui', 'missing']
        with self.assertRaisesRegex(ValueError, 'selected tests differ'):
            mod.build_summary(self.root, t, run_record=proof)

    def test_inconsistent_success_exit_status_rejected(self):
        t = self.transcript('  FAIL smc-ui\n')
        proof = self.record(t, {'smc-ui': 'FAIL'}, rc=0)
        with self.assertRaisesRegex(ValueError, 'exit status'):
            mod.build_summary(self.root, t, run_record=proof)


if __name__ == '__main__':
    unittest.main()
