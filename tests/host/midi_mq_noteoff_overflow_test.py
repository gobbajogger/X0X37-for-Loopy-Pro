#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Full MIDI-in MQ must retain Note Off under a CC flood.

Extracts production midi_in_put from usb.c. A queued Note On plus a full ring of
CCs must still accept Note Off by displacing a CC; plain CCs may still drop.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
u = (ROOT / "firmware/src/usb.c").read_text()
start = u.index("/* MIDI rings: 4-byte USB-MIDI event packets */")
end = u.index("static void midi_out_event(uint32_t pkt)")
ring = u[start:end]
# Extract keeps production midi_out_* decls; silence unused in this harness.
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

static int count_cin_note(unsigned cin, unsigned note)
{
    unsigned n = 0;
    for (uint32_t i = mi_r; i != mi_w; i++) {
        uint32_t p = midi_in_q[i % MQ];
        if ((p & 0xFu) == cin && ((p >> 16) & 0x7Fu) == note)
            n++;
    }
    return (int)n;
}

static int expect_note_off_kept(void)
{
    mi_w = mi_r = 0;
    if (!midi_in_put(mk(9, 0x90, 60, 100)))
        return 11;
    for (unsigned i = 0; i < MQ - 1u; i++)
        if (!midi_in_put(mk(0xB, 0xB0, 1, i & 127u)))
            return 12;
    if (!midi_in_put(mk(8, 0x80, 60, 0)))
        return 13;
    if (mi_w - mi_r != MQ)
        return 14;
    if (count_cin_note(9, 60) != 1)
        return 15;
    if (count_cin_note(8, 60) != 1)
        return 16;
    return 0;
}

static int expect_vel0_note_on_kept(void)
{
    mi_w = mi_r = 0;
    if (!midi_in_put(mk(9, 0x90, 61, 100)))
        return 21;
    for (unsigned i = 0; i < MQ - 1u; i++)
        if (!midi_in_put(mk(0xB, 0xB0, 2, i & 127u)))
            return 22;
    /* Note On velocity 0 is a release */
    if (!midi_in_put(mk(9, 0x90, 61, 0)))
        return 23;
    if (count_cin_note(9, 61) != 2)  /* on + vel0 still CIN 9 */
        return 24;
    return 0;
}

static int expect_cc_still_drops(void)
{
    mi_w = mi_r = 0;
    for (unsigned i = 0; i < MQ; i++)
        if (!midi_in_put(mk(0xB, 0xB0, 7, i & 127u)))
            return 31;
    if (midi_in_put(mk(0xB, 0xB0, 7, 0x55)))
        return 32;
    if (mi_w - mi_r != MQ)
        return 33;
    for (uint32_t i = mi_r; i != mi_w; i++) {
        if (((midi_in_q[i % MQ] >> 24) & 0x7Fu) == 0x55u)
            return 34;
    }
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = expect_note_off_kept()) != 0) {
        printf("FAIL note-off-kept rc=%d depth=%u on=%d off=%d\n",
               rc, (unsigned)(mi_w - mi_r), count_cin_note(9, 60), count_cin_note(8, 60));
        return 1;
    }
    if ((rc = expect_vel0_note_on_kept()) != 0) {
        printf("FAIL vel0-kept rc=%d\n", rc);
        return 2;
    }
    if ((rc = expect_cc_still_drops()) != 0) {
        printf("FAIL cc-drop rc=%d\n", rc);
        return 3;
    }
    puts("ok");
    return 0;
}
"""
)
with tempfile.TemporaryDirectory(prefix="x0x-mq-noteoff-") as d:
    cpath = Path(d) / "t.c"
    cpath.write_text(src)
    exe = Path(d) / "t"
    subprocess.check_call(
        ["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", str(cpath), "-o", str(exe)]
    )
    raise SystemExit(subprocess.call([str(exe)]))
