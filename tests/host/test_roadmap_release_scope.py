#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Release-selection trust-boundary regressions, using real Git objects."""
import unittest
import json
import hashlib
import io
import os
import subprocess
import sys
from unittest.mock import patch

import test_roadmap_completeness as fixture
from test_roadmap_completeness import git, table
import roadmap_completeness as rc


class ReleaseScopeTests(unittest.TestCase):
    setUp = fixture.CompletenessTests.setUp
    save_evidence = fixture.CompletenessTests.save_evidence
    save_selection = fixture.CompletenessTests.save_selection
    check = fixture.CompletenessTests.check
    save_report = fixture.CompletenessTests.save_report
    mutate = fixture.CompletenessTests.mutate
    verify = fixture.CompletenessTests.verify

    def test_omitting_mandatory_selection_cannot_regenerate_pass(self):
        self.selected = ["PR-821"]
        self.save_selection()
        self.save_evidence()
        first = self.save_report()
        self.assertFalse(first["passed"])
        unchanged_candidate = self.candidate
        self.selected = []
        self.save_evidence()
        self.assertEqual(self.candidate, unchanged_candidate)
        with self.assertRaisesRegex(rc.GateError, "independent immutable selection"):
            self.save_report()
        with self.assertRaisesRegex(rc.GateError, "independent immutable selection"):
            self.verify()

    def test_recommitting_reduced_manifest_cannot_drop_roadmap_selected_alias(self):
        authority = table([("#1", "in 0.0.24"), ("#2", "missing")]).replace(
            "| #1 Feature | #19 | in 0.0.24 | literal origin checked |",
            "| #1 Feature | #19 | in 0.0.24 | Selected release alias: **PR-821 → #1** |")
        self.mutate("docs/ROADMAP.md", authority)
        self.origin = self.candidate
        self.roadmap_blob = git(self.repo, "rev-parse", "HEAD:docs/ROADMAP.md")
        self.selected = ["PR-821"]
        self.selection_entries = [{"id": "PR-821", "canonical_id": "#1", "scope": "firmware"}]
        self.save_selection()
        self.save_evidence()
        self.assertFalse(self.save_report()["passed"])
        candidate = self.candidate
        self.selected = []
        self.selection_entries = []
        self.save_selection()
        self.save_evidence()
        self.assertEqual(self.candidate, candidate)
        with self.assertRaisesRegex(rc.GateError, "ROADMAP selected alias"):
            self.save_report()

    def test_declared_external_alias_cannot_be_remapped_or_laundered(self):
        authority = table([("#1", "in 0.0.24"), ("#2", "missing")]).replace(
            "| #1 Feature | #19 | in 0.0.24 | literal origin checked |",
            "| #1 Feature | #19 | in 0.0.24 | Selected release alias: **FIX-INSTALLER-BACKUP → #1** |")
        self.mutate("docs/ROADMAP.md", authority)
        self.origin = self.candidate
        self.roadmap_blob = git(self.repo, "rev-parse", "HEAD:docs/ROADMAP.md")
        self.selected = ["FIX-INSTALLER-BACKUP"]
        for canonical, scope in (("#2", "external"), ("#1", "firmware")):
            self.selection_entries = [{"id": self.selected[0], "canonical_id": canonical, "scope": scope,
                                       "external": {"repository": rc.INSTALLER_REPOSITORY,
                                                    "live_url": rc.INSTALLER_URL}}]
            self.save_selection()
            self.save_evidence()
            with self.assertRaisesRegex(rc.GateError, "ROADMAP selected alias"):
                self.check()

    def test_explicit_fix_id_is_preserved(self):
        rows = rc.parse_roadmap(table([("FIX-809-REVERT", "done")]))
        self.assertEqual(rows[-1]["id"], "FIX-809-REVERT")

    def test_explicit_fix_row_positive_missing_and_duplicate_cycle(self):
        self.mutate("docs/ROADMAP.md", table([("FIX-809-REVERT", "done")]))
        self.origin = self.candidate
        self.roadmap_blob = git(self.repo, "rev-parse", "HEAD:docs/ROADMAP.md")
        self.proofs = {"DONE-001": [self.proof], "FIX-809-REVERT": [self.proof]}
        self.save_selection()
        self.save_evidence()
        self.assertEqual(self.save_report()["summary"], {"PRESENT": 2, "MISSING": 0, "QUEUED": 0})
        self.assertTrue(self.verify()["passed"])
        self.proofs.pop("FIX-809-REVERT")
        self.save_evidence()
        self.assertEqual(self.check()["summary"]["MISSING"], 1)
        self.mutate("docs/ROADMAP.md", table([("FIX-809-REVERT", "done"), ("FIX-809-REVERT", "done")]))
        self.origin = self.candidate
        self.roadmap_blob = git(self.repo, "rev-parse", "HEAD:docs/ROADMAP.md")
        self.proofs["FIX-809-REVERT"] = [self.proof]
        self.save_selection()
        self.save_evidence()
        self.assertEqual(self.check()["summary"]["MISSING"], 2)

    def test_qualified_built_status_still_requires_and_accepts_source(self):
        self.mutate("docs/ROADMAP.md", table([("#1", "in 0.0.24 (historical integrated source; selected revert pending exact verification)")]))
        self.origin = self.candidate
        self.roadmap_blob = git(self.repo, "rev-parse", "HEAD:docs/ROADMAP.md")
        self.save_selection()
        self.save_evidence()
        assessed = self.check()
        self.assertEqual(assessed["summary"], {"PRESENT": 2, "MISSING": 0, "QUEUED": 0})

    def test_external_selection_cannot_use_firmware_proof_even_for_canonical_id(self):
        self.selected = ["#1"]
        self.selection_entries = [{"id": "#1", "canonical_id": "#1", "scope": "external",
                                   "external": {"repository": rc.INSTALLER_REPOSITORY,
                                                "live_url": rc.INSTALLER_URL}}]
        self.save_selection()
        self.save_evidence()
        self.assertFalse(self.check()["passed"], "firmware proof cannot satisfy external installer selection")

    def test_scope_substitution_is_rejected_against_independent_expected_pin(self):
        self.save_report()
        accepted_ref, accepted_blob = self.selection_ref, self.selection_blob
        self.selected = ["PR-821"]
        self.save_selection()
        with self.assertRaisesRegex(rc.GateError, "independent release selection changed"):
            self.verify()
        # Caller-supplied acceptance identity, not the report's own fields, controls verification.
        self.selection_ref, self.selection_blob = accepted_ref, accepted_blob
        self.assertTrue(self.verify()["passed"])

    def test_shared_canonical_row_does_not_supply_selected_alias_proofs(self):
        self.selected = ["PR-820", "FIX-SMC-ALIGN"]
        self.selection_entries = [{"id": alias, "canonical_id": "#1", "scope": "firmware"}
                                  for alias in self.selected]
        self.save_selection()
        self.save_evidence()
        self.assertEqual(self.check()["summary"]["MISSING"], 2)
        self.proofs["PR-820"] = [self.proof]
        self.save_evidence()
        self.assertEqual(self.check()["summary"]["MISSING"], 1)
        self.proofs["FIX-SMC-ALIGN"] = [self.proof]
        self.save_evidence()
        self.assertTrue(self.save_report()["passed"])
        self.assertTrue(self.verify()["passed"])

    def test_retired_shell_route_refuses_even_an_allegedly_passing_gate_before_io(self):
        # These stubs are the external process boundary, not the gate under test.
        # A gate exit 0 must never authorize the legacy force-push route.
        bindir = self.root / "bin"
        bindir.mkdir()
        log = self.root / "processes"
        for name in ("git", "gh", "python3"):
            executable = bindir / name
            executable.write_text('#!/bin/sh\nprintf "%s\\n" "'+name+' $*" >> "$PROCESS_LOG"\nexit 0\n')
            executable.chmod(0o755)
        result = subprocess.run(["sh", str(fixture.ROOT / "tools/publish_pages.sh"), "--shell",
                                 "--completeness-report", str(self.report), "--candidate", self.candidate,
                                 "--roadmap-ref", self.origin, "--completeness-evidence", str(self.map)],
                                env={**os.environ, "PATH": str(bindir)+os.pathsep+os.environ["PATH"],
                                     "PROCESS_LOG": str(log)}, capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertFalse(log.exists(), "retired route must reject before gate, Git or Pages process I/O")

    def test_package_rechecks_independent_selection_before_reading_package(self):
        self.save_report()
        command = [sys.executable, str(fixture.ROOT / "tools/package_nightly.py"),
                   str(self.root / "DO-NOT-READ.fwsc"), "--check", "--model", "elite",
                   "--controller-sha256", "0" * 64, "--controller-size", "4096", "--firmware-version", "0.0.24.5",
                   "--repo", str(self.repo), "--candidate", self.candidate,
                   "--roadmap-ref", self.origin, "--selection-ref", self.selection_ref,
                   "--selection-blob", "0" * 40, "--completeness-report", str(self.report),
                   "--completeness-evidence", str(self.map)]
        result = subprocess.run(command, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("independent release selection changed", result.stderr)
        self.assertNotIn("DO-NOT-READ.fwsc", result.stderr)

    def test_generation_and_verification_require_independent_selection_pin(self):
        self.save_report()
        with self.assertRaisesRegex(rc.GateError, "independent release selection revision"):
            rc.evaluate(self.repo, self.candidate, self.origin_tree, self.origin, self.roadmap_blob, self.map)
        with self.assertRaisesRegex(rc.GateError, "independent release selection revision"):
            rc.verify_pass(self.repo, self.report, self.candidate, self.origin, self.map)

    def test_invalid_external_repository_is_clean_cli_refusal_before_output(self):
        out = self.root / "DO-NOT-WRITE"
        command = [sys.executable, str(fixture.ROOT / "tools/publish_nightly.py"),
                   "--repo", str(self.repo), "--elite-record", "MISSING", "--elite-fwsc", "MISSING",
                   "--pro-record", "MISSING", "--pro-fwsc", "MISSING", "--out", str(out),
                   "--external-repo", "unexpected/repository=/tmp/not-accepted"]
        result = subprocess.run(command, capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn("invalid or duplicate external source repository", result.stderr)
        self.assertNotIn("Traceback", result.stderr)
        self.assertFalse(out.exists())

    def test_selection_cannot_omit_built_row_or_duplicate_alias(self):
        policy = json.loads((self.repo / rc.SELECTION_PATH).read_text())
        policy["required_roadmap_rows"].remove("#1")
        (self.repo / rc.SELECTION_PATH).write_text(json.dumps(policy))
        git(self.repo, "add", rc.SELECTION_PATH)
        git(self.repo, "commit", "-qm", "malformed selection fixture")
        self.selection_ref = git(self.repo, "rev-parse", "HEAD")
        self.selection_blob = git(self.repo, "rev-parse", "HEAD:"+rc.SELECTION_PATH)
        with self.assertRaisesRegex(rc.GateError, "built/accepted"):
            self.check()
        self.selected = ["PR-820"]
        self.selection_entries = [{"id": "PR-820", "canonical_id": "#1", "scope": "firmware"}] * 2
        self.save_selection()
        self.save_evidence()
        with self.assertRaisesRegex(rc.GateError, "duplicate selected fix"):
            self.check()

    def test_external_source_and_live_bytes_are_independent_and_fresh(self):
        external_repo = self.root / "external"
        external_repo.mkdir()
        git(external_repo, "init", "-q", "-b", "fixture")
        (external_repo / "install").mkdir()
        live = b"<p>Back up all loop slots before updating.</p>\n"
        (external_repo / "install/index.html").write_bytes(live)
        git(external_repo, "add", "install/index.html")
        git(external_repo, "commit", "-qm", "independent external installer fixture")
        commit = git(external_repo, "rev-parse", "HEAD")
        tree = git(external_repo, "rev-parse", "HEAD^{tree}")
        blob = git(external_repo, "rev-parse", "HEAD:install/index.html")
        self.selected = ["FIX-INSTALLER-BACKUP"]
        external = {"repository": rc.INSTALLER_REPOSITORY, "live_url": rc.INSTALLER_URL,
                    "commit": commit, "tree": tree, "blob": blob, "path": "install/index.html",
                    "live_sha256": hashlib.sha256(live).hexdigest()}
        self.selection_entries = [{"id": self.selected[0], "canonical_id": "#1", "scope": "external",
                                   "external": external}]
        self.proofs[self.selected[0]] = [{"kind": "external", "source": {
            "origin_commit": commit, "origin_tree": tree, "source_blob": blob,
            "source_path": "install/index.html", "kind": "blob",
            "needle": "Back up all loop slots before updating."}}]
        self.save_selection()
        self.save_evidence()
        self.assertFalse(self.check()["passed"], "missing independent source repository must fail")
        self.external_repositories = {rc.INSTALLER_REPOSITORY: external_repo}

        def response(body):
            stream = io.BytesIO(body)
            stream.geturl = lambda: rc.INSTALLER_URL
            return stream

        with patch.object(rc.urllib.request, "urlopen", side_effect=lambda url, timeout: response(live)) as read:
            self.assertTrue(self.save_report()["passed"])
            self.assertTrue(self.verify()["passed"])
            self.assertEqual(read.call_count, 2, "verification must re-read live bytes, not trust cached PASS")
        with patch.object(rc.urllib.request, "urlopen", return_value=response(b"old installer")):
            with self.assertRaisesRegex(rc.GateError, "stale or altered"):
                self.verify()
        (external_repo / "install/index.html").write_text("local workspace edits are not Git evidence")
        # Immutable source objects, not mutable worktree bytes, remain the authority.
        with patch.object(rc.urllib.request, "urlopen", side_effect=lambda url, timeout: response(live)):
            self.assertTrue(self.verify()["passed"])

        # A fresh selection pin cannot bind different served code to this source.
        different_live = b"<p>A different installer without the verified backup path.</p>\n"
        external["live_sha256"] = hashlib.sha256(different_live).hexdigest()
        prior_selection = self.selection_ref
        candidate = self.candidate
        self.save_selection()
        self.assertNotEqual(self.selection_ref, prior_selection)
        self.assertEqual(self.candidate, candidate)
        with patch.object(rc.urllib.request, "urlopen",
                          side_effect=lambda url, timeout: response(different_live)):
            assessed = self.save_report()
            installer = next(row for row in assessed["rows"] if row["id"] == self.selected[0])
            self.assertEqual(installer["result"], "MISSING")
            self.assertIn("live bytes differ from pinned source", installer["reason"])
            self.assertFalse(assessed["passed"])
            with self.assertRaisesRegex(rc.GateError, "completeness FAIL: 1 missing"):
                self.verify()


if __name__ == "__main__":
    unittest.main(verbosity=2)
