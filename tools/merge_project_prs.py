#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only plan for selected legacy Project PRs; never merge or retarget.

The old generic merger is retired. PR numbers and head SHAs are selection
filters, not local-proof receipts, authority or permission to override checks.
The existing assigned integrator must consume the exact candidate and current
base, validate applicable proof/reviews/permissions, and perform any separately
authorized normal merge through its supported route.

Both the default invocation and --dry-run only read GitHub. --json preserves
machine-readable reports. --print-onetime-setup now explains the retirement;
it makes no claim about current branch protection or a past ruleset unlock.

Usage:
  tools/merge_project_prs.py --dry-run --pr 164
  tools/merge_project_prs.py --json --pr 164 --pr 149
  tools/merge_project_prs.py --head <full-40-character-SHA> --repo OWNER/REPO
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from collections import defaultdict
from dataclasses import asdict, dataclass, field
from typing import Any, Callable, List, Optional, Sequence, Set


DEFAULT_REPO = "gobbajogger/X0X37-for-Loopy-Pro"
TRUNK = "main"
FORBIDDEN_BASES = frozenset({"nightly", "gh-pages"})
FIRMWARE_PREFIXES = ("firmware/",)
LIST_LIMIT = 100
ONETIME_SETUP = """READ-ONLY PLANNER: the generic merge executor is retired.
No merge, retarget, auto-merge, branch or protection write is performed.
Use the current project workflow and existing assigned integration owner.
Stored ruleset-unlock notes and --pr/--head filters are not merge authority.
Reuse current standing authorization; do not request routine owner approval.
Actual required review/check failures and action-specific denials remain binding.
"""


@dataclass
class PullRequest:
    number: int
    title: str
    url: str
    head_ref: str
    base_ref: str
    head_oid: str
    is_draft: bool
    mergeable: str
    merge_state_status: str
    review_decision: str
    auto_merge: Any = None
    files: List[str] = field(default_factory=list)
    check_failures: List[str] = field(default_factory=list)


@dataclass
class Action:
    number: int
    title: str
    url: str
    decision: str
    reason: str
    command: List[str] = field(default_factory=list)
    head_oid: str = ""
    base_ref: str = ""


class GhError(RuntimeError):
    """A required read failed or returned unverified/incomplete data."""


def run_gh(args: Sequence[str], *, check: bool = True) -> subprocess.CompletedProcess:
    # Keep a second guard at the actual subprocess boundary. No caller can turn
    # this helper into the old writer by passing a different gh command.
    if list(args[:2]) != ["pr", "list"]:
        raise GhError("read-only planner refuses non-list gh operations")
    completed = subprocess.run(["gh", *args], capture_output=True, text=True)
    if check and completed.returncode != 0:
        detail = (completed.stderr or completed.stdout or "").strip()
        raise GhError(f"required GitHub PR read failed: {detail or completed.returncode}")
    return completed


def load_open_pulls(repo: str, runner: Callable[..., subprocess.CompletedProcess] = run_gh) -> List[dict]:
    completed = runner([
        "pr", "list", "--repo", repo, "--state", "open", "--limit", str(LIST_LIMIT),
        "--json", "number,title,url,headRefName,baseRefName,headRefOid,isDraft,mergeable,"
        "mergeStateStatus,reviewDecision,autoMergeRequest,statusCheckRollup,files",
    ])
    if completed.returncode != 0:
        detail = (completed.stderr or completed.stdout or "").strip()
        raise GhError(f"required GitHub PR read failed: {detail or completed.returncode}")
    try:
        raw = json.loads(completed.stdout)
    except (TypeError, json.JSONDecodeError) as error:
        raise GhError("required GitHub PR read did not return JSON") from error
    if not isinstance(raw, list) or any(not isinstance(item, dict) for item in raw):
        raise GhError("required GitHub PR read did not return a PR list")
    return raw


def _check_failures(rollup: Any) -> List[str]:
    failures = []
    if not isinstance(rollup, list):
        return failures
    for check in rollup:
        if not isinstance(check, dict):
            continue
        result = (check.get("conclusion") or check.get("state") or "").upper()
        status = (check.get("status") or "").upper()
        if result in {"FAILURE", "ERROR", "TIMED_OUT", "CANCELLED", "ACTION_REQUIRED", "PENDING"} or status in {"IN_PROGRESS", "QUEUED", "PENDING"}:
            name = check.get("name") or check.get("context") or "check"
            failures.append(f"{name}:{result or status}")
    return failures


