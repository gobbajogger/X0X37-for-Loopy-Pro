#!/usr/bin/env python3
"""Executable independent-QA regression cases for issue comments 6853581734/6855181914."""

from __future__ import annotations

import copy
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from evidence_policy import audit_chronology_errors, approval_errors, installation_for_model
from evaluate_promotion import evaluate_install_verified, evaluate_owner_hardware_approval
from validate_release_status import (
    check_hardware_approval_scope, load_json, load_jsonl, validate_instance,
)

BASE = load_json(ROOT / "records" / "0.0.25.1-79280e76.json")
SCHEMA = load_json(ROOT / "schema" / "release-record.schema.json")
AUDIT = ROOT / "audit" / "audit.jsonl"


def receipt(record: dict, model: str) -> dict:
    return {
        "claimed_model": model,
        "model_proof": "board_id",
        "model_proof_model": model,
        "model_proof_time": "2026-11-13T11:20:00-06:00",
        "model_proof_source_link": "https://example.org/synthetic-model-proof/" + model,
        "model_confidence": "verified",
        "build_confidence": "verified",
        "installed_hash": record["models"][model]["image_sha256"],
    }


def approval(record: dict, model: str) -> dict:
    return {
        "model": model,
        "source_sha": record["source_sha"],
        "image_sha256": record["models"][model]["image_sha256"],
        "approved": True,
        "quoted": True,
        "quoted_words": f"I approve this exact DnBOX {model.title()} device build.",
        "time": "2026-11-13T23:20:00+10:00" if model == "elite" else "2026-11-13T23:21:00+10:00",
        "source_link": "https://example.org/synthetic-owner-proof/elite" if model == "elite"
                       else "https://example.org/synthetic-owner-proof/pro",
    }


def complete_fixture() -> dict:
    rec = copy.deepcopy(BASE)
    rec["install_verifications"] = [receipt(rec, "elite"), receipt(rec, "pro")]
    rec["hardware_approvals"] = [approval(rec, "elite"), approval(rec, "pro")]
    return rec


