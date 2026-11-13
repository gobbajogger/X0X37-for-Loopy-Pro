#!/usr/bin/env python3
"""Loopback-only browser admin and opt-in installer outcome receiver.

Owner authentication happens server-side through GitHub /user with an
environment-supplied token. Browser never receives the token. Commits are
disabled unless explicitly enabled and still only target the feature branch.
No device access, automatic promotion, publication or paid hosting.
"""

from __future__ import annotations

import argparse
import json
import os
import secrets
import sys
import threading
import time
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any
from urllib.parse import parse_qs, urlsplit

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from admin_action import (
    DEFAULT_BRANCH, DEFAULT_REPO, commit_files_via_api, github_authenticated_login,
)
from admin_outcomes import OutcomeError, prepare_owner_outcome, sanitize_installer_report
from evaluate_promotion import evaluate_record
from promotion_candidate import evaluate_candidate
from validate_release_status import (
    ALLOWED_ACTOR, apply_proposal, build_audit_event, check_proposal_audit_event,
    event_line, find_record, load_json, load_jsonl, load_records,
    next_prev_hash, raw_lines, validate_audit_append_only,
    validate_audit_schema, validate_proposal, validate_record,
)

AUDIT = ROOT / "audit" / "audit.jsonl"
RECORDS = ROOT / "records"
SCHEMA = ROOT / "schema" / "release-record.schema.json"
AUDIT_SCHEMA = ROOT / "schema" / "audit-event.schema.json"
MAX_BODY = 8192


def prepare_preview(payload: dict, *, owner_login: str,
                    records_dir: Path = RECORDS, audit_path: Path = AUDIT) -> dict:
    if owner_login != ALLOWED_ACTOR:
        raise OutcomeError("owner GitHub identity has not been verified")
    if not isinstance(payload, dict):
        raise OutcomeError("JSON object required")
    records = load_records(records_dir)
    rec = find_record(records, str(payload.get("catalog_id") or ""))
    if rec is None:
        raise OutcomeError("unknown exact build")
    proposal = prepare_owner_outcome(rec, payload, owner_login=owner_login)
    after = apply_proposal(rec, proposal)
    source_lines = raw_lines(audit_path.read_text(encoding="utf-8"))
    existing = load_jsonl(audit_path)
    event = build_audit_event(
        time=payload["observed_at"], actor=owner_login,
        action="owner-outcome:" + payload["stage"], record_id=rec["catalog_id"],
        from_states=dict(rec.get("states") or {}),
        to_states=dict(after.get("states") or {}),
        evidence_refs=[payload["source_link"]] if payload.get("source_link") else [],
        commit_sha=None,
        prev_sha256=next_prev_hash(source_lines[-1] if source_lines else None),
        note="Owner reported a scoped outcome; not installed-hash verification or approval.",
        appended_at=datetime.now(timezone.utc).isoformat(timespec="seconds"),
    )
    proposal["audit_event"] = event
    schema = load_json(SCHEMA)
    audit_schema = load_json(AUDIT_SCHEMA)
    issues = []
    issues += validate_proposal(rec, proposal, schema)
    issues += check_proposal_audit_event(proposal, rec, after, source_lines, existing)
    issues += validate_audit_schema([event], audit_schema)
    issues += validate_record(after, schema)
    next_text = audit_path.read_text(encoding="utf-8") + event_line(event) + "\n"
    issues += validate_audit_append_only(audit_path.read_text(encoding="utf-8"), next_text)
    errors = sorted(set(str(i) for i in issues))
    filename = next((name for name, r in records if r is rec), "")
    return {
        "accepted": not errors,
        "errors": errors,
        "record_id": rec["catalog_id"],
        "model": payload["model"],
        "package_sha256": payload["package_sha256"],
        "stage": payload["stage"],
        "confidence": "owner-reported; installed image unverified",
        "not_hardware_approved": True,
        "not_promoted": True,
        "filename": filename,
        "after_record": after,
        "audit_text": next_text,
    }


def public_preview(prepared: dict) -> dict:
    return {key: prepared[key] for key in (
        "accepted", "errors", "record_id", "model", "package_sha256", "stage",
        "confidence", "not_hardware_approved", "not_promoted",
    )}


