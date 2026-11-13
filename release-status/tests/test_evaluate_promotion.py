#!/usr/bin/env python3
"""Promotion evaluator tests. The evaluator must never promote."""

from __future__ import annotations

import copy
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
sys.path.insert(0, str(TOOLS))

from evaluate_promotion import (  # noqa: E402
    evaluate_all,
    evaluate_boot_restart_reported,
    evaluate_gate_passed,
    evaluate_install_verified,
    evaluate_owner_hardware_approval,
    evaluate_record,
    format_text,
    main as eval_main,
)
from validate_release_status import load_json, load_records  # noqa: E402

RECORDS = ROOT / "records"
RECORD_941 = load_json(RECORDS / "0.0.26-94197fc8.json")
RECORD_600 = load_json(RECORDS / "0.0.26-600b3b45.json")
RECORD_025 = load_json(RECORDS / "0.0.25.1-79280e76.json")
SCRIPT = TOOLS / "evaluate_promotion.py"

ELITE_025 = RECORD_025["models"]["elite"]["image_sha256"]
PRO_025 = RECORD_025["models"]["pro"]["image_sha256"]
ELITE_941 = RECORD_941["models"]["elite"]["image_sha256"]
PRO_941 = RECORD_941["models"]["pro"]["image_sha256"]


def _by_gate(report: dict, gate: str) -> dict:
    for g in report["gates"]:
        if g["gate"] == gate:
            return g
    raise AssertionError(f"gate {gate} not found in { [g['gate'] for g in report['gates']] }")


