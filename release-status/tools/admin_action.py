#!/usr/bin/env python3
"""Admin action CLI: append an authenticated audit event.

Default mode is dry-run. Dry-run writes nothing and makes no network write.
A GitHub commit is produced only when --commit is given, the actor is the
verified GitHub login gobbajogger, and the proposal passes the validator.

The actor is the GitHub login verified through the GitHub API (the
authenticated user for a proposed commit, or the commit author after a
commit). If that login is unavailable or is not gobbajogger, the result is
'actor not verified' and --commit is refused.

This tool does not promote a build automatically. Illegal promotions are
refused by the validator. It does not write to main, nightly, gh-pages,
or .github. It does not write to a device.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import urllib.error
import urllib.request
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Callable
from zoneinfo import ZoneInfo

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from validate_release_status import (  # noqa: E402
    ALLOWED_ACTOR,
    REQUIRED_ACTOR_IDENTITY,
    STATE_FLAGS,
    UsageError,
    apply_proposal,
    build_audit_event,
    check_proposal_audit_event,
    default_paths,
    event_line,
    find_record,
    github_commit_login,
    load_json,
    load_jsonl,
    load_records,
    next_prev_hash,
    raw_lines,
    record_id_of,
    states_equal,
    validate_audit_schema,
    validate_proposal,
    validate_record,
)

FORBIDDEN_BRANCHES = frozenset(
    {
        "main",
        "nightly",
        "gh-pages",
        "wip/one-build",
        "refs/heads/main",
        "refs/heads/nightly",
        "refs/heads/gh-pages",
        "refs/heads/wip/one-build",
    }
)
DEFAULT_ALLOWED_BRANCHES = frozenset(
    {
        "feature/rp-release-status-workflow-20261113",
        "lane/rp-release-status-workflow-20261113",
    }
)
DEFAULT_REPO = "gobbajogger/X0X37-for-Loopy-Pro"
DEFAULT_BRANCH = "feature/rp-release-status-workflow-20261113"
AUTHOR_NAME = "gobbajogger"
AUTHOR_EMAIL = "255244738+gobbajogger@users.noreply.github.com"
API_ROOT = "https://api.github.com"

LoginLookup = Callable[[], str | None]
CommitFn = Callable[[dict[str, Any]], dict[str, Any]]


def _token() -> str | None:
    return os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN") or None


def github_get_json(url: str, token: str | None, timeout: int = 20) -> dict[str, Any]:
    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": "dnbox-release-status-admin",
    }
    if token:
        headers["Authorization"] = f"Bearer {token}"
    req = urllib.request.Request(url, headers=headers, method="GET")
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return json.loads(resp.read().decode("utf-8"))


def github_request(
    method: str,
    url: str,
    token: str,
    body: dict[str, Any] | None = None,
    timeout: int = 30,
) -> dict[str, Any]:
    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": "dnbox-release-status-admin",
        "Authorization": f"Bearer {token}",
    }
    data = None
    if body is not None:
        data = json.dumps(body).encode("utf-8")
        headers["Content-Type"] = "application/json"
    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        raw = resp.read().decode("utf-8")
        return json.loads(raw) if raw else {}


def github_authenticated_login(token: str | None, lookup: LoginLookup | None = None) -> str | None:
    """Return the GitHub login of the authenticated user, or None."""
    if lookup is not None:
        return lookup()
    if not token:
        return None
    try:
        data = github_get_json(f"{API_ROOT}/user", token)
    except (OSError, urllib.error.URLError, TimeoutError, json.JSONDecodeError, ValueError):
        return None
    login = data.get("login")
    return str(login) if login else None


def verify_actor(
    *,
    token: str | None = None,
    login_lookup: LoginLookup | None = None,
    commit_sha: str | None = None,
    repo: str | None = None,
) -> dict[str, Any]:
    """Verify the actor through the GitHub API.

    For a proposed commit, the login is the authenticated user (GET /user).
    For an existing commit, the login is the commit author from the API.
    The JSONL actor field is not authentication.
    """
    login = None
    method = "unavailable"
    if commit_sha and repo and login_lookup is None:
        login = github_commit_login(repo, commit_sha)
        method = f"GET /repos/{repo}/commits/{commit_sha} author.login"
    else:
        login = github_authenticated_login(token, lookup=login_lookup)
        method = "GET /user login" if login_lookup is None else "injected login lookup"
    if login == ALLOWED_ACTOR:
        return {
            "verification": "verified",
            "actor": ALLOWED_ACTOR,
            "actor_identity": REQUIRED_ACTOR_IDENTITY,
            "login": login,
            "method": method,
            "message": f"actor verified: GitHub login {login} is the option C admin",
        }
    if not login:
        return {
            "verification": "actor not verified",
            "actor": "actor not verified",
            "actor_identity": "unverified",
            "login": None,
            "method": method,
            "message": "actor not verified: GitHub login unavailable",
        }
    return {
        "verification": "actor not verified",
        "actor": "actor not verified",
        "actor_identity": "unverified",
        "login": login,
        "method": method,
        "message": f"actor not verified: GitHub login {login!r} is not {ALLOWED_ACTOR!r}",
    }


def parse_set_state(items: list[str] | None) -> dict[str, bool]:
    out: dict[str, bool] = {}
    for item in items or []:
        if "=" not in item:
            raise UsageError(f"--set-state must be FLAG=true or FLAG=false, got {item!r}")
        key, raw = item.split("=", 1)
        key = key.strip()
        raw = raw.strip().lower()
        if key not in STATE_FLAGS:
            raise UsageError(f"unknown state flag {key!r}; known flags: {', '.join(STATE_FLAGS)}")
        if raw not in ("true", "false", "1", "0", "yes", "no"):
            raise UsageError(f"state value must be true or false, got {raw!r}")
        out[key] = raw in ("true", "1", "yes")
    return out


def default_time() -> str:
    now = datetime.now(ZoneInfo("America/Chicago"))
    return now.isoformat(timespec="seconds")


def record_filename(records: list[tuple[str, dict]], record_id: str, rec: dict) -> str:
    for name, item in records:
        if item is rec or item.get("catalog_id") == record_id or name == record_id:
            return name
    return record_id_of(rec) + ".json"


def pretty_json(obj: Any) -> str:
    return json.dumps(obj, indent=2, ensure_ascii=False) + "\n"


def build_action(
    *,
    records: list[tuple[str, dict]],
    record_schema: dict,
    audit_schema: dict,
    audit_path: Path,
    record_id: str,
    action: str,
    set_states: dict[str, bool],
    note: str | None,
    evidence_refs: list[str],
    evidence: dict[str, Any] | None,
    identity_confidence: dict[str, Any] | None,
    hardware_approvals: list[dict] | None,
    history: list[dict] | None,
    time: str,
    actor_info: dict[str, Any],
) -> dict[str, Any]:
    rec = find_record(records, record_id)
    if rec is None:
        raise UsageError(f"record {record_id!r} not found")
    rid = record_id_of(rec)
    filename = record_filename(records, record_id, rec)

    events = load_jsonl(audit_path) if audit_path.exists() else []
    lines = raw_lines(audit_path.read_text(encoding="utf-8")) if audit_path.exists() else []
    # from_states is the latest audit to_states for this record, else current states.
    from_states = dict(rec.get("states") or {})
    for ev in reversed(events):
        if ev.get("record_id") in (rid, rec.get("catalog_id")):
            from_states = dict(ev.get("to_states") or from_states)
            break

    proposal: dict[str, Any] = {"record_id": rid}
    if set_states:
        proposal["set_states"] = dict(set_states)
    if identity_confidence:
        proposal["identity_confidence"] = identity_confidence
    if evidence:
        proposal["evidence"] = evidence
    if hardware_approvals is not None:
        proposal["hardware_approvals"] = hardware_approvals
    if history is not None:
        proposal["history"] = history

    after = apply_proposal(rec, proposal)
    to_states = dict(after.get("states") or {})
    prev = next_prev_hash(lines[-1] if lines else None)

    verified = actor_info.get("verification") == "verified"
    # Only a verified gobbajogger login may appear as the audit actor.
    # An unverified run does not mint a valid-looking forged event.
    if verified:
        event = build_audit_event(
            time=time,
            actor=ALLOWED_ACTOR,
            action=action,
            record_id=rid,
            from_states=from_states,
            to_states=to_states,
            evidence_refs=list(evidence_refs),
            commit_sha=None,
            prev_sha256=prev,
            note=note,
            actor_identity=REQUIRED_ACTOR_IDENTITY,
            appended_at=datetime.now(timezone.utc).isoformat(timespec="seconds"),
        )
    else:
        event = {
            "time": time,
            "actor": "actor not verified",
            "actor_identity": "unverified",
            "action": action,
            "record_id": rid,
            "from_states": from_states,
            "to_states": to_states,
            "evidence_refs": list(evidence_refs),
            "commit_sha": None,
            "prev_sha256": prev,
        }
        if note is not None:
            event["note"] = note

    proposal["audit_event"] = event

    issues = []
    if verified:
        issues.extend(validate_proposal(rec, proposal, record_schema))
        issues.extend(check_proposal_audit_event(proposal, rec, after, lines, events))
        issues.extend(validate_audit_schema([event], audit_schema))
        issues.extend(validate_record(after, record_schema, filename=filename))
    else:
        issues_text = [
            "actor not verified: the proposal is not accepted and --commit is refused"
        ]
        # Still run the evidence validator so the operator sees gate failures.
        for issue in validate_proposal(rec, proposal, record_schema):
            issues.append(issue)
        issues_text.extend(str(i) for i in issues)
        return {
            "ok": False,
            "actor": actor_info,
            "record_id": rid,
            "filename": filename,
            "action": action,
            "from_states": from_states,
            "to_states": to_states,
            "proposal": proposal,
            "event": event,
            "event_line": None,
            "after_record": after,
            "issues": [str(i) for i in issues] if issues else [],
            "actor_issues": issues_text,
            "accepted": False,
            "commit_allowed": False,
        }

    accepted = not issues
    return {
        "ok": accepted,
        "actor": actor_info,
        "record_id": rid,
        "filename": filename,
        "action": action,
        "from_states": from_states,
        "to_states": to_states,
        "proposal": proposal,
        "event": event,
        "event_line": event_line(event) if accepted else None,
        "after_record": after,
        "issues": [str(i) for i in issues],
        "actor_issues": [],
        "accepted": accepted,
        "commit_allowed": accepted,
    }


def format_report(result: dict[str, Any], *, mode: str, commit_result: dict | None = None) -> str:
    lines: list[str] = []
    lines.append(f"DnBOX admin action ({mode})")
    actor = result.get("actor") or {}
    lines.append(f"Actor verification: {actor.get('verification')}")
    lines.append(f"  {actor.get('message')}")
    if actor.get("login"):
        lines.append(f"  GitHub login: {actor.get('login')}")
    lines.append(f"  Method: {actor.get('method')}")
    lines.append(f"Record: {result.get('record_id')}")
    lines.append(f"Action: {result.get('action')}")
    lines.append("From states:")
    lines.append(json.dumps(result.get("from_states"), indent=2, sort_keys=True))
    lines.append("To states:")
    lines.append(json.dumps(result.get("to_states"), indent=2, sort_keys=True))
    same = states_equal(result.get("from_states") or {}, result.get("to_states") or {})
    if same:
        lines.append("State change: none (documentation event only)")
    else:
        changed = []
        to_s = result.get("to_states") or {}
        from_s = result.get("from_states") or {}
        for k in sorted(set(from_s) | set(to_s)):
            if from_s.get(k) != to_s.get(k):
                changed.append(f"  {k}: {from_s.get(k)} -> {to_s.get(k)}")
        lines.append("State change:")
        lines.extend(changed)
    lines.append("Proposed audit event:")
    lines.append(json.dumps(result.get("event"), indent=2, sort_keys=True))
    if result.get("accepted"):
        lines.append("Validator: ACCEPT")
    else:
        lines.append("Validator: REJECT")
        for issue in (result.get("issues") or []) + (result.get("actor_issues") or []):
            lines.append(f"  {issue}")
    if mode == "dry-run":
        lines.append("Write: none (dry-run default). No file and no GitHub commit were written.")
        if result.get("commit_allowed"):
            lines.append("A later --commit would be allowed for this proposal, if the actor is still verified.")
        else:
            lines.append("--commit would be refused for this proposal.")
    elif commit_result:
        if commit_result.get("ok"):
            lines.append(f"Commit: {commit_result.get('sha')}")
            lines.append(f"Branch: {commit_result.get('branch')}")
            lines.append(f"Repo: {commit_result.get('repo')}")
        else:
            lines.append(f"Commit refused: {commit_result.get('error')}")
    lines.append("This tool does not promote a build automatically.")
    return "\n".join(lines) + "\n"


def assert_branch_allowed(branch: str, allowed: set[str] | frozenset[str]) -> None:
    name = branch.replace("refs/heads/", "")
    if name in FORBIDDEN_BRANCHES or branch in FORBIDDEN_BRANCHES:
        raise UsageError(
            f"refusing to commit on forbidden branch {branch!r}. "
            "This tool does not write to main, nightly, gh-pages, or wip/one-build."
        )
    if name not in allowed and branch not in allowed:
        raise UsageError(
            f"refusing to commit on branch {branch!r}. "
            f"Allowed branches: {', '.join(sorted(allowed))}."
        )


def commit_files_via_api(
    *,
    repo: str,
    branch: str,
    files: dict[str, str],
    message: str,
    token: str,
    expected_head: str | None = None,
) -> dict[str, Any]:
    """Create a fast-forward commit with the GitHub git data API (tree+blob).

    Does not force-push. Refuses if the branch moved.
    """
    assert_branch_allowed(branch, DEFAULT_ALLOWED_BRANCHES)
    if not token:
        raise UsageError("GH_TOKEN is required for --commit")
    ref = f"heads/{branch.replace('refs/heads/', '')}"
    ref_url = f"{API_ROOT}/repos/{repo}/git/ref/{ref}"
    ref_data = github_request("GET", ref_url, token)
    head = (ref_data.get("object") or {}).get("sha")
    if not head:
        raise UsageError(f"could not read branch tip for {branch}")
    if expected_head and head != expected_head:
        raise UsageError(f"branch {branch} moved (expected {expected_head}, got {head}); refusing to commit")
    commit = github_request("GET", f"{API_ROOT}/repos/{repo}/git/commits/{head}", token)
    base_tree = (commit.get("tree") or {}).get("sha")
    if not base_tree:
        raise UsageError("could not read the base tree")
    tree_items = []
    for path, content in files.items():
        if not path.startswith("release-status/"):
            raise UsageError(f"refusing to commit path outside release-status/: {path}")
        if path.startswith("release-status/") and "/../" in path:
            raise UsageError(f"refusing path {path}")
        blob = github_request(
            "POST",
            f"{API_ROOT}/repos/{repo}/git/blobs",
            token,
            {"content": content, "encoding": "utf-8"},
        )
        sha = blob.get("sha")
        if not sha:
            raise UsageError(f"blob create failed for {path}")
        tree_items.append({"path": path, "mode": "100644", "type": "blob", "sha": sha})
    tree = github_request(
        "POST",
        f"{API_ROOT}/repos/{repo}/git/trees",
        token,
        {"base_tree": base_tree, "tree": tree_items},
    )
    tree_sha = tree.get("sha")
    if not tree_sha:
        raise UsageError("tree create failed")
    # Chicago-offset author date.
    now = datetime.now(ZoneInfo("America/Chicago")).isoformat(timespec="seconds")
    new_commit = github_request(
        "POST",
        f"{API_ROOT}/repos/{repo}/git/commits",
        token,
        {
            "message": message,
            "tree": tree_sha,
            "parents": [head],
            "author": {"name": AUTHOR_NAME, "email": AUTHOR_EMAIL, "date": now},
            "committer": {"name": AUTHOR_NAME, "email": AUTHOR_EMAIL, "date": now},
        },
    )
    sha = new_commit.get("sha")
    if not sha:
        raise UsageError("commit create failed")
    # Fast-forward only. force is false.
    # Re-read tip to reduce a race, then PATCH without force.
    tip = github_request("GET", ref_url, token)
    tip_sha = (tip.get("object") or {}).get("sha")
    if tip_sha != head:
        raise UsageError(f"branch {branch} moved during commit ({tip_sha}); refusing to update the ref")
    updated = github_request(
        "PATCH",
        ref_url,
        token,
        {"sha": sha, "force": False},
    )
    new_sha = (updated.get("object") or {}).get("sha")
    if new_sha != sha:
        raise UsageError(f"ref update did not land on the new commit (got {new_sha})")
    return {"ok": True, "sha": sha, "branch": branch, "repo": repo, "parent": head}


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    paths = default_paths()
    p = argparse.ArgumentParser(
        description="Append an authenticated audit event. Dry-run is the default. Never promotes automatically."
    )
    p.add_argument("--records", type=Path, default=paths["records"])
    p.add_argument("--audit", type=Path, default=paths["audit"])
    p.add_argument("--schema", type=Path, default=paths["schema"])
    p.add_argument(
        "--audit-schema",
        type=Path,
        default=paths["schema"].parent / "audit-event.schema.json",
    )
    p.add_argument("--record-id", required=True, help="catalog_id or record filename")
    p.add_argument("--action", default="record-update", help="audit action label")
    p.add_argument(
        "--set-state",
        action="append",
        default=[],
        help="FLAG=true or FLAG=false. Repeatable.",
    )
    p.add_argument("--note", default=None)
    p.add_argument("--evidence-ref", action="append", default=[], help="Repeatable evidence URL or ref")
    p.add_argument("--evidence-json", type=Path, default=None, help="JSON object of proposal evidence")
    p.add_argument("--identity-json", type=Path, default=None)
    p.add_argument("--hardware-approvals-json", type=Path, default=None)
    p.add_argument("--history-json", type=Path, default=None)
    p.add_argument("--time", default=None, help="ISO-8601 event time. Default is now in America/Chicago.")
    p.add_argument(
        "--commit",
        action="store_true",
        help="Produce a GitHub commit on the allowed feature branch. Refused unless the actor is verified.",
    )
    p.add_argument("--repo", default=DEFAULT_REPO)
    p.add_argument("--branch", default=DEFAULT_BRANCH)
    p.add_argument(
        "--json",
        action="store_true",
        help="print a JSON report as well as the text (JSON goes to stdout, text to stderr when both)",
    )
    p.add_argument(
        "--allow-network-read",
        action="store_true",
        default=True,
        help=argparse.SUPPRESS,
    )
    return p.parse_args(argv)


def main(
    argv: list[str] | None = None,
    *,
    login_lookup: LoginLookup | None = None,
    commit_fn: CommitFn | None = None,
    token: str | None = None,
) -> int:
    try:
        args = parse_args(argv)
        if not args.records.is_dir():
            raise UsageError(f"records directory not found: {args.records}")
        if not args.audit.is_file():
            raise UsageError(f"audit file not found: {args.audit}")
        record_schema = load_json(args.schema)
        audit_schema = load_json(args.audit_schema)
        records = load_records(args.records)
        set_states = parse_set_state(args.set_state)
        evidence = load_json(args.evidence_json) if args.evidence_json else None
        identity = load_json(args.identity_json) if args.identity_json else None
        approvals = load_json(args.hardware_approvals_json) if args.hardware_approvals_json else None
        history = load_json(args.history_json) if args.history_json else None
        if evidence is not None and not isinstance(evidence, dict):
            raise UsageError("--evidence-json must be a JSON object")
        time = args.time or default_time()
        tok = token if token is not None else _token()
        actor_info = verify_actor(token=tok, login_lookup=login_lookup)
        result = build_action(
            records=records,
            record_schema=record_schema,
            audit_schema=audit_schema,
            audit_path=args.audit,
            record_id=args.record_id,
            action=args.action,
            set_states=set_states,
            note=args.note,
            evidence_refs=list(args.evidence_ref),
            evidence=evidence,
            identity_confidence=identity,
            hardware_approvals=approvals,
            history=history,
            time=time,
            actor_info=actor_info,
        )

        commit_result = None
        mode = "dry-run"
        if args.commit:
            mode = "commit"
            if not result.get("commit_allowed"):
                commit_result = {
                    "ok": False,
                    "error": "commit refused: actor is not verified or the proposal is not accepted",
                }
                text = format_report(result, mode=mode, commit_result=commit_result)
                sys.stdout.write(text)
                return 1
            try:
                assert_branch_allowed(args.branch, DEFAULT_ALLOWED_BRANCHES)
            except UsageError as exc:
                commit_result = {"ok": False, "error": str(exc)}
                text = format_report(result, mode=mode, commit_result=commit_result)
                sys.stdout.write(text)
                return 1
            # Prepare file contents: updated record + appended audit line.
            after = result["after_record"]
            filename = result["filename"]
            record_path = f"release-status/records/{filename}"
            audit_path = "release-status/audit/audit.jsonl"
            existing = args.audit.read_text(encoding="utf-8")
            if existing and not existing.endswith("\n"):
                existing += "\n"
            new_audit = existing + result["event_line"] + "\n"
            files = {
                record_path: pretty_json(after),
                audit_path: new_audit,
            }
            message = (
                f"Add audit event {result['action']} for {result['record_id']}.\n\n"
                "The actor is the verified GitHub login gobbajogger. "
                "This commit does not promote a build automatically.\n"
            )
            try:
                if commit_fn is not None:
                    commit_result = commit_fn(
                        {
                            "repo": args.repo,
                            "branch": args.branch,
                            "files": files,
                            "message": message,
                            "token": tok,
                        }
                    )
                else:
                    # Live GitHub write. Only reached when --commit is explicit
                    # and the actor is verified.
                    commit_result = commit_files_via_api(
                        repo=args.repo,
                        branch=args.branch,
                        files=files,
                        message=message,
                        token=tok or "",
                    )
            except UsageError as exc:
                commit_result = {"ok": False, "error": str(exc)}
                text = format_report(result, mode=mode, commit_result=commit_result)
                sys.stdout.write(text)
                return 1
            if not commit_result or not commit_result.get("ok"):
                text = format_report(result, mode=mode, commit_result=commit_result)
                sys.stdout.write(text)
                return 1

        text = format_report(result, mode=mode, commit_result=commit_result)
        sys.stdout.write(text)
        if args.json:
            payload = {"result": result, "mode": mode, "commit": commit_result}
            # Do not include tokens.
            sys.stdout.write(json.dumps(payload, indent=2, default=str) + "\n")
        if not result.get("accepted"):
            return 1
        if args.commit and (not commit_result or not commit_result.get("ok")):
            return 1
        return 0
    except UsageError as exc:
        sys.stderr.write(f"USAGE: {exc}\n")
        return 2
    except urllib.error.HTTPError as exc:
        sys.stderr.write(f"ERROR: GitHub API HTTP {exc.code}: {exc.reason}\n")
        return 2
    except urllib.error.URLError as exc:
        sys.stderr.write(f"ERROR: network {exc.reason}\n")
        return 2


if __name__ == "__main__":
    sys.exit(main())