class TestQABoundaries(unittest.TestCase):
    def test_positive_two_model_receipts_are_schema_valid_and_independent(self):
        rec = complete_fixture()
        self.assertEqual(validate_instance(rec, SCHEMA, SCHEMA, "$"), [])
        self.assertEqual(check_hardware_approval_scope(rec), [])
        for model in ("elite", "pro"):
            self.assertTrue(installation_for_model(rec, model)[0])
            self.assertEqual(evaluate_install_verified(rec, model)["status"], "pass")
            self.assertEqual(evaluate_owner_hardware_approval(rec, model)["status"], "pass")

    def test_elite_quote_cannot_approve_pro_even_with_correct_pro_hash(self):
        rec = complete_fixture()
        rec["hardware_approvals"][1]["quoted_words"] = rec["hardware_approvals"][0]["quoted_words"]
        self.assertTrue(approval_errors(rec["hardware_approvals"][1], rec))
        self.assertIn("elite-approval-not-pro", {i.rule for i in check_hardware_approval_scope(rec)})
        self.assertNotEqual(evaluate_owner_hardware_approval(rec, "pro")["status"], "pass")

    def test_pro_bare_approval_fails_with_valid_elite_quote(self):
        rec = complete_fixture()
        rec["hardware_approvals"][1] = {
            "model": "pro", "source_sha": rec["source_sha"],
            "image_sha256": rec["models"]["pro"]["image_sha256"], "approved": True,
        }
        self.assertTrue(validate_instance(rec, SCHEMA, SCHEMA, "$"))
        self.assertNotEqual(evaluate_owner_hardware_approval(rec, "pro")["status"], "pass")
        self.assertEqual(evaluate_owner_hardware_approval(rec, "elite")["status"], "pass")

    def test_model_less_history_quote_does_not_fill_missing_pro_approval(self):
        rec = complete_fixture()
        rec["hardware_approvals"] = [approval(rec, "elite")]
        rec.setdefault("history", []).append({
            "sequence": 200, "kind": "other", "source": "owner", "quoted": True,
            "confidence": "verified", "verified": True, "is_current": False,
            "time": "2026-11-13T23:21:00+10:00",
            "ref": "https://example.org/synthetic-owner-proof/pro",
            "text": "I approve the firmware", 
        })
        self.assertNotEqual(evaluate_owner_hardware_approval(rec, "pro")["status"], "pass")

    def test_source_link_not_ref_or_freeform_source(self):
        rec = complete_fixture()
        item = rec["hardware_approvals"][1]
        item.pop("source_link")
        item["ref"] = "https://example.org/synthetic-owner-proof/pro"
        self.assertTrue(validate_instance(rec, SCHEMA, SCHEMA, "$"))
        self.assertNotEqual(evaluate_owner_hardware_approval(rec, "pro")["status"], "pass")
        item.pop("ref")
        item["source_link"] = "owner in a chat, relayed"
        self.assertTrue(approval_errors(item, rec))
        item["source_link"] = "https://example.invalid/owner"
        self.assertTrue(approval_errors(item, rec))

    def test_wrong_model_hash_or_missing_owner_quote_fails(self):
        rec = complete_fixture()
        item = rec["hardware_approvals"][1]
        item["image_sha256"] = rec["models"]["elite"]["image_sha256"]
        self.assertNotEqual(evaluate_owner_hardware_approval(rec, "pro")["status"], "pass")
        item["image_sha256"] = rec["models"]["pro"]["image_sha256"]
        item["quoted"] = False
        self.assertNotEqual(evaluate_owner_hardware_approval(rec, "pro")["status"], "pass")

    def test_claimed_model_must_match_requested_model_proof(self):
        rec = copy.deepcopy(BASE)
        rec["install_verification"] = receipt(rec, "elite")
        self.assertEqual(evaluate_install_verified(rec, "pro")["status"], "missing")
        self.assertFalse(installation_for_model(rec, "pro")[0])
        rec["install_verifications"] = [receipt(rec, "pro")]
        rec["install_verifications"][0]["installed_hash"] = rec["models"]["elite"]["image_sha256"]
        self.assertEqual(evaluate_install_verified(rec, "pro")["status"], "fail")


    def test_hardware_proof_model_mismatch_blocks_other_model(self):
        rec = complete_fixture()
        rec["install_verifications"][1]["model_proof_model"] = "elite"
        self.assertFalse(installation_for_model(rec, "pro")[0])
        self.assertEqual(evaluate_install_verified(rec, "pro")["status"], "missing")

    def test_existing_append_only_audit_preserves_historical_times(self):
        legacy = load_jsonl(AUDIT)
        self.assertGreaterEqual(len(legacy), 5)
        self.assertEqual(audit_chronology_errors(legacy), [])
        self.assertTrue(any("10:00:00-06:00" in e["time"] for e in legacy))
        self.assertTrue(any("05:15:00-06:00" in e["time"] for e in legacy))

    def test_non_iso_impossible_time_and_append_regression_rejected(self):
        legacy = load_jsonl(AUDIT)
        for value in ("2026-02-30T11:00:00Z", "yesterday", "2026-11-13T11:00:00"):
            bad = copy.deepcopy(legacy)
            bad[0]["time"] = value
            self.assertTrue(audit_chronology_errors(bad), value)
        forward = copy.deepcopy(legacy)
        forward.append({**legacy[-1], "time": "2026-11-13T10:00:00Z",
                        "appended_at": "2026-11-13T17:00:00Z"})
        forward.append({**legacy[-1], "time": "2026-11-13T09:00:00Z",
                        "appended_at": "2026-11-13T16:59:00Z"})
        self.assertTrue(any("moved backward" in reason
                            for reason in audit_chronology_errors(forward)))


if __name__ == "__main__":
    unittest.main()
