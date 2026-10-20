#!/usr/bin/env python3
"""Verify retained host tooling identities without rerunning passing builds/tests."""
import hashlib
import json
from pathlib import Path
import platform
import subprocess
import sys

from PIL import __version__ as pillow_version

ROOT = Path(__file__).resolve().parents[2]
CANDIDATE = "46a3b3e8c781f9a8b503ac00013a6389bfb404b6"
CHECKED = "4e7b1e334f1d61f507f5cb94f202ceb8286f79a0"
AUDIT = "9a94229b3ff8751763351b403e980ac2a2a4cdf6"


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT).decode()


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(name):
    return json.loads((ROOT / name).read_text())


def main():
    inputs = read("evidence/host-setup-stage2/inputs.json")
    stage3 = read("evidence/host-setup-stage3/result.json")
    preservation = read("evidence/host-setup-stage3/source-preservation.json")
    test = read("evidence/host-setup-stage3/test/report.json")
    investigation = read("evidence/host-setup-stage3/investigation/report.json")
    source_mismatches = [p for p, h in inputs["source_sha256"].items()
                         if digest(ROOT / p) != h]
    evidence_paths = git("ls-tree", "-r", "--name-only", CHECKED,
                         "evidence/host-setup-stage2", "evidence/host-setup-stage3").splitlines()
    evidence_hashes = {p: digest(ROOT / p) for p in evidence_paths}
    evidence_mismatches = [p for p in evidence_paths if
                          hashlib.sha256(subprocess.check_output(
                              ["git", "show", CHECKED + ":" + p], cwd=ROOT)).hexdigest()
                          != evidence_hashes[p]]
    prior_hash_mismatches = [p for p, h in preservation["stage2_evidence_sha256"].items()
                             if evidence_hashes[p] != h]
    reviewed_mismatches = [p for p, h in investigation["reviewed_source_sha256"].items()
                          if digest(ROOT / p) != h]
    headers = {p: {"expected": h,
                   "actual": digest(Path("/Users/mac/Documents/x0x37/host_setup_implementation") / p)}
               for p, h in inputs["generated_headers_sha256"].items()}
    runtime = {"platform": platform.platform(), "python": sys.version,
               "pillow": pillow_version,
               "cc": subprocess.check_output(["cc", "--version"]).decode()}
    runtime_matches = {key: value == inputs[key] for key, value in runtime.items()}
    candidate_source_delta = git("diff", "--name-only", CANDIDATE, "--", ".", ":!evidence").splitlines()
    changed_sources = git("diff", "--name-only", AUDIT, CANDIDATE, "--", ".", ":!evidence").splitlines()
    expected_sources = sorted(["host/build_host.sh", "tests/host/test_host_setup.py"] +
                              ["tests/host/" + c + "_test.sh" for c in
                               ("smc_reachability", "smc_drum_mutes", "smc_performance_abort", "ui_feedback")])
    host = (ROOT / "host/build_host.sh").read_text()
    old_host = git("show", AUDIT + ":host/build_host.sh")
    setup = host[host.index('CC="'):host.index('[ "${1:-}" != --generate-only ]')]
    old_setup = old_host[old_host.index('CC="'):old_host.index('U="')]
    section_matches = {"asset_setup": setup == old_setup,
                       "default_compile": host[host.index('U="'):] == old_host[old_host.index('U="'):]}
    for name in expected_sources:
        if name.endswith("_test.sh"):
            new = (ROOT / name).read_text().replace("sh host/build_host.sh --generate-only", "sh host/build_host.sh")
            new = new.replace("# Host builder generates shared sample/font headers without a simulator binary.",
                              "# Host builder generates scratch binary and shared generated sample/font headers.")
            section_matches[name] = new == git("show", AUDIT + ":" + name)
    for name in ("tests/run_tests.sh", "tools/quickcheck.sh", "tools/gen_builtin_break.sh"):
        section_matches[name] = (ROOT / name).read_bytes() == subprocess.check_output(
            ["git", "show", AUDIT + ":" + name], cwd=ROOT)
    supplemental = test["new_checks"][0]
    checks = {
        "checked_commit_parent_is_candidate": git("rev-parse", CHECKED + "^").strip() == CANDIDATE,
        "checked_commit_is_lane_ancestor": subprocess.call(["git", "merge-base", "--is-ancestor", CHECKED, "HEAD"], cwd=ROOT) == 0,
        "source_manifest_matches": not source_mismatches,
        "retained_evidence_matches_checked_commit": not evidence_mismatches,
        "stage3_recorded_stage2_hashes_match": not prior_hash_mismatches,
        "reviewed_source_hashes_match": not reviewed_mismatches,
        "original_generated_headers_match": all(v["actual"] == v["expected"] for v in headers.values()),
        "recorded_runtime_versions_match": all(runtime_matches.values()),
        "candidate_source_unchanged": not candidate_source_delta,
        "candidate_source_scope_matches": sorted(changed_sources) == expected_sources,
        "preserved_sections_match": all(section_matches.values()),
        "stage3_reports_pass_exact_candidate": all(r["candidate"] == CANDIDATE for r in (stage3, test, investigation)) and
            stage3["state"] == test["result"] == investigation["result"] == "PASS" and not investigation["blocking_findings"],
        "supplemental_script_matches_test_report": digest(ROOT / supplemental["command"][1]) == supplemental["script_sha256"],
    }
    result = {"candidate": CANDIDATE, "candidate_tree": git("rev-parse", CANDIDATE + "^{tree}").strip(),
              "checked_evidence_commit": CHECKED, "audit_base": AUDIT,
              "result": "PASS" if all(checks.values()) else "FAIL", "checks": checks,
              "source_manifest_count": len(inputs["source_sha256"]), "source_mismatches": source_mismatches,
              "candidate_source_delta": candidate_source_delta, "changed_source_files": changed_sources,
              "evidence_sha256": evidence_hashes, "evidence_mismatches": evidence_mismatches,
              "prior_hash_mismatches": prior_hash_mismatches, "reviewed_mismatches": reviewed_mismatches,
              "original_lane_headers": headers, "runtime": runtime, "runtime_version_matches": runtime_matches,
              "section_matches": section_matches,
              "dependency_identity_limit": "Recorded versions match; full installed package bytes were not fingerprinted in stage2.",
              "execution_scope": "Read-only identity checks. No retained test, generator, probe, simulator or target build was rerun."}
    (Path(__file__).parent / "identity.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"result": result["result"], "checks": checks,
                      "source_files": result["source_manifest_count"], "evidence_files": len(evidence_hashes)}, indent=2))
    return 0 if all(checks.values()) else 1


if __name__ == "__main__":
    sys.exit(main())
