# SPDX-License-Identifier: GPL-3.0-only
"""Checks for the opt-in host gate-envelope and velocity model.

The synthetic cases do not use Chicago audio. The measurement case renders
bass303 and scores the approved derived reference. It does not change firmware.
"""
import hashlib
import subprocess
import tempfile
import unittest
from pathlib import Path

import numpy as np

import importlib.util

DIR = Path(__file__).resolve().parent
REPO = DIR.parents[1]


def load_gate():
    spec = importlib.util.spec_from_file_location("audio303_gate_amplitude", DIR / "gate_amplitude.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


GATE = load_gate()


class ImportAndSplit(unittest.TestCase):
    def test_import_does_not_need_a_renderer(self):
        self.assertTrue(GATE.REFERENCE.name.endswith("approved-amplitude-reference.json"))
        self.assertEqual(GATE.BASS303_SHA256, "321bcc141e6f234d098eb5180f47eae1afc3b6195fd08ecb5dbe54ccb8cd0a1f")

    def test_training_and_heldout_are_disjoint(self):
        train = set(GATE.TRAIN_CUTOFF_IDS) | {GATE.VELOCITY_TRAIN_ID}
        held = set(GATE.HELDOUT_TEMPORAL_IDS) | set(GATE.HISTORY_IDS) | set(GATE.REGRESSION_IDS)
        self.assertFalse(train & held)
        self.assertIn("cut0.6-n60-r0", held)
        self.assertIn("vel40-r0", train)
        self.assertNotIn("vel40-r0", GATE.HELDOUT_TEMPORAL_IDS)

    def test_slope_fit_refuses_a_heldout_id(self):
        audio = np.ones(44100)
        chicago = {item: [0.1] * 20 for item in GATE.TRAIN_CUTOFF_IDS}
        with self.assertRaises(ValueError):
            GATE.fit_slope(audio, 0, chicago, ("cut0.6-n60-r0",))


class MetricRules(unittest.TestCase):
    def test_identical_contours_have_zero_shape_error(self):
        env = [0.01, 0.02, 0.03, 0.025, 0.02, 0.018, 0.017, 0.016, 0.015, 0.01]
        score, excluded = GATE.temporal_rmse(env, env)
        self.assertEqual(excluded, [])
        self.assertAlmostEqual(score, 0.0, places=9)

    def test_low_energy_bin_is_excluded_not_dropped_from_the_row(self):
        ref = [0.1] * 12
        model = [0.1] * 12
        model[6] = 1e-12
        score, excluded = GATE.temporal_rmse(ref, model)
        self.assertEqual(excluded, [6])
        self.assertIsNotNone(score)

    def test_too_many_low_energy_bins_null_the_score(self):
        ref = [1e-12] * 12
        score, excluded = GATE.temporal_rmse(ref, ref)
        self.assertIsNone(score)
        self.assertEqual(excluded, list(range(2, 9)))

    def test_shape_time_follows_note_off_slide_and_retrigger(self):
        isolated = GATE.shape_time(44100, 0)
        self.assertEqual(isolated[0], 0.0)
        self.assertAlmostEqual(isolated[22049], 22049 / 44100)
        self.assertAlmostEqual(isolated[22050], 0.5)
        repeat = GATE.shape_time(88200, 1)
        self.assertEqual(repeat[44100], 0.0)
        self.assertAlmostEqual(repeat[66150], 0.5)
        slide = GATE.shape_time(44100, 2)
        self.assertAlmostEqual(slide[11025], 11025 / 44100)
        self.assertAlmostEqual(slide[33075], 33075 / 44100)
        retrigger = GATE.shape_time(44100, 3)
        self.assertEqual(retrigger[11025], 0.0)
        self.assertAlmostEqual(retrigger[33074], (33074 - 11025) / 44100)
        self.assertAlmostEqual(retrigger[33075], (33075 - 11025) / 44100)


class HostRenderer(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.renderer = Path(cls.tmp.name) / "production303"
        cls.compile_cmd, code = GATE.compile_renderer(cls.renderer)
        if code != 0:
            raise RuntimeError("renderer failed to compile")
        cls.report = GATE.build_report(cls.renderer, work_dir=Path(cls.tmp.name) / "audio")

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_firmware_source_is_unchanged(self):
        digest = hashlib.sha256((REPO / "firmware/src/dsp/bass303.c").read_bytes()).hexdigest()
        self.assertEqual(digest, GATE.BASS303_SHA256)
        diff = subprocess.run(["git", "diff", "--quiet", "HEAD", "--", "firmware"], cwd=REPO)
        self.assertEqual(diff.returncode, 0)

    def test_default_renders_match_each_other_and_the_reference(self):
        row = GATE.index_rows(GATE.load_reference())["cut0.6-n48-r0"]
        first = Path(self.tmp.name) / "a.f32"
        second = Path(self.tmp.name) / "b.f32"
        GATE.render_row(self.renderer, first, row)
        GATE.render_row(self.renderer, second, row)
        self.assertEqual(first.read_bytes(), second.read_bytes())
        self.assertLess(self.report["aggregates"]["dsp_agreement_max_temporal_rmse_db"], 1e-6)
        self.assertTrue(self.report["determinism"]["cut0.6-n48_r0_r1_byte_equal"])
        self.assertTrue(self.report["determinism"]["velocity40-n60_r0_r1_byte_equal"])

    def test_bad_gate_arguments_are_rejected(self):
        out = Path(self.tmp.name) / "bad.f32"
        base = [str(self.renderer), str(out), "4410", "48", "100", "64", "0", "0", "0", "1", "0", "64", "127", "0", "21", "0", "0", "0", "0"]
        missing = subprocess.run(base + ["--experimental-gate-model", "0", "0"], capture_output=True)
        self.assertEqual(missing.returncode, 10)
        wide = subprocess.run(base + ["--experimental-gate-model", "0", "0", "80"], capture_output=True)
        self.assertEqual(wide.returncode, 11)
        unknown = subprocess.run(base + ["--nope"], capture_output=True)
        self.assertEqual(unknown.returncode, 8)
        slide = subprocess.run(base + ["--experimental-slide-tau-ms", "120"], capture_output=True)
        self.assertEqual(slide.returncode, 0)

    def test_python_law_matches_the_host_flag(self):
        for item in self.report["python_vs_c"].values():
            self.assertLess(item["temporal_rmse_db"], 0.02)
            self.assertLess(abs(item["level_delta_db"]), 0.02)

    def test_heldout_shape_passes_and_velocity_margin_fails(self):
        report = self.report
        self.assertEqual(report["status"], "REJECTED")
        agg = report["aggregates"]
        self.assertGreaterEqual(agg["heldout_temporal_improvement_db"], GATE.MATERIAL_TEMPORAL_DB)
        self.assertGreater(agg["heldout_transfer_improvement_db"], 0.0)
        self.assertLess(agg["heldout_transfer_improvement_db"], GATE.MATERIAL_TRANSFER_DB)
        self.assertAlmostEqual(report["parameters"]["slope_db_per_s"], 8.23, places=2)
        points = dict(zip(
            report["heldout_transfer_points_db"]["ids"],
            report["heldout_transfer_points_db"]["model_minus_chicago"],
        ))
        baseline = dict(zip(
            report["heldout_transfer_points_db"]["ids"],
            report["heldout_transfer_points_db"]["baseline_minus_chicago"],
        ))
        self.assertLess(abs(points["vel80-r0"]), abs(baseline["vel80-r0"]))
        self.assertLess(abs(points["vel127-r0"]), abs(baseline["vel127-r0"]))
        self.assertGreater(abs(points["velocity80-n60-r0"]), abs(baseline["velocity80-n60-r0"]))
        self.assertGreater(abs(points["velocity127-n60-r0"]), abs(baseline["velocity127-n60-r0"]))
        self.assertEqual(report["model_nonfinite_samples"], 0)
        self.assertFalse(report["fit"]["slope_on_search_boundary"])
        for row in report["rows"]:
            if row["role"] == "determinism":
                continue
            self.assertEqual(row["excluded_primary_bins_model"], [])
            self.assertEqual(row["model_clipped_samples"], 0)
        attack = next(item for item in report["regressions"] if item["id"] == "attack-r0")
        self.assertGreater(attack["temporal_delta_db"], 1.0)

    def test_velocity_training_error_is_closed_and_anchor_gain_is_close(self):
        agg = self.report["aggregates"]
        self.assertLess(agg["train_velocity40_transfer_error_model_db"], 0.02)
        self.assertLess(agg["anchor_absolute_model_db"], 0.5)


if __name__ == "__main__":
    unittest.main(verbosity=2)
