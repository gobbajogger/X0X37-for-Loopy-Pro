#!/usr/bin/env python3
"""Candidate gates report PASS/MISSING/FAIL but never obtain deployment authority."""

from __future__ import annotations

import copy
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from promotion_candidate import evaluate_candidate
from validate_release_status import load_json

CURRENT = load_json(ROOT / "records/0.0.26-94197fc8.json")
RELEASED = load_json(ROOT / "records/0.0.25.1-79280e76.json")
WITHDRAWN = load_json(ROOT / "records/0.0.26-600b3b45.json")


class TestCandidate(unittest.TestCase):
    def test_elite_report_does_not_clear_pro(self):
        report = evaluate_candidate(CURRENT)
        self.assertFalse(report["gate_pass"])
        self.assertIn("install-verified:pro", report["blocking_gates"])
        self.assertIn("owner-hardware-approval:pro", report["blocking_gates"])
        self.assertFalse(report["promotion_authorized"])
        self.assertFalse(report["promotion_performed"])
        self.assertEqual(report["deployment_authority"], "not-confirmed")
        self.assertTrue(report["dry_run"])

    def test_published_main_0251_not_hardware_promoted(self):
        report = evaluate_candidate(RELEASED)
        self.assertEqual(report["channel"], "stable-MAIN")
        self.assertIn("boot-restart-reported", report["blocking_gates"])
        self.assertFalse(report["promotion_performed"])

    def test_withdrawn_candidate_cannot_be_ready(self):
        report = evaluate_candidate(WITHDRAWN)
        self.assertIn("withdrawn", report["blocking_gates"])
        self.assertFalse(report["promotion_authorized"])

    def test_even_fake_passing_evidence_does_not_flip_authority(self):
        record = copy.deepcopy(CURRENT)
        record["states"]["owner-hardware-approval"] = True
        report = evaluate_candidate(record)
        self.assertFalse(report["promotion_authorized"])
        self.assertFalse(report["promotion_performed"])


if __name__ == "__main__":
    unittest.main()
