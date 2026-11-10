#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compaction in usb.c moves each accept sequence with its packet.

Extracts production midi_in_put from usb.c, the same slice the overflow
harnesses compile. A full ring of CCs then a Note Off displaces one CC.
Every CC that remains must keep the sequence it was accepted with.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
u = (ROOT / "firmware/src/usb.c").read_text()
start = u.index("/* MIDI rings: 4-byte USB-MIDI event packets */")
end = u.index("static void midi_out_event(uint32_t pkt)")
ring = u[start:end]
ring = ring.replace(
    "static uint32_t midi_in_q[MQ], midi_out_q[MQ];\n"
    "static volatile uint32_t mi_w, mi_r, mo_w, mo_r;",
    "static uint32_t midi_in_q[MQ];\n"
    "static volatile uint32_t mi_w, mi_r;",
)
src = (
    "#include <stdint.h>\n#include <stdio.h>\n"
    "#define RING_PUBLISH() ((void)0)\n"
    "#define FM1_IRQ_TIMER5 63u\n"
    "static uint32_t fm1_irq_mask_save(unsigned n) { (void)n; return 0; }\n"
    "static void fm1_irq_mask_restore(unsigned n, uint32_t s) { (void)n; (void)s; }\n"
    + ring
    + r"""
static uint32_t mk(unsigned cin, unsigned st, unsigned d1, unsigned d2)
{
    return (cin & 15u) | (st << 8) | (d1 << 16) | (d2 << 24);
}

static int expect_sequences_follow_packets(void)
{
    uint32_t born[128];
    uint32_t i;
    unsigned n;
    mi_w = mi_r = 0;
    midi_seq_next = 1;
    for (n = 0; n < 128u; n++)
        born[n] = 0;
    for (i = 0; i < MQ; i++) {
        if (!midi_in_put(mk(0xB, 0xB0, 1, i)))
            return 11;
    }
    for (i = mi_r; i != mi_w; i++) {
        uint32_t p = midi_in_q[i % MQ];
        uint32_t d2 = (p >> 24) & 0x7Fu;
        if ((p & 0xFu) != 0xBu || born[d2])
            return 12;
        born[d2] = midi_in_seq[i % MQ];
    }
    if (!midi_in_put(mk(8, 0x80, 60, 0)))
        return 13;
    if (mi_w - mi_r != MQ)
        return 14;
    for (i = mi_r; i != mi_w; i++) {
        uint32_t p = midi_in_q[i % MQ];
        uint32_t seq = midi_in_seq[i % MQ];
        if ((p & 0xFu) == 0xBu) {
            uint32_t d2 = (p >> 24) & 0x7Fu;
            if (!born[d2] || seq != born[d2])
                return 15;
        } else if ((p & 0xFu) == 8u) {
            if (seq == 0 || seq == born[0])
                return 16;
        } else
            return 17;
    }
    return 0;
}

int main(void)
{
    int rc = expect_sequences_follow_packets();
    if (rc) {
        printf("FAIL seq-carry rc=%d\n", rc);
        return 1;
    }
    puts("ok");
    return 0;
}
"""
)
with tempfile.TemporaryDirectory(prefix="x0x-seq-carry-") as d:
    cpath = Path(d) / "t.c"
    cpath.write_text(src)
    exe = Path(d) / "t"
    subprocess.check_call(
        ["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", str(cpath), "-o", str(exe)]
    )
    raise SystemExit(subprocess.call([str(exe)]))
