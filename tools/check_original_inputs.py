#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Verify external original-engine headers; never generate or replace inputs."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re

MANIFEST_PATH = Path("experiments/original_synth/EXTERNAL-INPUTS.json")
MANIFEST_SHA256 = "9c7ac9b0a71e775fb7009e2d4035fce827b2c56c4e8e692a7a8910c9c6c881bc"


def verify(root, manifest=None, scope="active", expected_manifest_sha256=None):
    """Return metadata and errors for the ten active or all twelve local inputs.

    The manifest is relative to root unless an absolute path is supplied. A
    manifest hash can be required independently of its declared header hashes.
    Payload bytes are read only for hashing and are never included in the report.
    """
    root = Path(root).resolve()
    path = Path(manifest) if manifest is not None else MANIFEST_PATH
    if not path.is_absolute():
        path = root / path
    result = {"ok": False, "root": str(root), "manifest": str(path),
              "scope": scope, "inputs": [], "errors": []}
    errors = result["errors"]
    if scope not in ("active", "all"):
        errors.append("scope must be active or all")
        return result
    try:
        raw = path.read_bytes()
    except OSError as error:
        errors.append(f"cannot read manifest: {error}")
        return result
    result["manifest_sha256"] = hashlib.sha256(raw).hexdigest()
    if expected_manifest_sha256 is not None:
        if not re.fullmatch(r"[0-9a-f]{64}", expected_manifest_sha256):
            errors.append("expected manifest SHA256 must be 64 lowercase hexadecimal digits")
        elif result["manifest_sha256"] != expected_manifest_sha256:
            errors.append("manifest SHA256 mismatch")
    try:
        data = json.loads(raw)
    except (ValueError, UnicodeError) as error:
        errors.append(f"invalid manifest JSON: {error}")
        return result
    entries = data.get("excluded_data_inputs") if isinstance(data, dict) else None
    if not isinstance(entries, list) or len(entries) != 12:
        errors.append("manifest must declare exactly twelve external inputs")
        return result
    paths = set()
    active = 0
    for index, entry in enumerate(entries):
        if not isinstance(entry, dict):
            errors.append(f"input {index}: expected an object")
            continue
        name = entry.get("path")
        size = entry.get("bytes")
        digest = entry.get("sha256")
        required = entry.get("required_by_active_original_profile")
        relative = PurePosixPath(name) if isinstance(name, str) else None
        if (relative is None or not name or relative.is_absolute() or
                ".." in relative.parts or "\\" in name or str(relative) != name):
            errors.append(f"input {index}: path must be a normalized relative path")
        elif name in paths:
            errors.append(f"duplicate input path: {name}")
        else:
            paths.add(name)
        if type(size) is not int or size <= 0:
            errors.append(f"input {index}: bytes must be a positive integer")
        if not isinstance(digest, str) or not re.fullmatch(r"[0-9a-f]{64}", digest):
            errors.append(f"input {index}: invalid SHA256")
        if type(required) is not bool:
            errors.append(f"input {index}: required_by_active_original_profile must be boolean")
        elif required:
            active += 1
    if active != 10:
        errors.append("manifest must declare exactly ten active inputs")
    if errors:
        return result
    for entry in entries:
        if scope == "active" and not entry["required_by_active_original_profile"]:
            continue
        item = {"path": entry["path"], "expected_bytes": entry["bytes"],
                "expected_sha256": entry["sha256"], "ok": False}
        result["inputs"].append(item)
        try:
            payload = (root / entry["path"]).read_bytes()
        except OSError as error:
            errors.append(f"{entry['path']}: cannot read input: {error}")
            continue
        item["bytes"] = len(payload)
        item["sha256"] = hashlib.sha256(payload).hexdigest()
        item["ok"] = (item["bytes"] == item["expected_bytes"] and
                      item["sha256"] == item["expected_sha256"])
        if not item["ok"]:
            errors.append(f"{entry['path']}: byte count or SHA256 mismatch")
    result["ok"] = not errors
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--manifest", type=Path, help="manifest path, relative to root or absolute")
    parser.add_argument("--scope", choices=("active", "all"), default="active")
    parser.add_argument("--manifest-sha256", default=MANIFEST_SHA256,
                        help="required manifest hash (defaults to the committed profile pin)")
    parser.add_argument("--json", action="store_true", help="print a metadata-only JSON report")
    args = parser.parse_args()
    result = verify(args.root, args.manifest, args.scope, args.manifest_sha256)
    if args.json:
        print(json.dumps(result, indent=2))
    elif result["ok"]:
        print(f"original inputs: PASS ({len(result['inputs'])} {args.scope}, "
              f"manifest {result['manifest_sha256']})")
    else:
        print("original inputs: FAIL")
        for error in result["errors"]:
            print(error)
    return 0 if result["ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
