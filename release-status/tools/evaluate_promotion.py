#!/usr/bin/env python3
"""Promotion evaluator: report owner-required gates. Output only. Never promote.

For each release record and its channel, this tool reports whether each
owner-required gate is pass, fail, or missing, based on the evidence in
the record. It does not write records, audit lines, catalogs, or devices.
It does not infer hardware qualification from an upload or from emulation.
It never promotes a build automatically, even when every gate is pass.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from evidence_policy import approval_errors, installation_for_model, direct_source_link, parse_instant

from validate_release_status import (  # noqa: E402
    HARDWARE_PROMOTION_FLAGS,
    MODEL_PROOF_TYPES,
    PROMOTION_FLAGS,
    SHA256,
    UsageError,
    default_paths,
    emu360_exact_pass,
    firmware_readback_ok,
    is_usb_or_product_proof,
    load_json,
    load_records,
    models_of,
    offered_model_proof,
    record_id_of,
    resolve_claimed_model,
    resolve_installed_hash,
    unresolved_conflicts,
)

# Channels kept distinct. A nightly-hardware-test record is never treated
# as a MAIN promotion. A withdrawn record cannot be promoted.
CHANNELS = ("stable-MAIN", "nightly-hardware-test", "withdrawn")

# Gates evaluated for every record. Hardware gates are also split by model
# because an Elite result never implies a Pro result.
EVIDENCE_GATES = (
    "built",
    "gate-passed",
    "published-nightly",
    "installation-reported",
    "boot-restart-reported",
    "withdrawn",
)

HARDWARE_GATES = (
    "install-verified",
    "owner-hardware-approval",
)

PASS = "pass"
FAIL = "fail"
MISSING = "missing"

# Owner-required gates that must all pass before the owner can consider
# a hardware-approved MAIN promotion. Emulation and publication are not
# in this list: they are software or catalog facts, not hardware proof.
OWNER_REQUIRED_FOR_MAIN_PROMOTION = (
    "install-verified:elite",
    "install-verified:pro",
    "owner-hardware-approval:elite",
    "owner-hardware-approval:pro",
)


def _result(
    gate: str,
    status: str,
    reason: str,
    *,
    model: str | None = None,
    hardware: bool = False,
    notes: list[str] | None = None,
) -> dict[str, Any]:
    if status not in (PASS, FAIL, MISSING):
        raise ValueError(f"invalid gate status {status!r}")
    item: dict[str, Any] = {
        "gate": gate,
        "status": status,
        "reason": reason,
        "hardware_qualification": False if not hardware else status == PASS,
        "from_upload": False,
        "from_emulation": gate == "gate-passed",
    }
    if model is not None:
        item["model"] = model
    if notes:
        item["notes"] = list(notes)
    # Emulation and upload never count as hardware qualification.
    if gate == "gate-passed" or gate == "published-nightly" or gate == "built":
        item["hardware_qualification"] = False
    return item


def _history(record: dict) -> list[dict]:
    return [h for h in (record.get("history") or []) if isinstance(h, dict)]


def _install_history(record: dict) -> list[dict]:
    """Return history events that are install reports.

    Publish and withdrawal events are not install reports, even when their
    text mentions the installer or a recovery build.
    """
    kinds = {"install-and-boot-report", "install-report"}
    skip = {"publish", "withdrawal", "withdraw"}
    out = []
    for h in _history(record):
        kind = h.get("kind") or ""
        if kind in skip:
            continue
        events = h.get("events") or []
        event_words = [str(e).lower() for e in events]
        if kind in kinds or any(w == "install" or w.startswith("install ") for w in event_words):
            out.append(h)
    return out


def _boot_history(record: dict) -> list[dict]:
    """Return history events that report a boot or a restart observation.

    A withdrawal that mentions boot failures is not a boot-success report.
    A publish event is not a boot report.
    """
    skip = {"publish", "withdrawal", "withdraw"}
    out = []
    for h in _history(record):
        kind = h.get("kind") or ""
        if kind in skip:
            continue
        events = " ".join(str(e) for e in (h.get("events") or [])).lower()
        if (
            kind == "install-and-boot-report"
            or "boot" in events
            or "restart" in events
        ):
            out.append(h)
    return out


def evaluate_built(record: dict) -> dict[str, Any]:
    rid = record_id_of(record)
    models = models_of(record)
    if not models:
        return _result("built", MISSING, "no model images are present")
    missing = []
    failing = []
    for name, model in models.items():
        if not SHA256.fullmatch(str(model.get("image_sha256") or "")):
            missing.append(f"{name} image_sha256")
        if not isinstance(model.get("image_size"), int) or model.get("image_size") < 1:
            missing.append(f"{name} image_size")
        if model.get("controller_strategy") == "bundled":
            check = model.get("controller_check")
            if check is None:
                missing.append(f"{name} controller_check")
            elif check != "PASS":
                failing.append(f"{name} controller_check is {check!r}; bundled built requires PASS")
    if failing:
        return _result("built", FAIL, f"{rid}: " + "; ".join(failing))
    if missing:
        return _result("built", MISSING, f"{rid}: missing " + ", ".join(missing))
    return _result("built", PASS, "images exist and bundled controller checks are PASS")


def evaluate_gate_passed(record: dict) -> dict[str, Any]:
    """Evaluate the emu360 exact-image gate. This is never hardware proof."""
    items = [g for g in (record.get("gate_evidence") or []) if isinstance(g, dict)]
    emu = [g for g in items if g.get("kind") == "emu360-strict"]
    notes = [
        "gate-passed is an emulator result. It is not a hardware result.",
        "An upload or a catalog publication is not hardware qualification.",
    ]
    if not emu:
        return _result(
            "gate-passed",
            MISSING,
            "no emu360-strict evidence item is present",
            notes=notes,
        )
    exact_ok = False
    other = False
    reasons: list[str] = []
    for item in emu:
        if item.get("applies_to_image") == "other-inputs":
            other = True
            reasons.append("emu360 result applies to other-inputs, not the exact images")
        ok, reason = emu360_exact_pass(item, record)
        if ok:
            exact_ok = True
        elif reason:
            reasons.append(reason)
    if exact_ok:
        return _result(
            "gate-passed",
            PASS,
            "exact-image emu360 strict PASS (0 late, worst time under the threshold, matching image hashes). This is not a hardware result.",
            notes=notes,
        )
    if other:
        return _result(
            "gate-passed",
            FAIL,
            "an emu360 result on other inputs must not pass gate-passed for these images",
            notes=notes,
        )
    # Evidence exists but does not meet PASS conditions.
    return _result(
        "gate-passed",
        FAIL,
        reasons[0] if reasons else "emu360-strict evidence is present but does not meet PASS conditions",
        notes=notes,
    )


def evaluate_published_nightly(record: dict) -> dict[str, Any]:
    models = models_of(record)
    has_sha = any(
        SHA256.fullmatch(str(m.get("package_sha256") or m.get("image_sha256") or ""))
        for m in models.values()
    )
    missing = []
    if not record.get("catalog_id"):
        missing.append("catalog_id")
    if not record.get("published_pages_url"):
        missing.append("published_pages_url")
    if not has_sha:
        missing.append("published SHA-256")
    notes = ["A catalog publication or upload is not hardware qualification."]
    if missing:
        return _result(
            "published-nightly",
            MISSING,
            "missing " + ", ".join(missing),
            notes=notes,
        )
    if record.get("states", {}).get("withdrawn") or record.get("channel") == "withdrawn":
        return _result(
            "published-nightly",
            PASS,
            "the exact package was published, and was later withdrawn. A past publication is not a current offer and is not hardware qualification.",
            notes=notes + ["This record is withdrawn. Do not offer it for install."],
        )
    return _result(
        "published-nightly",
        PASS,
        "catalog id, Pages URL, and published SHA-256 are present. This is not hardware qualification.",
        notes=notes,
    )


def evaluate_installation_reported(record: dict) -> dict[str, Any]:
    reports = _install_history(record)
    if not reports:
        return _result(
            "installation-reported",
            MISSING,
            "no install report is present in history",
        )
    sources = [str(h.get("source") or "") for h in reports]
    return _result(
        "installation-reported",
        PASS,
        "an install report is present (" + "; ".join(sources) + "). A report is not install-verified and is not hardware approval.",
    )


def evaluate_boot_restart_reported(record: dict) -> dict[str, Any]:
    reports = _boot_history(record)
    if not reports:
        return _result(
            "boot-restart-reported",
            MISSING,
            "no boot or restart report is present in history",
        )
    # Web install success alone is not boot proof. The seeds that have a
    # boot report describe observed boot or restart, not a web-only success.
    web_only = any(
        (h.get("web_install_success_only") or h.get("web_install_success"))
        and h.get("boot_or_restart_observed") is not True
        for h in reports
    )
    if web_only:
        return _result(
            "boot-restart-reported",
            FAIL,
            "web install success alone is not boot, restart, or hardware proof",
        )
    claims_verified_proof = any(h.get("boot_restart_verified") is True for h in reports)
    conf = record.get("identity_confidence") or {}
    identity_verified = conf.get("model") == "verified" and conf.get("build") == "verified"
    if claims_verified_proof and not identity_verified:
        return _result(
            "boot-restart-reported",
            FAIL,
            "a verified-proof boot claim requires verified model and build identity",
        )
    note = (
        "Identity is verified."
        if claims_verified_proof and identity_verified
        else "This report is not verified proof."
    )
    sources = [str(h.get("source") or "") for h in reports]
    return _result(
        "boot-restart-reported",
        PASS,
        "a boot or restart report is present (" + "; ".join(sources) + "). " + note + " This is not hardware approval.",
        notes=["A web install success alone is not boot or hardware proof."],
    )


def evaluate_withdrawn(record: dict) -> dict[str, Any]:
    withdrawn = bool((record.get("states") or {}).get("withdrawn")) or record.get("channel") == "withdrawn"
    if not withdrawn:
        return _result(
            "withdrawn",
            PASS,
            "the record is not withdrawn",
            notes=["status pass here means 'not withdrawn' is consistent; it is not a promotion."],
        )
    if not record.get("withdraw_ref") or not record.get("withdraw_reason"):
        return _result(
            "withdrawn",
            FAIL,
            "withdrawn is set but a withdrawal ref or reason is missing",
        )
    return _result(
        "withdrawn",
        PASS,
        f"withdrawn: {record.get('withdraw_reason')} (ref {record.get('withdraw_ref')}). A withdrawn record cannot gain a promotion flag.",
        notes=["Do not offer this exact build for install."],
    )


def _model_tested_in_history(record: dict, model: str) -> list[dict]:
    want = model.lower()
    aliases = {want, "smk-37 " + want, "smk37 " + want}
    found = []
    for h in _history(record):
        raw = str(h.get("model") or "")
        if raw.lower() in aliases or want == raw.lower():
            found.append(h)
    return found


def evaluate_install_verified(record: dict, model: str) -> dict[str, Any]:
    """An independently scoped hardware receipt, not record-wide or browser proof."""
    gate = f"install-verified:{model}"
    notes = [
        "An Elite verification never implies a Pro verification.",
        "Web transfer acknowledgement and emulation are not device installation proof.",
    ]
    if model not in models_of(record):
        return _result(gate, FAIL, f"unknown model {model}", model=model, hardware=True, notes=notes)
    qualified, reason = installation_for_model(record, model)
    if not qualified:
        contradicted = "mismatch" in reason or "USB" in reason
        return _result(gate, FAIL if contradicted else MISSING,
                       f"{model}: {reason}. Elite never implies Pro. This is not install-verified.",
                       model=model, hardware=True, notes=notes)
    return _result(gate, PASS,
                   f"{model}: {reason}; model and build proof are independent.",
                   model=model, hardware=True, notes=notes)


def _quoted_owner_history(record: dict, model: str | None = None) -> list[dict]:
    """History is explanatory; no model-less quote may authorize a model."""
    if model is None:
        return []
    return [
        h for h in _history(record)
        if h.get("quoted") is True
        and str(h.get("model") or "").strip().lower() == model.lower()
        and "owner" in str(h.get("source") or "").lower()
        and parse_instant(h.get("time")) is not None
        and direct_source_link(h.get("source_link"))
    ]


def evaluate_owner_hardware_approval(record: dict, model: str) -> dict[str, Any]:
    gate = f"owner-hardware-approval:{model}"
    notes = [
        "A distinct model-named owner quotation/time/source_link and image identity are mandatory.",
        "Emulation, transfer ACK and relayed words are not hardware approval.",
        "Every model must have its own verified installation receipt.",
    ]
    models = models_of(record)
    if model not in models:
        return _result(gate, FAIL, f"unknown model {model}", model=model, hardware=True, notes=notes)
    approvals = [a for a in record.get("hardware_approvals", [])
                 if isinstance(a, dict) and a.get("model") == model and a.get("approved") is True]
    install = evaluate_install_verified(record, model)
    if not approvals:
        return _result(gate, MISSING,
                       f"{model}: no independent owner hardware approval for this exact build; "
                       f"install-verified is {install['status']}. Elite never implies Pro.",
                       model=model, hardware=True, notes=notes)
    reasons = []
    if len(approvals) != 1:
        reasons.append("multiple approval receipts are ambiguous")
    for approval in approvals:
        reasons.extend(approval_errors(approval, record))
    if install["status"] != PASS:
        reasons.append(f"install-verified for {model} is {install['status']}, not pass")
    if reasons:
        status = FAIL if any("does not match" in r or "mismatch" in r for r in reasons) else MISSING
        return _result(gate, status,
                       f"{model}: " + "; ".join(reasons) + ". This is not owner-hardware-approval.",
                       model=model, hardware=True, notes=notes)
    return _result(gate, PASS,
                   f"{model}: separately quoted owner approval/time/direct source_link matches this source "
                   "and image; exact-model install receipt passes.",
                   model=model, hardware=True, notes=notes)


def evaluate_record(record: dict) -> dict[str, Any]:
    rid = record_id_of(record)
    channel = record.get("channel") or ""
    version = record.get("version") or ""
    models = list(models_of(record).keys())
    gates: list[dict[str, Any]] = []
    gates.append(evaluate_built(record))
    gates.append(evaluate_gate_passed(record))
    gates.append(evaluate_published_nightly(record))
    gates.append(evaluate_installation_reported(record))
    gates.append(evaluate_boot_restart_reported(record))
    gates.append(evaluate_withdrawn(record))

    hardware_by_model: dict[str, list[dict[str, Any]]] = {}
    for model in models:
        items = [
            evaluate_install_verified(record, model),
            evaluate_owner_hardware_approval(record, model),
        ]
        hardware_by_model[model] = items
        gates.extend(items)

    conflicts = unresolved_conflicts(record)
    conflict_block = bool(conflicts)

    withdrawn = bool((record.get("states") or {}).get("withdrawn")) or channel == "withdrawn"
    # Owner-required MAIN promotion readiness: every model, both hardware gates.
    main_required = []
    for model in models:
        for item in hardware_by_model.get(model, []):
            main_required.append(item)

    missing_or_fail = [g for g in main_required if g["status"] != PASS]
    ready = (not withdrawn) and (not conflict_block) and (not missing_or_fail) and bool(main_required)

    if channel == "nightly-hardware-test":
        channel_note = (
            "Channel nightly-hardware-test is not a MAIN promotion. "
            "This report does not move the record to stable-MAIN."
        )
    elif channel == "withdrawn":
        channel_note = (
            "Channel withdrawn: this exact build must not be offered and cannot gain a promotion flag."
        )
    elif channel == "stable-MAIN":
        channel_note = (
            "Channel stable-MAIN. Hardware approval still requires install-verified and "
            "owner-hardware-approval on each model. An Elite result never implies Pro."
        )
    else:
        channel_note = f"Channel {channel or '(unset)'} is reported as stored. This tool does not change the channel."

    return {
        "record_id": rid,
        "version": version,
        "channel": channel,
        "source_sha": record.get("source_sha"),
        "states": dict(record.get("states") or {}),
        "identity_confidence": dict(record.get("identity_confidence") or {}),
        "models": models,
        "gates": gates,
        "hardware_gates_by_model": hardware_by_model,
        "elite_never_implies_pro": True,
        "unresolved_conflicts": conflicts,
        "withdrawn": withdrawn,
        "channel_note": channel_note,
        "owner_required_for_main_promotion": list(OWNER_REQUIRED_FOR_MAIN_PROMOTION)
        if set(models) >= {"elite", "pro"}
        else [f"install-verified:{m}" for m in models]
        + [f"owner-hardware-approval:{m}" for m in models],
        "owner_required_promotion_ready": ready,
        "automatic_promotion": "not-performed",
        "promotion_performed": False,
        "hardware_qualified": False if not ready else False,  # never auto-claim; owner decides
        "hardware_qualified_note": (
            "This tool never marks a build as hardware-qualified. "
            "Emulation, an upload, and a catalog publication are not hardware qualification. "
            "If every owner-required gate is pass, the owner still decides."
        ),
        "ready_reason": (
            "every owner-required hardware gate is pass on every model; this tool still does not promote"
            if ready
            else (
                "withdrawn: promotion is refused"
                if withdrawn
                else (
                    "unresolved conflict: promotion is refused"
                    if conflict_block
                    else "owner-required hardware gates are not all pass: "
                    + "; ".join(f"{g['gate']}={g['status']}" for g in missing_or_fail)
                )
            )
        ),
    }


def evaluate_all(records: list[tuple[str, dict]]) -> dict[str, Any]:
    reports = [evaluate_record(rec) for _name, rec in records]
    return {
        "schema": 1,
        "tool": "release-status/tools/evaluate_promotion.py",
        "role": "report-only",
        "automatic_promotion": "not-performed",
        "promotion_performed": False,
        "elite_never_implies_pro": True,
        "notes": [
            "This report does not promote a build, publish a package, or write a record.",
            "Hardware qualification is never inferred from an upload or from emulation.",
            "An Elite result never implies a Pro result.",
            "Channels stay distinct: nightly-hardware-test is not stable-MAIN; withdrawn stays withdrawn.",
        ],
        "records": reports,
    }


def format_text(report: dict[str, Any]) -> str:
    lines: list[str] = []
    lines.append("DnBOX promotion evaluator (report only, never promotes)")
    lines.append("Automatic promotion: not-performed")
    lines.append("Elite never implies Pro.")
    lines.append("Emulation and uploads are not hardware qualification.")
    lines.append("")
    for rec in report.get("records") or []:
        lines.append(f"== {rec.get('version')} ({rec.get('record_id')}) channel={rec.get('channel')} ==")
        lines.append(f"source_sha={rec.get('source_sha')}")
        lines.append(f"withdrawn={rec.get('withdrawn')}")
        lines.append(rec.get("channel_note") or "")
        for g in rec.get("gates") or []:
            model = f" [{g['model']}]" if g.get("model") else ""
            lines.append(f"  {g['status'].upper():7}  {g['gate']}{model}")
            lines.append(f"           {g['reason']}")
        lines.append(f"  owner_required_promotion_ready={rec.get('owner_required_promotion_ready')}")
        lines.append(f"  ready_reason: {rec.get('ready_reason')}")
        lines.append(f"  automatic_promotion={rec.get('automatic_promotion')}")
        lines.append("")
    lines.append("End of report. No promotion was performed.")
    return "\n".join(lines) + "\n"


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    paths = default_paths()
    p = argparse.ArgumentParser(
        description="Report owner-required promotion gates. Output only. Never promote."
    )
    p.add_argument("--records", type=Path, default=paths["records"], help="records directory")
    p.add_argument("--json", action="store_true", help="print JSON instead of text")
    p.add_argument("--output", type=Path, default=None, help="write the report to this path")
    return p.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    try:
        args = parse_args(argv)
        if not args.records.is_dir():
            raise UsageError(f"records directory not found: {args.records}")
        records = load_records(args.records)
        if not records:
            raise UsageError(f"no records found in {args.records}")
        report = evaluate_all(records)
        text = json.dumps(report, indent=2, sort_keys=True) + "\n" if args.json else format_text(report)
        if args.output:
            args.output.write_text(text)
        else:
            sys.stdout.write(text)
        return 0
    except UsageError as exc:
        sys.stderr.write(f"USAGE: {exc}\n")
        return 2


if __name__ == "__main__":
    sys.exit(main())