def parse_pull(raw: dict) -> PullRequest:
    files = raw.get("files")
    if not isinstance(files, list):
        raise GhError("PR file scope is missing; do not infer a safe candidate")
    if not isinstance(raw.get("isDraft"), bool) or isinstance(raw.get("number"), bool) or not isinstance(raw.get("number"), int) or raw["number"] < 1:
        raise GhError("PR identity or draft state is missing")
    paths = [item.get("path") if isinstance(item, dict) else item for item in files]
    if not paths or any(not isinstance(path, str) or not path for path in paths):
        raise GhError("PR file scope is empty or invalid")
    return PullRequest(
        number=raw["number"], title=str(raw.get("title") or ""), url=str(raw.get("url") or ""),
        head_ref=str(raw.get("headRefName") or ""), base_ref=str(raw.get("baseRefName") or ""),
        head_oid=str(raw.get("headRefOid") or ""), is_draft=raw["isDraft"],
        mergeable=str(raw.get("mergeable") or "UNKNOWN"),
        merge_state_status=str(raw.get("mergeStateStatus") or "UNKNOWN"),
        review_decision=str(raw.get("reviewDecision") or ""), auto_merge=raw.get("autoMergeRequest"),
        files=paths, check_failures=_check_failures(raw.get("statusCheckRollup")),
    )


def oid_matches(head_oid: str, proven_heads: Set[str]) -> bool:
    """Legacy selector name; only exact full SHA equality selects a candidate."""
    return bool(re.fullmatch(r"[0-9a-fA-F]{40}", head_oid)) and head_oid.lower() in {h.lower() for h in proven_heads}


def classify(pr: PullRequest, *, proven: bool) -> Action:
    """Report observations, never a verified merge decision. 'proven' is a selector."""
    action = Action(pr.number, pr.title, pr.url, "blocked", "", head_oid=pr.head_oid, base_ref=pr.base_ref)
    if not proven:
        action.decision, action.reason = "skip", "not selected by --pr or exact --head"
    elif not re.fullmatch(r"[0-9a-fA-F]{40}", pr.head_oid):
        action.reason = "full candidate head is unknown"
    elif pr.is_draft:
        action.reason = "draft"
    elif pr.base_ref in FORBIDDEN_BASES:
        action.reason = "shared integration/publication base; use its existing assigned owner"
    elif not pr.head_ref.startswith("cursor/") or (pr.base_ref != TRUNK and not pr.base_ref.startswith("cursor/")):
        action.decision, action.reason = "skip", "outside legacy cursor-to-main planning scope; use the current queue"
    elif any(path.startswith(FIRMWARE_PREFIXES) for path in pr.files):
        action.reason = "firmware/ change needs exact hardware and production authority before main"
    elif pr.mergeable != "MERGEABLE":
        action.reason = f"mergeability is {pr.mergeable or 'UNKNOWN'}"
    elif pr.merge_state_status != "CLEAN":
        action.reason = f"merge state is {pr.merge_state_status or 'UNKNOWN'}"
    elif pr.review_decision in {"CHANGES_REQUESTED", "REVIEW_REQUIRED"}:
        action.reason = f"review state is {pr.review_decision}"
    elif pr.check_failures:
        action.reason = "failed/pending checks need current required-check assessment: " + ", ".join(pr.check_failures)
    elif pr.base_ref != TRUNK:
        action.reason = "stack dependency needs existing integrator reconciliation; never retarget automatically"
    else:
        action.decision = "plan"
        action.reason = "selected candidate for existing integrator review; local proof, fresh base, required checks and authority remain unverified"
    if pr.auto_merge:
        action.reason += "; existing auto-merge setting observed and left unchanged"
    return action


