#!/usr/bin/env python3
"""Offline decision aid for the existing request coordinator; never calls GitHub."""
import argparse
import hashlib
import json
from pathlib import Path
import re

REPOSITORY_ID = 1426904592
REPOSITORY = "gobbajogger/X0X37-for-Loopy-Pro"
OWNER_ID = 255244738
OWNER_NODE = "U_kgDODza5wg"
GATES = ("musician_problem", "benefit", "expected_behavior", "existing_feature_reuse",
         "control_conflicts", "data_transport_model_impact", "acceptance_tests")


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, ensure_ascii=False,
                                     separators=(",", ":")).encode()).hexdigest()


def scope_hash(issue, spec):
    """Exact strings: whitespace, title and spec edits invalidate this scope."""
    return digest({"title": issue["title"], "body": issue["body"], "spec": spec})


def owner(actor):
    # Login is display text, never identity. Reject inconsistent supplied node IDs.
    return (actor.get("id") == OWNER_ID and actor.get("type") == "User"
            and actor.get("node_id", OWNER_NODE) == OWNER_NODE)


def pages(envelope, endpoint):
    """Normalized API pages, including a terminal empty page if Link is unavailable."""
    if envelope.get("read_status") != "success" or envelope.get("endpoint") != endpoint:
        raise ValueError("failed/missing read: " + endpoint)
    result = []
    expected = 1
    seen = set()
    for page in envelope["pages"]:
        if page["page"] != expected or page.get("status") != 200:
            raise ValueError("incomplete pagination: " + endpoint)
        if not isinstance(page["items"], list):
            raise ValueError("invalid items: " + endpoint)
        result.extend(page["items"])
        nxt = page["next_page"]
        if expected in seen:
            raise ValueError("pagination loop")
        seen.add(expected)
        if nxt is None:
            if page is not envelope["pages"][-1]:
                raise ValueError("pages after terminal page")
            return result
        if nxt != expected + 1:
            raise ValueError("pagination gap")
        expected = nxt
    raise ValueError("missing terminal page: " + endpoint)


def endpoint(number=None, tail=""):
    return f"/repos/{REPOSITORY}/issues" + (f"/{number}" if number else "") + tail


def approval(issue, h, reactions, receipts, boundary=None):
    """Receipts are coordinator-authenticated evidence, never issue-provided JSON."""
    for receipt in receipts:
        if (receipt.get("issue_id") != issue["id"] or receipt.get("scope_hash") != h
                or receipt.get("repository_id") != REPOSITORY_ID
                or not owner(receipt.get("actor", {}))
                or receipt.get("action") != "bounded_source_tests_docs_draft_pr"
                or not receipt.get("evidence_ref")):
            continue
        if (boundary and boundary.get("scope_hash") == h
                and receipt.get("reopens_after") != boundary["ref"]):
            continue
        if receipt.get("kind") == "owner_request":
            if owner(issue["user"]) and receipt.get("human_origin_verified") is True:
                return receipt
        if receipt.get("kind") == "opening_post_plus_one":
            if any(r.get("id") == receipt.get("reaction_id") and r.get("content") == "+1"
                   and owner(r.get("user", {})) for r in reactions):
                return receipt
    return None


