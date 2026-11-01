#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Fail-closed source completeness check for a pinned DnBOX candidate and ROADMAP.

This is a source *presence* gate, not functional/target/hardware qualification.
Offline git objects are the only source of code evidence. A PR number, branch,
commit message, test badge, or old audit claim never proves implementation.

Generate:
  python3 tools/roadmap_completeness.py --repo . --candidate COMMIT40 \
      --candidate-tree TREE40 --roadmap-ref ROADMAP_COMMIT40 \
      --roadmap-blob ROADMAP_BLOB40 --evidence docs/roadmap-source-evidence.json \
      --report /tmp/dnbox-completeness.json
Recheck for package/publication (always recomputes the evidence):
  python3 tools/roadmap_completeness.py --repo . --candidate COMMIT40 \
      --roadmap-ref ROADMAP_COMMIT40 --verify-report /tmp/dnbox-completeness.json

All refs are complete immutable object IDs. The publication caller MUST resolve
its current intended ROADMAP revision independently; replaying an old revision
can never authorize a newly selected revision.
"""
import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

SCHEMA = "dnbox-roadmap-completeness-v1"
EVIDENCE_SCHEMA = "dnbox-roadmap-source-evidence-v1"
SHA = re.compile(r"[0-9a-f]{40}\Z")
REQUIRED = {"done", "in 0.0.24", "verified and agreed by owner"}
QUEUED_PREFIXES = ("missing", "in progress", "waiting on owner", "approved", "open", "implementation pending")


class GateError(ValueError):
    pass


def _sha(value, label):
    if not isinstance(value, str) or SHA.fullmatch(value) is None:
        raise GateError(f"{label} must be an exact lowercase 40-character commit/tree/blob SHA")
    return value


def _path(value):
    if (not isinstance(value, str) or not value or value.startswith("/")
            or any(c in value for c in ("\\", "\x00", "\n", "\r"))
            or any(part in ("", ".", "..") for part in value.split("/"))):
        raise GateError(f"invalid evidence source path {value!r}")
    return value


def _git(repo, *arguments, binary=False):
    command = ["git", "-C", str(repo), *arguments]
    try:
        data = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              check=False)
    except OSError as exc:
        raise GateError(f"git invocation failed: {exc}") from exc
    if data.returncode:
        err = data.stderr.decode("utf-8", "replace").strip().splitlines()
        raise GateError(f"git object read failed ({arguments[0]}): {err[-1] if err else 'no details'}")
    return data.stdout if binary else data.stdout.decode("utf-8").strip()


def _object(repo, commit, path):
    return _git(repo, "rev-parse", f"{_sha(commit, 'commit')}:{_path(path)}")


def _content(repo, commit, path):
    return _git(repo, "show", f"{_sha(commit, 'commit')}:{_path(path)}", binary=True)


def _digest(data):
    return hashlib.sha256(data).hexdigest()


def _canonical(data):
    return json.dumps(data, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode("utf-8")


def _table(line):
    if not line.startswith("|") or not line.rstrip().endswith("|"):
        return None
    # A literal pipe inside code or link text is not a column break.
    # Reject unexpected column counts instead of silently truncating rows.
    return [part.strip().replace("\\|", "|") for part in re.split(r"(?<!\\)\|", line)[1:-1]]


def _row_id(title):
    if match := re.match(r"#(\d+)(?=\s|$)", title):
        return "#" + match.group(1)
    if match := re.match(r"PR\s+#(\d+)(?=\s|$)", title):
        return "PR-" + match.group(1)
    if match := re.match(r"(L\d+|M\d+|QA\d+)(?=\s|$)", title):
        return match.group(1)
    return "ROW-" + _digest(title.encode("utf-8"))[:12]


def parse_roadmap(text):
    """Return every row in the canonical tracker and the historical Done table."""
    rows = []
    section = None
    expected = None
    old_row = 0
    for lineno, line in enumerate(text.splitlines(), 1):
        if line.startswith("## "):
            heading = line[3:].strip().lower()
            section = "done" if heading == "done" else "tracker" if heading.startswith("master tracker") else None
            expected = None
        parts = _table(line)
        if section is None or parts is None:
            continue
        if all(re.fullmatch(r":?-{3,}:?", p or "") for p in parts):
            continue
        if expected is None:
            if section == "done" and parts == ["Area", "What", "Where"]:
                expected = 3
            elif section == "tracker" and parts == ["Item", "Source", "Status", "Evidence", "Next step"]:
                expected = 5
            else:
                raise GateError(f"unexpected {section} table header at ROADMAP line {lineno}")
            continue
        if len(parts) != expected or not all(parts):
            raise GateError(f"incomplete/ambiguous ROADMAP table row at line {lineno}")
        if section == "done":
            old_row += 1
            rows.append({"id": f"DONE-{old_row:03d}", "title": f"{parts[0]}: {parts[1]}",
                         "source": parts[2], "status": "done", "evidence": parts[2],
                         "line": lineno, "section": "historical Done"})
        else:
            rows.append({"id": _row_id(parts[0]), "title": parts[0], "source": parts[1],
                         "status": parts[2], "evidence": parts[3], "line": lineno,
                         "section": "Master tracker"})
    if not rows or not old_row or not any(r["section"] == "Master tracker" for r in rows):
        raise GateError("ROADMAP must contain both the historical Done and complete Master tracker tables")
    return rows


def _excerpt(data, start, end):
    if not isinstance(start, str) or not isinstance(end, str) or not start or not end:
        raise GateError("source span needs two nonempty distinct literal anchors")
    s, e = start.encode("utf-8"), end.encode("utf-8")
    if data.count(s) != 1 or data.count(e) != 1:
        raise GateError("source span anchors missing or ambiguous")
    at = data.index(s)
    after = data.index(e, at + len(s)) if e in data[at + len(s):] else -1
    if after < 0:
        raise GateError("source span anchors out of order")
    return data[at:after + len(e)]


def _verify_source(repo, candidate, proof):
    """Use independently pinned origin bytes; accept equivalent cherry-picks."""
    if not isinstance(proof, dict):
        raise GateError("source proof must be an object")
    origin = _sha(proof.get("origin_commit"), "origin commit")
    expected_tree = _sha(proof.get("origin_tree"), "origin tree")
    pinned_blob = _sha(proof.get("source_blob"), "source blob")
    src = _path(proof.get("source_path"))
    dst = _path(proof.get("candidate_path", src))
    if _git(repo, "rev-parse", f"{origin}^{{tree}}") != expected_tree:
        raise GateError("origin tree pin does not match its commit")
    if _object(repo, origin, src) != pinned_blob:
        raise GateError("origin file does not match the independently pinned blob")
    reference = _content(repo, origin, src)
    candidate_data = _content(repo, candidate, dst)
    kind = proof.get("kind")
    needle = proof.get("needle")
    if not isinstance(needle, str) or not needle or len(needle) < 8:
        raise GateError("proof needs a specific nonempty source signature of at least 8 characters")
    token = needle.encode("utf-8")
    if reference.count(token) != 1 or candidate_data.count(token) != 1:
        raise GateError("source signature absent or ambiguous in origin/candidate")
    if kind == "blob":
        if _object(repo, candidate, dst) != pinned_blob:
            raise GateError("candidate source blob differs from the approved origin")
    elif kind == "span":
        pinned_span = proof.get("span_sha256")
        if not isinstance(pinned_span, str) or not re.fullmatch(r"[0-9a-f]{64}", pinned_span):
            raise GateError("source span needs a pinned SHA256 digest")
        start, end = proof.get("start"), proof.get("end")
        if _digest(_excerpt(reference, start, end)) != pinned_span:
            raise GateError("origin source span does not match the pinned digest")
        if _digest(_excerpt(candidate_data, start, end)) != pinned_span:
            raise GateError("candidate source span differs from the verified origin")
    else:
        raise GateError("source proof kind must be blob or span")
    pr = proof.get("pr")
    if pr is not None and (not isinstance(pr, str) or re.fullmatch(r"#\d+", pr) is None):
        raise GateError("evidence PR must have the form #123")
    return {"origin_commit": origin, "origin_tree": expected_tree,
            "origin_blob": pinned_blob, "pr": pr, "path": dst, "kind": kind}


def _evidence(path):
    try:
        data = path.read_bytes()
        parsed = json.loads(data)
    except (OSError, ValueError) as exc:
        raise GateError(f"evidence map unreadable: {exc}") from exc
    if not isinstance(parsed, dict) or parsed.get("schema") != EVIDENCE_SCHEMA:
        raise GateError("unknown source evidence schema")
    if not isinstance(parsed.get("proofs"), dict) or not isinstance(parsed.get("required_release_rows"), list):
        raise GateError("source evidence must contain proofs and required_release_rows")
    if len(set(parsed["required_release_rows"])) != len(parsed["required_release_rows"]):
        raise GateError("duplicate selected release rows")
    if any(not isinstance(x, str) or not x for x in parsed["required_release_rows"]):
        raise GateError("invalid selected release row")
    return parsed, _digest(data)


def evaluate(repo, candidate, candidate_tree, roadmap_ref, roadmap_blob, evidence):
    repo = Path(repo).resolve()
    candidate = _sha(candidate, "candidate")
    candidate_tree = _sha(candidate_tree, "candidate tree")
    roadmap_ref = _sha(roadmap_ref, "ROADMAP revision")
    roadmap_blob = _sha(roadmap_blob, "ROADMAP blob")
    if _git(repo, "rev-parse", f"{candidate}^{{commit}}") != candidate:
        raise GateError("candidate commit is not an exact available git object")
    if _git(repo, "rev-parse", f"{candidate}^{{tree}}") != candidate_tree:
        raise GateError("candidate tree pin disagrees with the commit")
    if _git(repo, "rev-parse", f"{roadmap_ref}^{{commit}}") != roadmap_ref:
        raise GateError("ROADMAP revision is not an exact available commit")
    if _object(repo, roadmap_ref, "docs/ROADMAP.md") != roadmap_blob:
        raise GateError("ROADMAP blob pin disagrees with the revision")
    roadmap = _content(repo, roadmap_ref, "docs/ROADMAP.md").decode("utf-8")
    proof_map, map_sha = _evidence(Path(evidence))
    if proof_map.get("roadmap_blob") != roadmap_blob:
        raise GateError("evidence map belongs to a different ROADMAP revision")
    rows = parse_roadmap(roadmap)
    seen = set()
    duplicates = set()
    for row in rows:
        if row["id"] in seen:
            duplicates.add(row["id"])
        seen.add(row["id"])
    known = {r["id"] for r in rows}
    selected = set(proof_map["required_release_rows"])
    extra_map = set(proof_map["proofs"]) - known
    result = []
    for row in rows:
        identity = row["id"]
        raw_status = row["status"].lower().strip()
        required = raw_status in REQUIRED or identity in selected
        if identity in duplicates:
            category, reason, claims = "MISSING", "duplicate ROADMAP ID; mapping is ambiguous", []
        elif raw_status not in REQUIRED and not raw_status.startswith(QUEUED_PREFIXES):
            category, reason, claims = "MISSING", f"unrecognized ROADMAP status {row['status']!r}", []
        elif not required:
            category, reason, claims = "QUEUED", "explicitly not yet built/accepted for this release", []
        else:
            proofs = proof_map["proofs"].get(identity)
            if not isinstance(proofs, list) or not proofs:
                category, reason, claims = "MISSING", "required code lacks independently pinned source proofs", []
            else:
                claims = []
                errors = []
                for n, proof in enumerate(proofs, 1):
                    try:
                        claims.append(_verify_source(repo, candidate, proof))
                    except GateError as exc:
                        errors.append(f"proof {n}: {exc}")
                category = "MISSING" if errors else "PRESENT"
                reason = "; ".join(errors) if errors else "candidate bytes match independently pinned source evidence"
        result.append({**row, "result": category, "reason": reason, "verified_proofs": claims})
    for identity in sorted(selected - known):
        result.append({"id": identity, "title": identity, "source": "release selection",
                       "status": "required release fix not listed in ROADMAP", "evidence": "",
                       "section": "missing release-scope reference", "line": 0,
                       "result": "MISSING", "reason": "selected fix absent from pinned ROADMAP; sole writer must register it",
                       "verified_proofs": []})
    for identity in sorted(extra_map):
        result.append({"id": f"ORPHAN:{identity}", "title": identity, "source": "evidence map",
                       "status": "unresolved source mapping", "evidence": "",
                       "section": "orphan mapping", "line": 0,
                       "result": "MISSING", "reason": "proof references an unrecognized ROADMAP row",
                       "verified_proofs": []})
    count = {name: sum(r["result"] == name for r in result) for name in ("PRESENT", "MISSING", "QUEUED")}
    report = {"schema": SCHEMA, "candidate_commit": candidate, "candidate_tree": candidate_tree,
              "roadmap_ref": roadmap_ref, "roadmap_blob": roadmap_blob,
              "source_evidence_sha256": map_sha,
              "gate_source_sha256": _digest(Path(__file__).read_bytes()),
              "selected_release_rows": sorted(selected), "total_roadmap_rows": len(rows),
              "summary": count, "passed": count["MISSING"] == 0, "rows": result}
    report["report_sha256"] = _digest(_canonical(report))
    return report


def verify_pass(repo, report_file, candidate, roadmap_ref, evidence=None):
    """Used by package and publication. No trust in a cached PASS or edited JSON."""
    try:
        prior = json.loads(Path(report_file).read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        raise GateError(f"completeness report unreadable: {exc}") from exc
    if not isinstance(prior, dict) or prior.get("schema") != SCHEMA:
        raise GateError("completeness report schema missing or unsupported")
    if prior.get("candidate_commit") != _sha(candidate, "candidate"):
        raise GateError("stale completeness report: candidate commit changed")
    if prior.get("roadmap_ref") != _sha(roadmap_ref, "ROADMAP revision"):
        raise GateError("stale completeness report: ROADMAP revision changed")
    path = evidence if evidence is not None else Path(__file__).resolve().parent.parent / "docs/roadmap-source-evidence.json"
    recalculated = evaluate(repo, candidate, prior.get("candidate_tree"),
                            roadmap_ref, prior.get("roadmap_blob"), path)
    if recalculated != prior:
        raise GateError("stale or altered completeness report: source, evidence, tool or roadmap changed")
    if not recalculated["passed"]:
        raise GateError(f"ROADMAP completeness FAIL: {recalculated['summary']['MISSING']} missing required rows")
    return recalculated


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--repo", default=".")
    ap.add_argument("--candidate", required=True)
    ap.add_argument("--candidate-tree")
    ap.add_argument("--roadmap-ref", required=True)
    ap.add_argument("--roadmap-blob")
    ap.add_argument("--evidence", type=Path, default=Path(__file__).resolve().parent.parent / "docs/roadmap-source-evidence.json")
    ap.add_argument("--report", type=Path)
    ap.add_argument("--verify-report", type=Path)
    args = ap.parse_args(argv)
    try:
        if bool(args.verify_report) == bool(args.report):
            raise GateError("use exactly one of --report or --verify-report")
        if args.verify_report:
            checked = verify_pass(args.repo, args.verify_report, args.candidate,
                                  args.roadmap_ref, args.evidence)
        else:
            if not args.candidate_tree or not args.roadmap_blob:
                raise GateError("generation requires explicit --candidate-tree and --roadmap-blob")
            checked = evaluate(args.repo, args.candidate, args.candidate_tree,
                               args.roadmap_ref, args.roadmap_blob, args.evidence)
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(json.dumps(checked, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        s = checked["summary"]
        print(f"ROADMAP {checked['candidate_commit']} tree {checked['candidate_tree']} "
              f"roadmap {checked['roadmap_blob']} PRESENT={s['PRESENT']} "
              f"MISSING={s['MISSING']} QUEUED={s['QUEUED']}")
        return 0 if checked["passed"] else 1
    except (GateError, UnicodeError, json.JSONDecodeError) as exc:
        print(f"ROADMAP completeness: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
