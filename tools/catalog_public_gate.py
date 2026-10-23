#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Filter a staged Pages catalog to owner-approved, physically proven DEV firmware.

This tool never uploads artifacts. Report verification and site publication are
separate responsibilities; JSON attestations are claims until independently checked.
"""
import argparse
import copy
import json
import re
from pathlib import Path

SHA = re.compile(r"[0-9a-f]{64}\Z")
MODELS = {"elite": "SMK-37 Elite_900", "pro": "SMK-37 Pro_900"}
PASS_FIELDS = ("physical_device_pass", "exact_package_checked", "source_and_model_checked",
               "controller_checked", "nonhardware_gates_pass", "backup_readback_pass",
               "independent_review_pass", "owner_publication_approved", "evidence_report_reviewed")


def sha(value):
    return isinstance(value, str) and bool(SHA.fullmatch(value))


def admitted(build, model, pkg, proof):
    """A false result forbids the specific model; never inherit from another model."""
    if not isinstance(proof, dict) or build.get("status") != "dev-checked":
        return False
    if build.get("hardware_tested") is not True:
        return False
    if build.get("lifecycle") not in ("current", "superseded"):
        return False
    if build["lifecycle"] == "superseded" and proof.get("approved_rollback") is not True:
        return False
    if any(proof.get(k) is not True for k in PASS_FIELDS):
        return False
    if (proof.get("version") != build.get("version") or proof.get("model") != model or
        proof.get("source_commit") != build.get("source_commit") or
        proof.get("package_sha256") != pkg.get("sha256") or
        proof.get("product") != pkg.get("product")):
        return False
    if model not in MODELS or not isinstance(pkg.get("product"), str) or not pkg["product"].startswith(MODELS[model]):
        return False
    if not all(sha(x) for x in (pkg.get("sha256"), proof.get("controller_sha256"), proof.get("report_sha256"))):
        return False
    if not isinstance(proof.get("report_url"), str) or not proof["report_url"].startswith("https://"):
        return False
    if not isinstance(proof.get("device_id_ref"), str) or not proof["device_id_ref"]:
        return False
    return (type(pkg.get("size")) is int and pkg["size"] > 0 and
            isinstance(pkg.get("pkg"), str) and
            bool(re.fullmatch(r"firmware/[A-Za-z0-9._-]+\.fwsc", pkg["pkg"])))


def public_catalog(source, approvals):
    if not isinstance(source.get("versions"), list) or not isinstance(approvals, dict):
        raise ValueError("invalid catalog or attestations")
    selected = []
    for build in source["versions"]:
        accepted = {m: copy.deepcopy(pkg) for m, pkg in build.get("models", {}).items()
                    if admitted(build, m, pkg, approvals.get(str(build.get("version")) + "/" + m))}
        if accepted:
            item = copy.deepcopy(build)
            item["models"] = accepted
            item["physical_device_evidence"] = {m: approvals[build["version"] + "/" + m]["report_url"] for m in accepted}
            selected.append(item)
    selected.sort(key=lambda x: (str(x.get("built_at", "")), str(x.get("version", ""))), reverse=True)
    return {"schema": 2, "repo": source.get("repo"), "pages": source.get("pages"),
            "latest": selected[0]["version"] if selected else None, "versions": selected, "history": []}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--catalog", type=Path, required=True)
    ap.add_argument("--approved-device-evidence", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    filtered = public_catalog(json.loads(args.catalog.read_text()), json.loads(args.approved_device_evidence.read_text()))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(filtered, indent=2) + "\n")
    print("PUBLIC FILTER CHECKED: " + str(len(filtered["versions"])) + " hardware-proven DEV version(s), not published")


if __name__ == "__main__":
    main()
