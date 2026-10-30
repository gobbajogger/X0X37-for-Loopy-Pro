#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Full MIDI-in MQ must retain Start/Continue/Stop under a CC flood.

Sibling of midi_mq_noteoff_overflow_test.py (#592). Extracts production
midi_in_put from usb.c. A full ring of CCs must still accept realtime
transport (CIN 0xF, status 0xFA/0xFB/0xFC) by displacing a CC; plain CCs and
Clock may still drop. Note Off remain protected and must not be displaced.
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

static int count_rt(unsigned st)
{
    unsigned n = 0;
    for (uint32_t i = mi_r; i != mi_w; i++) {
        uint32_t p = midi_in_q[i % MQ];
        if ((p & 0xFu) == 0xFu && ((p >> 8) & 0xFFu) == st)
            n++;
    }
    return (int)n;
}

static int count_note_off(unsigned note)
{
    unsigned n = 0;
    for (uint32_t i = mi_r; i != mi_w; i++) {
        uint32_t p = midi_in_q[i % MQ];
        if ((p & 0xFu) == 8u && ((p >> 16) & 0x7Fu) == note)
            n++;
    }
    return (int)n;
}

static int expect_rt_kept(unsigned st)
{
    mi_w = mi_r = 0;
    for (unsigned i = 0; i < MQ; i++)
        if (!midi_in_put(mk(0xB, 0xB0, 16, i & 127u)))
            return 11;
    if (!midi_in_put(mk(0xF, st, 0, 0)))
        return 12;
    if (mi_w - mi_r != MQ)
        return 13;
    if (count_rt(st) != 1)
        return 14;
    return 0;
}

static int expect_rt_does_not_displace_note_off(void)
{
    mi_w = mi_r = 0;
    if (!midi_in_put(mk(8, 0x80, 60, 0)))
        return 21;
    for (unsigned i = 0; i < MQ - 1u; i++)
        if (!midi_in_put(mk(0xB, 0xB0, 1, i & 127u)))
            return 22;
    if (!midi_in_put(mk(0xF, 0xFC, 0, 0)))
        return 23;
    if (count_note_off(60) != 1)
        return 24;
    if (count_rt(0xFC) != 1)
        return 25;
    return 0;
}

static int expect_clock_still_drops(void)
{
    mi_w = mi_r = 0;
    for (unsigned i = 0; i < MQ; i++)
        if (!midi_in_put(mk(0xB, 0xB0, 7, i & 127u)))
            return 31;
    if (midi_in_put(mk(0xF, 0xF8, 0, 0)))
        return 32;
    if (count_rt(0xF8) != 0)
        return 33;
    return 0;
}

static int expect_cc_still_drops(void)
{
    mi_w = mi_r = 0;
    for (unsigned i = 0; i < MQ; i++)
        if (!midi_in_put(mk(0xB, 0xB0, 7, i & 127u)))
            return 41;
    if (midi_in_put(mk(0xB, 0xB0, 7, 0x55)))
        return 42;
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = expect_rt_kept(0xFC)) != 0) {
        printf("FAIL stop-kept rc=%d depth=%u stops=%d\n",
               rc, (unsigned)(mi_w - mi_r), count_rt(0xFC));
        return 1;
    }
    if ((rc = expect_rt_kept(0xFA)) != 0) {
        printf("FAIL start-kept rc=%d\n", rc);
        return 2;
    }
    if ((rc = expect_rt_kept(0xFB)) != 0) {
        printf("FAIL continue-kept rc=%d\n", rc);
        return 3;
    }
    if ((rc = expect_rt_does_not_displace_note_off()) != 0) {
        printf("FAIL rt-vs-noteoff rc=%d\n", rc);
        return 4;
    }
    if ((rc = expect_clock_still_drops()) != 0) {
        printf("FAIL clock-drop rc=%d\n", rc);
        return 5;
    }
    if ((rc = expect_cc_still_drops()) != 0) {
        printf("FAIL cc-drop rc=%d\n", rc);
        return 6;
    }
    puts("ok");
    return 0;
}
"""
)
with tempfile.TemporaryDirectory(prefix="x0x-mq-rt-") as d:
    cpath = Path(d) / "t.c"
    cpath.write_text(src)
    exe = Path(d) / "t"
    subprocess.check_call(
        ["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", str(cpath), "-o", str(exe)]
    )
    raise SystemExit(subprocess.call([str(exe)]))
