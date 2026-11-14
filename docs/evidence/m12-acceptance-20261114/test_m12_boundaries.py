#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Offline acceptance probes; never run the tests named in synthetic evidence.

Exit 1 means the checker lacks a probed acceptance boundary. These probes do not
claim a violation of its documented structural-only reporting contract. No Git
repository, firmware, package, network or device operation is needed.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile


def digest(data):
    return hashlib.sha256(data).hexdigest()


def run(checker, root, candidate):
    result = subprocess.run(
        ["sh", str(checker), "--strict", "--candidate-sha", candidate, str(root),
         str(root / "docs/roadmap-evidence")], capture_output=True, text=True,
    )
    return {"exit": result.returncode, "stdout": result.stdout, "stderr": result.stderr}


def probe(checker):
    with tempfile.TemporaryDirectory(prefix="m12-boundary-") as tmp:
        base = Path(tmp) / "base"
        evid = base / "docs/roadmap-evidence"
        evid.mkdir(parents=True)
        (base / "src").mkdir()
        (base / "tests/host").mkdir(parents=True)
        source = b"int selected_feature(void) { return 17; }\n"
        (base / "src/feature.c").write_bytes(source)
        (base / "tests/host/named_test.c").write_text("/* fixture only */\n")
        (base / "tests/run_tests.sh").write_text("#!/bin/sh\nrun fixture_selected true\n")
        # Synthetic independent fixture policy, NOT an owner release selection.
        scope = {"required_rows": ["#1", "#2"], "roadmap_commit": "a" * 40,
                 "roadmap_blob": "b" * 40}
        authority = Path(tmp) / "independent-selected-scope.json"
        authority.write_text(json.dumps(scope, sort_keys=True) + "\n")
        authority_sha = digest(authority.read_bytes())
        candidate = "c" * 40
        (evid / "REQUIRED_ROWS.txt").write_text("#1\n#2\n")
        for row in scope["required_rows"]:
            item = {"row_id": row, "roadmap_pin_commit": scope["roadmap_commit"],
                    "roadmap_blob": scope["roadmap_blob"], "status": "PRESENT",
                    "proof_source": "synthetic fixture, no test receipt",
                    "code_paths": ["src/feature.c"], "test_refs": [{
                        "file": "tests/host/named_test.c", "test": "fixture_selected",
                        "result": "PASS", "source_sha": candidate}]}
            (evid / (row + ".json")).write_text(json.dumps(item))

        cases = []

        def capture(name, path, pin, expected, meaning):
            observed = run(checker, path, pin)
            cases.append({"case": name, "expected_exit": expected, **observed,
                          "meets_acceptance": observed["exit"] == expected,
                          "meaning": meaning, "candidate": pin,
                          "independent_scope_sha256": digest(authority.read_bytes()),
                          "source_sha256": digest((path / "src/feature.c").read_bytes()),
                          "has_git_directory": (path / ".git").exists()})

        capture("structural_positive", base, candidate, 0,
                "Positive structural control; fixture claims are not real test proof.")
        missing = Path(tmp) / "missing"
        shutil.copytree(base, missing)
        (missing / "docs/roadmap-evidence/#2.json").unlink()
        capture("missing_json_control", missing, candidate, 1,
                "Required row deletion alone is detected by current strict checker.")
        (missing / "docs/roadmap-evidence/REQUIRED_ROWS.txt").write_text("#1\n")
        capture("shrunk_inventory", missing, candidate, 1,
                "Same independently selected #2 and same source; local inventory cannot waive it.")
        edited = Path(tmp) / "edited"
        shutil.copytree(base, edited)
        (edited / "src/feature.c").write_text("/* selected feature removed */\n")
        capture("changed_source_bytes", edited, candidate, 1,
                "File existence cannot bind source bytes to the claimed candidate.")
        wrong_pin = Path(tmp) / "wrong-roadmap"
        shutil.copytree(base, wrong_pin)
        for path in (wrong_pin / "docs/roadmap-evidence").glob("*.json"):
            item = json.loads(path.read_text())
            item["roadmap_pin_commit"] = "d" * 40
            item["roadmap_blob"] = "e" * 40
            path.write_text(json.dumps(item))
        capture("self_consistent_wrong_roadmap", wrong_pin, candidate, 1,
                "Internal agreement cannot replace independently expected ROADMAP pins.")
        assert digest(authority.read_bytes()) == authority_sha
        return cases


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--checker", required=True, type=Path)
    ap.add_argument("--historical-gate", type=Path)
    ap.add_argument("--roadmap", type=Path)
    ap.add_argument("--report", required=True, type=Path)
    args = ap.parse_args()
    cases = probe(args.checker.resolve())
    report = {"schema": "dnbox-m12-boundary-probes-v1",
              "checker_sha256": digest(args.checker.read_bytes()),
              "scope": "synthetic acceptance boundaries; not candidate qualification",
              "cases": cases, "acceptance_passed": all(c["meets_acceptance"] for c in cases)}
    if args.historical_gate and args.roadmap:
        spec = importlib.util.spec_from_file_location("m12_historical_gate", args.historical_gate)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        try:
            rows = module.parse_roadmap(args.roadmap.read_text())
            compatibility = {"result": "ACCEPTED", "rows": len(rows)}
        except module.GateError as exc:
            compatibility = {"result": "REJECTED", "reason": str(exc)}
        report["historical_parser_current_roadmap"] = {
            **compatibility, "gate_sha256": digest(args.historical_gate.read_bytes()),
            "roadmap_sha256": digest(args.roadmap.read_bytes())}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    for case in cases:
        print(f"{case['case']}: actual={case['exit']} expected={case['expected_exit']} "
              f"acceptance={'PASS' if case['meets_acceptance'] else 'FAIL'}")
    if "historical_parser_current_roadmap" in report:
        print("historical_parser_current_roadmap:", report["historical_parser_current_roadmap"])
    return 0 if report["acceptance_passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
