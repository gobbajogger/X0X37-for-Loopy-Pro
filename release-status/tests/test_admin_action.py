#!/usr/bin/env python3
"""Admin action CLI tests. Dry-run is the default. --commit is mocked."""

from __future__ import annotations

import io
import shutil
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
sys.path.insert(0, str(TOOLS))

import admin_action as admin  # noqa: E402
from validate_release_status import load_jsonl  # noqa: E402

RECORDS = ROOT / "records"
AUDIT = ROOT / "audit" / "audit.jsonl"
SCHEMA = ROOT / "schema" / "release-record.schema.json"
AUDIT_SCHEMA = ROOT / "schema" / "audit-event.schema.json"


def _base_args(tmpdir: Path | None = None, extra: list[str] | None = None) -> list[str]:
    records = RECORDS
    audit = AUDIT
    if tmpdir is not None:
        shutil.copytree(RECORDS, tmpdir / "records")
        (tmpdir / "audit").mkdir()
        shutil.copy(AUDIT, tmpdir / "audit" / "audit.jsonl")
        records = tmpdir / "records"
        audit = tmpdir / "audit" / "audit.jsonl"
    args = [
        "--records",
        str(records),
        "--audit",
        str(audit),
        "--schema",
        str(SCHEMA),
        "--audit-schema",
        str(AUDIT_SCHEMA),
        "--record-id",
        "nightly-79280e76-dev",
        "--action",
        "record-update",
        "--time",
        "2026-11-13T07:50:00-06:00",
        "--note",
        "test event",
    ]
    if extra:
        args.extend(extra)
    return args


def _run(args, **kwargs) -> tuple[int, str]:
    buf = io.StringIO()
    with redirect_stdout(buf):
        rc = admin.main(args, token="", **kwargs)
    return rc, buf.getvalue()


