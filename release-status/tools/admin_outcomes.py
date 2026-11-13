#!/usr/bin/env python3
"""Owner-admin outcomes and untrusted installer telemetry; never promotion authority."""

from __future__ import annotations

import copy
import re
from datetime import datetime, timezone

from evidence_policy import direct_source_link, parse_instant, quote_names_one_model

HEX64 = re.compile(r"^[0-9a-f]{64}$")
STAGES = frozenset({"installed-reported", "restart-reported", "restart-and-boot-reported", "tested-with-issue"})
REPORT_STAGES = ("transfer-started", "transfer-completed", "restart-observed", "boot-observed", "failed")
MAX_NOTE = 700


class OutcomeError(ValueError):
    pass


def _clean_note(value: object) -> str:
    if not isinstance(value, str) or len(value) > MAX_NOTE:
        raise OutcomeError("note must be a string of at most 700 characters")
    return value.strip()


def checked_package(record: dict, model: object, package_sha256: object) -> dict:
    if model not in ("elite", "pro"):
        raise OutcomeError("explicit Elite or Pro selection is required")
    if record.get("states", {}).get("withdrawn"):
        raise OutcomeError("withdrawn package cannot be used for a new owner test")
    candidate = (record.get("models") or {}).get(model)
    if not isinstance(candidate, dict):
        raise OutcomeError("model is absent from exact release record")
    expected = candidate.get("package_sha256")
    if not isinstance(package_sha256, str) or not HEX64.fullmatch(package_sha256):
        raise OutcomeError("64-character package SHA-256 is required")
    if expected != package_sha256:
        raise OutcomeError("selected package does not match this record and model")
    if not HEX64.fullmatch(str(candidate.get("image_sha256") or "")):
        raise OutcomeError("exact model image SHA-256 is unknown")
    return candidate


def prepare_owner_outcome(record: dict, entry: dict, *, owner_login: str) -> dict:
    """Author a scoped reported event. Never upgrade confidence to verified."""
    if owner_login != "gobbajogger":
        raise OutcomeError("actor not verified as the allowed GitHub owner")
    if not isinstance(entry, dict):
        raise OutcomeError("JSON outcome object required")
    model = entry.get("model")
    candidate = checked_package(record, model, entry.get("package_sha256"))
    if entry.get("catalog_id") != record.get("catalog_id"):
        raise OutcomeError("catalog build identity does not match")
    stage = entry.get("stage")
    if stage not in STAGES:
        raise OutcomeError("unknown owner reporting stage")
    observed_at = entry.get("observed_at")
    if parse_instant(observed_at) is None:
        raise OutcomeError("observed_at must be a real timezone-aware ISO 8601 instant")
    if parse_instant(observed_at) > datetime.now(timezone.utc):
        raise OutcomeError("future event times cannot be recorded as observed")
    note = _clean_note(entry.get("note", ""))
    quote = _clean_note(entry.get("quoted_words", ""))
    source_link = entry.get("source_link", "")
    if source_link and not direct_source_link(source_link):
        raise OutcomeError("source_link must be a direct non-local HTTPS URL")
    if quote and (not direct_source_link(source_link) or not quote_names_one_model(quote, model)):
        raise OutcomeError("quoted owner words need an independent model-named quote and direct source")
    history = copy.deepcopy(record.get("history") or [])
    for h in history:
        if isinstance(h, dict):
            h["is_current"] = False
    sequence = max((h.get("sequence", 0) for h in history if isinstance(h, dict)), default=0) + 1
    event_names = {
        "installed-reported": ["install"],
        "restart-reported": ["restart"],
        "restart-and-boot-reported": ["restart", "boot"],
        "tested-with-issue": ["test", "issue"],
    }
    kinds = {
        "installed-reported": "install-report",
        "restart-reported": "boot-restart-report",
        "restart-and-boot-reported": "install-and-boot-report",
        "tested-with-issue": "other",
    }
    history.append({
        "sequence": sequence,
        "time": observed_at,
        "time_precision": "exact",
        "order": "latest",
        "kind": kinds[stage],
        "source": "authenticated GitHub owner via local DnBOX admin",
        "quoted": bool(quote),
        "confidence": "reported",
        "verified": False,
        "is_current": True,
        "text": note or f"Owner recorded {stage} on {model}; exact installed image unverified.",
        "events": event_names[stage],
        "model": model.title(),
        "model_confidence": "reported",
        "build": record.get("version"),
        "build_confidence": "reported",
        "installed_hash": None,
        "installed_hash_confidence": "absent",
        "selected_package_sha256": candidate["package_sha256"],
        "selected_source_sha": record["source_sha"],
        "scope": {
            "full_functional_suite": False,
            "hardware_qualification_both_models": False,
            "promotion": False,
        },
        "notes": [
            "Package selection matches the catalog; installed image was not read back.",
            "This stage is owner-reported, not an automated installation or model verification.",
            "Only a separate per-model approval and measured readback can advance hardware gates.",
        ],
        **({"quoted_words": quote, "source_link": source_link} if quote else {}),
    })
    states = {}
    if stage in ("installed-reported", "restart-and-boot-reported", "tested-with-issue"):
        states["installation-reported"] = True
    if stage == "restart-and-boot-reported":
        states["boot-restart-reported"] = True
    return {
        "record_id": record["catalog_id"], "history": history,
        "current_report_sequence": sequence,
        "set_states": states,
    }


def sanitize_installer_report(record: dict, report: dict) -> dict:
    """Opt-in, privacy-minimal and UNTRUSTED. Never returns approval or states."""
    if not isinstance(report, dict) or report.get("consent") is not True:
        raise OutcomeError("explicit opt-in consent required")
    model = report.get("model")
    candidate = checked_package(record, model, report.get("package_sha256"))
    if report.get("catalog_id") != record.get("catalog_id"):
        raise OutcomeError("catalog identity mismatch")
    stage = report.get("stage")
    if stage not in REPORT_STAGES:
        raise OutcomeError("unrecognized installer stage")
    run_id = report.get("run_id")
    if not isinstance(run_id, str) or not re.fullmatch(r"[0-9a-f-]{36}", run_id):
        raise OutcomeError("ephemeral run UUID required")
    seq = report.get("sequence")
    if type(seq) is not int or not (1 <= seq <= 20):
        raise OutcomeError("bounded sequence number required")
    observed_at = report.get("observed_at")
    if parse_instant(observed_at) is None:
        raise OutcomeError("reported time must be ISO with timezone")
    if parse_instant(observed_at) > datetime.now(timezone.utc):
        raise OutcomeError("future installer event time")
    # No IP, USB vendor/product, serial, MAC, browser fingerprint or token.
    return {
        "catalog_id": record["catalog_id"],
        "source_sha": record["source_sha"],
        "model": model,
        "package_sha256": candidate["package_sha256"],
        "run_id": run_id,
        "sequence": seq,
        "stage": stage,
        "observed_at": observed_at,
        "confidence": "anonymous-unverified",
        "promotion_authority": False,
    }
