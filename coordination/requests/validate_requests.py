#!/usr/bin/env python3
"""Small offline request-ledger check. This is not a firmware gate."""
import argparse
import collections
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
LEDGER_PATH = "coordination/requests/requests.json"
CLOSED = {"COMPLETED", "CANCELLED", "SUPERSEDED"}

def validate(data, registry, previous=None):
    errors = []
    records = data.get("records", [])
    ids = [r.get("request_id") for r in records]
    live = {r.get("request_id"): r for r in records}
    dead = {t.get("request_id"): t for t in data.get("tombstones", [])}
    def require(ok, message):
        if not ok:
            errors.append(message)
    require(len(ids) == len(set(ids)), "duplicate request IDs")
    require(len(dead) == len(data.get("tombstones", [])), "duplicate tombstone IDs")
    require(not set(live) & set(dead), "live/tombstone ID collision")
    for r in records:
        rid = r.get("request_id")
        require(bool(re.fullmatch(r"LX-REQ-\d{3,}", rid or "")), "missing/invalid request ID")
        require(bool(str(r.get("status", "")).strip()), f"{rid}: missing status")
        for field in ("title", "completion_criterion"):
            require(bool(str(r.get(field, "")).strip()), f"{rid}: missing {field}")
        if r.get("status") not in CLOSED:
            for field in ("owner", "next_action"):
                require(isinstance(r.get(field), str) and bool(r[field].strip()), f"{rid}: missing {field}")
        for field in ("dependencies", "related_requests"):
            require(isinstance(r.get(field), list), f"{rid}: missing {field}")
            for target in r.get(field, []):
                require(target in live and target != rid, f"{rid}: invalid {field} target {target}")
        for level in ("source", "test", "build", "deployed", "hardware"):
            require(bool(r.get("evidence", {}).get(level, {}).get("state")), f"{rid}: missing {level} evidence")
        for ref in r.get("known_source_references", []):
            repo, number, kind = ref.get("repository"), ref.get("number"), ref.get("type")
            require(repo in data["scope"]["repositories"], f"{rid}: unknown repository")
            endpoint = "pull" if kind == "pull_request" else "issues"
            require(ref.get("url") == f"https://github.com/{repo}/{endpoint}/{number}", f"{rid}: invalid reference URL")
            if ref.get("head"):
                require(bool(re.fullmatch("[0-9a-f]{40}", ref["head"])), f"{rid}: invalid reference head")
    for rid, t in dead.items():
        successors = t.get("superseded_by", [])
        require(bool(t.get("reason")) and bool(successors) and all(x in live and x != rid for x in successors), f"{rid}: invalid supersession link")
    required = set(registry["initial_request_ids"])
    if previous:
        required |= {r["request_id"] for r in previous.get("records", [])}
        required |= {t["request_id"] for t in previous.get("tombstones", [])}
    for rid in required:
        require(rid in live or rid in dead, f"{rid}: erased without supersession link")
    branches = data.get("unclassified_preserved_branches", [])
    keys = [(b.get("name"), b.get("sha")) for b in branches]
    require(len(keys) == len(set(keys)), "duplicate preserved branch head")
    required_heads = {(b["name"], b["sha"]) for b in registry["initial_preserved_branch_heads"]}
    if previous:
        required_heads |= {(b["name"], b["sha"]) for b in previous.get("unclassified_preserved_branches", [])}
    require(required_heads <= set(keys), "erased preserved branch head")
    for b in branches:
        require(b.get("request_id") in live, "preserved branch has invalid request")
        require(bool(re.fullmatch("[0-9a-f]{40}", b.get("sha", ""))), "invalid preserved branch SHA")
    counts = dict(collections.Counter(b["status"] for b in branches))
    require(counts == data.get("preserved_branch_classification_counts"), "branch classification counts differ")
    return errors

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--ledger", type=Path, default=HERE / "requests.json")
    p.add_argument("--registry", type=Path, default=HERE / "continuity-registry.json")
    group = p.add_mutually_exclusive_group()
    group.add_argument("--previous", type=Path, help="previous accepted ledger JSON")
    group.add_argument("--base-ref", help="Git revision containing previous accepted ledger")
    args = p.parse_args()
    data = json.loads(args.ledger.read_text())
    registry = json.loads(args.registry.read_text())
    previous = json.loads(args.previous.read_text()) if args.previous else None
    if args.base_ref:
        raw = subprocess.check_output(["git", "show", f"{args.base_ref}:{LEDGER_PATH}"], text=True)
        previous = json.loads(raw)
    errors = validate(data, registry, previous)
    if errors:
        for error in errors:
            print("FAIL:", error)
        return 1
    print(f"PASS: {len(data['records'])} requests; {len(data['unclassified_preserved_branches'])} preserved heads; IDs, owners, actions, references, evidence and continuity")
    if not previous:
        print("Initial-registry check only; use --previous or --base-ref for cross-revision preservation.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
