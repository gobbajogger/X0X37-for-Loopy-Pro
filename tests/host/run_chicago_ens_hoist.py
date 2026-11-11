#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Pinned old/new Ensemble comparison; scratch output, strict float flags."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BASE = "a06e20189141ff5fb8766e2c6152be460f4356c3"
source = subprocess.check_output(
    ["git", "show", f"{BASE}:firmware/src/app/chicago_ens.h"], cwd=ROOT, text=True
)
start = source.index("static inline void chi_ens_process(")
end = source.index("static inline int chi_fx_is_ens_insert", start)
reference = source[start:end].replace("chi_ens_process", "ens_before_process")
cc = os.environ.get("CC", "cc")
with tempfile.TemporaryDirectory(prefix="chicago-ens-hoist-") as folder:
    out = Path(folder)
    (out / "ens_before.h").write_text(reference)
    for opt in ("-O2", "-Os"):
        command = [cc, "-std=c99", opt, "-Wall", "-Wextra", "-Werror", "-DX0X_HOST",
                   "-fno-fast-math", "-ffp-contract=off",
                   "-I" + str(out), "-Ifirmware/src/app", "-Ifirmware/src/dsp",
                   "tests/host/chicago_ens_hoist_test.c", "-lm", "-o", str(out / "test")]
        print("COMMAND:", " ".join(command), flush=True)
        subprocess.run(command, cwd=ROOT, check=True)
        subprocess.run([str(out / "test")], cwd=ROOT, check=True)
