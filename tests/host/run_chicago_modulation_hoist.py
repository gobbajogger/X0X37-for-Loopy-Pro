#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compare pinned old and current modulation TUs with shipping float flags."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BASE = "9231c26285c09356b49b2d9afbcd225377aae5c5"
PATH = "experiments/original_synth/chicago_modulation.c"
source = subprocess.check_output(["git", "show", f"{BASE}:{PATH}"], cwd=ROOT, text=True)
names = re.findall(r"^(?:int|void) (original_\w+)\(", source, re.MULTILINE)
cc = os.environ.get("CC", "cc")

def run(command):
    print("COMMAND:", " ".join(map(str, command)), flush=True)
    subprocess.run(command, cwd=ROOT, check=True)

with tempfile.TemporaryDirectory(prefix="chicago-mod-hoist-") as folder:
    out = Path(folder)
    before = out / "before.c"
    before.write_text(source)
    for opt in ("-O2", "-Os"):
        flags = [cc, "-std=c99", opt, "-Wall", "-Wextra", "-Werror", "-DNDEBUG",
                 "-fno-fast-math", "-ffp-contract=off", "-DORIGINAL_USE_SOFT_FMA", "-DX0X_FAST_FMA",
                 "-Iexperiments/original_synth"]
        run(flags + [f"-D{name}=before_{name}" for name in names] +
            ["-c", before, "-o", out / "before.o"])
        run(flags + ["tests/host/chicago_modulation_hoist_test.c", PATH,
                     out / "before.o", "experiments/original_synth/fused_reference.c", "-lm", "-o", out / "test"])
        run([out / "test"])
