"""Synthetic receipt/CSV checks; these tests do not execute or qualify firmware."""
import copy
import csv
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).parent))
from test_scene import declaration, matching, summary
from scene import ACTIVITY_METADATA
import isolation

TRACE_HASH = hashlib.sha256(b"synthetic trace identity for isolation unit tests").hexdigest()


def isolated(activity=None):
    value = matching()
    value["scene_a_usb_keys"] = [0] * 128
    value["scene_b_usb_keys"] = [0] * 128
    value["scene_909_hit"] = [0] * 88
    if activity is not None:
        value["scene_808_active"] = [activity]
    return value


def row(sequence=100, activity=None):
    return {"dma_seq": str(sequence), "coherent": "1", "scene_before": isolated(activity),
            "scene_after": isolated(activity), "break_loaded": "1", "break_mix_nonzero_writes": "512",
            "break_mix_peak_delta": "0.125", "break_mix_invalid": "0"}


def receipts(count=1, activity=False):
    trace = summary(count, start=20000, end=29000)
    trace.update(trace_sha256=TRACE_HASH, runner_source=isolation.RUNNER_SOURCE,
                 clock={"hz": isolation.HZ, "unit": "emulator_tick", "cycle_ticks": 16},
                 break_evidence={"loaded": "OBSERVED", "mix_contribution": "OBSERVED",
                                 "contributing_halves": count, "invalid_mix_halves": 0})
    if activity:
        trace["scene_observations"]["fields"].update(ACTIVITY_METADATA)
    pcm = {"status": "OBSERVED", "numeric_output": "NONZERO", "trace_sha256": TRACE_HASH,
           "pcm_sha256": hashlib.sha256(b"synthetic full PCM identity").hexdigest(),
           "window_pcm_sha256": hashlib.sha256(b"synthetic window PCM identity").hexdigest(),
           "window": {"start_ms": 20000, "end_ms": 29000},
           "clock": {"hz": isolation.HZ, "unit": "emulator_tick", "cycle_ticks": 16},
           "source_profile": {"firmware_source": isolation.FIRMWARE_SOURCE, "runner_source": isolation.RUNNER_SOURCE},
           "samples": {"scalar_samples": 1024 * count, "nonzero_samples": 900 * count,
                       "peak_absolute_int16": 16384}}
    return trace, pcm