class AdminHTTP(ThreadingHTTPServer):
    allow_reuse_address = True

    def __init__(self, address: tuple[str, int], *,
                 token: str | None = None, allow_commit: bool = False,
                 expected_head: str | None = None, report_file: Path | None = None,
                 owner_login: str | None = None) -> None:
        if address[0] not in ("127.0.0.1", "::1"):
            raise ValueError("admin must bind loopback only")
        super().__init__(address, Handler)
        self.token = token
        # Injection of owner_login is for offline tests only. The command-line
        # launcher NEVER passes it: startup always asks GitHub with GH_TOKEN.
        self.owner_login = owner_login if owner_login is not None else (
            github_authenticated_login(token) if token else None
        )
        self.owner_verified = self.owner_login == ALLOWED_ACTOR and bool(token or owner_login)
        self.allow_commit = allow_commit and self.owner_verified and bool(token and expected_head)
        self.expected_head = expected_head
        self.csrf = secrets.token_urlsafe(32)
        self.pending: dict[str, tuple[float, dict]] = {}
        self.report_file = report_file
        self.report_lock = threading.Lock()
        self.seen_reports: dict[str, int] = {}
        if report_file is not None and report_file.exists():
            if report_file.stat().st_size > 4_000_000:
                raise ValueError("existing report file exceeds privacy-minimal size cap")
            for line in report_file.read_text(encoding="utf-8").splitlines():
                try:
                    old = json.loads(line)
                    run = old.get("run_id")
                    seq = old.get("sequence")
                    if isinstance(run, str) and type(seq) is int:
                        self.seen_reports[run] = max(seq, self.seen_reports.get(run, 0))
                except (ValueError, AttributeError):
                    raise ValueError("invalid existing report log: refuse unsafe replay tracking")


