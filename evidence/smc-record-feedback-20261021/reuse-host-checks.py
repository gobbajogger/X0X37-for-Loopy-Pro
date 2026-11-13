#!/usr/bin/env python3
"""Verify retained Record and navigation feedback checks before integration builds."""
import hashlib
import json
import os
from pathlib import Path
import shutil


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
if digest(Path(shutil.which("cc")).resolve()) != record["compiler_sha256"]:
    raise SystemExit("Host compiler differs from the retained test compiler")
for name, default in (("CC", "cc"), ("X0X_HOST_DEFS", ""), ("X0X_BREAK_BANK", "")):
    if os.environ.get(name, default) != record["environment"][name]:
        raise SystemExit("Changed test environment: " + name)
for check in record["checks"]:
    log = (evidence / check["log"]).read_text()
    if check["returncode"] != 0 or not all(marker in log for marker in check["success_markers"]):
        raise SystemExit("Retained check did not pass: " + check["name"])
print("PASS: retained feedback checks match", len(record["source_sha256"]),
      "source inputs, compiler, environment and", len(record["log_sha256"]), "logs")
for check in record["checks"]:
    print("REUSED:", check["name"], check["log"])
