#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Offline regression coverage for the retired merger's read-only boundary."""

from __future__ import annotations

import importlib.util
import json
import os
import pathlib
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("merge_project_prs", ROOT / "tools" / "merge_project_prs.py")
assert SPEC and SPEC.loader
MOD = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MOD
SPEC.loader.exec_module(MOD)


def raw(number=1, **changes):
    item = dict(number=number, title=f"candidate {number}", url=f"https://example.test/{number}",
                headRefName=f"cursor/{number}", baseRefName="main", headRefOid=f"{number:040x}",
                isDraft=False, mergeable="MERGEABLE", mergeStateStatus="CLEAN", reviewDecision="",
                autoMergeRequest=None, statusCheckRollup=[], files=[{"path": "AGENTS.md"}])
    item.update(changes)
    return item


class ReadOnlyPlannerTest(unittest.TestCase):
    def plan(self, items, *, numbers=None, heads=None, dry_run=False):
        calls = []
        def runner(args, check=True):
            calls.append(list(args))
            self.assertEqual(args[:2], ["pr", "list"], "planner attempted a GitHub mutation")
            return subprocess.CompletedProcess(args, 0, json.dumps(items), "")
        result = MOD.execute("owner/repo", dry_run=dry_run, proven_prs=numbers or set(), proven_heads=heads or set(), runner=runner)
        self.assertEqual(len(calls), 1)
        self.assertTrue(all(not action.command for action in result))
        return result, calls

    def test_default_and_dry_run_only_plan(self):
        for flag in (False, True):
            with self.subTest(dry_run=flag):
                actions, _ = self.plan([raw()], numbers={1}, dry_run=flag)
                self.assertEqual(actions[0].decision, "plan")
                self.assertIn("remain unverified", actions[0].reason)
                self.assertEqual(actions[0].head_oid, f"{1:040x}")

    def test_pr_number_is_only_a_selector_not_verified_proof(self):
        actions, _ = self.plan([raw()], numbers={1})
        self.assertNotIn(actions[0].decision, {"merge", "merged", "armed", "auto-merge-enabled"})
        self.assertIn("local proof", actions[0].reason)
        self.assertIn("authority remain unverified", actions[0].reason)

    def test_selected_full_head_requires_exact_match(self):
        actions, _ = self.plan([raw(1), raw(2)], heads={f"{2:040X}"})
        self.assertEqual([action.number for action in actions], [2])
        for value in ("0000000", "g" * 40, "1" * 41):
            with self.subTest(value=value), self.assertRaises(MOD.GhError):
                MOD.execute("owner/repo", dry_run=False, proven_prs=set(), proven_heads={value}, runner=lambda *a, **k: self.fail("invalid selector caused a read"))

    def test_empty_selection_and_invalid_pr_never_read(self):
        for numbers in (set(), {0}, {-1}, {True}):
            with self.subTest(numbers=numbers), self.assertRaises(MOD.GhError):
                MOD.execute("owner/repo", dry_run=False, proven_prs=numbers, proven_heads=set(), runner=lambda *a, **k: self.fail("invalid selector caused a read"))

    def test_unknown_or_conflicting_mergeability_blocks_only_candidate(self):
        for state in ("UNKNOWN", "CONFLICTING", ""):
            with self.subTest(state=state):
                actions, _ = self.plan([raw(1, mergeable=state), raw(2)], numbers={1, 2})
                self.assertEqual([action.decision for action in actions], ["blocked", "plan"])

    def test_draft_and_protected_destinations_are_not_execution(self):
        for fields in ({"isDraft": True}, {"baseRefName": "nightly"}, {"baseRefName": "gh-pages"}, {"files": [{"path": "firmware/app.c"}]}):
            with self.subTest(fields=fields):
                actions, _ = self.plan([raw(**fields)], numbers={1})
                self.assertEqual(actions[0].decision, "blocked")

    def test_stack_is_preserved_without_retarget(self):
        actions, calls = self.plan([raw(10), raw(11, baseRefName="cursor/10")], numbers={10, 11})
        self.assertEqual([action.decision for action in actions], ["plan", "blocked"])
        self.assertEqual(actions[1].base_ref, "cursor/10")
        self.assertIn("never retarget", actions[1].reason)
        self.assertFalse(any("edit" in call or "merge" in call for call in calls))

    def test_failed_checks_never_fall_through_from_auto_merge(self):
        actions, calls = self.plan([raw(autoMergeRequest={"enabledAt": "now"}, statusCheckRollup=[{"name": "required", "conclusion": "FAILURE"}])], numbers={1})
        self.assertEqual(actions[0].decision, "blocked")
        self.assertIn("left unchanged", actions[0].reason)
        self.assertFalse(any("--admin" in call or "--auto" in call for call in calls))

    def test_pending_status_and_required_review_remain_observations(self):
        for changes in ({"statusCheckRollup": [{"context": "gate", "state": "PENDING"}]}, {"statusCheckRollup": [{"name": "gate", "status": "IN_PROGRESS"}]}, {"reviewDecision": "REVIEW_REQUIRED"}, {"reviewDecision": "CHANGES_REQUESTED"}, {"mergeStateStatus": "BLOCKED"}):
            with self.subTest(changes=changes):
                actions, _ = self.plan([raw(**changes)], numbers={1})
                self.assertEqual(actions[0].decision, "blocked")

    def test_healthy_auto_merge_is_neither_changed_nor_claimed_complete(self):
        actions, _ = self.plan([raw(autoMergeRequest={"enabledAt": "now"})], numbers={1})
        self.assertEqual(actions[0].decision, "plan")
        self.assertIn("left unchanged", actions[0].reason)

    def test_bad_selected_metadata_preserves_independent_plan(self):
        for invalid in (None, [], [{"path": ""}]):
            with self.subTest(files=invalid):
                actions, _ = self.plan([raw(1, files=invalid), raw(2)], numbers={1, 2})
                self.assertEqual([action.decision for action in actions], ["blocked", "plan"])

    def test_unselected_bad_metadata_does_not_suppress_selected_work(self):
        actions, _ = self.plan([raw(1, files=None), raw(2)], numbers={2})
        self.assertEqual([(action.number, action.decision) for action in actions], [(2, "plan")])

    def test_missing_selection_is_unresolved_not_done(self):
        actions, _ = self.plan([], numbers={1}, heads={"f" * 40})
        self.assertEqual(len(actions), 2)
        self.assertTrue(all(action.decision == "unresolved" for action in actions))

    def test_read_denial_is_not_retried_or_routed_to_a_write(self):
        calls = []
        def refused(args, check=True):
            calls.append(args)
            return subprocess.CompletedProcess(args, 1, "", "permission denied")
        with self.assertRaisesRegex(MOD.GhError, "permission denied"):
            MOD.execute("owner/repo", dry_run=False, proven_prs={1}, proven_heads=set(), runner=refused)
        self.assertEqual(len(calls), 1)
        self.assertEqual(calls[0][:2], ["pr", "list"])

    def test_empty_or_malformed_reads_are_unknown(self):
        for body in ("", "not JSON", "{}", "[null]"):
            with self.subTest(body=body[:20]), self.assertRaises(MOD.GhError):
                MOD.execute("owner/repo", dry_run=False, proven_prs={1}, proven_heads=set(), runner=lambda args, check=True: subprocess.CompletedProcess(args, 0, body, ""))

    def test_bounded_inventory_preserves_observed_selected_candidates(self):
        actions, _ = self.plan([raw(i + 1) for i in range(MOD.LIST_LIMIT)], numbers={1, 9999})
        self.assertEqual([(action.number, action.decision) for action in actions], [(1, "plan"), (9999, "unresolved")])
        self.assertTrue(all("inventory capped" in action.reason for action in actions))

    def test_subprocess_guard_rejects_mutation_before_execution(self):
        with mock.patch.object(MOD.subprocess, "run") as process:
            for args in (["pr", "merge", "1"], ["pr", "edit", "1"], ["api", "--method", "PUT"]):
                with self.subTest(args=args), self.assertRaises(MOD.GhError):
                    MOD.run_gh(args)
            process.assert_not_called()

    def test_cli_json_default_and_dry_run_with_fake_gh(self):
        with tempfile.TemporaryDirectory() as temp:
            folder = pathlib.Path(temp)
            fake = folder / "gh"
            fake.write_text("#!/usr/bin/env python3\nimport json,os,sys\nfrom pathlib import Path\nwith Path(os.environ['TEST_GH_LOG']).open('a') as out: out.write(json.dumps(sys.argv[1:])+'\\n')\nif sys.argv[1:3] != ['pr','list']: sys.exit(91)\nprint(os.environ['TEST_GH_RESPONSE'])\nsys.exit(int(os.environ.get('TEST_GH_EXIT','0')))\n")
            fake.chmod(0o755)
            log = folder / "calls.jsonl"
            env = {**os.environ, "PATH": str(folder) + os.pathsep + os.environ.get("PATH", ""), "TEST_GH_LOG": str(log), "TEST_GH_RESPONSE": json.dumps([raw(1), raw(2, mergeable="UNKNOWN")])}
            for args in (["--pr", "1", "--json"], ["--pr", "1,2", "--json", "--dry-run"]):
                run = subprocess.run([sys.executable, str(ROOT / "tools/merge_project_prs.py"), *args], env=env, capture_output=True, text=True)
                self.assertEqual(run.returncode, 2 if "1,2" in args else 0, run.stderr)
                report = json.loads(run.stdout)
                self.assertEqual(report[0]["decision"], "plan")
                self.assertTrue(all(not item["command"] for item in report))
            self.assertTrue(all(json.loads(line)[:2] == ["pr", "list"] for line in log.read_text().splitlines()))

    def test_setup_notice_has_no_live_unlock_claim(self):
        self.assertIn("READ-ONLY", MOD.ONETIME_SETUP)
        self.assertIn("not merge authority", MOD.ONETIME_SETUP)
        self.assertNotIn("required_approving_review_count = 0", MOD.ONETIME_SETUP)


if __name__ == "__main__":
    unittest.main()
