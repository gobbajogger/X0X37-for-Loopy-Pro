#!/usr/bin/env python3
"""Preflight for a full, zero-skip X0X37 host suite.

Run before tests/run_tests.sh. A missing optional reference is a setup FAIL here,
not a silently skipped comparison. No source files are modified.
"""

import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile

REFERENCE_INPUTS = (
    ("drum909", "NINEW9_DIR", "../schwung-9W9", "src/dsp/er99_engine.c"),
    ("drum808", "EIGHTW8_DIR", "../schwung-8W8", "src/dsp/sc808_engine.cpp"),
    ("sequencer", "TB3PO_REF", "../schwung-tb3po/src/dsp/tb3po.c", ""),
)


def reference_checks(root, names, environ=None):
    """Return (label, passed, detail) tuples for selected comparisons."""
    environ = os.environ if environ is None else environ
    selected = set(names)
    checks = []
    for name, variable, default, suffix in REFERENCE_INPUTS:
        if selected and name not in selected:
            continue
        raw = Path(environ.get(variable, default)).expanduser()
        path = (raw if raw.is_absolute() else root / raw) / suffix
        checks.append(("reference " + name, path.is_file(), str(path.resolve())))
    return checks


def package_checks(root, names, environ=None):
    """Detect the runner's otherwise uncounted SMK package skip."""
    environ = os.environ if environ is None else environ
    if names and "smk-package" not in names:
        return []
    sdk = environ.get("AC79_SDK", "")
    return [
        ("SMK app", (root / "build/smk37/x0x.bin").is_file(), "build/smk37/x0x.bin"),
        ("SMK loader", (root / "build/smk37/loader/ota.bin").is_file(), "build/smk37/loader/ota.bin"),
        ("AC79_SDK", bool(sdk) and Path(sdk).is_dir(), sdk or "not configured"),
    ]


def sanitizer_check(command):
    """Match the suite's ASan+UBSan link requirement, without running firmware."""
    with tempfile.TemporaryDirectory(prefix="x0x37-asan-preflight-") as directory:
        src = Path(directory) / "main.c"
        dst = Path(directory) / "main"
        src.write_text("int main(void){return 0;}\n", encoding="ascii")
        try:
            proc = subprocess.run(command + ["-fsanitize=address,undefined", "-o", str(dst), str(src)],
                                  capture_output=True, text=True, timeout=30, check=False)
        except (OSError, subprocess.TimeoutExpired) as error:
            return False, str(error)
        return proc.returncode == 0, (proc.stderr or "").strip()[-500:]


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2],
                        help="Source root containing tests/run_tests.sh")
    parser.add_argument("--refs-only", action="store_true",
                        help="Check reference sources only (useful for fixture validation)")
    parser.add_argument("--tests", nargs="*", default=None,
                        help="Reference checks needed for these test names; default: all")
    args = parser.parse_args(argv)
    root = args.root.resolve()
    checks = []
    selected = args.tests or []
    if not args.refs_only:
        for relative in ("tests/run_tests.sh", "host/build_host.sh"):
            path = root / relative
            checks.append((relative, path.is_file(), str(path)))
        for name in ("python3", "sh", "bash", "node", "c++"):
            result = shutil.which(name)
            checks.append(("tool " + name, result is not None, result or "not found in PATH"))
        cc = shlex.split(os.environ.get("CC", "cc"))
        compiler = shutil.which(cc[0]) if cc else None
        checks.append(("tool CC", compiler is not None, " ".join(cc) or "empty CC"))
        try:
            import PIL  # noqa: F401: exactly the module the host suite tests
            pillow = True
        except ImportError:
            pillow = False
        checks.append(("Python Pillow", pillow, "import PIL"))
        if compiler:
            ok, detail = sanitizer_check(cc)
            checks.append(("ASan+UBSan link", ok, detail or "compiler linked sanitizer probe"))
    checks.extend(reference_checks(root, selected))
    checks.extend(package_checks(root, selected))
    failures = 0
    for label, passed, detail in checks:
        print("PASS" if passed else "FAIL", label + ":", detail)
        failures += not passed
    print("PREFLIGHT:", len(checks) - failures, "PASS,", failures, "FAIL")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