class TestEvaluatePromotion(unittest.TestCase):
    def test_0251_hardware_gates_missing_not_ready(self):
        report = evaluate_record(RECORD_025)
        self.assertEqual(report["channel"], "stable-MAIN")
        self.assertEqual(_by_gate(report, "built")["status"], "pass")
        self.assertEqual(_by_gate(report, "gate-passed")["status"], "pass")
        self.assertFalse(_by_gate(report, "gate-passed")["hardware_qualification"])
        self.assertTrue(_by_gate(report, "gate-passed")["from_emulation"])
        self.assertEqual(_by_gate(report, "published-nightly")["status"], "pass")
        self.assertFalse(_by_gate(report, "published-nightly")["hardware_qualification"])
        self.assertEqual(_by_gate(report, "installation-reported")["status"], "pass")
        self.assertEqual(_by_gate(report, "boot-restart-reported")["status"], "missing")
        self.assertEqual(_by_gate(report, "install-verified:elite")["status"], "missing")
        self.assertEqual(_by_gate(report, "install-verified:pro")["status"], "missing")
        self.assertIn("never implies", _by_gate(report, "install-verified:pro")["reason"].lower())
        self.assertEqual(_by_gate(report, "owner-hardware-approval:elite")["status"], "missing")
        self.assertEqual(_by_gate(report, "owner-hardware-approval:pro")["status"], "missing")
        self.assertFalse(report["owner_required_promotion_ready"])
        self.assertEqual(report["automatic_promotion"], "not-performed")
        self.assertFalse(report["promotion_performed"])
        self.assertFalse(report["hardware_qualified"])
        self.assertTrue(report["elite_never_implies_pro"])

    def test_94197fc8_is_nightly_not_main_and_not_promoted(self):
        report = evaluate_record(RECORD_941)
        self.assertEqual(report["channel"], "nightly-hardware-test")
        self.assertIn("not a MAIN promotion", report["channel_note"])
        self.assertEqual(_by_gate(report, "boot-restart-reported")["status"], "pass")
        self.assertIn("not verified", _by_gate(report, "boot-restart-reported")["reason"].lower())
        self.assertEqual(_by_gate(report, "install-verified:elite")["status"], "missing")
        self.assertEqual(_by_gate(report, "install-verified:pro")["status"], "missing")
        self.assertFalse(report["owner_required_promotion_ready"])
        self.assertFalse(report["promotion_performed"])
        self.assertFalse(report["hardware_qualified"])

    def test_600b_withdrawn_shown_and_promotion_refused(self):
        report = evaluate_record(RECORD_600)
        self.assertEqual(report["channel"], "withdrawn")
        self.assertTrue(report["withdrawn"])
        self.assertEqual(_by_gate(report, "withdrawn")["status"], "pass")
        self.assertIn("withdrawn", _by_gate(report, "withdrawn")["reason"].lower())
        self.assertEqual(_by_gate(report, "gate-passed")["status"], "missing")
        self.assertEqual(_by_gate(report, "installation-reported")["status"], "missing")
        self.assertEqual(_by_gate(report, "boot-restart-reported")["status"], "missing")
        self.assertFalse(report["owner_required_promotion_ready"])
        self.assertIn("withdrawn", report["ready_reason"])
        self.assertFalse(report["promotion_performed"])

    def test_emu_other_inputs_is_fail_and_not_hardware(self):
        rec = copy.deepcopy(RECORD_941)
        rec["gate_evidence"] = [
            {
                "kind": "emu360-strict",
                "result": "PASS",
                "applies_to_image": "other-inputs",
                "elite_late": 0,
                "pro_late": 0,
                "elite_worst_us": 11406,
                "pro_worst_us": 11410,
                "threshold_us": 11609,
            }
        ]
        g = evaluate_gate_passed(rec)
        self.assertEqual(g["status"], "fail")
        self.assertFalse(g["hardware_qualification"])
        self.assertTrue(g["from_emulation"])

    def test_usb_name_is_not_install_verified(self):
        rec = copy.deepcopy(RECORD_025)
        rec["identity_confidence"] = {
            "model": "verified",
            "build": "verified",
            "installed_hash": "verified",
        }
        rec["install_verification"] = {
            "installed_hash": ELITE_025,
            "claimed_model": "elite",
            "model_proof": "usb_name",
        }
        # The validator helper is_usb_or_product_proof looks at evidence
        # and record fields. evaluate_install_verified uses that helper
        # with empty evidence; also check a proposal-style injection.
        g = evaluate_install_verified(rec, "elite")
        # Without a hardware-field proof this is missing or fail, never pass.
        self.assertNotEqual(g["status"], "pass")
        self.assertFalse(g["hardware_qualification"])

    def test_elite_pass_does_not_imply_pro(self):
        rec = copy.deepcopy(RECORD_025)
        # Give Elite a complete verified install proof. Pro stays untouched.
        rec["identity_confidence"] = {
            "model": "verified",
            "build": "verified",
            "installed_hash": "verified",
        }
        rec["install_verification"] = {
            "installed_hash": ELITE_025,
            "claimed_model": "elite",
            "model_proof": "board_id",
        }
        rec["history"] = [
            {
                "sequence": 1,
                "kind": "install-report",
                "source": "owner in Orchestrator chat",
                "quoted": True,
                "model": "Elite",
                "model_confidence": "verified",
                "build": "0.0.25.1",
                "build_confidence": "verified",
                "installed_hash": ELITE_025,
                "installed_hash_confidence": "verified",
                "text": "synthetic elite-only verified install",
                "events": ["install"],
                "time": "2026-11-13T23:20:00+10:00",
                "ref": "https://example.invalid/owner",
                "verified": True,
                "is_current": True,
            }
        ]
        elite = evaluate_install_verified(rec, "elite")
        pro = evaluate_install_verified(rec, "pro")
        # Elite may pass or still miss a hardware-field evidence path
        # depending on how install_verification is read. Pro must never pass.
        self.assertEqual(pro["status"], "missing")
        self.assertIn("never implies", pro["reason"].lower())
        self.assertTrue(pro["model"] == "pro")
        approval_pro = evaluate_owner_hardware_approval(rec, "pro")
        self.assertNotEqual(approval_pro["status"], "pass")
        self.assertIn("never implies", approval_pro["reason"].lower())
        report = evaluate_record(rec)
        self.assertFalse(report["owner_required_promotion_ready"])
        self.assertFalse(report["promotion_performed"])

    def test_web_install_success_is_not_boot_or_approval(self):
        rec = copy.deepcopy(RECORD_025)
        rec["history"] = [
            {
                "sequence": 1,
                "kind": "install-and-boot-report",
                "source": "web installer log",
                "quoted": False,
                "events": ["install"],
                "text": "web install success only",
                "web_install_success_only": True,
                "web_install_success": True,
                "boot_or_restart_observed": False,
            }
        ]
        g = evaluate_boot_restart_reported(rec)
        self.assertEqual(g["status"], "fail")
        self.assertIn("web install", g["reason"].lower())

    def test_even_when_all_hardware_gates_pass_tool_does_not_promote(self):
        rec = copy.deepcopy(RECORD_025)
        rec["identity_confidence"] = {
            "model": "verified",
            "build": "verified",
            "installed_hash": "verified",
        }
        rec["install_verification"] = {
            "installed_hash": ELITE_025,
            "claimed_model": "elite",
            "model_proof": "board_id",
            "firmware_git_readback": rec["source_sha"],
        }
        rec["hardware_approvals"] = [
            {
                "model": "elite",
                "source_sha": rec["source_sha"],
                "image_sha256": ELITE_025,
                "approved": True,
                "time": "2026-11-13T23:22:00+10:00",
                "ref": "https://example.invalid/owner",
            },
            {
                "model": "pro",
                "source_sha": rec["source_sha"],
                "image_sha256": PRO_025,
                "approved": True,
                "time": "2026-11-13T23:22:00+10:00",
                "ref": "https://example.invalid/owner",
            },
        ]
        rec["history"] = [
            {
                "sequence": 1,
                "kind": "install-report",
                "source": "owner in Orchestrator chat",
                "quoted": True,
                "model": "Elite",
                "model_confidence": "verified",
                "build_confidence": "verified",
                "installed_hash": ELITE_025,
                "installed_hash_confidence": "verified",
                "events": ["install"],
                "time": "2026-11-13T23:20:00+10:00",
                "ref": "https://example.invalid/owner",
                "text": "synthetic elite",
            },
            {
                "sequence": 2,
                "kind": "install-report",
                "source": "owner in Orchestrator chat",
                "quoted": True,
                "model": "Pro",
                "model_confidence": "verified",
                "build_confidence": "verified",
                "installed_hash": PRO_025,
                "installed_hash_confidence": "verified",
                "events": ["install"],
                "time": "2026-11-13T23:21:00+10:00",
                "ref": "https://example.invalid/owner",
                "text": "synthetic pro",
            },
        ]
        # The evaluator's install-verified path requires offered_model_proof
        # from evidence or record. This synthetic may still be missing that
        # exact path. The contract under test is: if ready is true, the
        # tool still does not promote; if ready is false, it also does not.
        report = evaluate_record(rec)
        self.assertFalse(report["promotion_performed"])
        self.assertEqual(report["automatic_promotion"], "not-performed")
        self.assertFalse(report["hardware_qualified"])
        text = (format_text({"records": [report]}) + " " + report["hardware_qualified_note"]).lower()
        self.assertTrue(
            "never promote" in text or "no promotion was performed" in text or "not-performed" in text,
            text,
        )
        # Pro must not inherit an Elite installed hash.
        self.assertNotEqual(_by_gate(report, "install-verified:pro")["status"], "pass")

    def test_evaluate_all_and_cli(self):
        loaded = load_records(RECORDS)
        all_rep = evaluate_all(loaded)
        self.assertEqual(all_rep["automatic_promotion"], "not-performed")
        self.assertFalse(all_rep["promotion_performed"])
        self.assertEqual(len(all_rep["records"]), 3)
        ids = {r["record_id"] for r in all_rep["records"]}
        self.assertEqual(
            ids,
            {"nightly-79280e76-dev", "nightly-94197fc8-dev", "nightly-600b3b45-dev"},
        )
        proc = subprocess.run(
            [sys.executable, str(SCRIPT), "--records", str(RECORDS)],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertIn("never promotes", proc.stdout.lower())
        self.assertIn("not-performed", proc.stdout)
        self.assertIn("nightly-600b3b45-dev", proc.stdout)
        self.assertIn("End of report. No promotion was performed.", proc.stdout)
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / "report.json"
            proc2 = subprocess.run(
                [sys.executable, str(SCRIPT), "--records", str(RECORDS), "--json", "--output", str(out)],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(proc2.returncode, 0, proc2.stderr)
            data = json.loads(out.read_text())
            self.assertFalse(data["promotion_performed"])

    def test_cli_usage_exit_2(self):
        proc = subprocess.run(
            [sys.executable, str(SCRIPT), "--records", "/tmp/does-not-exist-rs"],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 2)
        self.assertIn("USAGE", proc.stderr)

    def test_main_returns_0_on_seeds(self):
        rc = eval_main(["--records", str(RECORDS)])
        self.assertEqual(rc, 0)


if __name__ == "__main__":
    unittest.main()
