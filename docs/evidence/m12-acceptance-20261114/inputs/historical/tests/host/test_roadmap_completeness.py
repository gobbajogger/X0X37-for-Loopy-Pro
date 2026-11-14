#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Offline tests of the completeness gate, with actual Git source objects."""
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import roadmap_completeness as rc  # noqa: E402


def git(root, *args):
    env = dict(os.environ, GIT_AUTHOR_NAME="fixture", GIT_AUTHOR_EMAIL="fixture@example.invalid",
               GIT_COMMITTER_NAME="fixture", GIT_COMMITTER_EMAIL="fixture@example.invalid")
    return subprocess.check_output(["git", "-C", str(root), *args], env=env, text=True).strip()


def table(items):
    lines = ["# X0X37 roadmap", "", "## Done", "", "| Area | What | Where |",
             "|---|---|---|", "| Docs | Existing source | baseline |", "",
             "## Master tracker (status, evidence, next step)", "",
             "| Item | Source | Status | Evidence | Next step |", "|---|---|---|---|---|"]
    for identity, state in items:
        lines.append(f"| {identity} Feature | #19 | {state} | literal origin checked | keep |")
    return "\n".join(lines) + "\n"


class CompletenessTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="dnbox-roadmap-test-")
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.repo = self.root / "repo"
        self.repo.mkdir()
        git(self.repo, "init", "-q", "-b", "main")
        (self.repo / "src").mkdir()
        (self.repo / "docs").mkdir()
        (self.repo / "src" / "feature.c").write_text("int feature_present(void) { return 17; }\n")
        (self.repo / "docs" / "ROADMAP.md").write_text(table([("#1", "in 0.0.24"), ("#2", "missing")]))
        git(self.repo, "add", "src/feature.c", "docs/ROADMAP.md")
        git(self.repo, "commit", "-qm", "origin")
        self.origin = git(self.repo, "rev-parse", "HEAD")
        self.origin_tree = git(self.repo, "rev-parse", "HEAD^{tree}")
        self.src_blob = git(self.repo, "rev-parse", "HEAD:src/feature.c")
        self.roadmap_blob = git(self.repo, "rev-parse", "HEAD:docs/ROADMAP.md")
        self.proof = {
            "origin_commit": self.origin, "origin_tree": self.origin_tree,
            "source_blob": self.src_blob, "source_path": "src/feature.c",
            "kind": "blob", "needle": "int feature_present(void) { return 17; }", "pr": "#19",
        }
        self.map = self.root / "map.json"
        self.report = self.root / "report.json"
        self.selected = []
        self.selection_entries = None
        self.proofs = {"#1": [self.proof], "DONE-001": [self.proof]}
        self.save_selection()
        self.save_evidence()
        self.candidate = self.origin

    def save_selection(self):
        rows = rc.parse_roadmap(git(self.repo, "show", self.origin+":docs/ROADMAP.md"))
        policy = {"schema": rc.SELECTION_SCHEMA, "roadmap_ref": self.origin,
                  "roadmap_blob": self.roadmap_blob,
                  "required_roadmap_rows": sorted({r["id"] for r in rows
                                            if r["status"].lower().startswith(("done", "in 0.0.24", "verified and agreed by owner"))}),
                  "selected_fixes": self.selection_entries if self.selection_entries is not None else
                  [{"id": x, "canonical_id": x, "scope": "firmware"} for x in self.selected]}
        (self.repo / rc.SELECTION_PATH).write_text(json.dumps(policy)+"\n")
        git(self.repo, "add", rc.SELECTION_PATH)
        if subprocess.run(["git", "-C", str(self.repo), "diff", "--cached", "--quiet"]).returncode:
            git(self.repo, "commit", "-qm", "independent fixture release selection")
        self.selection_ref = git(self.repo, "rev-parse", "HEAD")
        self.selection_blob = git(self.repo, "rev-parse", "HEAD:"+rc.SELECTION_PATH)

    def verify(self, candidate=None, roadmap_ref=None):
        return rc.verify_pass(self.repo, self.report, candidate or self.candidate,
                              roadmap_ref or self.origin, self.map,
                              self.selection_ref, self.selection_blob,
                              getattr(self, "external_repositories", None))

    def save_evidence(self):
        self.map.write_text(json.dumps({"schema": rc.EVIDENCE_SCHEMA,
                                        "roadmap_blob": self.roadmap_blob,
                                        "required_release_rows": self.selected,
                                        "proofs": self.proofs}) + "\n")

    def check(self):
        return rc.evaluate(self.repo, self.candidate, git(self.repo, "rev-parse", self.candidate+"^{tree}"),
                           self.origin, self.roadmap_blob, self.map,
                           self.selection_ref, self.selection_blob,
                           getattr(self, "external_repositories", None))

    def save_report(self):
        result = self.check()
        self.report.write_text(json.dumps(result))
        return result

    def mutate(self, path, text):
        (self.repo / path).write_text(text)
        git(self.repo, "add", path)
        git(self.repo, "commit", "-qm", "change fixture")
        self.candidate = git(self.repo, "rev-parse", "HEAD")

    def test_present_and_queued(self):
        report = self.save_report()
        states = {r["id"]:r["result"] for r in report["rows"]}
        self.assertEqual(states, {"DONE-001":"PRESENT", "#1":"PRESENT", "#2":"QUEUED"})
        self.assertTrue(report["passed"])
        self.assertEqual(self.verify(), report)

    def test_required_missing_fails(self):
        self.proofs.pop("#1")
        self.save_evidence()
        report = self.save_report()
        self.assertFalse(report["passed"])
        self.assertEqual(report["summary"]["MISSING"], 1)
        with self.assertRaisesRegex(rc.GateError, "FAIL"):
            self.verify()

    def test_unresolved_reference_fails(self):
        self.proofs["#1"][0] = {**self.proof, "source_blob": "0" * 40}
        self.save_evidence()
        report = self.save_report()
        self.assertFalse(report["passed"])
        self.assertTrue(any("origin file does not match" in r["reason"] for r in report["rows"]))

    def test_missing_required_selected_row_fails_even_if_absent_from_roadmap(self):
        self.selected.append("PR-821")
        self.save_selection()
        self.save_evidence()
        report = self.save_report()
        self.assertIn("PR-821", [r["id"] for r in report["rows"] if r["result"] == "MISSING"])

    def test_stale_source_and_roadmap_rejected(self):
        self.save_report()
        old = self.candidate
        self.mutate("src/feature.c", "int feature_present(void) { return 18; }\n")
        with self.assertRaisesRegex(rc.GateError, "stale completeness report: candidate"):
            self.verify()
        with self.assertRaisesRegex(rc.GateError, "ROADMAP revision"):
            self.verify(old, self.candidate)
        changed = self.check()
        self.assertFalse(changed["passed"])
        self.assertTrue(any("source signature absent" in r["reason"] for r in changed["rows"]))

    def test_stale_evidence_and_report_edit_rejected(self):
        self.save_report()
        self.selected = ["#2"]
        self.save_evidence()
        with self.assertRaisesRegex(rc.GateError, "independent immutable selection"):
            self.verify()
        self.selected = []
        self.save_evidence()
        self.save_report()
        altered = json.loads(self.report.read_text()); altered["summary"]["PRESENT"] = 999
        self.report.write_text(json.dumps(altered))
        with self.assertRaisesRegex(rc.GateError, "stale or altered"):
            self.verify()

    def test_source_equivalence_without_ancestor_sha(self):
        # Cherry-pick/squash equivalent: target commit differs, source blob is identical.
        self.mutate("README.md", "unrelated docs and a new commit ID\n")
        report = self.save_report()
        self.assertNotEqual(self.candidate, self.origin)
        self.assertEqual(report["summary"], {"PRESENT":2,"MISSING":0,"QUEUED":1})
        self.assertTrue(report["passed"])
        self.assertEqual(self.verify(),report)

    def test_ambiguous_source_token_and_duplicate_row_fail(self):
        self.proofs["#1"][0]["needle"] = "feature_present"
        self.save_evidence()
        # repeated signature in an otherwise identical file means no valid blob proof
        self.mutate("src/feature.c", "int feature_present(void) { return 17; }\n// feature_present\n")
        self.assertFalse(self.check()["passed"])
        # Duplicate ID must fail closed even if evidence would otherwise pass.
        self.mutate("src/feature.c", "int feature_present(void) { return 17; }\n")
        self.mutate("docs/ROADMAP.md", table([("#1", "in 0.0.24"),("#1", "done")]))
        self.origin = self.candidate
        self.roadmap_blob = git(self.repo,"rev-parse","HEAD:docs/ROADMAP.md")
        self.save_selection()
        self.proofs["#1"][0]["needle"] = "int feature_present(void) { return 17; }"
        self.save_evidence()
        report = self.check()
        self.assertFalse(report["passed"])
        self.assertEqual(len([r for r in report["rows"] if r["id"] == "#1" and r["result"]=="MISSING"]),2)

    def test_cli_negative_exit_and_artifact(self):
        command=[sys.executable,str(ROOT/"tools/roadmap_completeness.py"),"--repo",str(self.repo),
                 "--candidate",self.candidate,"--candidate-tree",self.origin_tree,
                 "--roadmap-ref",self.origin,"--roadmap-blob",self.roadmap_blob,
                 "--selection-ref",self.selection_ref,"--selection-blob",self.selection_blob,
                 "--evidence",str(self.map),"--report",str(self.report)]
        good = subprocess.run(command,capture_output=True,text=True)
        self.assertEqual(good.returncode,0,good.stderr)
        self.proofs.clear();self.save_evidence()
        bad = subprocess.run(command,capture_output=True,text=True)
        self.assertEqual(bad.returncode,1,bad.stderr)
        check = subprocess.run([sys.executable,str(ROOT/"tools/roadmap_completeness.py"),
                                "--repo",str(self.repo),"--candidate",self.candidate,
                                "--roadmap-ref",self.origin,"--evidence",str(self.map),
                                "--selection-ref",self.selection_ref,"--selection-blob",self.selection_blob,
                                "--verify-report",str(self.report)],capture_output=True,text=True)
        self.assertEqual(check.returncode,2,check.stdout+check.stderr)


if __name__ == "__main__":
    unittest.main(verbosity=2)
