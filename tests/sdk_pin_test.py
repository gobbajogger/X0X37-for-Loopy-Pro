#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""tools/build.py SDK pins: AC79NN_SDK_V1.2.14_2026-10-13, and a mismatch stops the build.

- The pins are the V1.2.14 hashes of uboot.boot, cfg_tool.bin and cfg/eq_cfg_hw.bin.
- check_sdk() exits with the mismatch message for a wrong file (a fake SDK in a temporary directory).
- With AC79_SDK set (a V1.2.14 checkout, never in git), check_sdk() passes on it."""
import importlib.util
import os
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
spec = importlib.util.spec_from_file_location("x0x_build", ROOT / "tools/build.py")
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)
fails = 0


def check(ok, what):
    global fails
    print(f"{'PASS' if ok else 'FAIL'} {what}")
    fails += not ok


check(build.SDK_TAG == "AC79NN_SDK_V1.2.14_2026-10-13", "SDK tag is V1.2.14")
check(build.SDK_SHA256 == {
    "uboot.boot": "64ccc63e4df3238046fadb200f1b134311d8fe6ec6e3ccb8c8b81379231b867e",
    "cfg_tool.bin": "276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b",
    "cfg/eq_cfg_hw.bin": "179ac9f4501d2cf8c3bdbc06fcda1e46228b7ace7e339342c767bb6568de57d8",
}, "SDK pins are the V1.2.14 hashes")

with tempfile.TemporaryDirectory() as tmp:
    for rel in build.SDK_SHA256:
        f = Path(tmp, "cpu/wl82/tools", rel)
        f.parent.mkdir(parents=True, exist_ok=True)
        f.write_bytes(b"not the SDK")
    build.fm1pkg_make.SDK = tmp
    try:
        build.check_sdk()
        check(False, "a wrong SDK file stops the build")
    except SystemExit as e:
        check("does not match pinned AC79NN_SDK_V1.2.14_2026-10-13" in str(e),
              f"a wrong SDK file stops the build: {e}")

sdk = os.environ.get("AC79_SDK")
if sdk and Path(sdk).is_dir():
    build.fm1pkg_make.SDK = sdk
    try:
        build.check_sdk()
        check(True, f"AC79_SDK {sdk} matches the pins")
    except SystemExit as e:
        check(False, f"AC79_SDK {sdk}: {e}")
else:
    print("SKIP AC79_SDK not set: real SDK check not run")
sys.exit(1 if fails else 0)
