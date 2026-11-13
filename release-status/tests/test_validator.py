#!/usr/bin/env python3
"""Validator tests against the seed records and the audit file."""

from __future__ import annotations

import copy
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
sys.path.insert(0, str(TOOLS))

from validate_release_status import (  # noqa: E402
    load_json,
    load_jsonl,
    raw_lines,
    validate_all,
    validate_audit_append_only,
    validate_record,
    main as validator_main,
)

SCHEMA = load_json(ROOT / "schema" / "release-record.schema.json")
AUDIT_SCHEMA = load_json(ROOT / "schema" / "audit-event.schema.json")
RECORD_941 = load_json(ROOT / "records" / "0.0.26-94197fc8.json")
RECORD_600 = load_json(ROOT / "records" / "0.0.26-600b3b45.json")
AUDIT = ROOT / "audit" / "audit.jsonl"
SCRIPT = TOOLS / "validate_release_status.py"


def _run_cli(extra: list[str] | None = None, audit: Path | None = None) -> subprocess.CompletedProcess:
    cmd = [
        sys.executable,
        str(SCRIPT),
        "--records",
        str(ROOT / "records"),
        "--schema",
        str(ROOT / "schema" / "release-record.schema.json"),
        "--audit",
        str(audit or AUDIT),
        "--audit-schema",
        str(ROOT / "schema" / "audit-event.schema.json"),
    ]
    if extra:
        cmd.extend(extra)
    return subprocess.run(cmd, capture_output=True, text=True)


