#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Full MIDI-in MQ must retain Pitch Bend under a CC flood.

Sibling of midi_mq_noteoff_overflow_test.py / midi_mq_rt_overflow_test.py.
Mackie Control drives SMC faders via CIN 0xE; a CC knob storm must not drop
the latest bend. Pitch bend is not a hard keep — Note Off / Stop still displace
it — and same-channel bends coalesce.
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
    + ring
    + r"""
static uint32_t mk(unsigned cin, unsigned st, unsigned d1, unsigned d2)
{
    return (cin & 15u) | (st << 8) | (d1 << 16) | (d2 << 24);
}

static int count_pb(unsigned ch, unsigned d2)
{
    unsigned n = 0;
    for (uint32_t i = mi_r; i != mi_w; i++) {
        uint32_t p = midi_in_q[i % MQ];
        if ((p & 0xFu) == 0xEu && ((p >> 8) & 0x0Fu) == ch && ((p >> 24) & 0x7Fu) == d2)
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

static int expect_pb_kept(void)
{
    mi_w = mi_r = 0;
    for (unsigned i = 0; i < MQ; i++)
        if (!midi_in_put(mk(0xB, 0xB0, 16, i & 127u)))
            return 11;
    if (!midi_in_put(mk(0xE, 0xE0, 0, 0x55)))
        return 12;
    if (mi_w - mi_r != MQ)
        return 13;
    if (count_pb(0, 0x55) != 1)
        return 14;
    return 0;
}

static int expect_pb_coalesce_same_ch(void)
{
    mi_w = mi_r = 0;
    for (unsigned i = 0; i < MQ; i++)
        if (!midi_in_put(mk(0xB, 0xB0, 7, i & 127u)))
            return 21;
    if (!midi_in_put(mk(0xE, 0xE0, 0, 0x10)))
        return 22;
    if (!midi_in_put(mk(0xE, 0xE0, 0, 0x70)))
        return 23;
    if (count_pb(0, 0x10) != 0)
        return 24;
    if (count_pb(0, 0x70) != 1)
        return 25;
    return 0;
}

static int expect_note_off_displaces_pb(void)
{
    mi_w = mi_r = 0;
    for (unsigned i = 0; i < MQ; i++)
        if (!midi_in_put(mk(0xE, 0xE0 + (i & 15u), 0, i & 127u)))
            return 31;
    if (!midi_in_put(mk(8, 0x80, 60, 0)))
        return 32;
    if (count_note_off(60) != 1)
        return 33;
    return 0;
}

static int expect_stop_displaces_pb(void)
{
    mi_w = mi_r = 0;
    for (unsigned i = 0; i < MQ; i++)
        if (!midi_in_put(mk(0xE, 0xE0, 0, i & 127u)))
            return 41;
    if (!midi_in_put(mk(0xF, 0xFC, 0, 0)))
        return 42;
    if (count_rt(0xFC) != 1)
        return 43;
    return 0;
}

static int expect_cc_still_drops(void)
{
    mi_w = mi_r = 0;
    for (unsigned i = 0; i < MQ; i++)
        if (!midi_in_put(mk(0xB, 0xB0, 1, i & 127u)))
            return 51;
    if (midi_in_put(mk(0xB, 0xB0, 1, 0x55)))
        return 52;
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = expect_pb_kept()) != 0) {
        printf("FAIL pb-kept rc=%d depth=%u pb=%d\n",
               rc, (unsigned)(mi_w - mi_r), count_pb(0, 0x55));
        return 1;
    }
    if ((rc = expect_pb_coalesce_same_ch()) != 0) {
        printf("FAIL pb-coalesce rc=%d\n", rc);
        return 2;
    }
    if ((rc = expect_note_off_displaces_pb()) != 0) {
        printf("FAIL note-off-over-pb rc=%d off=%d\n", rc, count_note_off(60));
        return 3;
    }
    if ((rc = expect_stop_displaces_pb()) != 0) {
        printf("FAIL stop-over-pb rc=%d stop=%d\n", rc, count_rt(0xFC));
        return 4;
    }
    if ((rc = expect_cc_still_drops()) != 0) {
        printf("FAIL cc-drop rc=%d\n", rc);
        return 5;
    }
    puts("ok");
    return 0;
}
"""
)
d = tempfile.mkdtemp()
(Path(d) / "t.c").write_text(src)
subprocess.check_call(
    ["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", f"{d}/t.c", "-o", f"{d}/t"]
)
raise SystemExit(subprocess.call([f"{d}/t"]))
