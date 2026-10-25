#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Unit coverage for tools/merge_project_prs.py classification and order."""

from __future__ import annotations

import importlib.util
import pathlib
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
            pr(head_ref="cursor/stage5", base_ref="nightly", files=["docs/x.md"])
        )
        self.assertEqual(action.decision, "skip")
        self.assertIn("stage-5", action.reason)

    def test_skips_firmware_on_main(self):
        action = MOD.classify(pr(files=["firmware/app.c", "AGENTS.md"]))
        self.assertEqual(action.decision, "skip")
        self.assertIn("firmware/", action.reason)

    def test_main_preapproved_without_review(self):
        action = MOD.classify(
            pr(review_decision="REVIEW_REQUIRED", merge_state_status="BLOCKED")
        )
        self.assertEqual(action.decision, "merge")
        self.assertIn("pre-approved", action.reason)

    def test_stack_base_merges(self):
        action = MOD.classify(
            pr(head_ref="cursor/child", base_ref="cursor/parent", review_decision="")
        )
        self.assertEqual(action.decision, "merge")

    def test_conflicts_skipped(self):
        action = MOD.classify(pr(mergeable="CONFLICTING", merge_state_status="DIRTY"))
        self.assertEqual(action.decision, "skip")
        self.assertEqual(action.reason, "real conflicts")

    def test_onetime_setup_text_says_unlock_done(self):
        self.assertIn("done", MOD.ONETIME_SETUP.lower())
        self.assertIn("do not ask the owner again", MOD.ONETIME_SETUP.lower())
        self.assertIn(str(MOD.MAIN_RULESET_ID), MOD.ONETIME_SETUP)
        self.assertNotIn("please approve", MOD.ONETIME_SETUP.lower())


if __name__ == "__main__":
    unittest.main()