def execute(repo: str, *, dry_run: bool, proven_prs: Set[int], proven_heads: Set[str], runner: Callable[..., subprocess.CompletedProcess] = run_gh) -> List[Action]:
    """Compatibility entrypoint: dry_run=False is also read-only, never a writer."""
    if not proven_prs and not proven_heads:
        raise GhError("pass --pr N and/or --head full-SHA to select a bounded read-only plan")
    if any(not re.fullmatch(r"[0-9a-fA-F]{40}", head) for head in proven_heads):
        raise GhError("--head requires an exact full 40-character commit SHA; prefixes are not proof")
    if any(isinstance(number, bool) or not isinstance(number, int) or number < 1 for number in proven_prs):
        raise GhError("--pr requires positive PR numbers")
    raw = load_open_pulls(repo, runner=runner)
    results, seen_numbers, seen_heads = [], set(), set()
    for item in raw:
        selected = item.get("number") in proven_prs or oid_matches(str(item.get("headRefOid") or ""), proven_heads)
        if not selected:
            continue
        number = item.get("number") if isinstance(item.get("number"), int) else 0
        try:
            pr = parse_pull(item)
            results.append(classify(pr, proven=True))
            seen_numbers.add(pr.number)
            seen_heads.add(pr.head_oid.lower())
        except GhError as error:
            results.append(Action(number, str(item.get("title") or ""), str(item.get("url") or ""), "blocked", str(error)))
            seen_numbers.add(number)
            seen_heads.add(str(item.get("headRefOid") or "").lower())
    for number in sorted(proven_prs - seen_numbers):
        results.append(Action(number, "", "", "unresolved", "selected PR absent from this open-PR read; no completion or merge inferred"))
    for head in sorted({h.lower() for h in proven_heads} - seen_heads):
        results.append(Action(0, "", "", "unresolved", "selected head absent from this open-PR read; no completion or merge inferred", head_oid=head))
    if len(raw) >= LIST_LIMIT:
        for action in results:
            action.reason += f"; inventory capped at {LIST_LIMIT}, absence outside the observed batch remains unknown"
    return sorted(results, key=lambda action: (action.number, action.head_oid))


def format_text(actions: Sequence[Action]) -> str:
    lines = [ONETIME_SETUP.rstrip(), ""]
    counts: dict[str, int] = defaultdict(int)
    for action in actions:
        counts[action.decision] += 1
        lines.append(f"#{action.number} {action.decision}: {action.reason}\n  head={action.head_oid or 'UNKNOWN'} base={action.base_ref or 'UNKNOWN'}\n  {action.title}\n  {action.url}".rstrip())
    lines.append("Read-only observations: " + ", ".join(f"{key}={value}" for key, value in sorted(counts.items())))
    return "\n".join(lines) + "\n"


def _parse_pr_list(values: Sequence[str]) -> Set[int]:
    return {int(part.strip()) for value in values for part in value.split(",") if part.strip()}


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", default=DEFAULT_REPO, help="OWNER/REPO (default: %(default)s)")
    parser.add_argument("--dry-run", action="store_true", help="Compatibility flag: every invocation is read-only")
    parser.add_argument("--json", action="store_true", help="Print read-only observations as JSON")
    parser.add_argument("--pr", action="append", default=[], metavar="N", help="Selected PR number, repeatable or comma-separated; not proof")
    parser.add_argument("--head", action="append", default=[], metavar="SHA", help="Selected exact full head SHA, repeatable; not proof")
    parser.add_argument("--print-onetime-setup", action="store_true", help="Explain the retired writer and current owner route")
    args = parser.parse_args(argv)
    if args.print_onetime_setup:
        sys.stdout.write(ONETIME_SETUP)
        return 0
    try:
        actions = execute(args.repo, dry_run=args.dry_run, proven_prs=_parse_pr_list(args.pr), proven_heads={h.strip() for h in args.head if h.strip()})
    except (GhError, OSError, ValueError) as error:
        print(f"merge_project_prs: {error}", file=sys.stderr)
        return 1
    if args.json:
        print(json.dumps([asdict(action) for action in actions], indent=2))
    else:
        sys.stdout.write(format_text(actions))
    # 0 means the read-only report succeeded, never that a PR merged. Blocked
    # independent items are all reported before returning a non-success status.
    return 2 if any(action.decision in {"blocked", "unresolved"} for action in actions) else 0


if __name__ == "__main__":
    sys.exit(main())
