#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
from __future__ import annotations

import os
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import au_compare_battery  # noqa: E402


class AuCompareBatteryTest(unittest.TestCase):
    def tearDown(self) -> None:
        for key in (
            "AU_COMPARE_MODE",
            "AU_COMPARE_QUICK",
            "AU_COMPARE_FULL",
            "AU_COMPARE_BPM",
            "MIAMI_COMPARE_QUICK",
        ):
            os.environ.pop(key, None)

    def test_default_preset_full_timing(self) -> None:
        self.assertEqual(au_compare_battery.compare_mode(), "preset")
        self.assertEqual(au_compare_battery.compare_bpm(), 170.0)
        t = au_compare_battery.active_timing()
        self.assertEqual(t.frames, 48000)
        self.assertEqual(t.note_off_frame, 24000)

    def test_bpm_env_override(self) -> None:
        os.environ["AU_COMPARE_BPM"] = "120"
        self.assertEqual(au_compare_battery.compare_bpm(), 120.0)
        out = au_compare_battery.apply_timing_to_scenario({})
        self.assertEqual(out["bpm"], 120.0)

    def test_quick_shortens_all_presets_not_subset(self) -> None:
        os.environ["AU_COMPARE_QUICK"] = "1"
        scen = {"frames": 48000, "noteOnFrame": 4800, "noteOffFrame": 24000}
        out = au_compare_battery.apply_timing_to_scenario(scen)
        self.assertEqual(out["frames"], 19200)
        self.assertEqual(out["noteOnFrame"], 2400)
        self.assertEqual(out["noteOffFrame"], 18000)
        self.assertEqual(out["steadyWindowStart"], 4800)
        self.assertEqual(out["steadyWindowEnd"], 14400)
        # Steady voiced window must stay inside the gated note (FULL invariant).
        self.assertGreaterEqual(out["steadyWindowStart"], out["noteOnFrame"])
        self.assertLessEqual(out["steadyWindowEnd"], out["noteOffFrame"])
        self.assertEqual(len(au_compare_battery.MIAMI_MATRIX_CONFIGS), 7)

    def test_full_steady_inside_gate(self) -> None:
        t = au_compare_battery.FULL_TIMING
        self.assertGreaterEqual(t.steady_start, t.note_on_frame)
        self.assertLessEqual(t.steady_end, t.note_off_frame)
        self.assertLess(t.note_off_frame, t.frames)

    def test_full_overrides_quick(self) -> None:
        os.environ["AU_COMPARE_QUICK"] = "1"
        os.environ["AU_COMPARE_FULL"] = "1"
        self.assertFalse(au_compare_battery.compare_quick_enabled())
        self.assertEqual(au_compare_battery.active_timing().frames, 48000)

    def test_matrix_mode_secondary(self) -> None:
        os.environ["AU_COMPARE_MODE"] = "matrix"
        self.assertEqual(au_compare_battery.compare_mode(), "matrix")
        self.assertIn("diagnostic", au_compare_battery.harness_summary_line())


if __name__ == "__main__":
    unittest.main()
