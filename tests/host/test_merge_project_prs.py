#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Unit coverage for tools/merge_project_prs.py classification and order."""

from __future__ import annotations

import importlib.util
import pathlib
import subprocess
import sys
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "merge_project_prs",
    ROOT / "tools" / "merge_project_prs.py",
)
assert SPEC and SPEC.loader
MOD = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MOD
SPEC.loader.exec_module(MOD)


def pr(**kwargs):
    defaults = dict(
        number=1,
        title="t",
        url="https://example.test/1",
        head_ref="cursor/a",
        base_ref="main",
        head_oid="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        is_draft=False,
        mergeable="MERGEABLE",
        merge_state_status="CLEAN",
        review_decision="",
        auto_merge=None,
        files=["AGENTS.md"],
        check_failures=[],
    )
    defaults.update(kwargs)
    return MOD.PullRequest(**defaults)


class MergeProjectPrsTest(unittest.TestCase):
    def test_bottom_up_orders_stack(self):
        bottom = pr(number=10, head_ref="cursor/bottom", base_ref="main")
        mid = pr(number=11, head_ref="cursor/mid", base_ref="cursor/bottom")
        top = pr(number=12, head_ref="cursor/top", base_ref="cursor/mid")
        ordered = MOD.bottom_up_order([top, mid, bottom])
        self.assertEqual([p.number for p in ordered], [10, 11, 12])

    def test_skips_nightly_base(self):
        action = MOD.classify(
            pr(head_ref="cursor/stage5", base_ref="nightly", files=["docs/x.md"]),
            proven=True,
        )
        self.assertEqual(action.decision, "skip")
        self.assertIn("stage-5", action.reason)

    def test_skips_firmware_on_main(self):
        action = MOD.classify(pr(files=["firmware/app.c", "AGENTS.md"]), proven=True)
        self.assertEqual(action.decision, "skip")
        self.assertIn("firmware/", action.reason)

    def test_unproven_skipped_even_if_mergeable(self):
        action = MOD.classify(
            pr(review_decision="REVIEW_REQUIRED", merge_state_status="BLOCKED"),
            proven=False,
        )
        self.assertEqual(action.decision, "skip")
        self.assertIn("not in proven set", action.reason)

    def test_proven_main_merges(self):
        action = MOD.classify(pr(), proven=True)
        self.assertEqual(action.decision, "merge")
        self.assertIn("pre-approved", action.reason)

    def test_armed_with_check_failures_falls_through(self):
        action = MOD.classify(
            pr(auto_merge={"enabledAt": "now"}, check_failures=["Focused SMC host tests:FAILURE"]),
            proven=True,
        )
        self.assertEqual(action.decision, "merge")
        self.assertIn("fall through", action.reason)

    def test_armed_without_failures_stays_armed(self):
        action = MOD.classify(pr(auto_merge={"enabledAt": "now"}), proven=True)
        self.assertEqual(action.decision, "armed")

    def test_conflicts_skipped(self):
        action = MOD.classify(
            pr(mergeable="CONFLICTING", merge_state_status="DIRTY"), proven=True
        )
        self.assertEqual(action.decision, "skip")
        self.assertEqual(action.reason, "real conflicts")

    def test_onetime_setup_text_says_unlock_done(self):
        self.assertIn("done", MOD.ONETIME_SETUP.lower())
        self.assertIn("do not ask the owner again", MOD.ONETIME_SETUP.lower())
        self.assertIn(str(MOD.MAIN_RULESET_ID), MOD.ONETIME_SETUP)

    def test_execute_requires_proven_set(self):
        with self.assertRaises(MOD.GhError) as ctx:
            MOD.execute("owner/repo", dry_run=True, proven_prs=set(), proven_heads=set())
        self.assertIn("pass --pr", str(ctx.exception))

    def test_execute_retargets_child_after_parent_merge(self):
        bottom = pr(
            number=10,
            head_ref="cursor/bottom",
            base_ref="main",
            head_oid="bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
        )
        top = pr(
            number=11,
            head_ref="cursor/top",
            base_ref="cursor/bottom",
            head_oid="cccccccccccccccccccccccccccccccccccccccc",
        )
        calls = []

        def runner(args, check=True):
            calls.append(list(args))
            if args[:2] == ["pr", "list"]:
                payload = [
                    {
                        "number": bottom.number,
                        "title": bottom.title,
                        "url": bottom.url,
                        "headRefName": bottom.head_ref,
                        "baseRefName": bottom.base_ref,
                        "headRefOid": bottom.head_oid,
                        "isDraft": False,
                        "mergeable": "MERGEABLE",
                        "mergeStateStatus": "CLEAN",
                        "reviewDecision": "",
                        "autoMergeRequest": None,
                        "statusCheckRollup": [],
                        "files": [{"path": "AGENTS.md"}],
                    },
                    {
                        "number": top.number,
                        "title": top.title,
                        "url": top.url,
                        "headRefName": top.head_ref,
                        "baseRefName": top.base_ref,
                        "headRefOid": top.head_oid,
                        "isDraft": False,
                        "mergeable": "MERGEABLE",
                        "mergeStateStatus": "CLEAN",
                        "reviewDecision": "",
                        "autoMergeRequest": None,
                        "statusCheckRollup": [],
                        "files": [{"path": "AGENTS.md"}],
                    },
                ]
                body = __import__("json").dumps(payload)
                return subprocess.CompletedProcess(args, 0, body, "")
            # merge / edit succeed
            return subprocess.CompletedProcess(args, 0, "", "")

        actions = MOD.execute(
            "owner/repo",
            dry_run=False,
            proven_prs={10, 11},
            proven_heads=set(),
            runner=runner,
        )
        decisions = [(a.number, a.decision) for a in actions]
        self.assertIn((10, "merged"), decisions)
        self.assertIn((11, "retargeted"), decisions)
        self.assertIn((11, "merged"), decisions)
        edit = [c for c in calls if c[:2] == ["pr", "edit"]]
        self.assertTrue(any("--base" in c and "main" in c for c in edit), edit)


if __name__ == "__main__":
    unittest.main()
