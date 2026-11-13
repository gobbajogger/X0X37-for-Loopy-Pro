#!/usr/bin/env python3
"""Validate release-status records, the audit log, and proposed state changes.

Exit codes:
  0  All records and the proposal are valid.
  1  A schema or evidence error. The script prints the rule and the record id.
  2  A usage or file error.

This script does not push. It does not publish. It does not write to a device.
It does not rewrite an audit line.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, Iterable

from evidence_policy import approval_errors, installation_for_model, audit_chronology_errors

GIT_SHA = re.compile(r"^[0-9a-f]{40}$")
SHA256 = re.compile(r"^[0-9a-f]{64}$")
ZERO_PREV = "0" * 64

STATE_FLAGS = (
    "built",
    "gate-passed",
    "published-nightly",
    "installation-reported",
    "install-verified",
    "boot-restart-reported",
    "owner-hardware-approval",
    "withdrawn",
)

# Flags that count as a promotion. A withdrawn record cannot gain these.
PROMOTION_FLAGS = (
    "gate-passed",
    "published-nightly",
    "install-verified",
    "owner-hardware-approval",
)

HARDWARE_PROMOTION_FLAGS = (
    "install-verified",
    "owner-hardware-approval",
)

EMU360_THRESHOLD_US = 11609

ELF_BINARY_KEYS = frozenset(
    {
        "elf_bytes",
        "elf_binary",
        "elf_base64",
        "bytes",
        "binary",
        "content",
        "data",
        "payload",
    }
)

# Option C: the only permitted admin actor is this GitHub identity.
ALLOWED_ACTOR = "gobbajogger"
REQUIRED_ACTOR_IDENTITY = "github"

# Explicit hardware-field types for model proof. USB and product strings are not allowed.
MODEL_PROOF_TYPES = frozenset(
    {
        "board_id",
        "model_id",
        "flash_id",
        "mac",
        "firmware_readback",
        "backup_field",
    }
)

USB_OR_PRODUCT_RE = re.compile(
    r"usb(_|\s+)?(name|product|label|port)|product(\s+|_)?(string|name)|usb_name",
    re.IGNORECASE,
)


class Issue:
    def __init__(self, rule: str, record_id: str, message: str) -> None:
        self.rule = rule
        self.record_id = record_id
        self.message = message

    def __str__(self) -> str:
        rid = self.record_id or "(no record)"
        return f"ERROR rule={self.rule} record={rid}: {self.message}"


class UsageError(Exception):
    pass


# ---------------------------------------------------------------------------
# JSON Schema (draft-07 subset). No third-party dependency.
# ---------------------------------------------------------------------------

def _is_int(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def _is_number(value: Any) -> bool:
    return _is_int(value) or isinstance(value, float) and not isinstance(value, bool)


def _type_ok(value: Any, declared: str) -> bool:
    if declared == "object":
        return isinstance(value, dict)
    if declared == "array":
        return isinstance(value, list)
    if declared == "string":
        return isinstance(value, str)
    if declared == "integer":
        return _is_int(value)
    if declared == "number":
        return _is_number(value)
    if declared == "boolean":
        return isinstance(value, bool)
    if declared == "null":
        return value is None
    return False


def _resolve_ref(schema: dict, root: dict) -> dict:
    ref = schema.get("$ref")
    if not ref:
        return schema
    if not isinstance(ref, str) or not ref.startswith("#/"):
        raise UsageError(f"unsupported $ref: {ref!r}")
    node: Any = root
    for part in ref[2:].split("/"):
        part = part.replace("~1", "/").replace("~0", "~")
        if not isinstance(node, dict) or part not in node:
            raise UsageError(f"unresolved $ref: {ref!r}")
        node = node[part]
    if not isinstance(node, dict):
        raise UsageError(f"$ref did not resolve to an object: {ref!r}")
    return node


def validate_instance(
    instance: Any,
    schema: dict,
    root: dict | None = None,
    path: str = "$",
) -> list[str]:
    """Return a list of schema error strings. Empty means the instance is valid."""
    if root is None:
        root = schema
    errors: list[str] = []
    schema = _resolve_ref(schema, root)

    if "oneOf" in schema:
        ok = 0
        sub_errors: list[str] = []
        for i, sub in enumerate(schema["oneOf"]):
            sub_err = validate_instance(instance, sub, root, f"{path}/oneOf/{i}")
            if not sub_err:
                ok += 1
            else:
                sub_errors.extend(sub_err)
        if ok != 1:
            if ok == 0:
                errors.append(f"{path}: no oneOf option matched ({'; '.join(sub_errors[:3])})")
            else:
                errors.append(f"{path}: {ok} oneOf options matched, expected 1")
        # Still apply sibling keywords after oneOf? Draft-07 does. Keep it simple:
        # if oneOf is present with $ref-only options, siblings are rare. Continue.

    if "anyOf" in schema:
        any_ok = False
        any_errs: list[str] = []
        for i, sub in enumerate(schema["anyOf"]):
            sub_err = validate_instance(instance, sub, root, f"{path}/anyOf/{i}")
            if not sub_err:
                any_ok = True
                break
            any_errs.extend(sub_err)
        if not any_ok:
            errors.append(f"{path}: no anyOf option matched ({'; '.join(any_errs[:3])})")

    if "allOf" in schema:
        for i, sub in enumerate(schema["allOf"]):
            errors.extend(validate_instance(instance, sub, root, f"{path}/allOf/{i}"))

    declared = schema.get("type")
    if declared is not None:
        types = declared if isinstance(declared, list) else [declared]
        if not any(_type_ok(instance, t) for t in types):
            errors.append(f"{path}: expected type {declared}, got {type(instance).__name__}")
            return errors

    if "enum" in schema and instance not in schema["enum"]:
        errors.append(f"{path}: {instance!r} is not in enum {schema['enum']}")

    if "const" in schema and instance != schema["const"]:
        errors.append(f"{path}: expected const {schema['const']!r}")

    if isinstance(instance, str):
        if "pattern" in schema and re.search(schema["pattern"], instance) is None:
            # JSON Schema patterns are unanchored; our patterns are already anchored.
            errors.append(f"{path}: {instance!r} does not match pattern {schema['pattern']}")
        if "minLength" in schema and len(instance) < schema["minLength"]:
            errors.append(f"{path}: string shorter than minLength {schema['minLength']}")
        if "maxLength" in schema and len(instance) > schema["maxLength"]:
            errors.append(f"{path}: string longer than maxLength {schema['maxLength']}")

    if _is_number(instance):
        if "minimum" in schema and instance < schema["minimum"]:
            errors.append(f"{path}: {instance} is below minimum {schema['minimum']}")
        if "maximum" in schema and instance > schema["maximum"]:
            errors.append(f"{path}: {instance} is above maximum {schema['maximum']}")

    if isinstance(instance, list) and "items" in schema:
        if "minItems" in schema and len(instance) < schema["minItems"]:
            errors.append(f"{path}: fewer than minItems {schema['minItems']}")
        if "maxItems" in schema and len(instance) > schema["maxItems"]:
            errors.append(f"{path}: more than maxItems {schema['maxItems']}")
        items_schema = schema["items"]
        if isinstance(items_schema, dict):
            for i, item in enumerate(instance):
                errors.extend(validate_instance(item, items_schema, root, f"{path}[{i}]"))

    if isinstance(instance, dict):
        required = schema.get("required") or []
        for key in required:
            if key not in instance:
                errors.append(f"{path}: missing required field {key!r}")
        props = schema.get("properties") or {}
        additional = schema.get("additionalProperties", True)
        for key, value in instance.items():
            if key in props:
                errors.extend(
                    validate_instance(value, props[key], root, f"{path}.{key}")
                )
            elif additional is False:
                errors.append(f"{path}: unexpected field {key!r}")
            elif isinstance(additional, dict):
                errors.extend(
                    validate_instance(value, additional, root, f"{path}.{key}")
                )

    return errors


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise UsageError(f"file not found: {path}") from exc
    except OSError as exc:
        raise UsageError(f"cannot read {path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise UsageError(f"invalid JSON in {path}: {exc}") from exc


def load_jsonl(path: Path) -> list[dict]:
    try:
        text = path.read_text(encoding="utf-8")
    except FileNotFoundError as exc:
        raise UsageError(f"file not found: {path}") from exc
    except OSError as exc:
        raise UsageError(f"cannot read {path}: {exc}") from exc
    events: list[dict] = []
    if text == "":
        return events
    for i, line in enumerate(text.splitlines(), start=1):
        if line.strip() == "":
            raise UsageError(f"{path}:{i}: empty line is not allowed in the audit file")
        try:
            obj = json.loads(line)
        except json.JSONDecodeError as exc:
            raise UsageError(f"{path}:{i}: invalid JSON: {exc}") from exc
        if not isinstance(obj, dict):
            raise UsageError(f"{path}:{i}: audit event must be a JSON object")
        events.append(obj)
    if text.endswith("\n") is False:
        # A final newline is required so a later append cannot look like a rewrite
        # of the last line. Still accept for reading, but the rewrite check uses raw text.
        pass
    return events


def canonical_json(obj: Any) -> str:
    return json.dumps(obj, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def raw_lines(text: str) -> list[str]:
    if text == "":
        return []
    # Keep exact line bodies; a missing final newline is not itself a line.
    if text.endswith("\n"):
        return text.split("\n")[:-1]
    return text.split("\n")


# ---------------------------------------------------------------------------
# Audit: append-only, hash chain, option C (GitHub identity + JSONL).
# ---------------------------------------------------------------------------

def build_audit_event(
    *,
    time: str,
    actor: str,
    action: str,
    record_id: str,
    from_states: dict,
    to_states: dict,
    evidence_refs: list[str],
    commit_sha: str | None,
    prev_sha256: str,
    note: str | None = None,
    actor_identity: str = "github",
    appended_at: str | None = None,
) -> dict:
    event = {
        "time": time,
        "actor": actor,
        "actor_identity": actor_identity,
        "action": action,
        "record_id": record_id,
        "from_states": from_states,
        "to_states": to_states,
        "evidence_refs": evidence_refs,
        "commit_sha": commit_sha,
        "prev_sha256": prev_sha256,
    }
    if note is not None:
        event["note"] = note
    if appended_at is not None:
        event["appended_at"] = appended_at
    return event


def event_line(event: dict) -> str:
    return canonical_json(event)


def next_prev_hash(previous_line: str | None) -> str:
    if previous_line is None:
        return ZERO_PREV
    return sha256_text(previous_line)


def validate_audit_schema(events: Iterable[dict], schema: dict) -> list[Issue]:
    issues: list[Issue] = []
    for i, event in enumerate(events, start=1):
        record_id = str(event.get("record_id") or f"audit-line-{i}")
        for err in validate_instance(event, schema, schema, f"audit[{i}]"):
            issues.append(Issue("audit-schema", record_id, err))
    return issues


def validate_audit_chain(lines: list[str], events: list[dict], record_id: str = "audit") -> list[Issue]:
    issues: list[Issue] = []
    expected_prev = ZERO_PREV
    for i, (line, event) in enumerate(zip(lines, events), start=1):
        rid = str(event.get("record_id") or record_id)
        got = event.get("prev_sha256")
        if got != expected_prev:
            issues.append(
                Issue(
                    "audit-chain",
                    rid,
                    f"line {i}: prev_sha256 does not match the hash of the previous line",
                )
            )
        # The stored line must be the canonical form so a rewrite is detectable
        # and the next prev_sha256 is stable.
        if line != canonical_json(event):
            issues.append(
                Issue(
                    "audit-canonical",
                    rid,
                    f"line {i}: audit line is not canonical JSON (sorted keys, compact)",
                )
            )
        expected_prev = sha256_text(line)
    return issues


def validate_audit_append_only(previous_text: str, current_text: str, record_id: str = "audit") -> list[Issue]:
    """Refuse any rewrite or deletion of an existing audit line. Appends are allowed."""
    issues: list[Issue] = []
    prev_lines = raw_lines(previous_text)
    curr_lines = raw_lines(current_text)
    if len(curr_lines) < len(prev_lines):
        issues.append(
            Issue(
                "audit-rewrite",
                record_id,
                "audit lines were deleted; the audit file is append-only",
            )
        )
        return issues
    for i, (old, new) in enumerate(zip(prev_lines, curr_lines), start=1):
        if old != new:
            issues.append(
                Issue(
                    "audit-rewrite",
                    record_id,
                    f"line {i} was rewritten; the audit file is append-only",
                )
            )
    return issues


# ---------------------------------------------------------------------------
# Option C actor allowlist, model proof, installed-hash proof, git previous.
# ---------------------------------------------------------------------------

def check_audit_actor(event: dict, line_no: int) -> list[Issue]:
    """Option C: actor must be gobbajogger and actor_identity must be github."""
    issues: list[Issue] = []
    rid = str(event.get("record_id") or f"audit-line-{line_no}")
    actor = event.get("actor")
    if actor != ALLOWED_ACTOR:
        issues.append(
            Issue(
                "audit-actor",
                rid,
                f"line {line_no}: actor {actor!r} is not the option C allowlist value {ALLOWED_ACTOR!r}",
            )
        )
    identity = event.get("actor_identity")
    if identity != REQUIRED_ACTOR_IDENTITY:
        issues.append(
            Issue(
                "audit-actor-identity",
                rid,
                f"line {line_no}: actor_identity {identity!r} is required and must be {REQUIRED_ACTOR_IDENTITY!r}",
            )
        )
    return issues


def is_usb_or_product_proof(evidence: dict, record: dict) -> bool:
    """True if the offered model proof is a USB name or a product string.

    Only evidence, install_verification, and identity_confidence proof fields
    are inspected. History text is not inspected, because a record may narrate
    that a USB label did not prove a model.
    """
    iv = record.get("install_verification") or {}
    conf = record.get("identity_confidence") or {}
    blobs: list[str] = []
    for obj in (evidence, iv):
        if not isinstance(obj, dict):
            continue
        for key in (
            "usb_name",
            "usb_name_only",
            "usb_product",
            "usb_label",
            "usb_port",
            "product_string",
            "product_name",
        ):
            if obj.get(key):
                return True
        for key in ("model_proof", "model_note", "note", "proof", "proof_text"):
            val = obj.get(key)
            if isinstance(val, str) and val:
                blobs.append(val)
    note = conf.get("model_note")
    if isinstance(note, str) and note:
        blobs.append(note)
    for blob in blobs:
        if USB_OR_PRODUCT_RE.search(blob):
            return True
        # A model_proof value that is not an allowed enum and mentions product/USB.
        if "usb" in blob.lower() or "product string" in blob.lower():
            return True
    return False


def offered_model_proof(evidence: dict, record: dict) -> str | None:
    iv = record.get("install_verification") or {}
    proof = evidence.get("model_proof")
    if proof is None:
        proof = iv.get("model_proof")
    return proof if isinstance(proof, str) else None


def resolve_claimed_model(evidence: dict, record: dict) -> str | None:
    iv = record.get("install_verification") or {}
    for candidate in (
        evidence.get("installed_hash_model"),
        evidence.get("model"),
        iv.get("claimed_model"),
    ):
        if candidate in ("elite", "pro"):
            return candidate
    return None


def resolve_installed_hash(evidence: dict, record: dict) -> tuple[str | None, str]:
    """Return (hash_or_none, how). Does not treat a confidence flag as a hash."""
    iv = record.get("install_verification") or {}
    ev_hash = evidence.get("installed_hash")
    if isinstance(ev_hash, str) and ev_hash not in ("", "present", "absent"):
        return ev_hash, "evidence.installed_hash"
    iv_hash = iv.get("installed_hash")
    if isinstance(iv_hash, str) and iv_hash:
        return iv_hash, "install_verification.installed_hash"
    backup = iv.get("backup_field_hash") or evidence.get("backup_field_hash")
    if isinstance(backup, str) and backup:
        return backup, "backup_field"
    # A history event may store a verified installed hash, but only when it is
    # a real 64-hex value (not null).
    current = current_history(record)
    if current:
        h = current.get("installed_hash")
        if isinstance(h, str) and SHA256.fullmatch(h):
            return h, "history.installed_hash"
    return None, "absent"


def firmware_readback_ok(evidence: dict, record: dict) -> bool:
    """Firmware GIT readback of the exact source_sha is accepted as build proof."""
    iv = record.get("install_verification") or {}
    got = evidence.get("firmware_git_readback") or iv.get("firmware_git_readback")
    src = record.get("source_sha")
    return isinstance(got, str) and isinstance(src, str) and got == src


def states_equal(a: dict, b: dict) -> bool:
    for flag in STATE_FLAGS:
        if bool(a.get(flag)) != bool(b.get(flag)) or (flag in a) != (flag in b):
            # Require the flag to be present on both and equal.
            if a.get(flag) != b.get(flag):
                return False
        if flag not in a or flag not in b:
            return False
        if a.get(flag) != b.get(flag):
            return False
    return True


def latest_audit_event(events: list[dict], record_id: str) -> dict | None:
    latest = None
    for event in events:
        if event.get("record_id") == record_id:
            latest = event
    return latest


def check_states_match_audit(records: list[tuple[str, dict]], events: list[dict]) -> list[Issue]:
    issues: list[Issue] = []
    for filename, rec in records:
        rid = record_id_of(rec, filename)
        latest = latest_audit_event(events, rid)
        if latest is None:
            issues.append(
                Issue(
                    "audit-state-mismatch",
                    rid,
                    "no audit event exists for this record_id; every record must have a latest to_states",
                )
            )
            continue
        to_states = latest.get("to_states") or {}
        rec_states = rec.get("states") or {}
        if not states_equal(rec_states, to_states):
            issues.append(
                Issue(
                    "audit-state-mismatch",
                    rid,
                    "record states must equal the latest audit to_states for this record_id",
                )
            )
    return issues


def check_proposal_audit_event(
    proposal: dict,
    record: dict,
    after: dict,
    audit_lines: list[str],
    audit_events: list[dict],
) -> list[Issue]:
    """--propose must include a new audit event with the permitted actor."""
    issues: list[Issue] = []
    rid = record_id_of(record, str(proposal.get("record_id") or ""))
    event = proposal.get("audit_event")
    if not isinstance(event, dict):
        issues.append(
            Issue(
                "propose-audit-required",
                rid,
                "--propose requires a new audit event (audit_event) with the permitted actor",
            )
        )
        return issues
    issues.extend(check_audit_actor(event, (len(audit_events) + 1) if audit_events else 1))
    if event.get("appended_at") is None:
        issues.append(Issue("audit-append-time-required", rid,
                            "new audit proposals require appended_at; time is the historical event time"))
    for reason in audit_chronology_errors([*audit_events, event]):
        issues.append(Issue("audit-chronology", rid, reason))
    expected_prev = next_prev_hash(audit_lines[-1] if audit_lines else None)
    if event.get("prev_sha256") != expected_prev:
        issues.append(
            Issue(
                "audit-chain",
                rid,
                "proposal audit_event.prev_sha256 must match the hash of the last existing audit line",
            )
        )
    to_states = event.get("to_states") or {}
    if not states_equal(after.get("states") or {}, to_states):
        issues.append(
            Issue(
                "audit-state-mismatch",
                rid,
                "proposal audit_event.to_states must equal the proposed record states",
            )
        )
    if event.get("record_id") not in (rid, record.get("catalog_id"), proposal.get("record_id")):
        issues.append(
            Issue(
                "propose-audit-required",
                rid,
                "proposal audit_event.record_id must match the proposal record",
            )
        )
    return issues


def git_previous_audit(audit_path: Path) -> tuple[str | None, str]:
    """If audit_path is a tracked file in a git checkout, return the previous text.

    When the working tree differs from HEAD, HEAD is the previous snapshot
    (an uncommitted rewrite). When the working tree matches HEAD, the parent
    commit (HEAD^) is the previous snapshot (a committed rewrite).

    A bare directory that is not a git checkout cannot detect a rehashed
    rewrite. The first commit of an orphan branch has no parent and also
    cannot default --audit-previous.
    """
    audit_path = audit_path.resolve()
    try:
        top = subprocess.run(
            ["git", "-C", str(audit_path.parent), "rev-parse", "--show-toplevel"],
            capture_output=True,
            text=True,
            timeout=10,
        )
    except (OSError, subprocess.TimeoutExpired):
        return None, "git is not available"
    if top.returncode != 0:
        return None, "not a git checkout"
    root = Path(top.stdout.strip())
    try:
        rel = os.path.relpath(audit_path, root)
    except ValueError:
        return None, "audit path is not inside the git checkout"
    if rel.startswith(".."):
        return None, "audit path is not inside the git checkout"
    tracked = subprocess.run(
        ["git", "-C", str(root), "ls-files", "--error-unmatch", rel],
        capture_output=True,
        text=True,
        timeout=10,
    )
    if tracked.returncode != 0:
        return None, "audit file is not tracked in git"
    head = subprocess.run(
        ["git", "-C", str(root), "show", f"HEAD:{rel}"],
        capture_output=True,
        text=True,
        timeout=10,
    )
    if head.returncode != 0:
        return None, "audit file is not in HEAD"
    current = audit_path.read_text(encoding="utf-8")
    if raw_lines(head.stdout) != raw_lines(current):
        return head.stdout, "HEAD"
    parent = subprocess.run(
        ["git", "-C", str(root), "rev-parse", "HEAD^"],
        capture_output=True,
        text=True,
        timeout=10,
    )
    if parent.returncode != 0:
        return None, "no parent commit (orphan first commit)"
    prev = subprocess.run(
        ["git", "-C", str(root), "show", f"HEAD^:{rel}"],
        capture_output=True,
        text=True,
        timeout=10,
    )
    if prev.returncode != 0:
        return None, "audit file is not in the parent commit"
    return prev.stdout, "HEAD^"


BARE_AUDIT_WARNING = (
    "WARNING: no --audit-previous and not inside a git checkout with a previous "
    "audit snapshot; a rehashed rewrite of the audit file cannot be detected."
)


# ---------------------------------------------------------------------------
# Record helpers
# ---------------------------------------------------------------------------

def record_id_of(record: dict, fallback: str = "") -> str:
    return str(record.get("catalog_id") or fallback or "(unknown)")


def models_of(record: dict) -> dict[str, dict]:
    models = record.get("models") or {}
    return {k: v for k, v in models.items() if isinstance(v, dict)}


def current_history(record: dict) -> dict | None:
    history = record.get("history") or []
    current = [h for h in history if isinstance(h, dict) and h.get("is_current")]
    if len(current) == 1:
        return current[0]
    if len(current) > 1:
        return None  # ambiguous; a dedicated check reports this
    if history:
        numbered = [h for h in history if isinstance(h, dict) and _is_int(h.get("sequence"))]
        if numbered:
            return max(numbered, key=lambda h: h["sequence"])
    return None


def unresolved_conflicts(record: dict) -> list[dict]:
    out = []
    for item in record.get("conflicts") or []:
        if isinstance(item, dict) and item.get("status") == "unresolved":
            out.append(item)
    return out


# ---------------------------------------------------------------------------
# Evidence and state rules (design sections 4, 5, 7 and later owner orders).
# ---------------------------------------------------------------------------

def _add(issues: list[Issue], rule: str, rid: str, message: str) -> None:
    issues.append(Issue(rule, rid, message))



def check_hardware_approval_scope(record: dict, proposing: dict | None = None) -> list[Issue]:
    """Require independent owner quote, time, source and image for EACH model."""
    issues: list[Issue] = []
    rid = record_id_of(record)
    evidence = (proposing or {}).get("evidence") or {}
    approvals = list(record.get("hardware_approvals") or [])
    if proposing and proposing.get("hardware_approvals") is not None:
        approvals = list(proposing.get("hardware_approvals") or [])
    models = models_of(record)
    states = record.get("states") or {}
    approved_models: set[str] = set()
    quote_signatures: dict[tuple[str, str, str], str] = {}

    for item in approvals:
        if not isinstance(item, dict):
            continue
        model = item.get("model")
        errors = approval_errors(item, record)
        if item.get("approved") is True:
            signature = (str(item.get("time") or ""), str(item.get("source_link") or ""),
                         str(item.get("quoted_words") or ""))
            previous_model = quote_signatures.get(signature)
            if previous_model is not None and previous_model != model:
                errors.append("the same owner quotation/time/source cannot authorize two models")
            elif not errors:
                quote_signatures[signature] = model
        for err in errors:
            _add(issues, "elite-approval-not-pro", rid, f"{model}: {err}")
        if item.get("approved") is True and not errors:
            approved_models.add(model)

    if evidence.get("imply_pro_from_elite") or evidence.get("imply_elite_from_pro"):
        _add(issues, "elite-approval-not-pro", rid,
             "an Elite hardware approval never implies a Pro approval (or vice versa)")
    ev_model = evidence.get("approval_model") or evidence.get("model")
    if evidence.get("copy_approval_from") and ev_model and evidence["copy_approval_from"] != ev_model:
        _add(issues, "elite-approval-not-pro", rid, "an approval copied from another model is refused")

    if states.get("owner-hardware-approval"):
        missing = [model for model in models if model not in approved_models]
        if missing:
            _add(issues, "elite-approval-not-pro", rid,
                 "each model requires its own exact-build, quoted owner approval with time and "
                 "direct source_link; missing: " + ", ".join(missing))
    return issues


def local_git_commits_for(path: Path) -> list[str] | None:
    """Return commit SHAs that changed path, newest first, or None if git is unavailable."""
    path = path.resolve()
    try:
        top = subprocess.run(
            ["git", "-C", str(path.parent), "rev-parse", "--show-toplevel"],
            capture_output=True,
            text=True,
            timeout=10,
        )
    except (OSError, subprocess.TimeoutExpired):
        return None
    if top.returncode != 0:
        return None
    root = Path(top.stdout.strip())
    try:
        rel = os.path.relpath(path, root)
    except ValueError:
        return None
    if rel.startswith(".."):
        return None
    tracked = subprocess.run(
        ["git", "-C", str(root), "ls-files", "--error-unmatch", rel],
        capture_output=True,
        text=True,
        timeout=10,
    )
    if tracked.returncode != 0:
        return None
    log = subprocess.run(
        ["git", "-C", str(root), "log", "--follow", "--format=%H", "--", rel],
        capture_output=True,
        text=True,
        timeout=15,
    )
    if log.returncode != 0:
        return None
    shas = [line.strip() for line in log.stdout.splitlines() if line.strip()]
    return shas


def git_remote_repo(path: Path) -> str | None:
    """Return 'owner/repo' from the origin URL of the checkout that contains path."""
    path = path.resolve()
    try:
        top = subprocess.run(
            ["git", "-C", str(path.parent), "rev-parse", "--show-toplevel"],
            capture_output=True,
            text=True,
            timeout=10,
        )
    except (OSError, subprocess.TimeoutExpired):
        return None
    if top.returncode != 0:
        return None
    root = Path(top.stdout.strip())
    remote = subprocess.run(
        ["git", "-C", str(root), "remote", "get-url", "origin"],
        capture_output=True,
        text=True,
        timeout=10,
    )
    if remote.returncode != 0:
        return None
    url = remote.stdout.strip()
    # git@github.com:owner/repo.git or https://github.com/owner/repo.git
    m = re.search(r"github\.com[:/]([^/]+)/([^/]+?)(?:\.git)?$", url)
    if not m:
        return None
    return f"{m.group(1)}/{m.group(2)}"


def github_commit_login(repo: str, sha: str) -> str | None:
    """Return the GitHub login of the commit author, or None if unavailable.

    Uses the gh CLI when present. Does not treat a local author string as a
    verified GitHub login.
    """
    try:
        proc = subprocess.run(
            ["gh", "api", f"repos/{repo}/commits/{sha}", "--jq", ".author.login // .committer.login // empty"],
            capture_output=True,
            text=True,
            timeout=20,
        )
    except (OSError, subprocess.TimeoutExpired):
        return None
    if proc.returncode != 0:
        return None
    login = proc.stdout.strip()
    return login or None


def verify_audit_commits_github_login(
    audit_path: Path,
    *,
    commit_shas: list[str] | None = None,
    login_lookup=None,
    repo: str | None = None,
) -> list[Issue]:
    """Check that each commit which added audit.jsonl lines is by GitHub login gobbajogger.

    This check uses verified git / GitHub history only. The actor field inside
    the JSONL file is not authentication. If history or the GitHub login is
    unavailable, the result is 'actor not verified' and is not a pass.
    """
    issues: list[Issue] = []
    shas = commit_shas
    if shas is None:
        shas = local_git_commits_for(audit_path)
    if not shas:
        issues.append(
            Issue(
                "actor-not-verified",
                "audit",
                "actor not verified: no git history is available to check commits that added audit.jsonl lines",
            )
        )
        return issues
    if repo is None and login_lookup is None:
        repo = git_remote_repo(audit_path)
    for sha in shas:
        login = None
        if login_lookup is not None:
            login = login_lookup(sha)
        elif repo:
            login = github_commit_login(repo, sha)
        if not login:
            issues.append(
                Issue(
                    "actor-not-verified",
                    "audit",
                    f"actor not verified: GitHub login unavailable for commit {sha}",
                )
            )
            continue
        if login != ALLOWED_ACTOR:
            issues.append(
                Issue(
                    "audit-actor",
                    "audit",
                    f"commit {sha} that changed audit.jsonl is by GitHub login {login!r}, not {ALLOWED_ACTOR!r}",
                )
            )
    return issues


def check_elf_provenance(record: dict) -> list[Issue]:
    issues: list[Issue] = []
    rid = record_id_of(record)
    elf = record.get("elf_provenance")
    if elf is None:
        return issues
    if not isinstance(elf, dict):
        _add(issues, "elf-schema", rid, "elf_provenance must be an object")
        return issues

    def walk(obj: Any, path: str) -> None:
        if isinstance(obj, dict):
            for k, v in obj.items():
                here = f"{path}.{k}"
                if k.lower() in ELF_BINARY_KEYS:
                    _add(
                        issues,
                        "elf-binary-forbidden",
                        rid,
                        f"{here} is not allowed; ELF provenance holds hashes only",
                    )
                if isinstance(v, str) and len(v) > 4096:
                    _add(
                        issues,
                        "elf-binary-forbidden",
                        rid,
                        f"{here} is too long for a hash or fact field",
                    )
                walk(v, here)
        elif isinstance(obj, list):
            for i, v in enumerate(obj):
                walk(v, f"{path}[{i}]")

    walk(elf, "elf_provenance")

    if elf.get("source_sha") != record.get("source_sha"):
        _add(
            issues,
            "elf-source-sha-mismatch",
            rid,
            "elf_provenance.source_sha must equal the record source_sha",
        )

    rec_models = models_of(record)
    elf_models = elf.get("models") or {}
    if isinstance(elf_models, dict):
        for name, em in elf_models.items():
            if not isinstance(em, dict):
                continue
            rec_m = rec_models.get(name)
            if rec_m and em.get("image_sha256") != rec_m.get("image_sha256"):
                _add(
                    issues,
                    "elf-image-sha-mismatch",
                    rid,
                    f"elf_provenance.models.{name}.image_sha256 must equal the record image SHA-256",
                )
    # Toolchain stays null or "unknown" unless exact build provenance names it.
    # Do not infer a compiler from the SDK or from other context.
    toolchain = elf.get("toolchain")
    if toolchain not in (None, "", "unknown"):
        cited = elf.get("toolchain_source")
        if not (isinstance(cited, str) and cited.strip()):
            _add(
                issues,
                "toolchain-inferred",
                rid,
                "toolchain must stay null or unknown unless exact build provenance names it; no inference",
            )
    return issues


def emu360_exact_pass(item: dict, record: dict | None = None) -> tuple[bool, str | None]:
    """Return (ok, reason-if-not)."""
    if item.get("kind") != "emu360-strict":
        return False, "not emu360-strict"
    if item.get("result") != "PASS":
        return False, "result is not PASS"
    if item.get("applies_to_image") != "exact-image":
        return False, "applies_to_image is not exact-image"
    if item.get("elite_late") != 0 or item.get("pro_late") != 0:
        return False, "late is not 0 for both models"
    threshold = item.get("threshold_us")
    if not _is_int(threshold):
        threshold = EMU360_THRESHOLD_US
    elite = item.get("elite_worst_us")
    pro = item.get("pro_worst_us")
    if not _is_int(elite) or not _is_int(pro):
        return False, "worst-time values are missing"
    if elite >= threshold or pro >= threshold:
        return False, f"worst time is not under the threshold ({threshold})"
    if record is not None:
        models = models_of(record)
        for name, field in (("elite", "elite_image_sha256"), ("pro", "pro_image_sha256")):
            got = item.get(field)
            expected = (models.get(name) or {}).get("image_sha256")
            if not isinstance(got, str) or not SHA256.fullmatch(got):
                return False, f"{field} is required and must equal the record {name} image SHA-256"
            if expected and got != expected:
                return False, f"{field} must equal the record {name} image SHA-256 (relabeled other-input evidence is refused)"
    return True, None


def check_states(record: dict, *, proposing: dict | None = None) -> list[Issue]:
    issues: list[Issue] = []
    rid = record_id_of(record)
    states = record.get("states") or {}
    conf = record.get("identity_confidence") or {}
    evidence = (proposing or {}).get("evidence") or {}
    models = models_of(record)

    if states.get("built"):
        if not models:
            _add(issues, "built-requires-images", rid, "built requires at least one model image")
        for name, model in models.items():
            if not SHA256.fullmatch(str(model.get("image_sha256") or "")):
                _add(issues, "built-requires-images", rid, f"{name} image_sha256 is missing or not 64 hex")
            if not _is_int(model.get("image_size")) or model.get("image_size") < 1:
                _add(issues, "built-requires-images", rid, f"{name} image_size is missing")
            if model.get("controller_strategy") == "bundled":
                check = model.get("controller_check")
                if check != "PASS":
                    _add(
                        issues,
                        "bundled-controller-check-required",
                        rid,
                        f"{name} controller_check is {check!r}; bundled built requires PASS",
                    )

    if states.get("gate-passed"):
        items = [g for g in (record.get("gate_evidence") or []) if isinstance(g, dict)]
        emu = [g for g in items if g.get("kind") == "emu360-strict"]
        if not emu:
            _add(
                issues,
                "missing-gate-evidence",
                rid,
                "gate-passed requires an emu360-strict evidence item for the exact images",
            )
        else:
            # Any other-inputs item cannot be the reason for gate-passed.
            exact_ok = False
            other = False
            reasons: list[str] = []
            for item in emu:
                if item.get("applies_to_image") == "other-inputs":
                    other = True
                    reasons.append("emu360 result applies to other-inputs, not the exact 94197fc8 images")
                ok, reason = emu360_exact_pass(item, record)
                if ok:
                    exact_ok = True
                elif reason:
                    reasons.append(reason)
            if other and not exact_ok:
                _add(
                    issues,
                    "emu360-not-exact-image",
                    rid,
                    "an emu360 result on other inputs (for example d408) must not pass gate-passed for these images",
                )
            elif not exact_ok:
                # Distinguish late/threshold, missing image hashes, and missing evidence.
                if any("image SHA-256" in r or "image_sha256" in r for r in reasons):
                    _add(
                        issues,
                        "emu360-image-hash",
                        rid,
                        reasons[0] if reasons else "exact-image emu360 evidence must carry matching elite and pro image SHA-256 values",
                    )
                elif any("late" in r or "threshold" in r or "worst" in r for r in reasons):
                    _add(
                        issues,
                        "emu360-late-or-threshold",
                        rid,
                        reasons[0] if reasons else "emu360 strict PASS conditions were not met",
                    )
                else:
                    _add(
                        issues,
                        "missing-gate-evidence",
                        rid,
                        reasons[0] if reasons else "gate-passed is missing exact-image PASS evidence",
                    )

    if states.get("published-nightly"):
        has_sha = any(
            SHA256.fullmatch(str(m.get("package_sha256") or m.get("image_sha256") or ""))
            for m in models.values()
        )
        if not record.get("catalog_id") or not record.get("published_pages_url") or not has_sha:
            _add(
                issues,
                "published-nightly-evidence",
                rid,
                "published-nightly requires a catalog id, a Pages URL, and a published SHA-256",
            )

    if states.get("installation-reported"):
        history = [h for h in (record.get("history") or []) if isinstance(h, dict)]
        install_kinds = {"install-and-boot-report", "install-report", "recovery-history"}
        if not any(h.get("kind") in install_kinds or "install" in (h.get("events") or []) or "install" in (h.get("text") or "").lower() for h in history):
            if not evidence.get("report"):
                _add(
                    issues,
                    "installation-report-required",
                    rid,
                    "installation-reported requires a report object with a source and text",
                )

    if states.get("boot-restart-reported"):
        if evidence.get("web_install_success_only") or evidence.get("web_install_success"):
            if evidence.get("boot_or_restart_observed") is not True:
                _add(
                    issues,
                    "web-install-not-boot-proof",
                    rid,
                    "web install success alone must not pass boot-restart-reported as verified proof",
                )
        # A verified-proof claim needs verified identity. The flag itself may be
        # set from a relayed report (current state) and then lacks verification.
        if evidence.get("boot_restart_verified"):
            if conf.get("model") != "verified" or conf.get("build") != "verified":
                _add(
                    issues,
                    "web-install-not-boot-proof",
                    rid,
                    "boot-restart-reported is not verified proof when identity is not verified",
                )

    if states.get("install-verified"):
        model_c = conf.get("model")
        build_c = conf.get("build")
        hash_c = conf.get("installed_hash")
        if model_c == "inferred":
            _add(issues, "inferred-model", rid, "inferred model must not pass install-verified")
        elif model_c == "reported":
            _add(issues, "reported-model", rid, "a reported model must not pass install-verified")
        elif model_c == "absent" or model_c != "verified":
            _add(issues, "inferred-model", rid, "install-verified requires model confidence verified")
        if build_c == "inferred" or evidence.get("build_proof") == "current nightly":
            _add(
                issues,
                "inferred-build",
                rid,
                "inferred build 94197fc8 from 'current nightly' must not pass install-verified",
            )
        elif build_c != "verified":
            _add(issues, "inferred-build", rid, "install-verified requires exact-build confidence verified")

        # Model proof must be an explicit hardware-field type. Reject USB or
        # product-string proof regardless of flags.
        if is_usb_or_product_proof(evidence, record):
            _add(
                issues,
                "usb-name-not-model-proof",
                rid,
                "a USB name or product string must not pass as model proof for install-verified, regardless of flags",
            )
        else:
            proof = offered_model_proof(evidence, record)
            if proof not in MODEL_PROOF_TYPES:
                _add(
                    issues,
                    "usb-name-not-model-proof",
                    rid,
                    "install-verified requires an explicit hardware-field model proof "
                    "(board_id, model_id, flash_id, mac, firmware_readback, or backup_field)",
                )

        # Installed hash must actually be present and equal the claimed model's
        # image SHA-256. A confidence flag alone is not a hash (this is the
        # former inner-if bug). Alternatives: firmware GIT readback of
        # source_sha, or a backup field that names the image hash.
        stored_hash, hash_how = resolve_installed_hash(evidence, record)
        claimed = resolve_claimed_model(evidence, record)
        readback = firmware_readback_ok(evidence, record)
        backup_names = stored_hash if hash_how == "backup_field" else None
        has_image_proof = False
        if stored_hash and SHA256.fullmatch(stored_hash):
            if claimed and claimed in models:
                expected = models[claimed].get("image_sha256")
                if stored_hash == expected:
                    has_image_proof = True
                else:
                    _add(
                        issues,
                        "installed-hash-mismatch",
                        rid,
                        f"installed hash does not equal models.{claimed}.image_sha256",
                    )
            else:
                matches = [
                    name
                    for name, model in models.items()
                    if model.get("image_sha256") == stored_hash
                ]
                if len(matches) == 1:
                    has_image_proof = True
                else:
                    _add(
                        issues,
                        "installed-hash-mismatch",
                        rid,
                        "installed hash does not equal the image SHA-256 of the claimed model",
                    )
        elif not readback:
            _add(
                issues,
                "absent-installed-hash",
                rid,
                "install-verified requires a stored installed hash equal to "
                "models.<claimed model>.image_sha256 (or a firmware GIT readback "
                "of source_sha, or a backup field that names that hash). "
                "A confidence value alone is not enough.",
            )
        # A firmware GIT readback of source_sha may satisfy exact-build proof,
        # but only together with a matching installed or backup image hash, or
        # as the stated alternative when the backup/hash path is used.
        # If only readback is present with no image hash, accept as the
        # alternative exact-build proof only when claimed model is otherwise
        # verified; still require the hash when a hash was offered.
        if readback and not stored_hash:
            # Alternative exact-build path: firmware readback of source_sha.
            # Model still needs a hardware-field proof (checked above).
            has_image_proof = True

    if states.get("owner-hardware-approval"):
        if evidence.get("web_install_success_only") or evidence.get("web_install_success"):
            _add(
                issues,
                "web-install-not-hardware-approval",
                rid,
                "web install success alone must not pass owner-hardware-approval",
            )
        quoted = evidence.get("owner_words_quoted")
        source_is_relay = evidence.get("owner_source") in ("relay", "relayed", None) or evidence.get("relay")
        # Inspect current (and any) history for a quoted owner statement.
        history = [h for h in (record.get("history") or []) if isinstance(h, dict)]
        quoted_history = [
            h
            for h in history
            if h.get("quoted") is True and "owner" in (h.get("source") or "").lower()
        ]
        has_quote = quoted is True or (quoted is None and bool(quoted_history))
        has_time = bool(evidence.get("owner_timestamp")) or any(h.get("time") for h in quoted_history)
        has_link = bool(evidence.get("owner_source_link")) or any(h.get("ref") for h in quoted_history)
        if quoted is False or evidence.get("relay") or (source_is_relay and quoted is not True and not quoted_history):
            if not has_quote:
                _add(
                    issues,
                    "relay-not-owner-approval",
                    rid,
                    "relayed owner words with no quotation must not pass owner-hardware-approval",
                )
        if not has_quote or not has_time or not has_link:
            if "relay-not-owner-approval" not in {i.rule for i in issues if i.record_id == rid}:
                _add(
                    issues,
                    "relay-not-owner-approval",
                    rid,
                    "owner-hardware-approval requires the owner's own words, quoted, with a timestamp and a direct source link",
                )
        if not states.get("install-verified"):
            _add(
                issues,
                "owner-approval-requires-install-verified",
                rid,
                "owner-hardware-approval is refused when install-verified is false",
            )

    issues.extend(check_hardware_approval_scope(record, proposing))
    if states.get("install-verified") or states.get("owner-hardware-approval"):
        for model_name in models:
            qualified, reason = installation_for_model(record, model_name)
            if not qualified:
                _add(issues, "install-model-scope", rid, f"{model_name}: {reason}")

    if states.get("withdrawn"):
        if not record.get("withdraw_ref") or not record.get("withdraw_reason"):
            _add(
                issues,
                "withdraw-evidence",
                rid,
                "withdrawn requires a withdrawal ref and reason",
            )

    # Unresolved conflict (generic rule). The 0.0.26 seed has no conflict.
    if unresolved_conflicts(record):
        for flag in HARDWARE_PROMOTION_FLAGS:
            if states.get(flag):
                _add(
                    issues,
                    "unresolved-conflict-no-promotion",
                    rid,
                    f"a record with an unresolved conflict cannot set {flag}",
                )
        if evidence.get("promote") or (proposing or {}).get("promote"):
            _add(
                issues,
                "unresolved-conflict-no-promotion",
                rid,
                "a record with an unresolved conflict cannot promote",
            )

    # Full-suite PASS claim.
    if evidence.get("full_suite_pass"):
        leftover = evidence.get("suite_leftover") or []
        if leftover:
            _add(
                issues,
                "full-suite-incomplete",
                rid,
                "refuse a claim of full suite PASS when a fail, a skip, or a not-run item remains",
            )

    return issues


def check_history(record: dict) -> list[Issue]:
    issues: list[Issue] = []
    rid = record_id_of(record)
    history = record.get("history") or []
    if not history:
        return issues
    sequences = []
    current_count = 0
    for h in history:
        if not isinstance(h, dict):
            continue
        sequences.append(h.get("sequence"))
        if h.get("is_current"):
            current_count += 1
    if current_count > 1:
        _add(issues, "history-current", rid, "at most one history event may be is_current")
    marked = record.get("current_report_sequence")
    if marked is not None:
        current = current_history(record)
        if current and current.get("sequence") != marked:
            _add(
                issues,
                "history-current",
                rid,
                "current_report_sequence must name the current (latest) history event",
            )
    return issues


def check_withdrawn_transition(before: dict | None, after: dict) -> list[Issue]:
    issues: list[Issue] = []
    if before is None:
        return issues
    rid = record_id_of(after, record_id_of(before))
    before_states = before.get("states") or {}
    after_states = after.get("states") or {}
    if before_states.get("withdrawn") or after_states.get("withdrawn"):
        # Refuse gaining a promotion flag. A documented correction that keeps
        # withdrawn is allowed for non-promotion fields.
        if after_states.get("withdrawn"):
            for flag in PROMOTION_FLAGS:
                if after_states.get(flag) and not before_states.get(flag):
                    _add(
                        issues,
                        "withdrawn-no-promotion",
                        rid,
                        f"a withdrawn record must not gain promotion flag {flag}",
                    )
        # Clearing withdrawn in order to promote is also refused here unless
        # the caller is only documenting; we still refuse new promotion flags
        # gained in the same step while the record was withdrawn.
        if before_states.get("withdrawn") and not after_states.get("withdrawn"):
            gained = [f for f in PROMOTION_FLAGS if after_states.get(f) and not before_states.get(f)]
            if gained:
                _add(
                    issues,
                    "withdrawn-no-promotion",
                    rid,
                    "clearing withdrawn cannot be combined with gaining promotion flags",
                )
    return issues


def check_proposal_evidence(record: dict, proposal: dict) -> list[Issue]:
    """Rules that look at the proposal evidence even before / as well as states."""
    issues: list[Issue] = []
    rid = record_id_of(record, str(proposal.get("record_id") or ""))
    ev = proposal.get("evidence") or {}
    set_states = proposal.get("set_states") or {}
    merged_states = {**(record.get("states") or {}), **set_states}

    if set_states.get("install-verified") or (merged_states.get("install-verified") and set_states):
        if is_usb_or_product_proof(ev, record) or (
            ev.get("model_proof") is not None and ev.get("model_proof") not in MODEL_PROOF_TYPES
        ):
            _add(
                issues,
                "usb-name-not-model-proof",
                rid,
                "a USB name or product string must not pass as model proof for install-verified, regardless of flags",
            )

    if ev.get("web_install_success_only") or (ev.get("web_install_success") and ev.get("boot_or_restart_observed") is not True):
        if set_states.get("boot-restart-reported") or ev.get("boot_restart_verified") or merged_states.get("boot-restart-reported") and ev.get("boot_restart_verified"):
            _add(
                issues,
                "web-install-not-boot-proof",
                rid,
                "web install success alone must not pass boot-restart-reported as verified proof",
            )
        if set_states.get("owner-hardware-approval") or merged_states.get("owner-hardware-approval"):
            _add(
                issues,
                "web-install-not-hardware-approval",
                rid,
                "web install success alone must not pass owner-hardware-approval",
            )
        if set_states.get("install-verified"):
            _add(
                issues,
                "web-install-not-boot-proof",
                rid,
                "web install success alone must not pass install-verified",
            )

    if set_states.get("owner-hardware-approval") and ev.get("owner_words_quoted") is False:
        _add(
            issues,
            "relay-not-owner-approval",
            rid,
            "relayed owner words with no quotation must not pass owner-hardware-approval",
        )

    if set_states.get("gate-passed"):
        # If the proposal supplies replacement gate evidence, the merged record
        # check covers it. If it supplies none and the record also has none, fail.
        incoming = proposal.get("gate_evidence")
        existing = record.get("gate_evidence") or []
        items = incoming if incoming is not None else existing
        emu = [g for g in items if isinstance(g, dict) and g.get("kind") == "emu360-strict"]
        if not emu:
            _add(
                issues,
                "missing-gate-evidence",
                rid,
                "missing gate evidence blocks promotion (gate-passed)",
            )
        else:
            if all(g.get("applies_to_image") == "other-inputs" for g in emu):
                _add(
                    issues,
                    "emu360-not-exact-image",
                    rid,
                    "an emu360 result on other inputs (for example d408) must not pass gate-passed",
                )
            after_models = models_of(apply_proposal(record, proposal)) if incoming is not None else models_of(record)
            for g in emu:
                if g.get("applies_to_image") == "exact-image":
                    ok, reason = emu360_exact_pass(g, {"models": after_models, "catalog_id": rid})
                    if not ok and reason and ("image SHA-256" in reason or "image_sha256" in reason):
                        _add(
                            issues,
                            "emu360-image-hash",
                            rid,
                            reason or "relabeled other-input emu360 evidence must not pass gate-passed",
                        )

    if unresolved_conflicts(record) or unresolved_conflicts(proposal) or any(
        isinstance(c, dict) and c.get("status") == "unresolved"
        for c in (proposal.get("conflicts") or [])
    ):
        if any(set_states.get(f) for f in PROMOTION_FLAGS) or any(
            merged_states.get(f) and set_states.get(f) for f in HARDWARE_PROMOTION_FLAGS
        ) or proposal.get("promote") or ev.get("promote"):
            _add(
                issues,
                "unresolved-conflict-no-promotion",
                rid,
                "a record with an unresolved conflict cannot promote",
            )
        for flag in HARDWARE_PROMOTION_FLAGS:
            if set_states.get(flag) or (merged_states.get(flag) and flag in set_states):
                _add(
                    issues,
                    "unresolved-conflict-no-promotion",
                    rid,
                    f"a record with an unresolved conflict cannot set {flag}",
                )

    return issues


def apply_proposal(record: dict, proposal: dict) -> dict:
    out = copy.deepcopy(record)
    if "set_states" in proposal:
        out.setdefault("states", {}).update(proposal["set_states"])
    if "identity_confidence" in proposal:
        out.setdefault("identity_confidence", {}).update(proposal["identity_confidence"])
    if "gate_evidence" in proposal:
        out["gate_evidence"] = copy.deepcopy(proposal["gate_evidence"])
    if "history" in proposal:
        out["history"] = copy.deepcopy(proposal["history"])
    if "conflicts" in proposal:
        out["conflicts"] = copy.deepcopy(proposal["conflicts"])
    if "models" in proposal:
        out.setdefault("models", {})
        for name, model in proposal["models"].items():
            out["models"].setdefault(name, {}).update(model)
    if "withdraw_ref" in proposal:
        out["withdraw_ref"] = proposal["withdraw_ref"]
    if "withdraw_reason" in proposal:
        out["withdraw_reason"] = proposal["withdraw_reason"]
    if "hardware_approvals" in proposal:
        out["hardware_approvals"] = copy.deepcopy(proposal["hardware_approvals"])
    if "channel" in proposal:
        out["channel"] = proposal["channel"]
    ev = proposal.get("evidence") or {}
    iv_src = proposal.get("install_verification")
    if iv_src or ev.get("installed_hash") or ev.get("model_proof") or ev.get("model") or ev.get("firmware_git_readback") or ev.get("backup_field_hash"):
        iv = out.setdefault("install_verification", {})
        if isinstance(iv_src, dict):
            iv.update(iv_src)
        if isinstance(ev.get("installed_hash"), str) and ev["installed_hash"] not in ("", "present", "absent"):
            iv["installed_hash"] = ev["installed_hash"]
        claimed = ev.get("installed_hash_model") or ev.get("model")
        if claimed in ("elite", "pro"):
            iv["claimed_model"] = claimed
        if ev.get("model_proof") in MODEL_PROOF_TYPES:
            iv["model_proof"] = ev["model_proof"]
        if ev.get("firmware_git_readback"):
            iv["firmware_git_readback"] = ev["firmware_git_readback"]
        if ev.get("backup_field_hash"):
            iv["backup_field_hash"] = ev["backup_field_hash"]
    return out


def validate_record(record: dict, schema: dict, *, filename: str = "") -> list[Issue]:
    rid = record_id_of(record, filename)
    issues: list[Issue] = []
    for err in validate_instance(record, schema, schema, "$"):
        issues.append(Issue("schema", rid, err))
    # Format checks that tests name explicitly, even if schema already covers them.
    src = record.get("source_sha")
    if src is not None and not (isinstance(src, str) and GIT_SHA.fullmatch(src)):
        issues.append(Issue("source-sha-format", rid, "source_sha must be 40 lowercase hex characters"))
    for name, model in models_of(record).items():
        for field in ("image_sha256", "package_sha256", "controller_sha256"):
            val = model.get(field)
            if val is not None and not (isinstance(val, str) and SHA256.fullmatch(val)):
                issues.append(Issue("sha256-format", rid, f"{name}.{field} must be 64 lowercase hex characters"))
    issues.extend(check_elf_provenance(record))
    issues.extend(check_history(record))
    issues.extend(check_states(record))
    return issues


def validate_proposal(record: dict, proposal: dict, schema: dict) -> list[Issue]:
    issues: list[Issue] = []
    issues.extend(check_proposal_evidence(record, proposal))
    after = apply_proposal(record, proposal)
    issues.extend(validate_record(after, schema, filename=record_id_of(record)))
    issues.extend(check_states(after, proposing=proposal))
    issues.extend(check_withdrawn_transition(record, after))
    # Deduplicate identical issues (same rule, record, message).
    seen: set[tuple[str, str, str]] = set()
    unique: list[Issue] = []
    for issue in issues:
        key = (issue.rule, issue.record_id, issue.message)
        if key in seen:
            continue
        seen.add(key)
        unique.append(issue)
    return unique


def find_record(records: list[tuple[str, dict]], record_id: str) -> dict | None:
    for filename, rec in records:
        if rec.get("catalog_id") == record_id or filename == record_id or filename.startswith(record_id):
            return rec
        if record_id in filename:
            return rec
    return None


def load_records(records_dir: Path) -> list[tuple[str, dict]]:
    if not records_dir.is_dir():
        raise UsageError(f"records directory not found: {records_dir}")
    out: list[tuple[str, dict]] = []
    paths = sorted(records_dir.glob("*.json"))
    if not paths:
        raise UsageError(f"no record files in {records_dir}")
    for path in paths:
        data = load_json(path)
        if not isinstance(data, dict):
            raise UsageError(f"{path} must contain a JSON object")
        out.append((path.name, data))
    return out


def validate_all(
    *,
    records: list[tuple[str, dict]],
    record_schema: dict,
    audit_path: Path | None,
    audit_schema: dict | None,
    audit_previous_path: Path | None = None,
    proposal: dict | None = None,
    require_proposal_audit: bool = False,
    bare_audit_warning: list[str] | None = None,
    commit_shas: list[str] | None = None,
    login_lookup=None,
    github_repo: str | None = None,
    skip_commit_actor_check: bool = False,
) -> list[Issue]:
    issues: list[Issue] = []
    for filename, rec in records:
        issues.extend(validate_record(rec, record_schema, filename=filename))

    events: list[dict] = []
    lines: list[str] = []
    current_text = ""
    if audit_path is not None:
        current_text = audit_path.read_text(encoding="utf-8")
        events = load_jsonl(audit_path)
        lines = raw_lines(current_text)
        if audit_schema is not None:
            issues.extend(validate_audit_schema(events, audit_schema))
        issues.extend(validate_audit_chain(lines, events))
        for reason in audit_chronology_errors(events):
            issues.append(Issue("audit-chronology", "audit", reason))
        for i, event in enumerate(events, start=1):
            issues.extend(check_audit_actor(event, i))
        previous_text = None
        if audit_previous_path is not None:
            previous_text = audit_previous_path.read_text(encoding="utf-8")
        else:
            previous_text, src = git_previous_audit(audit_path)
            if previous_text is None and bare_audit_warning is not None:
                bare_audit_warning.append(f"{BARE_AUDIT_WARNING} ({src})")
        if previous_text is not None:
            issues.extend(validate_audit_append_only(previous_text, current_text))
        issues.extend(check_states_match_audit(records, events))
        # Option C actor proof is the GitHub login on commits that added
        # audit lines, not the actor field inside the JSONL file.
        if not skip_commit_actor_check:
            issues.extend(
                verify_audit_commits_github_login(
                    audit_path,
                    commit_shas=commit_shas,
                    login_lookup=login_lookup,
                    repo=github_repo,
                )
            )

    if proposal is not None:
        rid = str(proposal.get("record_id") or "")
        rec = find_record(records, rid) if rid else None
        if rec is None:
            issues.append(Issue("proposal", rid or "(missing)", "proposal record_id does not match a loaded record"))
        else:
            issues.extend(validate_proposal(rec, proposal, record_schema))
            if require_proposal_audit or proposal.get("audit_event") is not None or "audit_event" in proposal:
                after = apply_proposal(rec, proposal)
                issues.extend(
                    check_proposal_audit_event(proposal, rec, after, lines, events)
                )
            elif require_proposal_audit:
                issues.append(
                    Issue(
                        "propose-audit-required",
                        rid,
                        "--propose requires a new audit event with the permitted actor",
                    )
                )
        # A proposal may also include a replacement audit text to test rewrites.
        if "audit_text" in proposal and audit_path is not None:
            issues.extend(validate_audit_append_only(current_text, proposal["audit_text"]))
    return issues


def default_paths() -> dict[str, Path]:
    root = Path(__file__).resolve().parents[1]
    return {
        "records": root / "records",
        "schema": root / "schema" / "release-record.schema.json",
        "audit_schema": root / "schema" / "audit-event.schema.json",
        "audit": root / "audit" / "audit.jsonl",
    }


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    defaults = default_paths()
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--records", type=Path, default=defaults["records"])
    p.add_argument("--schema", type=Path, default=defaults["schema"])
    p.add_argument("--audit", type=Path, default=defaults["audit"])
    p.add_argument("--audit-schema", type=Path, default=defaults["audit_schema"])
    p.add_argument(
        "--audit-previous",
        type=Path,
        default=None,
        help=(
            "Previous audit.jsonl snapshot. Refuses any rewrite of an existing line. "
            "When omitted inside a git checkout, defaults to the previous commit's "
            "audit.jsonl (HEAD if the working tree is dirty, otherwise HEAD^). "
            "A bare run that is not in a git checkout cannot detect a rehashed rewrite."
        ),
    )
    p.add_argument(
        "--propose",
        type=Path,
        default=None,
        help=(
            "Optional CHANGE.json. Checks one state change and does not write the record. "
            "The file must include a new audit_event with actor gobbajogger and "
            "actor_identity github."
        ),
    )
    return p.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    try:
        args = parse_args(argv)
        record_schema = load_json(args.schema)
        if not isinstance(record_schema, dict):
            raise UsageError("record schema must be a JSON object")
        audit_schema = load_json(args.audit_schema) if args.audit_schema else None
        records = load_records(args.records)
        proposal = load_json(args.propose) if args.propose else None
        if proposal is not None and not isinstance(proposal, dict):
            raise UsageError("proposal must be a JSON object")
        bare_warning: list[str] = []
        issues = validate_all(
            records=records,
            record_schema=record_schema,
            audit_path=args.audit,
            audit_schema=audit_schema,
            audit_previous_path=args.audit_previous,
            proposal=proposal,
            require_proposal_audit=args.propose is not None,
            bare_audit_warning=bare_warning,
        )
    except UsageError as exc:
        print(f"USAGE: {exc}", file=sys.stderr)
        return 2
    except FileNotFoundError as exc:
        print(f"USAGE: file not found: {exc}", file=sys.stderr)
        return 2

    for warning in bare_warning:
        print(warning, file=sys.stderr)
    if issues:
        for issue in issues:
            print(issue, file=sys.stderr)
        return 1
    print("OK: all records and the audit file are valid.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
