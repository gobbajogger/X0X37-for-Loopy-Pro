#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Measurement orchestration contracts; no firmware build or render required."""
import importlib.util
import json
import tempfile
import unittest
import wave
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("live_a_mix_report", ROOT / "tools/live_a_mix_report.py")
report = importlib.util.module_from_spec(spec)
spec.loader.exec_module(report)


class MeasurementContracts(unittest.TestCase):
    def test_generator_compiler_invocations_preserve_arguments_and_return_code(self):
        arguments = ["-O2", "-ffp-contract=off", "-w", "-Ifirmware/src", "-Ibuild/gen",
                     "-o", "build/host/gen_builtin_break.tmp", "tools/gen_builtin_break.c",
                     "firmware/src/dsp/drum909.c", "firmware/src/dsp/x0x_param.c", "-lm"]
        with tempfile.TemporaryDirectory() as directory:
            root, out = Path(directory) / "source", Path(directory) / "adapter"
            for returncode in (0, 23):
                with self.subTest(returncode=returncode), \
                        patch.object(report.subprocess, "run", return_value=SimpleNamespace(returncode=returncode)) as run, \
                        patch.object(report, "source_snapshot", side_effect=AssertionError("generator must not snapshot")) as snapshot:
                    result = report.compile_host(root, out, "ccache clang", arguments)
                    self.assertEqual(result, returncode)
                    run.assert_called_once_with(["ccache", "clang", *arguments], cwd=root, check=False)
                    snapshot.assert_not_called()
                    self.assertFalse(out.exists(), "generator must not create adapter output")

    def test_host_compiler_invocations_reject_duplicate_host_or_malformed_output(self):
        invalid = [
            ["host/x0x_host.c", "host/x0x_host.c", "-o", "build/host/x0x_host"],
            ["host/x0x_host.c"],
            ["host/x0x_host.c", "-o"],
            ["host/x0x_host.c", "-o", "-lm"],
            ["host/x0x_host.c", "-o", ""],
            ["host/x0x_host.c", "-o", "first", "-o", "second"],
            ["-o", "host/x0x_host.c"],
        ]
        with tempfile.TemporaryDirectory() as directory:
            root, out = Path(directory) / "source", Path(directory) / "adapter"
            for arguments in invalid:
                with self.subTest(arguments=arguments), \
                        patch.object(report.subprocess, "run") as run, \
                        patch.object(report, "source_snapshot") as snapshot:
                    with self.assertRaisesRegex(ValueError, "compiler invocation"):
                        report.compile_host(root, out, "cc", arguments)
                    run.assert_not_called()
                    snapshot.assert_not_called()
                    self.assertFalse(out.exists())

    def test_jobs_cover_solos_factory_mix_and_each_bank_in_each_slot(self):
        jobs = report.jobs()
        names = [job["name"] for job in jobs]
        self.assertEqual(len(names), len(set(names)))
        for part in ("909", "808", "303A", "303B", "BREAK"):
            self.assertIn(part, names)
        self.assertIn("MIX", names)
        for slot in (0, 1):
            for bank in (0, 1):
                rows = [job for job in jobs if job.get("slot") == slot and job.get("bank") == bank]
                self.assertEqual({job["mode"] for job in rows}, {"solo", "mix"})
                for job in rows:
                    self.assertEqual(job["preset"], 0)
                    part = "303A" if slot == 0 else "303B"
                    self.assertIn(part, job["kwargs"]["play"])
                    self.assertNotIn("303B" if slot == 0 else "303A", job["kwargs"]["play"])

    def test_bank_fixture_keeps_matching_script_and_raw_solo_policy(self):
        calls = []
        def script(wav, **kwargs):
            calls.append((wav, kwargs))
            return "wait 300\ntap PLAY\nwav fixture.wav\nwait 15360\nwavstop\ntap PLAY\n"
        mix = SimpleNamespace(script=script)
        for job in report.jobs():
            if "slot" not in job:
                continue
            text = report.make_script(mix, job, 8)
            self.assertTrue(text.startswith("load 0\n"))
            self.assertIn("expect playing 1\nwav fixture.wav", text)
            self.assertIn("expect tempo 125\n", text)
            self.assertIn("expect ppat3 1\n", text)
            self.assertEqual(calls[-1][1]["raw"], job["mode"] == "solo")
            self.assertEqual(calls[-1][1]["bars"], 8)
            self.assertIn("# matched factory A line in both slots", text)

    def test_analyser_delegates_unchanged_and_refuses_wrong_wav_format(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "tone.wav"
            with wave.open(str(path), "wb") as wav:
                wav.setnchannels(2)
                wav.setsampwidth(2)
                wav.setframerate(44100)
                wav.writeframes(b"\x01\x00\x01\x00" * 20)
            result = {"lu": -20.0, "peak": -4.0}
            calls = []
            mix = SimpleNamespace(analyse=lambda value: calls.append(value) or result)
            self.assertIs(report.analyse_wav(mix, path), result)
            self.assertEqual(calls, [path])
            with wave.open(str(path), "wb") as wav:
                wav.setnchannels(1)
                wav.setsampwidth(2)
                wav.setframerate(48000)
                wav.writeframes(b"\x01\x00" * 20)
            with self.assertRaisesRegex(ValueError, "44.1 kHz"):
                report.analyse_wav(mix, path)
            self.assertEqual(len(calls), 1)

    def test_runtime_evidence_refuses_missing_or_wrong_admitted_bank(self):
        state = {"phase": "start", "slot": 1, "bank": 0, "preset": 0,
                 "requested_bank": 0, "requested_preset": 0, "status": 1,
                 "pending": 0, "edited": 0, "sound_bank": 0,
                 "engine_id": 0x4F434831, "source_preset": 0, "matched": 1,
                 "line_hash": "12345678"}
        log = "\n".join("LIVE_A_STATE " + json.dumps(dict(state, phase=phase, slot=slot))
                        for phase in ("start", "end") for slot in (0, 1))
        job = {"slot": 1, "bank": 0, "preset": 0}
        self.assertEqual(len(report.verify_runtime(log, job)), 4)
        with self.assertRaisesRegex(ValueError, "runtime"):
            report.verify_runtime("", job)
        bad = log.replace('"bank": 0', '"bank": 1')
        with self.assertRaisesRegex(ValueError, "bank"):
            report.verify_runtime(bad, job)
        for field, value in (("pending", 2), ("status", -1), ("edited", 1), ("preset", 1)):
            bad = log.replace(json.dumps(field) + ": " + str(state[field]),
                              json.dumps(field) + ": " + str(value))
            with self.subTest(field=field), self.assertRaisesRegex(ValueError, "preset/status"):
                report.verify_runtime(bad, job)
        with self.assertRaisesRegex(ValueError, "musical fixture"):
            report.verify_runtime(log.replace('"matched": 1', '"matched": 0'), job)

    def test_comparison_matches_rows_and_refuses_changed_fixture_or_missing_rows(self):
        before = {"schema": 1, "fixture": {"bars": 8, "samples": {}}, "rows": [
            {"name": "303A Chicago solo", "fixture_hash": "same", "metrics": {"lu": -20, "peak": -5}}]}
        after = {"schema": 1, "fixture": before["fixture"], "rows": [
            {"name": "303A Chicago solo", "fixture_hash": "same", "metrics": {"lu": -19, "peak": -4.5}}]}
        rows = report.compare(before, after)
        self.assertEqual(rows[0]["delta_lu"], 1)
        self.assertEqual(rows[0]["delta_peak"], 0.5)
        after["rows"][0]["runtime"] = [{"bank": 1}]
        with self.assertRaisesRegex(ValueError, "runtime"):
            report.compare(before, after)
        del after["rows"][0]["runtime"]
        after["rows"][0]["fixture_hash"] = "different"
        with self.assertRaisesRegex(ValueError, "fixture"):
            report.compare(before, after)
        after["rows"] = []
        with self.assertRaisesRegex(ValueError, "rows"):
            report.compare(before, after)

    def test_report_tables_keep_before_after_labels_and_delta_sign(self):
        rows = [{"name": "303A Chicago solo", "before_lu": -20.0, "after_lu": -21.0,
                 "delta_lu": -1.0, "before_peak": -5.0, "after_peak": -4.0, "delta_peak": 1.0}]
        with tempfile.TemporaryDirectory() as directory:
            report.write_tables(Path(directory), rows, "base -> candidate; deltas = after - before")
            markdown = (Path(directory) / "table.md").read_text()
            self.assertIn("base -> candidate", markdown)
            self.assertIn("left channel only", markdown)
            self.assertIn("-1.000", markdown)
            import csv
            with (Path(directory) / "table.csv").open(newline="") as stream:
                parsed = list(csv.DictReader(stream))
            self.assertEqual(float(parsed[0]["delta_lu"]), -1.0)
            self.assertEqual(float(parsed[0]["delta_peak"]), 1.0)


if __name__ == "__main__":
    unittest.main()
