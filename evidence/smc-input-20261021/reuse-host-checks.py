#!/usr/bin/env python3
"""Verify the exact retained host-test inputs and logs before a target build."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


evidence = Path(__file__).resolve().parent
record = json.loads((evidence / "host-evidence.json").read_text())
root = Path.cwd()
for name, expected in record["source_sha256"].items():
    if digest(root / name) != expected:
        raise SystemExit("Changed checked input: " + name)
for name, expected in record["log_sha256"].items():
    if digest(evidence / name) != expected:
        raise SystemExit("Changed retained log: " + name)
compiler = Path(shutil.which("cc")).resolve()
if digest(compiler) != record["compiler_sha256"]:
    raise SystemExit("Host compiler differs from the retained test compiler")
for check in record["checks"]:
    log = (evidence / check["log"]).read_text()
    if check["returncode"] != 0 or not all(marker in log for marker in check["success_markers"]):
        raise SystemExit("Retained check did not pass: " + check["name"])
print("PASS: retained host checks match all", len(record["source_sha256"]),
      "source inputs, compiler and", len(record["log_sha256"]), "logs")
for check in record["checks"]:
    print("REUSED:", check["name"], check["log"])
