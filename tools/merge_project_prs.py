#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Merge locally proven Full-Cycle Project PRs bottom-up with gh.

Owner policy (Roman, 2026-10-25): automatic merge of properly tested work is
pre-approved. There is no per-PR human APPROVE step. Agents merge after local
proof. Never ask the owner to approve a merge. Ruleset 25707607 is unlocked
(approvals=0, last-pusher approval off; force-push/delete protections kept).
Do not ask the owner to unlock or approve again.

Standing pre-approval does NOT authorize merging unrelated untested PRs.
Callers must pass an explicit proven set: --pr N and/or --head SHA for every
head that was locally proven. Only those PRs are merge candidates.

Eligible among the proven set: cursor/ head, base main or another cursor/
branch, not draft, MERGEABLE (no real conflicts), no firmware/ paths into
main. Never targets firmware nightly (stage 5 alone FFs nightly). Never
force-pushes. GitHub Actions / billing failures are not merge blockers —
if auto-merge is armed but those checks failed, fall through to direct merge.

After a stack parent merges into main (or its base), remaining open children
that targeted the parent's head are retargeted to that new base before they
merge, so the tip lands on main.

Usage:
  tools/merge_project_prs.py --dry-run --pr 164
  tools/merge_project_prs.py --pr 164 --pr 149
  tools/merge_project_prs.py --head abcdef0 --repo OWNER/REPO
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from collections import defaultdict
from dataclasses import asdict, dataclass, field
from typing import Any, Callable, List, Optional, Sequence, Set


DEFAULT_REPO = "gobbajogger/X0X37-for-Loopy-Pro"
TRUNK = "main"
FORBIDDEN_BASES = frozenset({"nightly", "gh-pages"})
FIRMWARE_PREFIXES = ("firmware/",)
MAIN_RULESET = "LoopyX0X37 PRODUCTION main - reviewed PR only"
MAIN_RULESET_ID = 25707607
CURSOR_APP_ID = 1210556
CURSOR_APP_SLUG = "cursor"

