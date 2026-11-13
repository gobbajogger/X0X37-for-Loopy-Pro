#!/usr/bin/env python3
"""Owner-reported Elite tests: model elite, hash absent, Pro untested, approval false."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from validate_release_status import load_json, validate_record  # noqa: E402

SCHEMA = load_json(ROOT / "schema" / "release-record.schema.json")
RECORD_941 = load_json(ROOT / "records" / "0.0.26-94197fc8.json")
RECORD_025 = load_json(ROOT / "records" / "0.0.25.1-79280e76.json")


def _current(record: dict) -> dict:
    current = [h for h in record["history"] if h.get("is_current")]
    assert len(current) == 1
    return current[0]


def _all_text(record: dict) -> str:
    parts = list(record.get("notes") or [])
    for h in record.get("history") or []:
        parts.append(h.get("text") or "")
        parts.extend(h.get("notes") or [])
        parts.append(h.get("source") or "")
    conf = record.get("identity_confidence") or {}
    parts.extend(
        [
            conf.get("model_note") or "",
            conf.get("build_note") or "",
            conf.get("installed_hash_note") or "",
        ]
    )
    return "\n".join(parts)


class TestOwnerReportedElite(unittest.TestCase):
    def test_0251_owner_reported_elite_hash_absent_pro_untested_no_approval(self):
        rec = RECORD_025
        issues = validate_record(rec, SCHEMA, filename="0.0.25.1-79280e76.json")
        self.assertEqual([str(i) for i in issues], [])
        self.assertTrue(rec["states"]["installation-reported"])
        self.assertFalse(rec["states"]["install-verified"])
        self.assertFalse(rec["states"]["owner-hardware-approval"])
        self.assertEqual(rec["identity_confidence"]["model"], "reported")
        self.assertEqual(rec["identity_confidence"]["build"], "absent")
        self.assertEqual(rec["identity_confidence"]["installed_hash"], "absent")
        current = _current(rec)
        self.assertEqual(current["model"], "Elite")
        self.assertEqual(current["model_confidence"], "reported")
        self.assertIsNone(current["installed_hash"])
        self.assertEqual(current["installed_hash_confidence"], "absent")
        self.assertTrue(current["quoted"])
        self.assertEqual(current["source"], "owner in Orchestrator chat, relayed by Orchestrator")
        self.assertFalse(any((h.get("model") or "").lower() == "pro" for h in rec["history"]))
        blob = _all_text(rec)
        self.assertIn("Pro was not tested", blob)
        self.assertIn("I used elite", blob)
        self.assertIn("Bro I tested a 25,1 and 26 already", blob)
        self.assertFalse(rec.get("hardware_approvals"))

    def test_026_owner_stated_elite_hash_absent_pro_untested_no_approval(self):
        rec = RECORD_941
        issues = validate_record(rec, SCHEMA, filename="0.0.26-94197fc8.json")
        self.assertEqual([str(i) for i in issues], [])
        self.assertTrue(rec["states"]["installation-reported"])
        self.assertTrue(rec["states"]["boot-restart-reported"])
        self.assertFalse(rec["states"]["install-verified"])
        self.assertFalse(rec["states"]["owner-hardware-approval"])
        self.assertEqual(rec["identity_confidence"]["model"], "reported")
        self.assertEqual(rec["identity_confidence"]["installed_hash"], "absent")
        current = _current(rec)
        self.assertEqual(current["model"], "Elite")
        self.assertEqual(current["model_confidence"], "reported")
        self.assertIsNone(current["installed_hash"])
        self.assertEqual(current["installed_hash_confidence"], "absent")
        self.assertTrue(current["quoted"])
        self.assertEqual(current["source"], "owner in Orchestrator chat, relayed by Orchestrator")
        self.assertIn("I used elite", "\n".join(current.get("notes") or []))
        self.assertFalse(any((h.get("model") or "").lower() == "pro" for h in rec["history"]))
        blob = _all_text(rec)
        self.assertIn("Pro was not tested", blob)
        self.assertIn("Elite never implies Pro", blob)
        self.assertFalse(rec.get("hardware_approvals"))


if __name__ == "__main__":
    unittest.main()