class Handler(BaseHTTPRequestHandler):
    server: AdminHTTP

    def log_message(self, *args: Any) -> None:
        # Do not log browser IPs, query strings, user agents or device IDs.
        pass

    def _respond(self, status: int, data: dict) -> None:
        body = json.dumps(data, sort_keys=True).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("Referrer-Policy", "no-referrer")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _static(self, name: str, mime: str) -> None:
        body = (ROOT / "admin" / name).read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", mime)
        self.send_header("Content-Security-Policy",
                         "default-src 'none'; script-src 'self'; style-src 'self'; "
                         "connect-src 'self'; form-action 'self'; base-uri 'none'")
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("Referrer-Policy", "no-referrer")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self) -> None:
        if self.path == "/":
            return self._static("index.html", "text/html; charset=utf-8")
        if self.path == "/app.js":
            return self._static("app.js", "text/javascript; charset=utf-8")
        if self.path == "/public":
            body = (ROOT / "public" / "index.html").read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        if self.path == "/catalog/catalog.json":
            body = (ROOT / "catalog" / "catalog.json").read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        if urlsplit(self.path).path == "/api/promotion-candidate":
            query = parse_qs(urlsplit(self.path).query, keep_blank_values=False)
            selected = query.get("catalog_id", [])
            if len(selected) != 1 or not selected[0]:
                return self._respond(400, {"error": "one catalog_id is required"})
            rec = find_record(load_records(RECORDS), selected[0])
            if rec is None:
                return self._respond(404, {"error": "exact build not found"})
            return self._respond(200, evaluate_candidate(rec))
        if self.path == "/api/status":
            records = load_records(RECORDS)
            summaries = []
            for _, rec in records:
                summaries.append({
                    "catalog_id": rec["catalog_id"],
                    "version": rec["version"],
                    "channel": rec.get("channel"),
                    "withdrawn": rec.get("states", {}).get("withdrawn", False),
                    "models": {
                        model: {"package_sha256": image.get("package_sha256"),
                                "image_sha256": image.get("image_sha256")}
                        for model, image in rec.get("models", {}).items()
                    },
                    "gates": {g["gate"]: g["status"] for g in evaluate_record(rec)["gates"]},
                })
            return self._respond(200, {
                "owner_verified": self.server.owner_verified,
                "commit_enabled": self.server.allow_commit,
                "branch": DEFAULT_BRANCH,
                "csrf": self.server.csrf if self.server.owner_verified else None,
                "records": summaries,
                "report_receiver_enabled": self.server.report_file is not None,
            })
        return self._respond(404, {"error": "not found"})

    def _body(self) -> dict:
        if self.headers.get("Content-Type", "").split(";")[0].strip() != "application/json":
            raise OutcomeError("application/json is required")
        length = self.headers.get("Content-Length", "")
        if not length.isdecimal() or not (1 <= int(length) <= MAX_BODY):
            raise OutcomeError("bounded Content-Length is required")
        obj = json.loads(self.rfile.read(int(length)).decode("utf-8"))
        if not isinstance(obj, dict):
            raise OutcomeError("JSON object required")
        return obj

    def do_POST(self) -> None:
        expected_origin = f"http://127.0.0.1:{self.server.server_port}"
        if self.headers.get("Origin") != expected_origin:
            return self._respond(403, {"error": "same-origin browser request required"})
        try:
            obj = self._body()
            if self.path == "/api/installer-report":
                if self.server.report_file is None:
                    raise OutcomeError("opt-in report receiver is disabled")
                rec = find_record(load_records(RECORDS), obj.get("catalog_id", ""))
                if rec is None:
                    raise OutcomeError("unknown exact build")
                sanitized = sanitize_installer_report(rec, obj)
                key = sanitized["run_id"]
                sequence = sanitized["sequence"]
                with self.server.report_lock:
                    if sequence <= self.server.seen_reports.get(key, 0):
                        raise OutcomeError("duplicate or out-of-order installer report")
                    if len(self.server.seen_reports) >= 10000 and key not in self.server.seen_reports:
                        raise OutcomeError("report receiver capacity reached")
                    path = self.server.report_file
                    path.parent.mkdir(parents=True, exist_ok=True)
                    if path.exists() and path.stat().st_size > 4_000_000:
                        raise OutcomeError("report receiver storage cap reached")
                    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_APPEND, 0o600)
                    with os.fdopen(fd, "a", encoding="utf-8") as fh:
                        fh.write(json.dumps(sanitized, sort_keys=True) + "\n")
                    self.server.seen_reports[key] = sequence
                return self._respond(202, {
                    "recorded": True, "confidence": "anonymous-unverified",
                    "authorizes_hardware_approval": False, "promoted": False,
                })

            if not self.server.owner_verified:
                return self._respond(403, {"error": "GitHub owner actor not verified"})
            if self.headers.get("X-DnBOX-CSRF") != self.server.csrf:
                return self._respond(403, {"error": "invalid CSRF token"})
            if self.path == "/api/preview":
                prepared = prepare_preview(obj, owner_login=self.server.owner_login)
                nonce = secrets.token_urlsafe(18)
                self.server.pending[nonce] = (time.monotonic(), prepared if prepared["accepted"] else {})
                return self._respond(200, {**public_preview(prepared), "preview_id": nonce})

            if self.path == "/api/commit":
                if not self.server.allow_commit:
                    return self._respond(403, {"error": "commit disabled; preview is dry-run"})
                nonce = obj.get("preview_id")
                pending = self.server.pending.pop(nonce, None)
                prepared = pending[1] if pending and time.monotonic() - pending[0] <= 300 else None
                if not prepared or not prepared.get("accepted"):
                    raise OutcomeError("missing, expired or already consumed accepted preview")
                files = {
                    f"release-status/records/{prepared['filename']}":
                        json.dumps(prepared["after_record"], indent=2, ensure_ascii=False) + "\n",
                    "release-status/audit/audit.jsonl": prepared["audit_text"],
                }
                landed = commit_files_via_api(
                    repo=DEFAULT_REPO, branch=DEFAULT_BRANCH, files=files,
                    message="release-status: record authenticated owner scoped device report",
                    token=self.server.token, expected_head=self.server.expected_head,
                )
                self.server.allow_commit = False  # local snapshot is stale after a commit
                return self._respond(200, {
                    "commit": landed["sha"], "report": public_preview(prepared),
                    "promotion": "not-performed", "device_write": False,
                })
            return self._respond(404, {"error": "not found"})
        except (OutcomeError, json.JSONDecodeError, UnicodeDecodeError, ValueError) as exc:
            return self._respond(422, {"error": str(exc)})
        except OSError:
            return self._respond(500, {"error": "local storage or GitHub API error; inspect operator console"})


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--allow-commit", action="store_true",
                        help="Explicitly allow ONE owner-verified feature-branch commit")
    parser.add_argument("--expected-head", help="Full verified feature branch SHA for commit lease")
    parser.add_argument("--enable-reports", action="store_true",
                        help="Enable optional anonymous, untrusted local JSONL reporting")
    parser.add_argument("--report-file", type=Path,
                        default=Path.home() / ".local/state/dnbox/installer-reports.jsonl")
    args = parser.parse_args(argv)
    token = os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN")
    if args.allow_commit and (not token or not args.expected_head or len(args.expected_head) != 40):
        parser.error("--allow-commit requires GH_TOKEN and --expected-head of 40 hex characters")
    httpd = AdminHTTP(("127.0.0.1", args.port), token=token,
                      allow_commit=args.allow_commit,
                      expected_head=args.expected_head,
                      report_file=args.report_file if args.enable_reports else None)
    print(f"DnBOX admin http://127.0.0.1:{httpd.server_port}/ "
          f"actor={'verified' if httpd.owner_verified else 'not verified'} "
          f"commit={'enabled once' if httpd.allow_commit else 'disabled'} "
          f"reports={'enabled' if args.enable_reports else 'disabled'}")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        httpd.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
