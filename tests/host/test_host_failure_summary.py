#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host-suite failure reporter regression, using synthetic logs only."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('host_failure_summary', ROOT / 'host_failure_summary.py')
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


class FailureSummaryTests(unittest.TestCase):
    def test_parallel_results(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / '.results').mkdir()
            (root / '.results/0001').write_text('  ok fastmath\n')
            (root / '.results/0002').write_text('  FAIL smc-ui (see build/host/smc-ui.log)\n')
            (root / '.results/0002.fail').touch()
            (root / '.results/0002.time').write_text('1 s smc-ui\n')
            (root / 'smc-ui.log').write_text('failed compilation\nfatal error: header missing\n')
            r = mod.build_summary(root)
            self.assertEqual(r['counts'], {'ok': 1, 'FAIL': 1, 'not-run': 0})
            self.assertIn('fatal error:', r['failures']['smc-ui']['error'])

    def test_serial_transcript_ignores_stale_parallel_results(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / '.results').mkdir()
            (root / '.results/0002').write_text('  FAIL stale\n')
            transcript = root / 'full.log'
            transcript.write_text('  ok fastmath\n  FAIL sound-matrix (see build/host/sound-matrix.log)\n     extra detail\n')
            (root / 'sound-matrix.log').write_text('AssertionError: bad colour\n')
            r = mod.build_summary(root, transcript)
            self.assertEqual(list(r['failures']), ['sound-matrix'])
            self.assertEqual(r['counts']['ok'], 1)

    def test_plain_compiler_error(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            transcript = root / 'run.log'
            transcript.write_text('  FAIL compiler\n')
            (root / 'compiler.log').write_text('clang: error: this symbol is unknown\n')
            report = mod.build_summary(root, transcript)
            self.assertIn('error:', report['failures']['compiler']['error'])

    def test_missing_logs_not_relabelled(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            transcript = root / 'run.log'
            transcript.write_text('  FAIL test-a\n')
            report = mod.build_summary(root, transcript)
            self.assertEqual(report['counts']['FAIL'], 1)
            self.assertEqual(report['failures']['test-a']['error'], 'LOG MISSING')

    def test_baseline_comparison(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            transcript = root / 'run.log'
            transcript.write_text('  FAIL kept\n  FAIL new\n')
            baseline = root / 'baseline.json'
            baseline.write_text('{"failures": {"kept": {}, "old": {}}}')
            report = mod.build_summary(root, transcript, baseline)
            self.assertEqual(report['comparison'], {
                'new_failures': ['new'], 'preexisting_failures': ['kept'], 'cleared_failures': ['old']})


    def test_nested_diagnostics_do_not_create_extra_failures(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            transcript = root / 'run.log'
            transcript.write_text(
                '  ok first\\n  FAIL actual (see build/host/actual.log)\\n'
                '       FAIL fake-from-test-output\\n'
                '       ok fake-pass\\n'
                '       not-run fake-skip\\n'
            .replace('\\n', '\n'))
            (root / 'actual.log').write_text('AssertionError: real failure\\n'.replace('\\n', '\n'))
            report = mod.build_summary(root, transcript)
            self.assertEqual(report['counts'], {'ok': 1, 'FAIL': 1, 'not-run': 0})
            self.assertEqual(set(report['failures']), {'actual'})

    def test_parallel_results_ignore_indented_tail(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / '.results').mkdir()
            (root / '.results/0001').write_text(
                '  FAIL actual (see build/host/actual.log)\\n'
                '       FAIL fake-from-diagnostic\\n'.replace('\\n', '\n'))
            (root / 'actual.log').write_text('AssertionError: real failure\\n'.replace('\\n', '\n'))
            report = mod.build_summary(root)
            self.assertEqual(report['counts']['FAIL'], 1)
            self.assertEqual(set(report['failures']), {'actual'})

    def test_no_status_is_error(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / '.results').mkdir()
            with self.assertRaisesRegex(ValueError, 'No suite results'):
                mod.build_summary(root)


if __name__ == '__main__':
    unittest.main()