class IsolationTests(unittest.TestCase):
    def check(self, rows=None, trace=None, pcm=None, *, bind_digest=True, raw_csv=None):
        rows = [row()] if rows is None else rows
        default_trace, default_pcm = receipts(len(rows))
        trace = default_trace if trace is None else copy.deepcopy(trace)
        pcm = default_pcm if pcm is None else copy.deepcopy(pcm)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "per_half.csv"
            if raw_csv is not None:
                path.write_bytes(raw_csv)
            else:
                with path.open("w", newline="") as output:
                    writer = csv.DictWriter(output, fieldnames=list(row()))
                    writer.writeheader()
                    for value in rows:
                        value = dict(value)
                        for name in ("scene_before", "scene_after"):
                            value[name] = json.dumps(value[name])
                        writer.writerow(value)
            if bind_digest:
                trace["per_half_csv_sha256"] = hashlib.sha256(path.read_bytes()).hexdigest()
            return isolation.check_break_only(path, declaration(), trace, pcm)

    def test_exact_derivation_removes_only_the_eight_909_toggle_lines(self):
        full = (Path(__file__).parent / "representative" / "scene.keys").read_bytes()
        derived = isolation.break_only_keys(full)
        self.assertEqual(hashlib.sha256(derived).hexdigest(),
                         "8d9adb5687d5af1cfe37231e527e2786b79aa2d8cf9c2fd968535594f8ac853b")
        self.assertEqual(len(full) - len(derived), 98)
        expected_lines = [line for line in full.splitlines(keepends=True)
                          if not line.startswith((b"8000:", b"8100:"))]
        self.assertEqual(derived, b"".join(expected_lines))
        self.assertIn(b"9200:k5@127,\n", derived)  # BREAK programming remains unchanged.
        self.assertIn(b"18500:SEQ PLAY,\n", derived)

    def test_unknown_or_already_derived_script_is_rejected(self):
        full = (Path(__file__).parent / "representative" / "scene.keys").read_bytes()
        for value in (b"", full + b"\n", full.replace(b"8000", b"8001"), isolation.break_only_keys(full), bytearray(full)):
            with self.subTest(bytes=len(value)), self.assertRaisesRegex(ValueError, "SHA256"):
                isolation.break_only_keys(value)

    def test_positive_witness_is_conditional_partial_and_not_full_scene_attribution(self):
        result = self.check()
        self.assertEqual(result["status"], "NUMERICAL_BREAK_OUTPUT_OBSERVED")
        self.assertEqual(result["checked_boundaries"], 2)
        self.assertTrue(result["partial_duration"])
        self.assertEqual(result["observed_window"], {"start_ms": 20000, "end_ms": 29000})
        self.assertFalse(result["input_isolation"]["verified_by_this_function"])
        self.assertEqual(result["human_audibility"], "UNVERIFIED")
        self.assertEqual(result["full_scene_break_dac_mix"], "NOT_PROVEN")
        self.assertFalse(result["representative_scene_qualified"])

    def test_optional_808_activity_is_preserved_and_nonzero_rejected(self):
        trace, pcm = receipts(activity=True)
        self.assertEqual(self.check([row(activity=0)], trace, pcm)["status"], "NUMERICAL_BREAK_OUTPUT_OBSERVED")
        self.assertEqual(self.check([row(activity=1)], trace, pcm)["status"], "NOT_OBSERVED")

    def test_any_synth_key_or_909_hit_rejects_the_isolated_run(self):
        for name, index in (("scene_a_usb_keys", 48), ("scene_b_usb_keys", 64), ("scene_909_hit", 0), ("scene_909_hit", 87)):
            value = row(); value["scene_after"][name][index] = 1
            with self.subTest(name=name, index=index):
                self.assertEqual(self.check([value])["status"], "NOT_OBSERVED")

    def test_other_required_scene_conditions_cannot_be_ignored(self):
        for name, replacement in (("scene_a_kind", [1]), ("scene_b_status", [0]),
                                  ("scene_brk_steps", [0, 0, 0, 0]), ("scene_seq_playing", [0])):
            value = row(); value["scene_before"][name] = replacement
            with self.subTest(name=name):
                self.assertEqual(self.check([value])["status"], "NOT_OBSERVED")
        value = row(); value["scene_before"]["scene_808_hit"][0] = 1
        self.assertEqual(self.check([value])["status"], "NOT_OBSERVED")

    def test_all_boundaries_are_checked_beyond_full_scene_failure_example_limit(self):
        rows = [row(sequence=i + 1) for i in range(25)]
        rows[-1]["scene_after"]["scene_b_kind"] = [0]
        result = self.check(rows)
        self.assertEqual(result["status"], "NOT_OBSERVED")
        self.assertEqual(result["checked_boundaries"], 50)
        self.assertEqual(result["failing_boundaries"], 1)
        self.assertEqual(result["failures"][0]["dma_seq"], 25)

    def test_missing_loaded_evidence_zero_contribution_or_invalid_mix_rejected(self):
        for name, value in (("break_loaded", "0"), ("break_mix_nonzero_writes", "0"),
                            ("break_mix_peak_delta", "0"), ("break_mix_invalid", "1")):
            sample = row(); sample[name] = value
            with self.subTest(name=name):
                self.assertEqual(self.check([sample])["status"], "NOT_OBSERVED")
        trace, pcm = receipts(); trace["break_evidence"].pop("loaded")
        self.assertEqual(self.check(trace=trace, pcm=pcm)["status"], "NOT_OBSERVED")

    def test_pcm_from_another_trace_or_window_cannot_be_associated(self):
        for field, value in (("trace_sha256", "0" * 64), ("window_pcm_sha256", None)):
            trace, pcm = receipts(); pcm[field] = value
            with self.subTest(field=field):
                self.assertEqual(self.check(trace=trace, pcm=pcm)["status"], "INCOMPLETE")
        trace, pcm = receipts(); pcm["window"]["end_ms"] = 30000
        self.assertEqual(self.check(trace=trace, pcm=pcm)["status"], "INCOMPLETE")

    def test_zero_or_unobserved_pcm_cannot_be_a_break_output_witness(self):
        for field, value in (("nonzero_samples", 0), ("peak_absolute_int16", 0), ("scalar_samples", 0)):
            trace, pcm = receipts(); pcm["samples"][field] = value
            with self.subTest(field=field):
                self.assertEqual(self.check(trace=trace, pcm=pcm)["status"], "NOT_OBSERVED")
        trace, pcm = receipts(); pcm["status"] = "NO_SAMPLES"
        self.assertEqual(self.check(trace=trace, pcm=pcm)["status"], "NOT_OBSERVED")

    def test_csv_digest_and_trace_status_are_required(self):
        trace, pcm = receipts(); trace["per_half_csv_sha256"] = "0" * 64
        self.assertEqual(self.check(trace=trace, pcm=pcm, bind_digest=False)["status"], "INCOMPLETE")
        trace, pcm = receipts(); trace["status"] = "INCOMPLETE"
        self.assertEqual(self.check(trace=trace, pcm=pcm)["status"], "INCOMPLETE")

    def test_malformed_csv_or_nonfinite_mix_never_produces_a_witness(self):
        self.assertEqual(self.check(raw_csv=b"dma_seq\n1\n")["status"], "INCOMPLETE")
        value = row(); value["break_mix_peak_delta"] = "NaN"
        self.assertEqual(self.check([value])["status"], "INCOMPLETE")
        value = row(); value["scene_after"]["scene_a_usb_keys"] = [0]
        self.assertNotEqual(self.check([value])["status"], "NUMERICAL_BREAK_OUTPUT_OBSERVED")


if __name__ == "__main__":
    unittest.main()
