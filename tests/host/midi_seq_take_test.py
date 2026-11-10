#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Firmware midi_seq_take skips 0 across the 16-bit wrap.

Compiles that function out of firmware/src/usb.c on the host. No firmware
source is edited. The counter is set to 0xFFFF, taken, then taken again.
The stamp stored after the wrap and the stamp issued by the second take
are both 1, never 0.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
usb = (ROOT / "firmware/src/usb.c").read_text()
start = usb.index("static uint16_t midi_seq_next = 1;")
end = usb.index("/* When a MIDI ring is full", start)
take = usb[start:end]
if "midi_seq_take" not in take:
    raise SystemExit("midi_seq_take was not extracted from usb.c")
src = (
    "#include <stdint.h>\n#include <stdio.h>\n"
    + take
    + r"""
int main(void)
{
    uint16_t first, pending, second;
    midi_seq_next = 0xFFFFu;
    first = midi_seq_take();
    pending = midi_seq_next;
    second = midi_seq_take();
    if (first != 0xFFFFu || pending != 1u || second != 1u ||
        first == 0u || pending == 0u || second == 0u) {
        printf("FAIL usb midi_seq_take wrap: first=%u pending=%u second=%u\n",
               first, pending, second);
        return 1;
    }
    printf("midi seq take: 0 failures\n");
    return 0;
}
"""
)
with tempfile.TemporaryDirectory(prefix="x0x-midi-seq-take-") as d:
    cpath = Path(d) / "t.c"
    cpath.write_text(src)
    exe = Path(d) / "t"
    subprocess.check_call(
        ["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", str(cpath), "-o", str(exe)]
    )
    raise SystemExit(subprocess.call([str(exe)]))
