#!/usr/bin/env python3
"""Focused actual owner outcome and optional telemetry regression tests."""

from __future__ import annotations

import copy
import sys
import unittest
from datetime import datetime, timedelta, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from admin_outcomes import (
    OutcomeError, checked_package, prepare_owner_outcome, sanitize_installer_report,
)
from validate_release_status import apply_proposal, load_json, validate_instance

RECORD = load_json(ROOT / "records/0.0.25.1-79280e76.json")
SCHEMA = load_json(ROOT / "schema/release-record.schema.json")
PAST = "2026-11-13T10:00:00-06:00"
RUN_ID = "12345678-abcd-4321-aaaa-abcdef987654"


def selected(model="elite"):
    return {
        "catalog_id": RECORD["catalog_id"],
        "model": model,
        "package_sha256": RECORD["models"][model]["package_sha256"],
    }


class TestAdminOutcomes(unittest.TestCase):
    def test_selected_model_and_immutable_package_identity(self):
        self.assertEqual(
            checked_package(RECORD, "elite", selected()["package_sha256"]),
            RECORD["models"]["elite"],
        )
        with self.assertRaises(OutcomeError):
            checked_package(RECORD, "pro", selected()["package_sha256"])
        with self.assertRaises(OutcomeError):
            checked_package(RECORD, "Elite", selected()["package_sha256"])
        with self.assertRaises(OutcomeError):
            checked_package(RECORD, "elite", "0" * 64)

    def test_withdrawn_record_refuses_new_owner_test(self):
        withdrawn = load_json(ROOT / "records/0.0.26-600b3b45.json")
        with self.assertRaises(OutcomeError):
            checked_package(withdrawn, "elite", withdrawn["models"]["elite"]["package_sha256"])

    def test_owner_reports_only_one_model_no_hardware_approval(self):
        original = copy.deepcopy(RECORD)
        payload = {
            **selected(), "stage": "restart-and-boot-reported",
            "observed_at": PAST, "note": "Owner saw an actual restart and normal boot.",
        }
        proposal = prepare_owner_outcome(original, payload, owner_login="gobbajogger")
        after = apply_proposal(original, proposal)
        self.assertEqual(validate_instance(after, SCHEMA, SCHEMA, "$"), [])
        self.assertEqual(proposal["current_report_sequence"], after["history"][-1]["sequence"])
        self.assertTrue(after["states"]["boot-restart-reported"])
        self.assertFalse(after["states"]["install-verified"])
        self.assertFalse(after["states"]["owner-hardware-approval"])
        self.assertEqual(after["history"][-1]["model"], "Elite")
        self.assertIsNone(after["history"][-1]["installed_hash"])
        self.assertEqual(after["history"][-1]["model_confidence"], "reported")
        self.assertEqual(after["history"][-1]["selected_package_sha256"], selected()["package_sha256"])
        self.assertFalse(any(h.get("model") == "Pro" for h in after["history"]))
        self.assertEqual(RECORD, original)  # no mutation

    def test_upload_ack_and_restart_do_not_claim_normal_boot(self):
        payload = {**selected(), "stage": "restart-reported", "observed_at": PAST}
        after = apply_proposal(RECORD, prepare_owner_outcome(RECORD, payload, owner_login="gobbajogger"))
        self.assertFalse(after["states"]["boot-restart-reported"])
        self.assertFalse(after["states"]["install-verified"])

    def test_owner_auth_and_unlinked_quote_refused(self):
        payload = {**selected(), "stage": "installed-reported", "observed_at": PAST}
        with self.assertRaises(OutcomeError):
            prepare_owner_outcome(RECORD, payload, owner_login="unverified")
        payload["quoted_words"] = "I tested a build"
        payload["source_link"] = "https://example.org/quoted/elite"
        with self.assertRaises(OutcomeError):
            prepare_owner_outcome(RECORD, payload, owner_login="gobbajogger")
        payload["quoted_words"] = "I tested Elite"
        payload["source_link"] = "relay: yes"
        with self.assertRaises(OutcomeError):
            prepare_owner_outcome(RECORD, payload, owner_login="gobbajogger")

    def test_timestamp_rejects_missing_offset_impossible_and_future(self):
        payload = {**selected(), "stage": "installed-reported", "observed_at": PAST}
        for observed in ("2026-02-30T14:00:00Z", "2026-11-13T11:00:00"):
            payload["observed_at"] = observed
            with self.assertRaises(OutcomeError):
                prepare_owner_outcome(RECORD, payload, owner_login="gobbajogger")
        payload["observed_at"] = (datetime.now(timezone.utc) + timedelta(days=1)).isoformat()
        with self.assertRaises(OutcomeError):
            prepare_owner_outcome(RECORD, payload, owner_login="gobbajogger")

    def test_anonymous_telemetry_is_opt_in_untrusted_and_privacy_minimal(self):
        payload = {
            **selected(), "stage": "transfer-completed", "consent": True,
            "run_id": RUN_ID, "sequence": 2, "observed_at": PAST,
            "serial": "DO-NOT-PERSIST", "mac": "DO-NOT-PERSIST",
            "usb_product": "DO-NOT-PERSIST", "browser": "DO-NOT-PERSIST",
        }
        result = sanitize_installer_report(RECORD, payload)
        self.assertEqual(result["confidence"], "anonymous-unverified")
        self.assertFalse(result["promotion_authority"])
        for forbidden in ("serial", "mac", "usb_product", "browser", "installed_hash", "approval"):
            self.assertNotIn(forbidden, result)

    def test_unconsented_and_other_build_reports_rejected(self):
        base = {
            **selected(), "stage": "boot-observed", "consent": False,
            "run_id": RUN_ID, "sequence": 1, "observed_at": PAST,
        }
        with self.assertRaises(OutcomeError):
            sanitize_installer_report(RECORD, base)
        base["consent"] = True
        base["package_sha256"] = RECORD["models"]["pro"]["package_sha256"]
        with self.assertRaises(OutcomeError):
            sanitize_installer_report(RECORD, base)
        base["package_sha256"] = RECORD["models"]["elite"]["package_sha256"]
        base["sequence"] = 0
        with self.assertRaises(OutcomeError):
            sanitize_installer_report(RECORD, base)


if __name__ == "__main__":
    unittest.main()
