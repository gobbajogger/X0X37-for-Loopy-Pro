#!/usr/bin/env python3
"""Negative and positive promotion-gate tests."""

from __future__ import annotations

import copy
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from validate_release_status import (  # noqa: E402
    load_json,
    validate_proposal,
    validate_record,
)

SCHEMA = load_json(ROOT / "schema" / "release-record.schema.json")
RECORD_941 = load_json(ROOT / "records" / "0.0.26-94197fc8.json")
RECORD_600 = load_json(ROOT / "records" / "0.0.26-600b3b45.json")


def rules(issues) -> list[str]:
    return [i.rule for i in issues]


def propose(record, proposal):
    return validate_proposal(record, proposal, SCHEMA)


class TestPromotionGates(unittest.TestCase):
    def test_inferred_model_must_not_pass_install_verified(self):
        issues = propose(
            RECORD_941,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {"install-verified": True},
                "identity_confidence": {"model": "inferred", "build": "verified", "installed_hash": "verified"},
                "evidence": {"installed_hash": "95b3195f4767ef5df33b365197b01203873e5eec3d7fa497c73e5e509484a4a6"},
            },
        )
        self.assertIn("inferred-model", rules(issues), [str(i) for i in issues])

    def test_inferred_build_from_current_nightly_must_not_pass_install_verified(self):
        issues = propose(
            RECORD_941,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {"install-verified": True},
                "identity_confidence": {"model": "verified", "build": "inferred", "installed_hash": "verified"},
                "evidence": {"build_proof": "current nightly", "installed_hash": "present"},
            },
        )
        self.assertIn("inferred-build", rules(issues), [str(i) for i in issues])

    def test_absent_installed_hash_must_not_pass_install_verified(self):
        issues = propose(
            RECORD_941,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {"install-verified": True},
                "identity_confidence": {"model": "verified", "build": "verified", "installed_hash": "absent"},
                "evidence": {"installed_hash": None},
            },
        )
        self.assertIn("absent-installed-hash", rules(issues), [str(i) for i in issues])

    def test_usb_name_only_must_not_pass_install_verified(self):
        issues = propose(
            RECORD_941,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {"install-verified": True},
                "identity_confidence": {"model": "verified", "build": "verified", "installed_hash": "verified"},
                "evidence": {
                    "model_proof": "usb_name",
                    "usb_name_only": True,
                    "usb_name": "FM-1",
                    "installed_hash": "present",
                },
            },
        )
        self.assertIn("usb-name-not-model-proof", rules(issues), [str(i) for i in issues])

    def test_web_install_success_alone_not_verified_boot_or_approval(self):
        issues = propose(
            RECORD_941,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {
                    "boot-restart-reported": True,
                    "owner-hardware-approval": True,
                },
                "evidence": {
                    "web_install_success_only": True,
                    "web_install_success": True,
                    "boot_restart_verified": True,
                    "owner_words_quoted": False,
                    "owner_source": "relay",
                },
            },
        )
        r = rules(issues)
        self.assertIn("web-install-not-boot-proof", r, [str(i) for i in issues])
        self.assertIn("web-install-not-hardware-approval", r, [str(i) for i in issues])
        # Approval also fails because this is a relay and install-verified is still false.
        self.assertTrue(
            "relay-not-owner-approval" in r or "owner-approval-requires-install-verified" in r,
            [str(i) for i in issues],
        )

    def test_relayed_unquoted_words_must_not_pass_owner_hardware_approval(self):
        issues = propose(
            RECORD_941,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {"owner-hardware-approval": True},
                "evidence": {
                    "owner_words_quoted": False,
                    "owner_source": "relay",
                    "relay": True,
                },
            },
        )
        self.assertIn("relay-not-owner-approval", rules(issues), [str(i) for i in issues])

    def test_emu360_d408_other_inputs_must_not_pass_gate_passed(self):
        # Replace exact-image evidence with the d408 other-inputs figures from
        # the nightly catalog text. Those numbers must not pass gate-passed
        # for the 94197fc8 images.
        d408 = [
            {
                "kind": "emu360-strict",
                "result": "PASS",
                "elite_worst_us": 11406,
                "pro_worst_us": 11410,
                "elite_late": 0,
                "pro_late": 0,
                "threshold_us": 11609,
                "ref": "nightly versions.json d408 other-inputs (not a measurement of the 94197fc8 images)",
                "applies_to_image": "other-inputs",
                "note": "Unchanged d408 production inputs. Not a measurement of these image hashes.",
            }
        ]
        issues = propose(
            RECORD_941,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {"gate-passed": True},
                "gate_evidence": d408,
                "evidence": {"emu360_applies_to_image": "other-inputs", "emu360_note": "d408"},
            },
        )
        self.assertIn("emu360-not-exact-image", rules(issues), [str(i) for i in issues])

        # Also: a record that already claims gate-passed with only d408 evidence fails.
        rec = copy.deepcopy(RECORD_941)
        rec["gate_evidence"] = d408
        rec["states"]["gate-passed"] = True
        issues2 = validate_record(rec, SCHEMA)
        self.assertIn("emu360-not-exact-image", rules(issues2), [str(i) for i in issues2])

    def test_withdrawn_600b_must_not_gain_a_promotion_flag(self):
        for flag in ("gate-passed", "install-verified", "owner-hardware-approval"):
            with self.subTest(flag=flag):
                proposal = {"record_id": "nightly-600b3b45-dev", "set_states": {flag: True}}
                if flag == "gate-passed":
                    proposal["gate_evidence"] = [
                        {
                            "kind": "emu360-strict",
                            "result": "PASS",
                            "elite_worst_us": 11000,
                            "pro_worst_us": 11000,
                            "elite_late": 0,
                            "pro_late": 0,
                            "threshold_us": 11609,
                            "ref": "synthetic",
                            "applies_to_image": "exact-image",
                        }
                    ]
                if flag == "install-verified":
                    proposal["identity_confidence"] = {
                        "model": "verified",
                        "build": "verified",
                        "installed_hash": "verified",
                    }
                    proposal["evidence"] = {"installed_hash": "present"}
                if flag == "owner-hardware-approval":
                    proposal["set_states"]["install-verified"] = True
                    proposal["identity_confidence"] = {
                        "model": "verified",
                        "build": "verified",
                        "installed_hash": "verified",
                    }
                    proposal["evidence"] = {
                        "owner_words_quoted": True,
                        "owner_timestamp": "2026-11-13T12:00:00-06:00",
                        "owner_source_link": "https://example.invalid/owner",
                        "installed_hash": "present",
                    }
                issues = propose(RECORD_600, proposal)
                self.assertIn(
                    "withdrawn-no-promotion",
                    rules(issues),
                    f"{flag}: {[str(i) for i in issues]}",
                )

    def test_controller_check_fail_must_not_pass_built(self):
        rec = copy.deepcopy(RECORD_941)
        rec["models"]["elite"]["controller_check"] = "FAIL"
        rec["models"]["pro"]["controller_check"] = "FAIL"
        rec["states"]["built"] = True
        issues = validate_record(rec, SCHEMA)
        self.assertIn("bundled-controller-check-required", rules(issues), [str(i) for i in issues])

        issues_p = propose(
            RECORD_941,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {"built": True},
                "models": {
                    "elite": {"controller_check": "FAIL"},
                    "pro": {"controller_check": "FAIL"},
                },
            },
        )
        self.assertIn("bundled-controller-check-required", rules(issues_p), [str(i) for i in issues_p])

    def test_controller_check_not_run_must_not_pass_built(self):
        rec = copy.deepcopy(RECORD_941)
        rec["models"]["elite"]["controller_check"] = "NOT_RUN"
        rec["models"]["pro"]["controller_check"] = "NOT_RUN"
        rec["states"]["built"] = True
        issues = validate_record(rec, SCHEMA)
        self.assertIn("bundled-controller-check-required", rules(issues), [str(i) for i in issues])

    def test_missing_gate_evidence_blocks_promotion(self):
        rec = copy.deepcopy(RECORD_941)
        rec["gate_evidence"] = []
        rec["states"]["gate-passed"] = True
        issues = validate_record(rec, SCHEMA)
        self.assertIn("missing-gate-evidence", rules(issues), [str(i) for i in issues])

        # A proposal that sets gate-passed without evidence also fails.
        bare = copy.deepcopy(RECORD_941)
        bare["gate_evidence"] = []
        bare["states"]["gate-passed"] = False
        issues_p = propose(
            bare,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {"gate-passed": True},
                "gate_evidence": [],
            },
        )
        self.assertIn("missing-gate-evidence", rules(issues_p), [str(i) for i in issues_p])

    def test_positive_relayed_report_sets_reported_flags_not_verified_or_approval(self):
        # The 94197fc8 seed itself is this positive case, driven by the latest
        # (current) report. Inferred flags stay. install-verified and
        # owner-hardware-approval stay false.
        issues = validate_record(RECORD_941, SCHEMA)
        self.assertEqual([str(i) for i in issues], [])
        states = RECORD_941["states"]
        self.assertTrue(states["installation-reported"])
        self.assertTrue(states["boot-restart-reported"])
        self.assertFalse(states["install-verified"])
        self.assertFalse(states["owner-hardware-approval"])
        self.assertEqual(RECORD_941["identity_confidence"]["model"], "inferred")
        self.assertEqual(RECORD_941["identity_confidence"]["build"], "inferred")
        self.assertEqual(RECORD_941["identity_confidence"]["installed_hash"], "absent")
        current = RECORD_941["history"][1]
        self.assertTrue(current["is_current"])
        self.assertFalse(current["quoted"])
        self.assertEqual(current["source"], "owner-reported via Dot relay")
        self.assertIsNone(current.get("quoted_words"))
        self.assertFalse(current["scope"]["full_functional_suite"])
        self.assertEqual(current["confidence"], "reported")
        self.assertIn("automatic restart", current["events"])
        self.assertIn("boot at normal speed", current["events"])

        # A proposal that only restates those reported flags also passes,
        # as long as it does not claim verification or approval.
        issues_p = propose(
            RECORD_941,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {
                    "installation-reported": True,
                    "boot-restart-reported": True,
                    "install-verified": False,
                    "owner-hardware-approval": False,
                },
                "evidence": {
                    "report": True,
                    "owner_words_quoted": False,
                    "owner_source": "relay",
                    "boot_or_restart_observed": True,
                },
            },
        )
        self.assertEqual([str(i) for i in issues_p], [])

    def test_synthetic_unresolved_conflict_cannot_promote(self):
        # Generic rule only. The 0.0.26 seed has no conflict flag.
        self.assertNotIn("conflicts", RECORD_941)
        rec = copy.deepcopy(RECORD_941)
        rec["conflicts"] = [
            {
                "kind": "restart-outcome",
                "status": "unresolved",
                "event_ids": [1, 2],
                "summary": "synthetic unresolved conflict for the generic gate test",
            }
        ]
        # The seed's existing reported flags remain valid; hardware promotion must fail.
        issues_ok = validate_record(rec, SCHEMA)
        self.assertEqual([str(i) for i in issues_ok], [])
        issues = propose(
            rec,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {"install-verified": True, "owner-hardware-approval": True},
                "identity_confidence": {
                    "model": "verified",
                    "build": "verified",
                    "installed_hash": "verified",
                },
                "evidence": {
                    "installed_hash": "present",
                    "owner_words_quoted": True,
                    "owner_timestamp": "2026-11-13T12:00:00-06:00",
                    "owner_source_link": "https://example.invalid/owner",
                    "promote": True,
                },
                "promote": True,
                "conflicts": rec["conflicts"],
            },
        )
        self.assertIn("unresolved-conflict-no-promotion", rules(issues), [str(i) for i in issues])


    def test_pro_hash_claimed_as_elite_must_not_pass_install_verified(self):
        elite = RECORD_941["models"]["elite"]["image_sha256"]
        pro = RECORD_941["models"]["pro"]["image_sha256"]
        self.assertNotEqual(elite, pro)
        issues = propose(
            RECORD_941,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {"install-verified": True},
                "identity_confidence": {
                    "model": "verified",
                    "build": "verified",
                    "installed_hash": "verified",
                },
                "evidence": {
                    "model": "elite",
                    "installed_hash": pro,
                    "installed_hash_model": "elite",
                    "model_proof": "board_id",
                },
            },
        )
        self.assertIn("installed-hash-mismatch", rules(issues), [str(i) for i in issues])

    def test_foreign_hash_must_not_pass_install_verified(self):
        foreign = "a" * 64
        issues = propose(
            RECORD_941,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {"install-verified": True},
                "identity_confidence": {
                    "model": "verified",
                    "build": "verified",
                    "installed_hash": "verified",
                },
                "evidence": {
                    "model": "elite",
                    "installed_hash": foreign,
                    "model_proof": "flash_id",
                },
            },
        )
        self.assertIn("installed-hash-mismatch", rules(issues), [str(i) for i in issues])

    def test_confidence_only_without_stored_hash_must_not_pass_install_verified(self):
        rec = copy.deepcopy(RECORD_941)
        rec["states"]["install-verified"] = True
        rec["identity_confidence"] = {
            "model": "verified",
            "build": "verified",
            "installed_hash": "verified",
        }
        issues = validate_record(rec, SCHEMA)
        self.assertIn("absent-installed-hash", rules(issues), [str(i) for i in issues])

    def test_empty_evidence_proposal_must_not_pass_install_verified(self):
        issues = propose(
            RECORD_941,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {"install-verified": True},
                "identity_confidence": {
                    "model": "verified",
                    "build": "verified",
                    "installed_hash": "verified",
                },
                "evidence": {},
            },
        )
        self.assertIn("absent-installed-hash", rules(issues), [str(i) for i in issues])

    def test_usb_product_string_without_flags_must_not_pass(self):
        issues = propose(
            RECORD_941,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {"install-verified": True},
                "identity_confidence": {
                    "model": "verified",
                    "build": "verified",
                    "installed_hash": "verified",
                    "model_note": "model taken from USB name FM-1",
                },
                "evidence": {
                    "installed_hash": RECORD_941["models"]["elite"]["image_sha256"],
                    "usb_name": "FM-1",
                    "model_proof": "USB product string FM-1",
                    "model": "elite",
                },
            },
        )
        self.assertIn("usb-name-not-model-proof", rules(issues), [str(i) for i in issues])

    def test_relabeled_d408_exact_image_without_matching_hashes_fails(self):
        issues = propose(
            RECORD_941,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {"gate-passed": True},
                "gate_evidence": [
                    {
                        "kind": "emu360-strict",
                        "result": "PASS",
                        "elite_worst_us": 11406,
                        "pro_worst_us": 11410,
                        "elite_late": 0,
                        "pro_late": 0,
                        "threshold_us": 11609,
                        "ref": "d408 numbers relabeled as exact-image",
                        "applies_to_image": "exact-image",
                    }
                ],
            },
        )
        self.assertIn("emu360-image-hash", rules(issues), [str(i) for i in issues])



    def test_elite_approval_never_implies_pro(self):
        elite = RECORD_941["models"]["elite"]["image_sha256"]
        pro = RECORD_941["models"]["pro"]["image_sha256"]
        elite_approval = {
            "model": "elite",
            "source_sha": RECORD_941["source_sha"],
            "image_sha256": elite,
            "approved": True,
            "quoted": True,
            "quoted_words": "synthetic quoted words for the Elite-only scope test only",
            "time": "2026-11-13T12:00:00-06:00",
            "source_link": "https://example.invalid/owner-elite",
        }
        # Overall owner-hardware-approval true with Elite only must fail.
        issues = propose(
            RECORD_941,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {
                    "install-verified": True,
                    "owner-hardware-approval": True,
                },
                "identity_confidence": {
                    "model": "verified",
                    "build": "verified",
                    "installed_hash": "verified",
                },
                "hardware_approvals": [elite_approval],
                "evidence": {
                    "installed_hash": elite,
                    "model": "elite",
                    "model_proof": "board_id",
                    "owner_words_quoted": True,
                    "owner_timestamp": "2026-11-13T12:00:00-06:00",
                    "owner_source_link": "https://example.invalid/owner-elite",
                    "imply_pro_from_elite": True,
                },
            },
        )
        self.assertIn("elite-approval-not-pro", rules(issues), [str(i) for i in issues])

        # Copying an Elite approval onto Pro must fail.
        issues2 = propose(
            RECORD_941,
            {
                "record_id": "nightly-94197fc8-dev",
                "set_states": {"owner-hardware-approval": True, "install-verified": True},
                "identity_confidence": {
                    "model": "verified",
                    "build": "verified",
                    "installed_hash": "verified",
                },
                "hardware_approvals": [
                    elite_approval,
                    {
                        "model": "pro",
                        "source_sha": RECORD_941["source_sha"],
                        "image_sha256": pro,
                        "approved": True,
                        "note": "copied from Elite approval",
                    },
                ],
                "evidence": {
                    "installed_hash": elite,
                    "model": "pro",
                    "model_proof": "board_id",
                    "owner_words_quoted": True,
                    "owner_timestamp": "2026-11-13T12:00:00-06:00",
                    "owner_source_link": "https://example.invalid/owner-elite",
                    "copy_approval_from": "elite",
                    "approval_model": "pro",
                },
            },
        )
        self.assertIn("elite-approval-not-pro", rules(issues2), [str(i) for i in issues2])

        # Elite-only scoped approval, overall flag false, is not a Pro approval.
        rec = copy.deepcopy(RECORD_941)
        rec["hardware_approvals"] = [elite_approval]
        rec["states"]["owner-hardware-approval"] = False
        issues3 = validate_record(rec, SCHEMA)
        # May fail other rules (install-verified still false) but must not
        # treat Pro as approved. The overall flag is false, so the scope
        # rule does not fire; Pro is not approved.
        self.assertFalse(rec["states"]["owner-hardware-approval"])
        self.assertTrue(all(a.get("model") != "pro" or not a.get("approved") for a in rec["hardware_approvals"]))

    def test_toolchain_is_not_inferred(self):
        rec = copy.deepcopy(RECORD_941)
        rec["elf_provenance"]["toolchain"] = "arm-none-eabi-gcc (inferred)"
        rec["elf_provenance"].pop("toolchain_source", None)
        issues = validate_record(rec, SCHEMA)
        self.assertIn("toolchain-inferred", rules(issues), [str(i) for i in issues])
        # null remains accepted
        rec2 = copy.deepcopy(RECORD_941)
        rec2["elf_provenance"]["toolchain"] = None
        issues2 = validate_record(rec2, SCHEMA)
        self.assertNotIn("toolchain-inferred", rules(issues2), [str(i) for i in issues2])

    def test_94197fc8_is_nightly_hardware_test_not_promoted(self):
        self.assertEqual(RECORD_941["channel"], "nightly-hardware-test")
        self.assertFalse(RECORD_941["states"]["owner-hardware-approval"])
        self.assertFalse(RECORD_941["states"]["install-verified"])
        self.assertFalse(RECORD_941["states"]["withdrawn"])



if __name__ == "__main__":
    unittest.main()
