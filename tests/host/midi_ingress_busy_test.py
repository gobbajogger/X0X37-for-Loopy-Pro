#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise actual USB/TRS ingress through the production full MIDI ring."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
usb = (ROOT / "firmware/src/usb.c").read_text()
uart = (ROOT / "firmware/src/midi_uart.c").read_text()


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index("\n}\n", start) + 3]


start = usb.index("/* MIDI rings: 4-byte USB-MIDI event packets */")
end = usb.index("static void midi_out_event(uint32_t pkt)")
ring = usb[start:end].replace(
    "static uint32_t midi_in_q[MQ], midi_out_q[MQ];\n"
    "static volatile uint32_t mi_w, mi_r, mo_w, mo_r;",
    "static uint32_t midi_in_q[MQ];\nstatic volatile uint32_t mi_w, mi_r;",
)
production = ring + function(usb, "static void ep1_rx(void)")
production += function(uart, "static int um_put(uint32_t pkt)")
with tempfile.TemporaryDirectory(prefix="qb-midi-ingress-") as temp:
    directory = Path(temp)
    (directory / "midi_ingress_production.h").write_text(production)
    exe = directory / "test"
    subprocess.run(shlex.split(os.environ.get("CC", "cc")) +
                   ["-std=c11", "-O2", "-Wall", "-Wextra", "-Werror"] +
                   shlex.split(os.environ.get("CFLAGS", "")) +
                   ["-I", str(directory), str(ROOT / "tests/host/midi_ingress_busy_test.c"),
                    "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
