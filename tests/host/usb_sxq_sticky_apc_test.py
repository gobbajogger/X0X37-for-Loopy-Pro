#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Run actual APC enqueue/retry and EP1 drain, with only SIE I/O modelled."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
usb = (ROOT / "firmware/src/usb.c").read_text()
start = usb.index("/* Send a complete SysEx message through the same USB-MIDI event queue")
end = usb.index("/* ------------------------------------------------------- descriptors --- */")
production = usb[start:end]


def function(signature):
    start = usb.index(signature)
    return usb[start:usb.index("\n}\n", start) + 3]


production += function("static void ep1_put_pkt(uint32_t n, uint32_t pkt)")
production += function("static void ep1_tx(void)")
with tempfile.TemporaryDirectory(prefix="qb-usb-apc-retry-") as temp:
    directory = Path(temp)
    (directory / "usb_apc_retry_production.h").write_text(production)
    for ota in (0, 1):
        exe = directory / ("test-ota" + str(ota))
        subprocess.run(shlex.split(os.environ.get("CC", "cc")) +
                       ["-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-DFELUCCA_OTA=" + str(ota)] +
                       shlex.split(os.environ.get("CFLAGS", "")) +
                       ["-I", str(directory), str(ROOT / "tests/host/usb_sxq_sticky_apc_test.c"),
                        "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