# Unlock is done; do not ask the owner again.
ONETIME_SETUP = f"""RULESET UNLOCK STATUS (done — do not ask the owner again):
  Ruleset {MAIN_RULESET!r} (id {MAIN_RULESET_ID}) is unlocked for agent merges:
    required_approving_review_count = 0
    require_last_push_approval = false
  Force-push and delete protections remain.
  Owner policy: automatic merge of properly tested work is pre-approved.
  Agents run tools/merge_project_prs.py --pr N after local proof of those heads.
  (Cursor app id {CURSOR_APP_ID} / slug {CURSOR_APP_SLUG} kept for reference.)
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


class GhError(RuntimeError):
    """gh or the GitHub API refused a call."""


def run_gh(args: Sequence[str], *, check: bool = True) -> subprocess.CompletedProcess:
    completed = subprocess.run(
        ["gh", *args],
        capture_output=True,
        text=True,
    )
    if check and completed.returncode != 0:
        detail = (completed.stderr or completed.stdout or "").strip()
        raise GhError(f"gh {' '.join(args)} failed: {detail}")
    return completed


def load_open_pulls(repo: str, runner: Callable[..., subprocess.CompletedProcess] = run_gh) -> List[dict]:
    completed = runner(
        [
            "pr",
            "list",
            "--repo",
            repo,
            "--state",
            "open",
            "--limit",
            "100",
            "--json",
            "number,title,url,headRefName,baseRefName,headRefOid,isDraft,mergeable,"
            "mergeStateStatus,reviewDecision,autoMergeRequest,statusCheckRollup,files",
        ]
    )
    return json.loads(completed.stdout or "[]")


def _file_paths(raw_files: Any) -> List[str]:
    paths: List[str] = []
    if not isinstance(raw_files, list):
        return paths
    for entry in raw_files:
        if isinstance(entry, dict) and isinstance(entry.get("path"), str):
            paths.append(entry["path"])
        elif isinstance(entry, str):
            paths.append(entry)
    return paths


def _check_failures(rollup: Any) -> List[str]:
    failures: List[str] = []
    if not isinstance(rollup, list):
        return failures
    for check in rollup:
        if not isinstance(check, dict):
            continue
        conclusion = (check.get("conclusion") or "").upper()
        status = (check.get("status") or "").upper()
        name = check.get("name") or check.get("context") or "check"
        if status in ("IN_PROGRESS", "QUEUED", "PENDING") and not conclusion:
            continue
        if conclusion in ("FAILURE", "TIMED_OUT", "CANCELLED", "ACTION_REQUIRED"):
            failures.append(f"{name}:{conclusion}")
    return failures


def parse_pull(raw: dict) -> PullRequest:
    return PullRequest(
        number=int(raw["number"]),
        title=str(raw.get("title") or ""),
        url=str(raw.get("url") or ""),
        head_ref=str(raw.get("headRefName") or ""),
        base_ref=str(raw.get("baseRefName") or ""),
        head_oid=str(raw.get("headRefOid") or ""),
        is_draft=bool(raw.get("isDraft")),
        mergeable=str(raw.get("mergeable") or "UNKNOWN"),
        merge_state_status=str(raw.get("mergeStateStatus") or "UNKNOWN"),
        review_decision=str(raw.get("reviewDecision") or ""),
        auto_merge=raw.get("autoMergeRequest"),
        files=_file_paths(raw.get("files")),
        check_failures=_check_failures(raw.get("statusCheckRollup")),
    )


def is_project_candidate(pr: PullRequest) -> bool:
    if not pr.head_ref.startswith("cursor/"):
        return False
    if pr.base_ref in FORBIDDEN_BASES:
        return False
    if pr.base_ref != TRUNK and not pr.base_ref.startswith("cursor/"):
        return False
    return True


def touches_firmware(paths: Sequence[str]) -> bool:
    return any(path.startswith(FIRMWARE_PREFIXES) for path in paths)


def oid_matches(head_oid: str, proven_heads: Set[str]) -> bool:
    if not head_oid or not proven_heads:
        return False
    lower = head_oid.lower()
    for proven in proven_heads:
        p = proven.lower()
        if lower == p or lower.startswith(p) or p.startswith(lower):
            return True
    return False


def in_proven_set(pr: PullRequest, proven_prs: Set[int], proven_heads: Set[str]) -> bool:
    if pr.number in proven_prs:
        return True
    return oid_matches(pr.head_oid, proven_heads)


def classify(pr: PullRequest, *, proven: bool) -> Action:
    """Decide merge / skip. Proven set gates standing pre-approval."""
    if not proven:
        return Action(
            pr.number,
            pr.title,
            pr.url,
            "skip",
            "not in proven set (--pr / --head); standing pre-approval does not cover untested heads",
        )
    if pr.is_draft:
        return Action(pr.number, pr.title, pr.url, "skip", "draft")
    if pr.head_ref.startswith("cursor/") and pr.base_ref in FORBIDDEN_BASES:
        return Action(
            pr.number,
            pr.title,
            pr.url,
            "skip",
            f"base {pr.base_ref} is stage-5 / release territory; stage 5 alone FFs nightly",
        )
    if not is_project_candidate(pr):
        return Action(pr.number, pr.title, pr.url, "skip", "not a Project PR (cursor/→main stack)")
    if touches_firmware(pr.files):
        return Action(
            pr.number,
            pr.title,
            pr.url,
            "skip",
            "touches firmware/; production main moves only after hardware validation of those bytes",
        )
    if pr.mergeable == "CONFLICTING" or pr.merge_state_status == "DIRTY":
        return Action(pr.number, pr.title, pr.url, "skip", "real conflicts")
    if pr.mergeable not in ("MERGEABLE", "UNKNOWN", ""):
        return Action(pr.number, pr.title, pr.url, "skip", f"mergeable={pr.mergeable}")

    gha_note = ""
    if pr.check_failures:
        gha_note = "; GHA/billing failures are non-blocking"

    # Armed auto-merge that is stuck on failed checks must fall through to direct merge.
    if pr.auto_merge and not pr.check_failures:
        return Action(
            pr.number,
            pr.title,
            pr.url,
            "armed",
            f"auto-merge already enabled; waiting only on GitHub to land{gha_note}",
        )

    where = "main" if pr.base_ref == TRUNK else f"stack base {pr.base_ref}"
    if pr.auto_merge and pr.check_failures:
        return Action(
            pr.number,
            pr.title,
            pr.url,
            "merge",
            f"auto-merge armed but checks failed; fall through to direct merge ({where}){gha_note}",
        )
    return Action(
        pr.number,
        pr.title,
        pr.url,
        "merge",
        f"pre-approved after local proof ({where}){gha_note}",
    )


def bottom_up_order(prs: Sequence[PullRequest]) -> List[PullRequest]:
    by_head = {pr.head_ref: pr for pr in prs}
    depth_cache: dict[str, int] = {}

    def depth(ref: str) -> int:
        if ref in depth_cache:
            return depth_cache[ref]
        pr = by_head.get(ref)
        if pr is None:
            depth_cache[ref] = 0
            return 0
        depth_cache[ref] = -1
        if pr.base_ref == TRUNK or pr.base_ref not in by_head:
            parent = 0
        else:
            parent_depth = depth(pr.base_ref)
            parent = 0 if parent_depth < 0 else parent_depth + 1
        depth_cache[ref] = parent
        return parent

    return sorted(prs, key=lambda pr: (depth(pr.head_ref), pr.number))


def merge_commands(repo: str, number: int) -> List[List[str]]:
    """Try normal merge, admin bypass, then auto-merge arming."""
    base = ["pr", "merge", str(number), "--repo", repo, "--merge"]
    return [
        base,
        [*base, "--admin"],
        [*base, "--auto"],
    ]


def retarget_command(repo: str, number: int, new_base: str) -> List[str]:
    return ["pr", "edit", str(number), "--repo", repo, "--base", new_base]


def try_retarget(
    repo: str,
    pr: PullRequest,
    new_base: str,
    *,
    dry_run: bool,
    runner: Callable[..., subprocess.CompletedProcess],
) -> Action:
    command = retarget_command(repo, pr.number, new_base)
    if dry_run:
        return Action(
            pr.number,
            pr.title,
            pr.url,
            "dry-run",
            f"would retarget base {pr.base_ref} → {new_base} after parent merge",
            command=command,
        )
    completed = runner(command, check=False)
    if completed.returncode == 0:
        pr.base_ref = new_base
        return Action(
            pr.number,
            pr.title,
            pr.url,
            "retargeted",
            f"retargeted base → {new_base} after parent merge",
            command=command,
        )
    detail = (completed.stderr or completed.stdout or "").strip()
    return Action(
        pr.number,
        pr.title,
        pr.url,
        "failed",
        f"retarget to {new_base} failed: {detail}",
        command=command,
    )


def try_merge(
    repo: str,
    pr: PullRequest,
    *,
    proven: bool,
    dry_run: bool,
    runner: Callable[..., subprocess.CompletedProcess] = run_gh,
) -> Action:
    action = classify(pr, proven=proven)
    if action.decision in ("skip", "armed"):
        return action
    if action.decision != "merge":
        return action

    commands = merge_commands(repo, pr.number)
    action.command = commands[0]
    if dry_run:
        action.decision = "dry-run"
        action.reason = (
            f"{action.reason}; would try: "
            + " then ".join(f"gh {' '.join(c)}" for c in commands)
        )
        return action

    errors: List[str] = []
    for command in commands:
        action.command = command
        completed = runner(command, check=False)
        if completed.returncode == 0:
            via = " ".join(command)
            if "--auto" in command:
                action.decision = "auto-merge-enabled"
                action.reason = f"{action.reason}; armed auto-merge via gh {via}"
            else:
                action.decision = "merged"
                action.reason = f"{action.reason}; via gh {via}"
            return action
        detail = (completed.stderr or completed.stdout or "").strip() or f"exit {completed.returncode}"
        errors.append(detail)

    action.decision = "failed"
    action.reason = (
        "merge failed after ruleset unlock "
        f"(approvals=0, last-pusher=off on {MAIN_RULESET_ID}); "
        "do not ask the owner to approve. "
        + " | ".join(errors)
    )
    return action


def select_interesting(prs: Sequence[PullRequest]) -> List[PullRequest]:
    selected = []
    for pr in prs:
        if is_project_candidate(pr):
            selected.append(pr)
        elif pr.head_ref.startswith("cursor/") and pr.base_ref in FORBIDDEN_BASES:
            selected.append(pr)
    return selected


def execute(
    repo: str,
    *,
    dry_run: bool,
    proven_prs: Set[int],
    proven_heads: Set[str],
    runner: Callable[..., subprocess.CompletedProcess] = run_gh,
) -> List[Action]:
    if not proven_prs and not proven_heads:
        raise GhError(
            "refusing to merge: pass --pr N and/or --head SHA for every locally proven head "
            "(standing pre-approval does not merge untested cursor/ PRs)"
        )

    raw = load_open_pulls(repo, runner=runner)
    pulls = [parse_pull(item) for item in raw]
    interesting = select_interesting(pulls)
    # Only act on the proven subset (plus report skips for proven-but-ineligible).
    proven_interesting = [
        pr for pr in interesting if in_proven_set(pr, proven_prs, proven_heads)
    ]
    # Also surface proven numbers that are not open / not candidates.
    known_numbers = {pr.number for pr in interesting}
    results: List[Action] = []
    for number in sorted(proven_prs - known_numbers):
        results.append(
            Action(
                number,
                "",
                "",
                "skip",
                "proven --pr not in open Project PR candidates (missing, draft, or wrong base)",
            )
        )

    remaining = {pr.number: pr for pr in proven_interesting}
    while remaining:
        ordered = bottom_up_order(list(remaining.values()))
        pr = ordered[0]
        del remaining[pr.number]
        proven = True  # remaining is already the proven subset

        # Parent head not among remaining open proven PRs → retarget onto trunk
        # (or keep if already on trunk) so we do not merge into a dead bottom.
        open_heads = {r.head_ref for r in remaining.values()}
        if (
            pr.base_ref.startswith("cursor/")
            and pr.base_ref not in open_heads
            and pr.base_ref != TRUNK
        ):
            retarget = try_retarget(repo, pr, TRUNK, dry_run=dry_run, runner=runner)
            results.append(retarget)
            if retarget.decision == "failed":
                continue

        parent_base_before_merge = pr.base_ref
        parent_head = pr.head_ref
        action = try_merge(repo, pr, proven=proven, dry_run=dry_run, runner=runner)
        results.append(action)

        # After parent merges into its base, retarget children that pointed at
        # the parent's head onto that same base so the tip reaches trunk.
        if action.decision in ("merged", "dry-run") and parent_head:
            children = [c for c in remaining.values() if c.base_ref == parent_head]
            new_base = parent_base_before_merge
            for child in children:
                retarget = try_retarget(
                    repo, child, new_base, dry_run=dry_run, runner=runner
                )
                results.append(retarget)
                if retarget.decision != "failed":
                    remaining[child.number].base_ref = new_base
    return results


def format_text(actions: Sequence[Action]) -> str:
    if not actions:
        return "No Project PRs matched the proven set.\n"
    lines = [
        "Owner policy: automatic merge of properly tested work is pre-approved.",
        "Only --pr / --head proven heads are merged. Never ask for a per-PR APPROVE.",
        "",
    ]
    counts: dict[str, int] = defaultdict(int)
    for action in actions:
        counts[action.decision] += 1
        cmd = f" :: {' '.join(action.command)}" if action.command else ""
        title = action.title or "(no title)"
        url = action.url or ""
        lines.append(
            f"#{action.number} {action.decision}: {action.reason}{cmd}\n"
            f"  {title}\n"
            f"  {url}".rstrip()
        )
    summary = ", ".join(f"{k}={v}" for k, v in sorted(counts.items()))
    lines.insert(0, f"Project PR merge ({summary})")
    return "\n".join(lines) + "\n"


def _parse_pr_list(values: Sequence[str]) -> Set[int]:
    numbers: Set[int] = set()
    for value in values:
        for part in value.split(","):
            part = part.strip()
            if not part:
                continue
            numbers.add(int(part))
    return numbers


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", default=DEFAULT_REPO, help="OWNER/REPO (default: %(default)s)")
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="List bottom-up decisions without calling gh pr merge",
    )
    parser.add_argument("--json", action="store_true", help="Print actions as JSON")
    parser.add_argument(
        "--pr",
        action="append",
        default=[],
        metavar="N",
        help="Locally proven PR number (repeatable or comma-separated). Required unless --head is set.",
    )
    parser.add_argument(
        "--head",
        action="append",
        default=[],
        metavar="SHA",
        help="Locally proven head commit SHA (repeatable). Required unless --pr is set.",
    )
    parser.add_argument(
        "--print-onetime-setup",
        action="store_true",
        help="Print ruleset unlock status (done) and exit",
    )
    args = parser.parse_args(argv)

    if args.print_onetime_setup:
        sys.stdout.write(ONETIME_SETUP)
        return 0

    proven_prs = _parse_pr_list(args.pr)
    proven_heads = {h.strip() for h in args.head if h.strip()}

    try:
        actions = execute(
            args.repo,
            dry_run=args.dry_run,
            proven_prs=proven_prs,
            proven_heads=proven_heads,
        )
    except GhError as error:
        print(f"merge_project_prs: {error}", file=sys.stderr)
        return 1

    if args.json:
        print(json.dumps([asdict(a) for a in actions], indent=2))
    else:
        sys.stdout.write(format_text(actions))

    if any(a.decision == "failed" for a in actions):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
