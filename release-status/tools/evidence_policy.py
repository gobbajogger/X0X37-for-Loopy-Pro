#!/usr/bin/env python3
"""Fail-closed, model-specific release proof checks. No deployment authority."""

from __future__ import annotations

import re
from datetime import datetime, timezone
from urllib.parse import urlsplit

MODEL_PROOF_TYPES = frozenset({
    "board_id", "model_id", "flash_id", "mac",
    "firmware_readback", "backup_field",
})
ISO_INSTANT = re.compile(
    r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}"
    r"(?:\.\d{1,6})?(?:Z|[+-]\d{2}:\d{2})$"
)


def parse_instant(value: object) -> datetime | None:
    """An actual ISO 8601 instant, not a vaguely date-like string."""
    if not isinstance(value, str) or not ISO_INSTANT.fullmatch(value):
        return None
    try:
        moment = datetime.fromisoformat(value.replace("Z", "+00:00"))
        if moment.utcoffset() is None or abs(moment.utcoffset().total_seconds()) > 14 * 3600:
            return None
        return moment.astimezone(timezone.utc)
    except (ValueError, OverflowError):
        return None


def direct_source_link(value: object) -> bool:
    """Check a non-local HTTPS evidence link. Authenticity needs owner login."""
    if not isinstance(value, str) or not value or value != value.strip():
        return False
    if any(c.isspace() or ord(c) < 32 for c in value):
        return False
    try:
        parsed = urlsplit(value)
        host = (parsed.hostname or "").lower().rstrip(".")
        if parsed.scheme != "https" or not host or not parsed.path.startswith("/"):
            return False
        if parsed.username or parsed.password or parsed.port not in (None, 443):
            return False
        if host in ("localhost", "127.0.0.1", "::1") or host.endswith((".localhost", ".invalid", ".test")):
            return False
        return "." in host
    except ValueError:
        return False


def exact_model(raw: object) -> str | None:
    if not isinstance(raw, str):
        return None
    normalized = raw.strip().lower()
    return normalized if normalized in ("elite", "pro") else None


def quote_names_one_model(words: object, model: str) -> bool:
    """A shared or ambiguous 'I tested it' quotation is never both approvals."""
    if not isinstance(words, str) or not words.strip():
        return False
    mentioned = {m for m in ("elite", "pro") if re.search(r"\b" + m + r"\b", words, re.I)}
    return mentioned == {model}


def approval_errors(item: dict, record: dict) -> list[str]:
    errors = []
    model = exact_model(item.get("model"))
    models = record.get("models") or {}
    if not model or model not in models:
        return ["approval model must identify exactly one record model"]
    if item.get("source_sha") != record.get("source_sha"):
        errors.append("source SHA does not match the approved build")
    if item.get("image_sha256") != (models[model] or {}).get("image_sha256"):
        errors.append("image SHA-256 does not match the approved model")
    if item.get("approved") is True:
        if item.get("quoted") is not True or not quote_names_one_model(item.get("quoted_words"), model):
            errors.append("distinct owner quotation must explicitly name this model only")
        if parse_instant(item.get("time")) is None:
            errors.append("per-model owner quotation time must be ISO 8601 with an offset")
        if not direct_source_link(item.get("source_link")):
            errors.append("per-model owner quotation requires a direct HTTPS source_link")
    return errors


def installation_for_model(record: dict, model: str) -> tuple[bool, str]:
    """No record-wide proof, other-model history or browser ACK can verify install."""
    models = record.get("models") or {}
    if model not in models:
        return False, "unknown model"
    receipts = [r for r in record.get("install_verifications", [])
                if isinstance(r, dict) and r.get("claimed_model") == model]
    if not receipts:
        legacy = record.get("install_verification") or {}
        if legacy.get("claimed_model") == model:
            receipts = [legacy]
    if len(receipts) != 1:
        return False, "missing or ambiguous exact-model installation receipt; Elite never implies Pro"
    receipt = receipts[0]
    if receipt.get("model_proof") not in MODEL_PROOF_TYPES:
        return False, "missing hardware-field model proof (USB name is not model proof)"
    if receipt.get("model_confidence") != "verified" or receipt.get("build_confidence") != "verified":
        return False, "model and exact build confidence must both be verified per model"
    wanted_hash = (models[model] or {}).get("image_sha256")
    supplied_hash = receipt.get("installed_hash") or receipt.get("backup_field_hash")
    if supplied_hash is not None:
        if supplied_hash != wanted_hash:
            return False, "installed image hash mismatches this model"
    elif receipt.get("firmware_git_readback") != record.get("source_sha"):
        return False, "no matching installed hash or firmware source readback"
    return True, "independent hardware-field and exact-build proof for this model"


def audit_chronology_errors(events: list[dict]) -> list[str]:
    """JSONL line order is append order; historical event time may be earlier."""
    issues = []
    last_append = None
    append_started = False
    for line_no, event in enumerate(events, 1):
        event_time = parse_instant(event.get("time"))
        if event_time is None:
            issues.append(f"line {line_no}: invalid ISO event time")
        recorded = event.get("appended_at")
        if recorded is None:
            if append_started:
                issues.append(f"line {line_no}: appended_at missing after tracked append chronology began")
            continue
        append_started = True
        appended = parse_instant(recorded)
        if appended is None:
            issues.append(f"line {line_no}: invalid ISO appended_at")
            continue
        if last_append is not None and appended < last_append:
            issues.append(f"line {line_no}: appended_at moved backward")
        if event_time is not None and event_time > appended:
            issues.append(f"line {line_no}: historical event time is later than append time")
        last_append = appended
    return issues