class TestAdminAction(unittest.TestCase):
    def test_dry_run_default_writes_nothing(self):
        original = AUDIT.read_text()
        rc, out = _run(_base_args(), login_lookup=lambda: "gobbajogger")
        self.assertEqual(rc, 0, out)
        self.assertIn("dry-run", out)
        self.assertIn("Write: none", out)
        self.assertIn("actor verified", out)
        self.assertIn("gobbajogger", out)
        self.assertIn("Validator: ACCEPT", out)
        self.assertEqual(AUDIT.read_text(), original)
        self.assertNotIn("--commit", " ".join(_base_args()))

    def test_actor_not_verified_when_login_unavailable(self):
        original = AUDIT.read_text()
        rc, out = _run(_base_args(), login_lookup=lambda: None)
        self.assertEqual(rc, 1, out)
        self.assertIn("actor not verified", out)
        self.assertIn("--commit would be refused", out)
        self.assertEqual(AUDIT.read_text(), original)

    def test_actor_not_verified_when_login_is_not_gobbajogger(self):
        rc, out = _run(_base_args(), login_lookup=lambda: "evil-bot")
        self.assertEqual(rc, 1, out)
        self.assertIn("actor not verified", out)
        self.assertIn("evil-bot", out)
        # Must not mint a valid-looking gobbajogger event.
        self.assertIn('"actor": "actor not verified"', out)

    def test_illegal_hardware_approval_is_rejected(self):
        rc, out = _run(
            _base_args(extra=["--set-state", "owner-hardware-approval=true"]),
            login_lookup=lambda: "gobbajogger",
        )
        self.assertEqual(rc, 1, out)
        self.assertIn("Validator: REJECT", out)
        self.assertIn("install-verified is false", out)
        self.assertIn("Elite approval never implies a Pro approval", out)
        self.assertIn("--commit would be refused", out)

    def test_withdrawn_cannot_gain_promotion(self):
        rc, out = _run(
            _base_args(extra=["--record-id", "nightly-600b3b45-dev", "--set-state", "gate-passed=true"]),
            login_lookup=lambda: "gobbajogger",
        )
        self.assertEqual(rc, 1, out)
        self.assertIn("Validator: REJECT", out)

    def test_commit_refused_when_actor_not_verified_and_fn_not_called(self):
        called = {"n": 0}

        def boom(_payload):
            called["n"] += 1
            raise AssertionError("commit_fn must not be called")

        original = AUDIT.read_text()
        rc, out = _run(
            _base_args(extra=["--commit"]),
            login_lookup=lambda: None,
            commit_fn=boom,
        )
        self.assertEqual(rc, 1, out)
        self.assertEqual(called["n"], 0)
        self.assertIn("Commit refused", out)
        self.assertEqual(AUDIT.read_text(), original)

    def test_commit_refused_on_main_even_when_verified(self):
        called = {"n": 0}

        def boom(_payload):
            called["n"] += 1
            return {"ok": True, "sha": "deadbeef"}

        rc, out = _run(
            _base_args(extra=["--commit", "--branch", "main"]),
            login_lookup=lambda: "gobbajogger",
            commit_fn=boom,
        )
        self.assertEqual(rc, 1, out)
        self.assertEqual(called["n"], 0)
        self.assertIn("forbidden branch", out.lower() + "refusing to commit on forbidden branch main")
        # Usage goes to stderr if raised as UsageError after accept; we catch and report.
        # format_report Commit refused should appear.
        self.assertTrue("forbidden" in out.lower() or "refusing" in out.lower() or "USAGE" in out or "main" in out)

    def test_commit_refused_on_gh_pages(self):
        with self.assertRaises(admin.UsageError):
            admin.assert_branch_allowed("gh-pages", admin.DEFAULT_ALLOWED_BRANCHES)
        with self.assertRaises(admin.UsageError):
            admin.assert_branch_allowed("nightly", admin.DEFAULT_ALLOWED_BRANCHES)
        with self.assertRaises(admin.UsageError):
            admin.assert_branch_allowed("wip/one-build", admin.DEFAULT_ALLOWED_BRANCHES)
        # Allowed branches do not raise.
        admin.assert_branch_allowed("feature/rp-release-status-workflow-20261113", admin.DEFAULT_ALLOWED_BRANCHES)

    def test_verified_commit_calls_fn_with_release_status_paths_only(self):
        captured = {}

        def fake(payload):
            captured.update(payload)
            return {
                "ok": True,
                "sha": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
                "branch": payload["branch"],
                "repo": payload["repo"],
            }

        with tempfile.TemporaryDirectory() as td:
            tdir = Path(td)
            args = _base_args(tdir, extra=["--commit", "--branch", "feature/rp-release-status-workflow-20261113"])
            rc, out = _run(args, login_lookup=lambda: "gobbajogger", commit_fn=fake)
            self.assertEqual(rc, 0, out)
            self.assertIn("Commit: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", out)
            self.assertTrue(captured)
            paths = set(captured["files"])
            self.assertTrue(paths)
            for p in paths:
                self.assertTrue(p.startswith("release-status/"), p)
            self.assertIn("release-status/audit/audit.jsonl", paths)
            self.assertTrue(any(p.startswith("release-status/records/") for p in paths))
            # The prepared audit is a strict append of one canonical line.
            original_lines = (tdir / "audit" / "audit.jsonl").read_text().splitlines()
            new_lines = captured["files"]["release-status/audit/audit.jsonl"].splitlines()
            self.assertEqual(new_lines[: len(original_lines)], original_lines)
            self.assertEqual(len(new_lines), len(original_lines) + 1)
            # Local files on disk are unchanged: only the commit payload holds the new text.
            self.assertEqual(len(load_jsonl(tdir / "audit" / "audit.jsonl")), len(original_lines))

    def test_illegal_commit_does_not_call_fn(self):
        called = {"n": 0}

        def boom(_payload):
            called["n"] += 1
            return {"ok": True, "sha": "x"}

        rc, out = _run(
            _base_args(extra=["--commit", "--set-state", "install-verified=true"]),
            login_lookup=lambda: "gobbajogger",
            commit_fn=boom,
        )
        self.assertEqual(rc, 1, out)
        self.assertEqual(called["n"], 0)
        self.assertIn("Validator: REJECT", out)

    def test_usage_unknown_flag(self):
        buf = io.StringIO()
        err = io.StringIO()
        # parse_set_state raises UsageError, main returns 2
        rc, out = _run(
            _base_args(extra=["--set-state", "not-a-flag=true"]),
            login_lookup=lambda: "gobbajogger",
        )
        self.assertEqual(rc, 2)

    def test_verify_actor_helpers(self):
        v = admin.verify_actor(token=None, login_lookup=lambda: "gobbajogger")
        self.assertEqual(v["verification"], "verified")
        self.assertEqual(v["actor"], "gobbajogger")
        n = admin.verify_actor(token=None, login_lookup=lambda: None)
        self.assertEqual(n["verification"], "actor not verified")
        self.assertEqual(n["actor"], "actor not verified")
        w = admin.verify_actor(token=None, login_lookup=lambda: "someone-else")
        self.assertEqual(w["verification"], "actor not verified")
        self.assertEqual(w["login"], "someone-else")


if __name__ == "__main__":
    unittest.main()
