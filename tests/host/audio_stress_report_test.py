#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The report must keep maxima, account for every block and refuse target claims."""
import importlib.util
import csv
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('stress_report', ROOT/'tools/audio_stress_report.py')
report = importlib.util.module_from_spec(spec)
spec.loader.exec_module(report)


class StressReportTest(unittest.TestCase):
    def rows(self):
        return [dict(block=str(i), cpu_ns=str(cpu), wall_ns=str(cpu+100),
                     chicago_cpu_ns=str(chicago), miami_cpu_ns='1000000',
                     ui_cpu_ns='900000')
                for i, cpu, chicago in [(0, 1000000, 0), (1, 12000000, 2000000)]]

    def test_peak_cannot_be_hidden_by_the_average(self):
        result = report.summarize(self.rows())
        self.assertEqual(result['render_cpu']['peak_ns'], 12000000)
        self.assertEqual(result['render_cpu']['average_ns'], 6500000)
        self.assertEqual(result['render_cpu']['worst_block'], 1)
        self.assertEqual(result['render_cpu']['over_deadline_blocks'], 1)
        self.assertLess(result['render_cpu']['margin_ns'], 0)
        self.assertFalse(result['host_timing_within_requested_limits'])

    def test_engine_budget_is_fifteen_percent_of_the_half(self):
        result = report.summarize(self.rows())
        self.assertEqual(result['chicago_cpu']['peak_ns'], 2000000)
        self.assertEqual(result['chicago_cpu']['over_limit_blocks'], 1)
        self.assertEqual(result['miami_cpu']['over_limit_blocks'], 0)

    def test_no_host_result_is_target_or_dropout_evidence(self):
        rows = self.rows()[:1]
        rows[0]['miami_cpu_ns'] = '0'
        result = report.summarize(rows)
        self.assertTrue(result['host_timing_within_requested_limits'])
        self.assertEqual(result['target_acceptance'], 'NOT_RUN')
        self.assertIsNone(result['hardware_dropouts'])

    def test_missing_duplicate_or_invalid_blocks_are_rejected(self):
        for rows in [[], self.rows()[1:], self.rows()*2]:
            with self.assertRaises(ValueError):
                report.summarize(rows)
        for value in ['nan', 'inf', '-1', 'oops']:
            rows = self.rows()
            rows[1]['cpu_ns'] = value
            with self.assertRaises(ValueError):
                report.summarize(rows)

    def test_cli_enforces_budget_and_keeps_target_unrun(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            csv_path, output = root/'blocks.csv', root/'report.json'
            with csv_path.open('w', newline='') as stream:
                rows = self.rows()
                writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
                writer.writeheader()
                writer.writerows(rows)
            command = [sys.executable, str(ROOT/'tools/audio_stress_report.py'),
                       '--csv', str(csv_path), '--out', str(output)]
            result = subprocess.run(command + ['--require-host-limits'], capture_output=True, text=True)
            self.assertEqual(result.returncode, 1, result.stderr)
            self.assertEqual(json.loads(output.read_text())['target_acceptance'], 'NOT_RUN')
            self.assertIn('host timing limits FAIL', result.stdout)
            pcm = root/'audio.s32'
            pcm.write_bytes(b'bad')
            output.unlink()
            result = subprocess.run(command + ['--pcm', str(pcm)], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('PCM length must be', result.stderr)
            self.assertFalse(output.exists())


if __name__ == '__main__':
    unittest.main()
