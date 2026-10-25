#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Native per-input production primitive fault injection; no device claim."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="smc-ble-input-") as tmp:
    exe = Path(tmp) / "test"
    command = [os.environ.get("CC", "cc"), "-std=c99", "-O2", "-g",
               "-Wall", "-Wextra", "-Werror",
               *shlex.split(os.environ.get("SAN_FLAGS", "")),
               "-I" + str(root / "firmware/src")]
    if os.environ.get("SMC_INPUT_HEADER"):
        command += ['-DSMC_INPUT_HEADER="' + os.environ["SMC_INPUT_HEADER"] + '"']
    command += [str(root / "tests/host/smc_ble_input_test.c"), "-o", str(exe)]
    subprocess.run(command, check=True)
    subprocess.run([str(exe)], check=True)
