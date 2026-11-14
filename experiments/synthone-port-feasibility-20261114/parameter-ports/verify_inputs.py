#!/usr/bin/env python3
"""Verify pinned sibling dependencies and caller-provided public master source."""
import hashlib
import json
import os
from pathlib import Path
import re
import sys

HERE = Path(__file__).resolve().parent
EXPECTED_DEPENDENCIES_SHA256 = "c4911bcdc21e02bf70e327e9d24472e8d02eb78688ec5b9cb1da21d20cb696fe"
COMMIT = "6466a3715c96b7ecf1dd255a218cbd571408a314"
AMBIENT_COMPILER_PATHS = ("CPATH", "C_INCLUDE_PATH", "CPLUS_INCLUDE_PATH",
                          "OBJC_INCLUDE_PATH", "GCC_EXEC_PREFIX", "COMPILER_PATH", "LIBRARY_PATH")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def check_bytes(path, count, sha256, blob_sha):
    require(path.is_file() and not path.is_symlink(), "input must be a regular file: " + path.name)
    blob = path.read_bytes()
    require(len(blob) == count, "input size mismatch: " + path.name)
    require(hashlib.sha256(blob).hexdigest() == sha256, "input SHA-256 mismatch: " + path.name)
    identity = hashlib.sha1(b"blob " + str(len(blob)).encode() + b"\0" + blob).hexdigest()
    require(identity == blob_sha, "input Git blob mismatch: " + path.name)


def dependency_data():
    dependency_path = HERE / "DEPENDENCIES.json"
    require(hashlib.sha256(dependency_path.read_bytes()).hexdigest() == EXPECTED_DEPENDENCIES_SHA256,
            "dependency manifest changed")
    dependencies = json.loads(dependency_path.read_text())
    for row in dependencies["dependencies"]:
        check_bytes(HERE / row["path"], row["bytes"], row["sha256"], row["git_blob_sha"])
    for row in dependencies["project_inputs"]:
        check_bytes(HERE / row["path"], row["bytes"], row["sha256"], row["git_blob_sha"])
    allowed = set(dependencies["project_allowlist"]) | {"REVIEW.md"}
    for path in HERE.rglob("*"):
        relative = path.relative_to(HERE)
        if relative.parts[0] in ("build", "__pycache__"):
            continue
        require(not path.is_symlink(), "unverified project symlink: " + str(relative))
        if path.is_file():
            require(relative.as_posix() in allowed, "unverified project file: " + str(relative))
    approved = {(HERE / row["path"]).resolve() for row in
                dependencies["dependencies"] + dependencies["project_inputs"]}
    for path in approved:
        if path.suffix not in (".c", ".cpp", ".h", ".hpp"):
            continue
        for quoted in re.findall(r'^\s*#\s*include\s*"([^"\n]+)"', path.read_text(), re.MULTILINE):
            neighbor = path.parent / quoted
            if neighbor.exists():
                require(neighbor.resolve() in approved,
                        "unverified quoted-include neighbor: " + neighbor.name)
    return dependencies


def master_inputs():
    manifest = json.loads((HERE / "../public-source-manifest.json").read_text())
    selected = [row for row in manifest
                if row["repository"] == "AudioKit/AudioKitSynthOne" and row["commit"] == COMMIT
                and ("/DSP/Kernel/S1DSPKernel" in row["path"]
                     or row["path"].endswith("/DSP/S1Parameter.h")
                     or row["path"].endswith("/DSP/Audio Unit/S1AudioUnit.mm"))]
    require(len(selected) == 16, "expected exact sixteen-file master control closure")
    return selected


def verify(source_root):
    for key in AMBIENT_COMPILER_PATHS:
        require(not os.environ.get(key), "ambient compiler path rejected: " + key)
    dependencies = dependency_data()
    selected = master_inputs()
    for row in selected:
        check_bytes(source_root / row["path"], row["size"], row["sha256"], row["blob_sha"])
    kernel = source_root / "AudioKitSynthOne/DSP/Kernel"
    parameters = (kernel / "S1DSPKernel+parameters.mm").read_text()
    setters = (kernel / "S1DSPKernel+getSetSynthParameters.mm").read_text()
    require("return parameters[i];" in parameters and
            "const float actualValue = getParameter(portamentoHalfTime);" in setters and
            "updatePortamento(actualValue);" in setters, "current-output half-time cadence changed")
    require("s1p[i].portamento->htime = ht;" in parameters, "uniform half-time update changed")
    return {"verified_base_dependencies": len(dependencies["dependencies"]),
            "verified_master_files": len(selected), "source_commit": COMMIT}


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: python3 verify_inputs.py SYNTHONE_SOURCE_ROOT")
    try:
        print(json.dumps(verify(Path(sys.argv[1]).resolve())))
    except (OSError, ValueError, KeyError, TypeError) as error:
        print("input verification failed: " + str(error), file=sys.stderr)
        raise SystemExit(2)