class TestValidator(unittest.TestCase):
    def test_94197fc8_seed_passes_as_written(self):
        issues = validate_record(RECORD_941, SCHEMA, filename="0.0.26-94197fc8.json")
        self.assertEqual([str(i) for i in issues], [])
        states = RECORD_941["states"]
        self.assertTrue(states["built"])
        self.assertTrue(states["gate-passed"])
        self.assertTrue(states["published-nightly"])
        self.assertTrue(states["installation-reported"])
        self.assertFalse(states["install-verified"])
        self.assertTrue(states["boot-restart-reported"])
        self.assertFalse(states["owner-hardware-approval"])
        self.assertFalse(states["withdrawn"])
        self.assertEqual(RECORD_941["identity_confidence"]["model"], "inferred")
        self.assertEqual(RECORD_941["identity_confidence"]["build"], "inferred")
        self.assertEqual(RECORD_941["identity_confidence"]["installed_hash"], "absent")
        # Ordered history: earlier recovery, latest success is current. No conflict.
        self.assertNotIn("conflicts", RECORD_941)
        self.assertEqual(RECORD_941["current_report_sequence"], 2)
        kinds = [h["kind"] for h in RECORD_941["history"]]
        self.assertEqual(kinds, ["recovery-history", "install-and-boot-report"])
        self.assertFalse(RECORD_941["history"][0]["is_current"])
        self.assertTrue(RECORD_941["history"][1]["is_current"])
        self.assertIsNone(RECORD_941["elf_provenance"]["toolchain"])
        self.assertFalse(RECORD_941["elf_provenance"]["published"])

    def test_600b_seed_passes_as_withdrawn(self):
        issues = validate_record(RECORD_600, SCHEMA, filename="0.0.26-600b3b45.json")
        self.assertEqual([str(i) for i in issues], [])
        self.assertTrue(RECORD_600["states"]["withdrawn"])
        self.assertFalse(RECORD_600["states"]["install-verified"])
        self.assertFalse(RECORD_600["states"]["owner-hardware-approval"])
        self.assertFalse(RECORD_600["states"]["gate-passed"])
        self.assertTrue(RECORD_600["states"]["built"])
        self.assertEqual(
            RECORD_600["withdraw_ref"],
            "https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/120",
        )

    def test_seeds_and_audit_validate_together(self):
        issues = validate_all(
            records=[
                ("0.0.26-94197fc8.json", RECORD_941),
                ("0.0.26-600b3b45.json", RECORD_600),
            ],
            record_schema=SCHEMA,
            audit_path=AUDIT,
            audit_schema=AUDIT_SCHEMA,
        )
        self.assertEqual([str(i) for i in issues], [])

    def test_audit_rewrite_fails(self):
        original = AUDIT.read_text(encoding="utf-8")
        lines = raw_lines(original)
        self.assertGreaterEqual(len(lines), 1)
        rewritten = json.loads(lines[0])
        rewritten["note"] = "this rewrite must be refused"
        new_lines = [json.dumps(rewritten, sort_keys=True, separators=(",", ":"))] + lines[1:]
        new_text = "\n".join(new_lines) + "\n"
        issues = validate_audit_append_only(original, new_text)
        self.assertTrue(issues, "a rewrite of an audit line must fail")
        self.assertTrue(any(i.rule == "audit-rewrite" for i in issues), [i.rule for i in issues])

    def test_audit_append_is_allowed(self):
        original = AUDIT.read_text(encoding="utf-8")
        appended = original + json.dumps({"extra": "append-only-ok"}) + "\n"
        issues = validate_audit_append_only(original, appended)
        self.assertEqual([str(i) for i in issues], [])

    def test_cli_exit_0_on_seeds(self):
        result = _run_cli()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("OK:", result.stdout)

    def test_cli_exit_2_on_missing_file(self):
        result = _run_cli(extra=["--propose", "/tmp/does-not-exist-release-status-propose.json"])
        self.assertEqual(result.returncode, 2, result.stderr)

    def test_cli_exit_1_on_rewritten_audit(self):
        original = AUDIT.read_text(encoding="utf-8")
        lines = raw_lines(original)
        rewritten = json.loads(lines[0])
        rewritten["note"] = "rewritten"
        new_text = json.dumps(rewritten) + "\n" + "\n".join(lines[1:]) + "\n"
        with tempfile.TemporaryDirectory() as tmp:
            prev = Path(tmp) / "prev.jsonl"
            curr = Path(tmp) / "curr.jsonl"
            prev.write_text(original, encoding="utf-8")
            curr.write_text(new_text, encoding="utf-8")
            result = _run_cli(extra=["--audit-previous", str(prev)], audit=curr)
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertIn("audit-rewrite", result.stderr)

    def test_audit_seed_uses_github_identity(self):
        events = load_jsonl(AUDIT)
        self.assertEqual(len(events), 3)
        for event in events:
            self.assertEqual(event["actor"], "gobbajogger")
            self.assertEqual(event["actor_identity"], "github")
            self.assertIsNone(event["commit_sha"])


    def test_evil_bot_actor_rejected(self):
        from validate_release_status import (
            canonical_json,
            sha256_text,
            validate_all,
        )
        original = AUDIT.read_text(encoding="utf-8")
        lines = raw_lines(original)
        new_event = {
            "time": "2026-11-13T12:00:00-06:00",
            "actor": "evil-bot",
            "actor_identity": "github",
            "action": "set-states",
            "record_id": "nightly-94197fc8-dev",
            "from_states": dict(RECORD_941["states"]),
            "to_states": dict(RECORD_941["states"]),
            "evidence_refs": [],
            "commit_sha": None,
            "prev_sha256": sha256_text(lines[-1]),
        }
        new_text = original if original.endswith("\n") else original + "\n"
        new_text = new_text + canonical_json(new_event) + "\n"
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "audit.jsonl"
            path.write_text(new_text, encoding="utf-8")
            issues = validate_all(
                records=[
                    ("0.0.26-94197fc8.json", RECORD_941),
                    ("0.0.26-600b3b45.json", RECORD_600),
                ],
                record_schema=SCHEMA,
                audit_path=path,
                audit_schema=AUDIT_SCHEMA,
            )
        self.assertTrue(any(i.rule == "audit-actor" for i in issues), [str(i) for i in issues])

    def test_anonymous_actor_and_missing_identity_rejected(self):
        from validate_release_status import canonical_json, sha256_text, validate_all
        original = AUDIT.read_text(encoding="utf-8")
        lines = raw_lines(original)
        events = load_jsonl(AUDIT)
        e = dict(events[-1])
        e["actor"] = "anonymous"
        e.pop("actor_identity", None)
        e["prev_sha256"] = sha256_text(lines[0]) if False else sha256_text(lines[-2]) if len(lines) > 1 else ("0" * 64)
        # Rebuild a full file: keep all but last, last is anonymous without identity
        new_lines = lines[:-1] + [canonical_json(e)]
        # repair chain from the start
        from validate_release_status import ZERO_PREV
        repaired = []
        prev = ZERO_PREV
        raw_kept = lines[:-1]
        for line in raw_kept:
            repaired.append(line)
            prev = sha256_text(line)
        e["prev_sha256"] = prev
        repaired.append(canonical_json(e))
        new_text = "\n".join(repaired) + "\n"
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "audit.jsonl"
            path.write_text(new_text, encoding="utf-8")
            issues = validate_all(
                records=[
                    ("0.0.26-94197fc8.json", RECORD_941),
                    ("0.0.26-600b3b45.json", RECORD_600),
                ],
                record_schema=SCHEMA,
                audit_path=path,
                audit_schema=AUDIT_SCHEMA,
            )
        rules = [i.rule for i in issues]
        self.assertTrue("audit-actor" in rules, [str(i) for i in issues])
        self.assertTrue("audit-actor-identity" in rules or "audit-schema" in rules, [str(i) for i in issues])

    def test_states_must_equal_latest_audit_to_states(self):
        from validate_release_status import validate_all
        rec = copy.deepcopy(RECORD_941)
        rec["states"]["install-verified"] = True
        issues = validate_all(
            records=[
                ("0.0.26-94197fc8.json", rec),
                ("0.0.26-600b3b45.json", RECORD_600),
            ],
            record_schema=SCHEMA,
            audit_path=AUDIT,
            audit_schema=AUDIT_SCHEMA,
        )
        self.assertTrue(any(i.rule == "audit-state-mismatch" for i in issues), [str(i) for i in issues])

    def test_cli_propose_requires_audit_event_with_permitted_actor(self):
        with tempfile.TemporaryDirectory() as tmp:
            propose = Path(tmp) / "change.json"
            propose.write_text(
                json.dumps({
                    "record_id": "nightly-94197fc8-dev",
                    "set_states": {"installation-reported": True},
                }),
                encoding="utf-8",
            )
            result = _run_cli(extra=["--propose", str(propose)])
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertIn("propose-audit-required", result.stderr)

    def test_git_checkout_defaults_audit_previous(self):
        """Inside a git checkout, a rehashed rewrite is caught without --audit-previous."""
        import subprocess
        from validate_release_status import (
            canonical_json,
            load_jsonl,
            sha256_text,
            ZERO_PREV,
        )
        original = AUDIT.read_text(encoding="utf-8")
        events = load_jsonl(AUDIT)
        # Reorder and repair hashes (internally consistent rewrite)
        evs = list(reversed([copy.deepcopy(e) for e in events]))
        prev = ZERO_PREV
        new_lines = []
        for e in evs:
            e["prev_sha256"] = prev
            line = canonical_json(e)
            new_lines.append(line)
            prev = sha256_text(line)
        rewritten = "\n".join(new_lines) + "\n"
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            (repo / "release-status" / "audit").mkdir(parents=True)
            (repo / "release-status" / "records").mkdir(parents=True)
            (repo / "release-status" / "schema").mkdir(parents=True)
            import shutil
            shutil.copytree(ROOT / "records", repo / "release-status" / "records", dirs_exist_ok=True)
            shutil.copytree(ROOT / "schema", repo / "release-status" / "schema", dirs_exist_ok=True)
            audit = repo / "release-status" / "audit" / "audit.jsonl"
            audit.write_text(original, encoding="utf-8")
            def git(*args):
                return subprocess.run(
                    ["git", "-C", str(repo), *args],
                    check=True,
                    capture_output=True,
                    text=True,
                )
            git("init")
            git("config", "user.name", "gobbajogger")
            git("config", "user.email", "255244738+gobbajogger@users.noreply.github.com")
            git("add", "release-status")
            git("commit", "-m", "original audit")
            # Second commit is the rewrite
            audit.write_text(rewritten, encoding="utf-8")
            git("add", "release-status/audit/audit.jsonl")
            git("commit", "-m", "rehashed rewrite")
            # Run validator with no --audit-previous; should default to HEAD^
            result = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "--records", str(repo / "release-status" / "records"),
                    "--schema", str(repo / "release-status" / "schema" / "release-record.schema.json"),
                    "--audit", str(audit),
                    "--audit-schema", str(repo / "release-status" / "schema" / "audit-event.schema.json"),
                ],
                capture_output=True,
                text=True,
                cwd=str(repo),
            )
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("audit-rewrite", result.stderr)

    def test_bare_run_warns_it_cannot_detect_rehashed_rewrite(self):
        """A bare directory (no git, no --audit-previous) cannot detect a rehashed rewrite.

        The CLI still exits 0 on an internally consistent file and prints a warning.
        This documents the limitation; the git-checkout default covers the real gate.
        """
        result = _run_cli()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("rehashed rewrite", result.stderr)
        self.assertIn("cannot be detected", result.stderr)




if __name__ == "__main__":
    unittest.main()
