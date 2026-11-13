#!/usr/bin/env python3
"""Pure promotion candidate evaluator; deliberately no publication machinery."""

from __future__ import annotations

from evaluate_promotion import evaluate_record

REQUIRED = (
    "built", "gate-passed", "published-nightly",
    "installation-reported", "boot-restart-reported",
    "install-verified:elite", "install-verified:pro",
    "owner-hardware-approval:elite", "owner-hardware-approval:pro",
)


def evaluate_candidate(record: dict) -> dict:
    """Show gate status for BOTH models, separate from promotion authority."""
    report = evaluate_record(record)
    gates = {g["gate"]: g for g in report["gates"]}
    missing = [name for name in REQUIRED if gates.get(name, {}).get("status") != "pass"]
    withdrawn = bool(record.get("states", {}).get("withdrawn"))
    if withdrawn:
        missing.insert(0, "withdrawn")
    if record.get("channel") not in ("stable-MAIN", "nightly-hardware-test"):
        missing.insert(0, "channel")
    return {
        "catalog_id": record.get("catalog_id"),
        "source_sha": record.get("source_sha"),
        "channel": record.get("channel"),
        "gate_pass": not missing,
        "blocking_gates": missing,
        "model_gate_status": {
            model: {
                "install_verified": gates.get(f"install-verified:{model}", {}).get("status", "missing"),
                "owner_hardware_approval": gates.get(f"owner-hardware-approval:{model}", {}).get("status", "missing"),
            }
            for model in ("elite", "pro")
        },
        "deployment_authority": "not-confirmed",
        "promotion_authorized": False,
        "promotion_performed": False,
        "dry_run": True,
        "notice": "A passing gate snapshot is not a signed authority to publish or promote. "
                  "This tool cannot update firmware releases, main, nightly or devices.",
    }
