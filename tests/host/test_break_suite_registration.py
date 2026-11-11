#!/usr/bin/env python3
"""Keep BREAK behavior checks registered in the full host suite.

Only text and path assertions. No device, samples or private input required.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "tests/run_tests.sh"
REQUIRED = {
    "adaptive-break-dsp": "sh tests/host/run_adaptive_break_dsp.sh",
    "adaptive-break-controls": "sh tests/host/run_adaptive_break_controls.sh",
    "adaptive-break-storage": "sh tests/host/run_adaptive_break_storage.sh",
    "adaptive-break-midi-pair": "python3 tests/host/run_adaptive_break_midi_pair.py",
    "break-performance": "sh tests/host/run_break_performance.sh",
    "builtin-break-output": "python3 tests/host/test_builtin_break_output.py",
}


def main() -> int:
    source = RUNNER.read_text(encoding="utf-8")
    errors = []
    for name, command in REQUIRED.items():
        matches = re.findall(r"^run\s+" + re.escape(name) + r"\s+(.+)$", source, re.M)
        if matches != [command]:
            errors.append(f"{name}: expected exactly one '{command}' registration; found {matches!r}")
        source_path = ROOT / command.split()[-1]
        if not source_path.is_file():
            errors.append(f"{name}: missing executable input {source_path.relative_to(ROOT)}")
    cls_match = re.search(r"(?m)^\s*adaptive-break-controls\|adaptive-break-storage(?:\|[^\n]*)?\) TEST_CLASS=G ;;$", source)
    if not cls_match:
        errors.append("BREAK controls/storage must share generated host inputs in class G")
    if errors:
        print("BREAK suite registration FAIL:", *errors, sep="\n", file=sys.stderr)
        return 1
    print(f"BREAK suite registration: {len(REQUIRED)}/{len(REQUIRED)} checks present once, class G verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
