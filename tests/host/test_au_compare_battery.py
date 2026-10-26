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
            "MIAMI_COMPARE_QUICK",
        ):
            os.environ.pop(key, None)

    def test_default_preset_full_timing(self) -> None:
        self.assertEqual(au_compare_battery.compare_mode(), "preset")
        t = au_compare_battery.active_timing()
        self.assertEqual(t.frames, 48000)
        self.assertEqual(t.note_off_frame, 24000)

    def test_quick_shortens_all_presets_not_subset(self) -> None:
        os.environ["AU_COMPARE_QUICK"] = "1"
        scen = {"frames": 48000, "noteOnFrame": 4800, "noteOffFrame": 24000}
        out = au_compare_battery.apply_timing_to_scenario(scen)
        self.assertEqual(out["frames"], 19200)
        self.assertEqual(out["noteOffFrame"], 12000)
        self.assertEqual(len(au_compare_battery.MIAMI_MATRIX_CONFIGS), 7)

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
