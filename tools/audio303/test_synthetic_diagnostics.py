# SPDX-License-Identifier: GPL-3.0-only
"""Original synthetic diagnostics for the Chicago host analysis math.

These tones are generated in this file. They are not private captures, and the
tests do not render firmware or claim Chicago equivalence.
"""
import hashlib
import importlib.util
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest
import warnings

import numpy as np

DIR = pathlib.Path(__file__).resolve().parent
REPO = DIR.parents[1]
BASS303_SHA256 = "321bcc141e6f234d098eb5180f47eae1afc3b6195fd08ecb5dbe54ccb8cd0a1f"
SR = 44100
VALID_FEATURE_KEYS = {
    "pitch_fit_at_search_boundary",
    "pitch_interpretation",
    "f0",
    "cents",
    "rms",
    "peak",
    "relative_harmonics_db",
    "envelope50ms",
    "onset_threshold_frame",
    "tail_last100ms_rms",
}


def load_script(filename):
    path = DIR / filename
    spec = importlib.util.spec_from_file_location(f"audio303_{path.stem}", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def nominal_hz(note):
    return 440 * 2 ** ((note - 69) / 12)


def harmonic_tone(frequency, amps, seconds=1.0, phase=0.3):
    """Steady cosine stack at a known fundamental. Built here; not a recording."""
    count = int(seconds * SR)
    t = np.arange(count) / SR
    x = np.zeros(count)
    for partial, amp in enumerate(amps, start=1):
        x += amp * np.cos(2 * np.pi * partial * frequency * t + phase * partial)
    return x


def spectrum(shift):
    base = [0, -10, -12, -14, -18, -22, -28, -45, -50, -55, -60, -70]
    return {"relative_harmonics_db": [base[0]] + [v + shift for v in base[1:]], "rms": 1.0}


class ImportSafety(unittest.TestCase):
    def test_import_does_not_touch_private_files(self):
        for name in ("analyze.py", "contour.py", "slide_experiment.py", "measure.py"):
            code = (
                "import importlib.util, pathlib, sys\n"
                f"path = pathlib.Path({str(DIR / name)!r})\n"
                "spec = importlib.util.spec_from_file_location('probe_' + path.stem, path)\n"
                "module = importlib.util.module_from_spec(spec)\n"
                "spec.loader.exec_module(module)\n"
                "assert callable(getattr(module, 'main', None)) or path.name == 'measure.py'\n"
            )
            done = subprocess.run([sys.executable, "-c", code], cwd="/tmp", capture_output=True, text=True)
            self.assertEqual(done.returncode, 0, done.stderr)
            self.assertEqual(done.stdout, "")

    def test_scripts_share_measure_and_keep_private_io_behind_main(self):
        analyze = load_script("analyze.py")
        contour = load_script("contour.py")
        slide = load_script("slide_experiment.py")
        self.assertIs(analyze.measure, contour.measure)
        self.assertIs(contour.measure, slide.measure)
        source = (DIR / "measure.py").read_text()
        self.assertNotIn("manifest", source)
        self.assertNotIn("evidence", source)
        for name in ("analyze.py", "contour.py", "slide_experiment.py"):
            text = (DIR / name).read_text()
            self.assertIn("def main(", text)
            self.assertIn('if __name__ == "__main__"', text)
        with tempfile.TemporaryDirectory() as td:
            root = pathlib.Path(td)
            for module in (analyze, contour, slide):
                with self.assertRaises(FileNotFoundError):
                    module.main(root)
                self.assertEqual(list(root.rglob("*")), [])


class KnownSpectra(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m = load_script("analyze.py").measure
        cls.amps = [1, 0.7, 0.45, 0.3, 0.2, 0.12, 0.08, 0.05, 0.03, 0.02, 0.015, 0.01]
        cls.note = 48
        cls.f0 = nominal_hz(cls.note)
        cls.tone = harmonic_tone(cls.f0, cls.amps)
        cls.feat = cls.m.features(cls.tone, cls.note)

    def test_pitch_and_relative_harmonics(self):
        expected = 20 * np.log10(np.array(self.amps) / self.amps[0])
        got = np.array(self.feat["relative_harmonics_db"])
        self.assertFalse(self.feat["pitch_fit_at_search_boundary"])
        self.assertLess(abs(self.feat["cents"]), 0.05)
        self.assertLess(abs(self.feat["f0"] - self.f0), 0.02)
        self.assertLess(np.max(np.abs(got - expected)), 0.01)
        self.assertEqual(set(self.feat), VALID_FEATURE_KEYS)
        self.assertGreaterEqual(np.count_nonzero(got[1:] >= -40), 3)

    def test_contour_tracks_the_same_tone(self):
        hz = self.m.contour_hz(self.tone, self.note)
        self.assertEqual(len(hz), 5)
        self.assertTrue(all(abs(value - self.f0) < 0.15 for value in hz))
        sample = self.m.apparent_hz_at(self.tone, 0.5)
        self.assertEqual(set(sample), {"center_seconds", "apparent_hz", "normalized_fit_residual"})
        self.assertLess(sample["normalized_fit_residual"], 0.1)
        note60 = nominal_hz(60)
        higher = self.m.apparent_hz_at(harmonic_tone(note60, self.amps), 0.5, scale=2 ** ((60 - 48) / 12))
        self.assertLess(abs(higher["apparent_hz"] - note60), 0.15)

    def test_trajectory_rmse_cents(self):
        err = self.m.trajectory_rmse_cents([200.0, 200.0], [200.0, 100.0])
        self.assertAlmostEqual(err, 1200 / np.sqrt(2), places=6)


class RejectionAndNulls(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m = load_script("analyze.py").measure

    def test_partial_count_threshold(self):
        def harms(n_strong):
            levels = [0.0] + [-10.0 if i < n_strong else -50.0 for i in range(11)]
            return {"relative_harmonics_db": levels}

        self.assertEqual(self.m.distance(harms(2), harms(2)), 999.0)
        self.assertEqual(self.m.distance(harms(3), harms(3)), 0.0)
        exact = {"relative_harmonics_db": [0, -40, -40, -40, -41, -50, -55, -60, -65, -70, -75, -80]}
        self.assertEqual(self.m.distance(exact, exact), 0.0)

    def test_fitted_sine_rejects_and_json_nulls_the_row(self):
        sine = self.m.features(harmonic_tone(nominal_hz(48), [1.0]), 48)
        rich = self.m.features(harmonic_tone(nominal_hz(48), [1.0, 0.5, 0.25, 0.125, 0.08, 0.04]), 48)
        self.assertLess(np.count_nonzero(np.array(sine["relative_harmonics_db"])[1:] >= -40), 3)
        self.assertEqual(self.m.distance(sine, rich), 999.0)
        row = dict(
            train_db=self.m.distance(sine, rich),
            holdout_db=0.2,
            baseline_train_db=999.0,
            baseline_holdout_db=1.5,
            production_cutoff=104,
            production_wave=0,
            production_envmod=32,
            gain_db=-1.25,
            holdout_gain_error_db=0.4,
            usable_exploratory_anchor=False,
        )
        self.m.finalize_metric_rows([row])
        self.assertIsNone(row["train_db"])
        self.assertIsNone(row["holdout_db"])
        self.assertIsNone(row["baseline_train_db"])
        self.assertEqual(row["baseline_holdout_db"], 1.5)
        self.assertIsNone(row["production_cutoff"])
        self.assertIsNone(row["production_wave"])
        self.assertIsNone(row["production_envmod"])
        self.assertIsNone(row["gain_db"])
        self.assertIsNone(row["holdout_gain_error_db"])
        self.assertFalse(row["usable_exploratory_anchor"])
        self.assertEqual(row["rejection_reason"], self.m.REJECTION_REASON)
        parsed = json.loads(json.dumps(row))
        self.assertIsNone(parsed["train_db"])
        self.assertIsNone(parsed["holdout_db"])
        self.assertIsNone(parsed["production_cutoff"])

    def test_baseline_reject_does_not_drop_a_usable_train_row(self):
        row = dict(train_db=1.0, holdout_db=2.0, baseline_train_db=999.0, baseline_holdout_db=1.5, production_cutoff=10, gain_db=-3.0)
        self.m.null_rejected_metrics(row)
        self.assertNotIn("rejection_reason", row)
        self.assertEqual(row["train_db"], 1.0)
        self.assertEqual(row["holdout_db"], 2.0)
        self.assertIsNone(row["baseline_train_db"])
        self.assertEqual(row["production_cutoff"], 10)
        self.assertEqual(row["gain_db"], -3.0)


class BoundaryReporting(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m = load_script("analyze.py").measure
        cls.amps = [1, 0.6, 0.3, 0.2, 0.1, 0.05]

    def test_search_edges(self):
        note = 48
        nominal = nominal_hz(note)
        high = self.m.features(harmonic_tone(nominal * 1.02, self.amps), note)
        low = self.m.features(harmonic_tone(nominal * 0.98, self.amps), note)
        self.assertTrue(high["pitch_fit_at_search_boundary"])
        self.assertTrue(low["pitch_fit_at_search_boundary"])
        # The search limits are nominal*0.99 and nominal*1.01. Those ratios are not
        # the same distance in cents, so each edge has its own expected report.
        self.assertLess(abs(high["cents"] - 1200 * np.log2(1.01)), 0.05)
        self.assertLess(abs(low["cents"] - 1200 * np.log2(0.99)), 0.05)
        self.assertLess(abs(high["f0"] - nominal * 1.01), 0.001)
        self.assertLess(abs(low["f0"] - nominal * 0.99), 0.001)


class ZeroNonfinite(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m = load_script("analyze.py").measure

    def test_zero_and_nonfinite_report_null_pitch(self):
        with warnings.catch_warnings():
            warnings.simplefilter("error", RuntimeWarning)
            zero = self.m.features(np.zeros(SR), 48)
            missing = self.m.features(np.full(SR, np.nan), 48)
            infinite = self.m.features(np.full(SR, np.inf), 48)
            empty = self.m.features(np.array([]), 48)
        for report, status in ((zero, "zero"), (empty, "zero"), (missing, "nonfinite"), (infinite, "nonfinite")):
            self.assertEqual(report["input_status"], status)
            self.assertIsNone(report["f0"])
            self.assertIsNone(report["relative_harmonics_db"])
            self.assertIsNone(report["pitch_fit_at_search_boundary"])
            parsed = json.loads(json.dumps(report))
            self.assertIsNone(parsed["f0"])
            self.assertNotIn("NaN", json.dumps(report))
        self.assertEqual(self.m.distance(zero, missing), 999.0)
        window = self.m.apparent_hz_at(np.zeros(SR), 0.5)
        self.assertEqual(window["input_status"], "zero")
        self.assertIsNone(window["apparent_hz"])
        dirty = harmonic_tone(nominal_hz(48), [1, 0.5, 0.25, 0.125])
        outside = dirty.copy()
        outside[100] = np.nan
        self.assertEqual(self.m.features(outside, 48)["input_status"], "nonfinite")
        dirty[22000] = np.nan
        bad = self.m.apparent_hz_at(dirty, 0.5)
        self.assertEqual(bad["input_status"], "nonfinite")
        self.assertIsNone(bad["apparent_hz"])
        with self.assertRaises(ValueError):
            self.m.contour_hz(np.zeros(SR), 48)


class GainIndependence(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m = load_script("analyze.py").measure
        cls.a = harmonic_tone(nominal_hz(48), [1, 0.7, 0.45, 0.3, 0.2, 0.12, 0.08, 0.05])
        cls.b = harmonic_tone(nominal_hz(48), [1, 0.2, 0.5, 0.15, 0.4, 0.08, 0.25, 0.05])

    def test_scale_changes_rms_not_shape_or_pitch(self):
        fa = self.m.features(self.a, 48)
        fb = self.m.features(self.b, 48)
        fag = self.m.features(self.a * 2.5, 48)
        fbg = self.m.features(self.b * 0.4, 48)
        self.assertLess(abs(fa["f0"] - fag["f0"]), 1e-6)
        self.assertLess(np.max(np.abs(np.array(fa["relative_harmonics_db"]) - np.array(fag["relative_harmonics_db"]))), 1e-6)
        self.assertAlmostEqual(fag["rms"] / fa["rms"], 2.5, places=6)
        self.assertLess(abs(self.m.distance(fa, fb) - self.m.distance(fag, fbg)), 1e-6)
        edited = dict(fa)
        edited["rms"] = fa["rms"] * 9
        self.assertEqual(self.m.distance(fa, fb), self.m.distance(edited, fb))
        gain = float(self.m.db(fa["rms"] / fag["rms"]))
        self.assertAlmostEqual(gain, float(self.m.db(1 / 2.5)), places=6)
        self.assertGreater(abs(gain), 1.0)
        ca = self.m.apparent_hz_at(self.a, 0.5)
        cb = self.m.apparent_hz_at(self.a * 3.5, 0.5)
        self.assertLess(abs(ca["apparent_hz"] - cb["apparent_hz"]), 1e-4)


class TrainHoldoutSelection(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m = load_script("analyze.py").measure

    def test_tuple_tie_break_and_train_distance_win(self):
        ref = spectrum(0)
        same = spectrum(0)
        picked = self.m.select_candidate(ref, [(same, 1, 80, "later"), (same, 0, 120, "earlier")])
        self.assertEqual(picked[1:], (0, 120, "earlier"))
        near = spectrum(0.2)
        far = spectrum(12)
        picked = self.m.select_candidate(ref, [(far, 0, 0, "low"), (near, 1, 127, "high")])
        self.assertEqual(picked[-1], "high")

    def test_holdout_cannot_change_the_selected_pot(self):
        pots = list(range(0, 128, 8)) + [127]
        ref = spectrum(0)
        far = spectrum(12)
        metrics = {
            "cut0.2-n48-r0.au": ref,
            "cut0.2-n60-r0.au": far,
            "cut0.2-n48-r0.dsp": far,
            "cut0.2-n60-r0.dsp": far,
        }
        for w in (0, 1):
            for p in pots:
                metrics[f"grid-n48-w{w}-c{p}.dsp"] = ref if (w, p) == (1, 40) else far
                metrics[f"grid-n60-w{w}-c{p}.dsp"] = ref if (w, p) == (1, 40) else far
        row = self.m.grid_calibration_row(metrics, 0.2)
        self.assertEqual((row["production_wave"], row["production_cutoff"]), (1, 40))
        self.assertEqual(row["train_db"], 0.0)
        self.assertGreater(row["holdout_db"], 3)
        decoy = metrics["grid-n60-w0-c8.dsp"]
        self.assertLess(self.m.distance(metrics["cut0.2-n60-r0.au"], decoy), row["holdout_db"])
        poisoned = dict(metrics)
        for key in list(poisoned):
            if "n60" in key:
                poisoned[key] = spectrum(3)
        again = self.m.grid_calibration_row(poisoned, 0.2)
        self.assertEqual((again["production_wave"], again["production_cutoff"]), (1, 40))
        self.assertEqual(again["train_db"], row["train_db"])
        self.assertNotEqual(again["holdout_db"], row["holdout_db"])

    def test_expanded_holdout_resolver_runs_after_train_choice(self):
        ref = spectrum(0)
        far = spectrum(12)
        metrics = {"cut0.4-n48-r0.au": ref, "cut0.4-n60-r0.au": far}
        for p in list(range(0, 128, 8)) + [127]:
            metrics[f"grid-n48-w0-c{p}.dsp"] = far
        for env in (0, 32, 64, 96, 127):
            for pot in list(range(0, 128, 16)) + [127]:
                metrics[f"expanded-n48-e{env}-c{pot}.dsp"] = ref if (env, pot) == (64, 48) else far
        called = []

        def resolve(hk):
            called.append(hk)
            return ref

        row = self.m.expanded_calibration_row(metrics, 0.4, resolve)
        self.assertEqual(called, ["expanded-n60-e64-c48.dsp"])
        self.assertEqual(row["production_envmod"], 64)
        self.assertEqual(row["production_cutoff"], 48)
        self.assertEqual(row["production_wave"], 0)
        self.assertEqual(row["train_db"], 0.0)
        self.assertGreater(row["holdout_db"], 3)


class FirmwareUnchanged(unittest.TestCase):
    def test_bass303_hash_and_tree(self):
        digest = hashlib.sha256((REPO / "firmware/src/dsp/bass303.c").read_bytes()).hexdigest()
        self.assertEqual(digest, BASS303_SHA256)
        diff = subprocess.run(["git", "diff", "--quiet", "HEAD", "--", "firmware"], cwd=REPO)
        self.assertEqual(diff.returncode, 0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