def evaluate(snapshot, ledger, checkpoint):
    """Return a proposal only. Caller owns evidence authentication and serialized writes."""
    result = {"schema": 1, "state": "BLOCKED", "reason": "unknown input",
              "items": [], "claim": None,
              "input_identity": {"observation": digest(snapshot), "ledger": digest(ledger),
                                 "checkpoint": digest(checkpoint)}}
    try:
        if snapshot.get("schema") != 1 or checkpoint.get("schema") != 1:
            raise ValueError("unsupported/missing schema")
        if snapshot.get("read_status") != "success":
            raise ValueError("failed/incomplete/stale observation")
        repo = snapshot["repository"]
        if repo.get("id") != REPOSITORY_ID or not owner(repo.get("owner", {})):
            raise ValueError("repository/owner identity mismatch")
        source = snapshot["source_closure"]
        if (not source or not all(isinstance(v, str) and re.fullmatch(r"[0-9a-f]{40}|[0-9a-f]{64}", v)
                                 for v in source.values())):
            raise ValueError("missing current source closure")
        current_source = digest(source)
        if snapshot["issues"].get("query") != {"state": "all"}:
            raise ValueError("issue collection must include open and closed issues")
        listed = pages(snapshot["issues"], endpoint())
        issues = {}
        numbers = {}
        for issue in listed:
            if "pull_request" in issue:
                continue
            if type(issue.get("id")) is not int or type(issue.get("number")) is not int:
                raise ValueError("missing stable issue identity")
            if issue["id"] in issues and issues[issue["id"]] != issue:
                raise ValueError("conflicting duplicate issue")
            if issue["number"] in numbers and numbers[issue["number"]] != issue["id"]:
                raise ValueError("issue number identity collision")
            issues[issue["id"]] = issue
            numbers[issue["number"]] = issue["id"]
        records = ledger["records"]
        tombstones = {t["request_id"]: t for t in ledger.get("tombstones", [])}
        ids = [r["request_id"] for r in records]
        if len(ids) != len(set(ids)) or not all(re.fullmatch(r"LX-REQ-\d{3,}", x) for x in ids):
            raise ValueError("invalid/duplicate ledger request ID")
        reserved = set(ids) | set(tombstones)
        if set(ids) & set(tombstones) or any(not re.fullmatch(r"LX-REQ-\d{3,}", x) for x in reserved):
            raise ValueError("invalid tombstone identity")
        for t in tombstones.values():
            if not t.get("superseded_by") or not set(t["superseded_by"]) <= set(ids):
                raise ValueError("tombstone requires retained successors")
        mapping = {k: list(v) for k, v in checkpoint.get("issue_map", {}).items()}
        for key, values in mapping.items():
            if not values or len(values) != len(set(values)) or not set(values) <= reserved:
                raise ValueError("invalid canonical issue mapping: " + key)
        accepted = dict(checkpoint.get("approvals", {}))
        cancelled = dict(checkpoint.get("cancellations", {}))
        proposed = {"schema": 1, "issue_map": mapping, "approvals": accepted, "cancellations": cancelled}
        next_id = max([int(x.rsplit("-", 1)[1]) for x in reserved] + [0]) + 1
        proposals = []
        for iid, listed_issue in sorted(issues.items()):
            evidence = snapshot["details"][str(iid)]
            if evidence.get("read_status") != "success":
                raise ValueError("missing current issue detail")
            issue = evidence["issue"]
            if (issue.get("id") != iid or issue.get("number") != listed_issue["number"]
                    or "pull_request" in issue or issue.get("url") != "https://api.github.com" + endpoint(issue["number"])):
                raise ValueError("detail identity mismatch")
            if (issue.get("state") not in ("open", "closed") or not isinstance(issue.get("title"), str)
                    or not isinstance(issue.get("body"), str)):
                raise ValueError("malformed issue scope/state")
            # All reads finish before any actionable plan is returned.
            timeline = pages(evidence["timeline"], endpoint(issue["number"], "/timeline"))
            key = f"{REPOSITORY_ID}:{iid}"
            linked = set(mapping.get(key, []))
            for rid in list(linked):
                if rid in tombstones:
                    linked.update(tombstones[rid]["superseded_by"])
            for record in records:
                for ref in record.get("known_source_references", []):
                    if (ref.get("type") == "issue" and ref.get("repository") == REPOSITORY
                            and ref.get("number") == issue["number"]):
                        linked.add(record["request_id"])
            if linked:
                mapping[key] = sorted(linked)
            spec = evidence.get("spec", {})
            h = scope_hash(issue, spec)
            item = {"issue_key": key, "issue_number": issue["number"], "scope_hash": h,
                    "source_hash": current_source, "request_ids": sorted(linked),
                    "state": "BLOCKED", "reason": "owner authority unknown"}
            old = accepted.get(key)
            old_scope = old.get("effective_scope_hash", old.get("scope_hash")) if old else None
            assessment = evidence.get("assessment", {})
            eq = assessment.get("scope_equivalence", {})
            equivalent = (old and old_scope != h and eq.get("previous_scope_hash") == old_scope
                          and eq.get("current_scope_hash") == h and eq.get("material_change") is False
                          and eq.get("evidence_ref") and assessment.get("scope_hash") == h
                          and assessment.get("source_hash") == current_source)
            # Cancellation is normalized by a trusted adapter from an explicit owner
            # instruction, not arbitrary community text or a removed reaction.
            action_scopes = {h}
            if old and (old_scope == h or equivalent):
                action_scopes.update([old_scope, old.get("scope_hash")])
                for link in old.get("scope_equivalences", []):
                    action_scopes.update([link.get("previous_scope_hash"), link.get("current_scope_hash")])
            cancellations = [e for e in timeline if e.get("event") == "owner_cancelled"
                             and owner(e.get("actor", {})) and e.get("evidence_ref")
                             and e.get("scope_hash") in action_scopes]
            closed_events = [e for e in timeline if e.get("event") == "closed"]
            if cancellations or closed_events or issue["state"] == "closed":
                if ((issue["state"] == "closed" and not closed_events) or (closed_events
                    and (not closed_events[-1].get("id") or not closed_events[-1].get("created_at")))):
                    raise ValueError("closed issue lacks exact closure event evidence")
                cancelled[key] = {"scope_hash": h, "ref": digest(
                    [e["evidence_ref"] for e in cancellations] + (["closed", iid,
                    closed_events[-1]["id"], closed_events[-1]["created_at"]] if closed_events else []))}
            boundary = cancelled.get(key)
            valid_old = (old and (old_scope == h or equivalent) and old.get("accepted") is True
                         and not old.get("cancelled") and old.get("repository_id") == REPOSITORY_ID
                         and old.get("issue_id") == iid and owner(old.get("actor", {}))
                         and old.get("action") == "bounded_source_tests_docs_draft_pr"
                         and old.get("evidence_ref") and (not boundary or boundary.get("scope_hash") != h
                             or old.get("reopens_after") == boundary["ref"]))
            # Reuse accepted exact-scope approval after checking current boundaries.
            needs_reactions = not valid_old and any(
                r.get("issue_id") == iid and r.get("scope_hash") == h
                and r.get("kind") == "opening_post_plus_one"
                for r in snapshot.get("approval_receipts", []))
            reactions = (pages(evidence["reactions"], endpoint(issue["number"], "/reactions"))
                         if needs_reactions else [])
            fresh = approval(issue, h, reactions, snapshot.get("approval_receipts", []), boundary)
            reopened = (boundary and ((fresh and fresh.get("reopens_after") == boundary["ref"])
                        or (valid_old and old.get("reopens_after") == boundary["ref"])))
            if issue["state"] == "closed" or (cancellations and not reopened):
                item.update(state="CANCELLED", reason="closed issue or explicit owner cancellation")
                # Keep immutable receipt; a later reopening needs a new approval receipt.
                if old:
                    accepted[key] = dict(old, cancelled=True)
            else:
                if boundary and boundary.get("scope_hash") == h and not reopened:
                    fresh = None
                authorized = old if valid_old else fresh
                if authorized:
                    accepted[key] = dict(authorized, accepted=True)
                    if equivalent and authorized is old:
                        accepted[key] = dict(accepted[key], effective_scope_hash=h,
                            scope_equivalences=old.get("scope_equivalences", []) + [eq])
                    missing = [g for g in GATES if not isinstance(assessment.get(g), str)
                               or not assessment[g].strip()]
                    if (missing or assessment.get("scope_hash") != h
                            or assessment.get("source_hash") != current_source
                            or not assessment.get("evidence_refs")):
                        item["reason"] = "needs proportionate source-backed feature assessment: " + ", ".join(missing)
                    elif assessment.get("resolution") != "ready":
                        item["reason"] = "resolve ambiguity/conflict: " + assessment.get("proposed_resolution", "required")
                    elif linked:
                        item.update(state="ACCEPTED", reason="existing ledger work; coordinator continues existing IDs")
                    else:
                        rid = f"LX-REQ-{next_id:03d}"
                        next_id += 1
                        item.update(state="PLANNED", request_ids=[rid], reason="bounded implementation eligible")
                        proposals.append(item)
                elif old and old_scope != h:
                    item["reason"] = "scope edited; reevaluate and establish authority for current scope"
            result["items"].append(item)
        # Tracked issues missing from a purported full state=all read are unknown.
        if any(k not in {f"{REPOSITORY_ID}:{i}" for i in issues} for k in set(mapping) | set(accepted) | set(cancelled)):
            raise ValueError("tracked issue absent from complete current read")
        result.update(state="PLANNED" if proposals else ("BLOCKED" if any(
                          i["state"] == "BLOCKED" for i in result["items"]) else "ACCEPTED"), reason="complete decision",
                      proposed_checkpoint=proposed)
        if proposals:
            result["claim"] = dict(proposals[0], owner="existing sole request coordinator",
                                   action="serialize ledger append, then route one bounded source/test/doc unit",
                                   limits="no merge/publication/device/security authority")
        # No new map is accepted until the coordinator actually appends a record.
        return result
    except (KeyError, TypeError, ValueError, AttributeError) as error:
        result.update(state="BLOCKED", reason="unknown: " + str(error), items=[], claim=None)
        result.pop("proposed_checkpoint", None)
        return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("snapshot", type=Path)
    p.add_argument("--ledger", type=Path, required=True)
    p.add_argument("--checkpoint", type=Path, required=True)
    args = p.parse_args()
    try:
        result = evaluate(json.loads(args.snapshot.read_text()), json.loads(args.ledger.read_text()),
                          json.loads(args.checkpoint.read_text()))
    except (OSError, ValueError) as error:
        result = {"schema": 1, "state": "BLOCKED", "reason": "unknown: " + str(error), "claim": None}
    print(json.dumps(result, sort_keys=True, indent=2, ensure_ascii=False))
    return 2 if result["state"] == "BLOCKED" else 0


if __name__ == "__main__":
    raise SystemExit(main())
