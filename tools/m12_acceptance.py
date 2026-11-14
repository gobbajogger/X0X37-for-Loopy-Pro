#!/usr/bin/env python3
"""Authenticate explicit M12 policy/evidence inputs; never select release scope.

Expected pins and accepted receipts are trust roots supplied by the caller, not
discovered from the evidence being checked. This offline tool performs no tests,
publication, checkout, Git mutation or device operation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path, PurePosixPath

SHA1 = re.compile(r"[0-9a-f]{40}\Z")
SHA256 = re.compile(r"[0-9a-f]{64}\Z")
LEVELS = {"source", "behavior", "host", "native", "device"}
STATUSES = {"PASS", "FAIL", "NOT_RUN"}
HELD_REQUESTS = {"#119": "DEFERRED", "26.7": "DECLINED", "M5": "UNDEFINED"}
MAX_BYTES = 16 * 1024 * 1024


class GateError(Exception):
    """An input is unavailable, unauthenticated or inconsistent."""


def require(condition, message):
    if not condition:
        raise GateError(message)


def object_value(value, label):
    require(isinstance(value, dict), f"{label}: expected object")
    return value


def list_value(value, label):
    require(isinstance(value, list), f"{label}: expected list")
    return value


def string(value, label):
    require(isinstance(value, str) and bool(value.strip()), f"{label}: expected nonempty string")
    return value


def digest(value, label, pattern=SHA1):
    require(isinstance(value, str) and pattern.fullmatch(value) is not None,
            f"{label}: expected full lowercase digest")
    return value


def safe_path(value):
    string(value, "path")
    try:
        value.encode("utf-8")
    except UnicodeError as exc:
        raise GateError("repository path is not valid UTF-8") from exc
    require(not value.startswith("/") and "\\" not in value
            and not any(ord(c) < 32 or ord(c) == 127 for c in value)
            and all(part not in {"", ".", ".."} for part in value.split("/")),
            f"unsafe repository path: {value!r}")
    return value


def unique_pairs(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, f"duplicate JSON key: {key}")
        result[key] = value
    return result


def invalid_constant(value):
    raise GateError(f"invalid JSON constant: {value}")


def read_json(raw, label):
    require(len(raw) <= MAX_BYTES, f"{label}: input too large")
    try:
        value = json.loads(raw.decode("utf-8"), object_pairs_hook=unique_pairs,
                           parse_constant=invalid_constant)
    except RecursionError as exc:
        raise GateError(f"{label}: excessive JSON nesting") from exc
    except (ValueError, UnicodeError) as exc:
        raise GateError(f"{label}: invalid UTF-8 JSON") from exc
    return object_value(value, label)


def text_blob(raw, label):
    try:
        return raw.decode("utf-8")
    except UnicodeError as exc:
        raise GateError(f"{label}: expected UTF-8 text") from exc


class GitObjects:
    """Hash-checked immutable objects, with replacements and lazy fetch disabled."""

    def __init__(self, repo):
        self.repo = str(Path(repo).resolve())
        self.cache = {}

    def read(self, kind, sha):
        digest(sha, f"{kind} object")
        key = kind, sha
        if key not in self.cache:
            env = dict(os.environ, GIT_NO_REPLACE_OBJECTS="1", GIT_NO_LAZY_FETCH="1",
                       GIT_TERMINAL_PROMPT="0", GIT_OPTIONAL_LOCKS="0")
            try:
                result = subprocess.run(["git", "--no-replace-objects", "-C", self.repo,
                                         "cat-file", kind, sha], stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE, env=env, timeout=15)
            except (OSError, subprocess.TimeoutExpired) as exc:
                raise GateError(f"unavailable Git {kind} object: {sha}") from exc
            require(result.returncode == 0, f"unavailable Git {kind} object: {sha}")
            raw = result.stdout
            require(len(raw) <= MAX_BYTES, f"Git {kind} object too large: {sha}")
            actual = hashlib.sha1(f"{kind} {len(raw)}\0".encode() + raw).hexdigest()
            require(actual == sha, f"Git {kind} object byte/hash mismatch: {sha}")
            self.cache[key] = raw
        return self.cache[key]

    def commit_tree(self, commit):
        raw = self.read("commit", commit)
        first = raw.split(b"\n", 1)[0]
        require(first.startswith(b"tree "), f"commit has no root tree: {commit}")
        try:
            tree = first[5:].decode("ascii")
        except UnicodeError as exc:
            raise GateError("invalid commit tree digest") from exc
        digest(tree, "commit tree")
        self.read("tree", tree)
        return tree

    def tree_entries(self, tree):
        raw = self.read("tree", tree)
        entries = {}
        offset = 0
        while offset < len(raw):
            end = raw.find(b"\0", offset)
            require(end >= 0 and end + 21 <= len(raw), "malformed Git tree")
            header = raw[offset:end].split(b" ", 1)
            require(len(header) == 2, "malformed Git tree entry")
            mode, name = header
            require(name not in entries, "duplicate Git tree entry")
            entries[name] = (mode, raw[end + 1:end + 21].hex())
            offset = end + 21
        return entries

    def blob_at(self, commit, path):
        parts = safe_path(path).split("/")
        tree = self.commit_tree(commit)
        for index, part in enumerate(parts):
            entry = self.tree_entries(tree).get(part.encode("utf-8"))
            require(entry is not None, f"missing candidate path: {path}")
            mode, sha = entry
            if index == len(parts) - 1:
                require(mode in {b"100644", b"100755"}, f"not a regular source file: {path}")
                return sha, self.read("blob", sha)
            require(mode in {b"40000", b"040000"}, f"not a directory: {path}")
            tree = sha
        raise GateError("empty source path")


def pinned_blob(git, commit, path, expected_blob):
    actual, raw = git.blob_at(digest(commit, "commit"), safe_path(path))
    require(actual == digest(expected_blob, "expected blob"), f"blob pin disagreement: {path}")
    return raw


def visible_lines(text):
    """Ignore fenced examples so they cannot invent canonical records."""
    fence = None
    for line in text.splitlines():
        marker = re.match(r"^ {0,3}(`{3,}|~{3,})", line)
        if marker:
            run = marker.group(1)
            if fence is None:
                fence = (run[0], len(run))
            elif run[0] == fence[0] and len(run) >= fence[1] and not line[marker.end():].strip():
                fence = None
            yield ""  # A code fence also interrupts any preceding Markdown table.
            continue
        if fence is None:
            yield line
    require(fence is None, "unclosed Markdown code fence")


def canonical_records(git, roadmap_commit, root_path, root_blob, queue_pins):
    root = text_blob(pinned_blob(git, roadmap_commit, root_path, root_blob), root_path)
    parent = PurePosixPath(root_path).parent
    expected = {str(parent / "roadmap" / f"{name}.md") for name in ("Complete", "Working", "Pending")}
    pins = {}
    for pin in list_value(queue_pins, "policy queues"):
        pin = object_value(pin, "queue pin")
        path = safe_path(pin.get("path"))
        require(path not in pins, f"duplicate queue pin: {path}")
        pins[path] = digest(pin.get("blob"), "queue blob")
    require(set(pins) == expected, "policy must pin all three canonical queues")
    records = {}
    for path, blob in sorted(pins.items()):
        relative = str(PurePosixPath(path).relative_to(parent))
        require(re.search(r"\]\(" + re.escape(relative) + r"(?:#[^)]*)?\)", root) is not None,
                f"ROADMAP root does not link canonical queue: {path}")
        text = text_blob(pinned_blob(git, roadmap_commit, path, blob), path)
        current = None
        table_header = None
        in_table = False
        for line in visible_lines(text):
            heading = re.fullmatch(r"##\s+(.+?)\s*", line)
            if heading:
                request = heading.group(1)
                require(request not in records, f"duplicate canonical request: {request}")
                current = {"request_id": request, "document": path, "source_section": None,
                           "disposition": "UNKNOWN", "rows": []}
                records[request] = current
                table_header, in_table = None, False
            elif current is not None:
                source = re.fullmatch(r"\*\*Preserved source section:\*\*\s*(.+)", line)
                disposition = re.fullmatch(r"\*\*Current disposition:\*\*\s*(.+)", line)
                if source:
                    require(current["source_section"] is None, "duplicate source-section identity")
                    current["source_section"] = source.group(1)
                    table_header, in_table = None, False
                elif disposition:
                    current["disposition"] = disposition.group(1)
                    table_header, in_table = None, False
                elif current["source_section"] is not None:
                    stripped = line.strip()
                    if not stripped.startswith("|"):
                        table_header, in_table = None, False
                        continue
                    body = stripped[1:-1] if stripped.endswith("|") else stripped[1:]
                    cells = [cell.strip() for cell in body.split("|")]
                    if all(re.fullmatch(r":?-{3,}:?", cell) for cell in cells):
                        in_table = table_header is not None and len(table_header) == len(cells)
                        table_header = None
                    elif in_table:
                        current["rows"].append(cells[0])
                    else:
                        table_header = cells
    return records


def paths_list(value, label):
    paths = [safe_path(path) for path in list_value(value, label)]
    require(bool(paths) and len(paths) == len(set(paths)), f"{label}: empty or duplicate paths")
    return paths


def verify_proof(git, candidate, expected_paths, proof):
    found = {}
    for entry in list_value(proof, "source proof"):
        entry = object_value(entry, "source proof entry")
        path = safe_path(entry.get("path"))
        require(path not in found, f"duplicate proof path: {path}")
        found[path] = entry
    require(set(found) == set(expected_paths), "source proof does not match policy input closure")
    for path, entry in found.items():
        blob, raw = git.blob_at(candidate, path)
        require(digest(entry.get("blob"), "proof blob") == blob, f"source blob disagreement: {path}")
        require(digest(entry.get("sha256"), "proof SHA-256", SHA256) == hashlib.sha256(raw).hexdigest(),
                f"source byte disagreement: {path}")


def accepted_receipt(git, obligation, row, pins, stage, request):
    expected = object_value(obligation["receipt"], "accepted receipt pointer")
    observed = object_value(row.get("receipt"), "evidence receipt pointer")
    require(observed == expected, "receipt differs from independently accepted policy")
    raw = pinned_blob(git, expected.get("commit"), expected.get("path"), expected.get("blob"))
    require(digest(expected.get("sha256"), "receipt SHA-256", SHA256) == hashlib.sha256(raw).hexdigest(),
            "accepted receipt byte disagreement")
    receipt = read_json(raw, "accepted receipt")
    require(receipt.get("schema") == "dnbox-m12-receipt-v1", "unsupported receipt schema")
    for field in ("candidate_commit", "candidate_tree", "roadmap_commit", "roadmap_blob"):
        require(receipt.get(field) == pins[field], f"receipt pin disagreement: {field}")
    for field, expected_value in {"stage": stage, "request_id": request,
                                  "obligation_id": obligation["id"], "level": obligation["level"],
                                  "status": row["status"]}.items():
        require(receipt.get(field) == expected_value, f"receipt disagreement: {field}")
    string(receipt.get("command"), "receipt command")
    exit_code = receipt.get("exit_code")
    require(type(exit_code) is int and exit_code == 0, "PASS receipt must have observed exit 0")
    verify_proof(git, pins["candidate_commit"], obligation["paths"], receipt.get("inputs"))


def evaluate(args):
    tool_hash = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    require(digest(args.tool_sha256, "expected adapter SHA-256", SHA256) == tool_hash,
            "adapter bytes differ from independently expected tool")
    git = GitObjects(args.repo)
    candidate = digest(args.candidate, "candidate commit")
    candidate_tree = digest(args.candidate_tree, "candidate tree")
    require(git.commit_tree(candidate) == candidate_tree, "candidate root tree disagreement")
    root_path = safe_path(args.roadmap_path)
    root_blob = digest(args.roadmap_blob, "ROADMAP root blob")
    roadmap_commit = digest(args.roadmap_commit, "ROADMAP commit")
    pinned_blob(git, roadmap_commit, root_path, root_blob)
    policy_raw = pinned_blob(git, args.policy_commit, args.policy_path, args.policy_blob)
    policy = read_json(policy_raw, "accepted policy")
    require(policy.get("schema") == "dnbox-m12-policy-v1", "unsupported policy schema")
    stage = string(policy.get("stage"), "policy stage")
    records = canonical_records(git, roadmap_commit, root_path, root_blob, policy.get("queues"))
    selected = list_value(policy.get("selected"), "selected policy records")
    require(bool(selected), "accepted policy has no selected obligations")
    required = {}
    selected_ids = set()
    for selection in selected:
        selection = object_value(selection, "policy selection")
        request = string(selection.get("request_id"), "selected request")
        require(request not in selected_ids, f"duplicate selected request: {request}")
        selected_ids.add(request)
        require(request not in HELD_REQUESTS, f"owner-held request cannot be selected: {request}")
        require(request in records, f"selected request is absent from canonical queues: {request}")
        record = records[request]
        require(string(selection.get("source_section"), "expected source section") == record["source_section"],
                f"original source-section disagreement: {request}")
        row_id = string(selection.get("row"), "expected original row")
        require(record["rows"].count(row_id) == 1, f"missing or duplicate original table row: {request}")
        obligations = list_value(selection.get("obligations"), "selected obligations")
        require(bool(obligations), f"empty selected obligations: {request}")
        for obligation in obligations:
            obligation = object_value(obligation, "obligation")
            identifier = string(obligation.get("id"), "obligation ID")
            key = request, identifier
            require(key not in required, f"duplicate required obligation: {request}/{identifier}")
            require(string(obligation.get("level"), "evidence level") in LEVELS,
                    f"unknown evidence level: {request}/{identifier}")
            paths_list(obligation.get("paths"), "policy input closure")
            required[key] = obligation
    try:
        evidence_raw = Path(args.evidence).read_bytes()
    except OSError as exc:
        raise GateError("evidence input is unavailable") from exc
    evidence = read_json(evidence_raw, "evidence")
    require(evidence.get("schema") == "dnbox-m12-evidence-v1", "unsupported evidence schema")
    pins = {"candidate_commit": candidate, "candidate_tree": candidate_tree,
            "roadmap_commit": roadmap_commit, "roadmap_blob": root_blob,
            "policy_commit": args.policy_commit, "policy_blob": args.policy_blob,
            "tool_sha256": tool_hash}
    for field, expected in pins.items():
        require(evidence.get(field) == expected, f"evidence pin disagreement: {field}")
    rows = {}
    for row in list_value(evidence.get("rows"), "evidence rows"):
        row = object_value(row, "evidence row")
        key = string(row.get("request_id"), "evidence request"), string(row.get("obligation_id"), "evidence obligation")
        require(key in required, f"evidence claims an unselected obligation: {key}")
        require(key not in rows, f"duplicate evidence obligation: {key}")
        require(string(row.get("status"), "evidence status") in STATUSES,
                f"unknown evidence status: {key}")
        rows[key] = row
    results = []
    for (request, identifier), obligation in required.items():
        result = {"request_id": request, "obligation_id": identifier,
                  "level": obligation["level"], "status": "NOT_RUN"}
        row = rows.get((request, identifier))
        if row is None:
            result["reason"] = "required evidence is missing"
        elif row["status"] != "PASS":
            result.update(status=row["status"], reason="negative or unrun result retained; not qualified")
        elif obligation["level"] == "source":
            verify_proof(git, candidate, obligation["paths"], row.get("proof"))
            result.update(status="PASS", proof_kind="git-source-bytes")
        elif "receipt" not in obligation:
            result["reason"] = "no independently pinned accepted receipt; source presence is insufficient"
        else:
            accepted_receipt(git, obligation, row, pins, stage, request)
            result.update(status="PASS", proof_kind="accepted-receipt")
        results.append(result)
    return {"schema": "dnbox-m12-result-v1", "accepted": all(r["status"] == "PASS" for r in results),
            "acceptance_scope": "selected-policy consistency only", "stage": stage,
            "inputs": pins, "evidence_sha256": hashlib.sha256(evidence_raw).hexdigest(),
            "results": results, "errors": [],
            "unselected": [{"request_id": key, "disposition": record["disposition"],
                            "document": record["document"]} for key, record in records.items()
                           if key not in selected_ids],
            "limitations": ["Caller must independently accept expected pins, policy and receipts.",
                            "No tests were executed by this validator; receipt consistency is not independent execution proof.",
                            "No release, publisher, package, device or production approval is granted."]}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    for flag in ("repo", "candidate", "candidate-tree", "roadmap-commit", "roadmap-path",
                 "roadmap-blob", "policy-commit", "policy-path", "policy-blob", "tool-sha256", "evidence"):
        parser.add_argument("--" + flag, required=True)
    args = parser.parse_args(argv)
    try:
        result = evaluate(args)
    except GateError as exc:
        result = {"schema": "dnbox-m12-result-v1", "accepted": False,
                  "errors": [str(exc)], "results": []}
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if result["accepted"] else 1


if __name__ == "__main__":
    sys.exit(main())
