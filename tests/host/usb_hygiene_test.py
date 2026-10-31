#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile the production USB parser/receiver or ring with host boundary stubs."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "firmware/src/usb.c").read_text()


def function(signature):
    start = source.index(signature)
    return source[start:source.index("\n}\n", start) + 3]


mode = sys.argv[1]
if mode == "sysex":
    start = source.index("static struct {\n    uint8_t up, config")
    state = source[start:source.index("} usb;", start) + len("} usb;")]
    start = source.index("static uint8_t sx_frame[")
    ota_state = source[start:source.index("#define SXQ", start)]
    # sx_busy belongs to the transmit path, which this receive harness never uses.
    ota_state = ota_state.replace("sx_ready, sx_collect, sx_busy", "sx_ready, sx_collect")
    production = state + "\n#if FELUCCA_OTA\n" + ota_state + "#endif\n"
    if "static int usb_update_admit(" in source:
        # LIVE-A update guard: transport/note admission is tested by usb-live-update;
        # this parser harness keeps the idle-device answer (admit).
        production += "static int usb_update_admit(void) { return 1; }\n"
    production += function("static void sysex_byte(uint8_t b)")
    production += function("static void ep1_rx(void)")
    production += "#if FELUCCA_OTA\n" + function("static int ota_frame_get(")
    production += function("static void ota_frame_done(void)") + "#endif\n"
    cases = [("ota", ["-DFELUCCA_OTA=1"]), ("no-ota", ["-DFELUCCA_OTA=0"])]
    test = ROOT / "tests/host/usb_sysex_parser_test.c"
elif mode == "ring":
    start = source.index("#define MQ ")
    production = source[start:source.index("\n", start) + 1]
    for signature in ("static int midi_pkt_is_note_off(", "static int midi_pkt_is_rt_transport(",
                      "static int midi_pkt_is_link_end(", "static int midi_pkt_is_keep(",
                      "static int midi_pkt_is_pitch_bend(", "static int midi_ring_put_note_off("):
        production += function(signature)
    cases = [("loader", ["-DFELUCCA_LOADER=1"])]
    test = ROOT / "tests/host/midi_ring_displace_test.c"
else:
    raise SystemExit("usage: usb_hygiene_test.py sysex|ring")

with tempfile.TemporaryDirectory(prefix="x0x-usb-" + mode + "-") as temp:
    directory = Path(temp)
    (directory / "usb_hygiene_production.h").write_text(production)
    for label, defines in cases:
        exe = directory / label
        subprocess.check_call(shlex.split(os.environ.get("CC", "cc")) +
                              ["-std=c11", "-O2", "-Wall", "-Wextra", "-Werror"] +
                              shlex.split(os.environ.get("CFLAGS", "")) + defines +
                              ["-I", str(directory), str(test), "-o", str(exe)])
        subprocess.check_call([str(exe)])
