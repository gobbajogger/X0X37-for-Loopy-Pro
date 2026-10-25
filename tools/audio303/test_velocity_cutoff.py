# SPDX-License-Identifier: GPL-3.0-only
"""Host checks for velocity→cutoff measurement (no joint gain model)."""
import hashlib
import json
import pathlib
import subprocess
import tempfile
import unittest

import numpy as np

DIR = pathlib.Path(__file__).resolve().parent
REPO = DIR.parents[1]


def load_vcut():
    import importlib.util

    path = DIR / "velocity_cutoff.py"
    spec = importlib.util.spec_from_file_location("audio303_velocity_cutoff", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


class VelocityCutoffMeasurement(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.vcut = load_vcut()
        cls.work = pathlib.Path(tempfile.mkdtemp(prefix="vcut-test-"))
        cls.renderer = cls.vcut.compile_renderer(cls.work / "production303")
        cls.report = cls.vcut.build_report(renderer=cls.renderer, work_dir=cls.work)

    def test_bass303_hash_matches_packet(self):
        actual = hashlib.sha256((REPO / "firmware/src/dsp/bass303.c").read_bytes()).hexdigest()
        self.assertEqual(actual, self.vcut.BASS303_SHA256)
        self.assertEqual(self.report["bass303_sha256_actual"], actual)

    def test_rejected_joint_model_not_present(self):
        self.assertFalse((DIR / "gate_amplitude.py").exists())
        self.assertFalse(self.report["rejected_joint_model"]["imported"])
        self.assertEqual(
            self.report["rejected_joint_model"]["tip"],
            "a33c2de208c227beab7203fbb2de9e244052eff6",
        )

    def test_law_neutral_and_monotonic(self):
        self.assertEqual(self.vcut.velocity_gain_law(100), 1.0)
        self.assertEqual(self.vcut.velocity_cutoff_law(100), 1.0)
        self.assertLess(self.vcut.velocity_cutoff_law(40), self.vcut.velocity_cutoff_law(80))
        self.assertLess(self.vcut.velocity_cutoff_law(80), self.vcut.velocity_cutoff_law(100))
        self.assertLess(self.vcut.velocity_cutoff_law(100), self.vcut.velocity_cutoff_law(127))
        self.assertLess(self.vcut.velocity_gain_law(40), self.vcut.velocity_gain_law(100))
        self.assertGreater(self.vcut.velocity_gain_law(127), self.vcut.velocity_gain_law(100))

    def test_host_matches_stored_dsp_and_gate_au_flat(self):
        self.assertTrue(self.report["host_matches_stored_dsp"])
        self.assertTrue(self.report["train"]["au_gate_brightness_flat"])
        self.assertTrue(self.report["train"]["host_velocity_cutoff_active"])
        self.assertEqual(self.report["status"], "MEASUREMENT_COMPLETE_NO_GATE_FILTER_FIT")

    def test_brightness_and_rms_are_separated(self):
        bright = self.report["train"]["host_brightness_transfer_db"]["vel40-r0"]
        rms = self.report["train"]["host_rms_transfer_db"]["vel40-r0"]
        self.assertLess(bright, -0.5)
        self.assertLess(rms, -5.0)
        # RMS transfer is not explained by brightness alone (level path is active too).
        self.assertLess(rms, bright - 1.0)

    def test_heldout_host_brightness_monotone_with_velocity(self):
        n60 = self.report["heldout"]["host_brightness_transfer_vs_n60_vel100_db"]
        self.assertLess(n60["velocity40-n60-r0"], n60["velocity80-n60-r0"])
        self.assertLess(n60["velocity80-n60-r0"], 0.0)
        self.assertGreater(n60["velocity127-n60-r0"], 0.0)

    def test_c_state_probe_pot_unchanged(self):
        """velocity_cutoff moves; BASS303_CUTOFF pot readback stays at the set value."""
        src = self.work / "vcut_state_probe.c"
        src.write_text(
            r"""
#include <stdio.h>
#include <math.h>
#include "bass303.h"
int main(void){
  bass303_t b; float out[256];
  bass303_init(&b);
  bass303_set(&b, BASS303_CUTOFF, 64);
  bass303_set(&b, BASS303_ENVMOD, 0);
  bass303_set(&b, BASS303_FENV, 1);
  bass303_set(&b, BASS303_FSUSTAIN, 127);
  int pot0 = bass303_get(&b, BASS303_CUTOFF);
  bass303_note_on_velocity(&b, 48, 0, 0, 40);
  bass303_render(&b, out, 256);
  int pot1 = bass303_get(&b, BASS303_CUTOFF);
  float g40 = b.velocity_gain, c40 = b.velocity_cutoff;
  bass303_note_on_velocity(&b, 48, 0, 1, 127); /* slide: keep gate */
  bass303_render(&b, out, 256);
  int pot2 = bass303_get(&b, BASS303_CUTOFF);
  float g127 = b.velocity_gain, c127 = b.velocity_cutoff;
  printf("%d %d %d %.8f %.8f %.8f %.8f\n", pot0, pot1, pot2, g40, c40, g127, c127);
  return 0;
}
"""
        )
        exe = self.work / "vcut_state_probe"
        cmd = [
            "cc",
            "-std=c99",
            "-O2",
            "-ffp-contract=off",
            f"-I{REPO / 'firmware/src'}",
            f"-I{REPO / 'firmware/src/dsp'}",
            str(src),
            str(REPO / "firmware/src/dsp/bass303.c"),
            "-lm",
            "-o",
            str(exe),
        ]
        done = subprocess.run(cmd, capture_output=True, text=True)
        self.assertEqual(done.returncode, 0, done.stderr)
        run = subprocess.run([str(exe)], capture_output=True, text=True)
        self.assertEqual(run.returncode, 0, run.stderr)
        parts = run.stdout.strip().split()
        self.assertEqual(len(parts), 7)
        pot0, pot1, pot2 = map(int, parts[:3])
        g40, c40, g127, c127 = map(float, parts[3:])
        self.assertEqual(pot0, 64)
        self.assertEqual(pot1, 64)
        self.assertEqual(pot2, 64)
        self.assertAlmostEqual(g40, 0.4, places=5)
        self.assertAlmostEqual(c40, self.vcut.velocity_cutoff_law(40), places=5)
        self.assertAlmostEqual(g127, 1.27, places=5)
        self.assertAlmostEqual(c127, self.vcut.velocity_cutoff_law(127), places=5)
        self.assertLess(c40, 1.0)
        self.assertGreater(c127, 1.0)

    def test_write_results_roundtrip(self):
        md = self.work / "out.md"
        js = self.work / "out.json"
        self.vcut.write_results(self.report, md_path=md, json_path=js)
        again = json.loads(js.read_text())
        self.assertEqual(again["status"], self.report["status"])
        self.assertIn(self.report["status"], md.read_text())


if __name__ == "__main__":
    unittest.main()
