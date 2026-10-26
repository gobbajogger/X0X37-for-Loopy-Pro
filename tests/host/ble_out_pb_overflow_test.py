#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Full BLE MIDI-out mirror must retain Pitch Bend under a CC flood.

BLE sibling of midi_out_mq_pb_overflow_test.py. Notify backpressure must not
strand Mackie fader Pitch Bend feedback into Loopy.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
b = (ROOT / "firmware/src/ble.c").read_text()


def function(source, name):
    m = re.search(r"^[^\n]*\b" + re.escape(name) + r"\([^\n]*\)\s*\{", source, re.M)
    if not m:
        raise ValueError("Missing " + name)
    depth, end = 1, m.end()
    while depth:
        if end >= len(source):
            raise ValueError("Unterminated " + name)
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[m.start() : end]


src = (
    "#include <stdint.h>\n#include <stdio.h>\n"
    "#define RING_PUBLISH() ((void)0)\n"
    "#define BLE_OQ 56u\n"
    "static uint32_t ble_oq[BLE_OQ];\n"
    "static struct { volatile uint32_t ow, orr; union { int h; } host; } ble_r;\n"
    "static int ble_central_role, ble_host_on = 1;\n"
    "static int bleh_notifying(void *h) { (void)h; return 1; }\n"
    + function(b, "ble_out_is_note_off")
    + "\n"
    + function(b, "ble_out_is_pitch_bend")
    + "\n"
    + function(b, "ble_out")
    + r"""
static uint32_t mk(unsigned cin, unsigned st, unsigned d1, unsigned d2)
{
    return (cin & 15u) | (st << 8) | (d1 << 16) | (d2 << 24);
}

static int count_pb(unsigned ch, unsigned d2)
{
    unsigned n = 0;
    for (uint32_t i = ble_r.orr; i != ble_r.ow; i++) {
        uint32_t p = ble_oq[i % BLE_OQ];
        if ((p & 0xFu) == 0xEu && ((p >> 8) & 0x0Fu) == ch && ((p >> 24) & 0x7Fu) == d2)
            n++;
    }
    return (int)n;
}

static int count_cin_note(unsigned cin, unsigned note)
{
    unsigned n = 0;
    for (uint32_t i = ble_r.orr; i != ble_r.ow; i++) {
        uint32_t p = ble_oq[i % BLE_OQ];
        if ((p & 0xFu) == cin && ((p >> 16) & 0x7Fu) == note)
            n++;
    }
    return (int)n;
}

static int expect_pb_kept(void)
{
    ble_r.ow = ble_r.orr = 0;
    for (unsigned i = 0; i < BLE_OQ; i++)
        ble_out(mk(0xB, 0xB0, 1, i & 127u));
    ble_out(mk(0xE, 0xE0, 0, 0x55));
    if (ble_r.ow - ble_r.orr != BLE_OQ)
        return 14;
    if (count_pb(0, 0x55) != 1)
        return 15;
    return 0;
}

static int expect_note_off_over_pb(void)
{
    ble_r.ow = ble_r.orr = 0;
    for (unsigned i = 0; i < BLE_OQ; i++)
        ble_out(mk(0xE, 0xE0 + (i & 15u), 0, i & 127u));
    ble_out(mk(8, 0x80, 60, 0x7F));
    if (count_cin_note(8, 60) != 1)
        return 24;
    return 0;
}

static int expect_cc_still_drops(void)
{
    ble_r.ow = ble_r.orr = 0;
    for (unsigned i = 0; i < BLE_OQ; i++)
        ble_out(mk(0xB, 0xB0, 7, i & 127u));
    uint32_t before = ble_r.ow;
    ble_out(mk(0xB, 0xB0, 7, 0x55));
    if (ble_r.ow != before)
        return 32;
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = expect_pb_kept()) != 0) {
        printf("FAIL ble pb-kept rc=%d depth=%u pb=%d\n",
               rc, (unsigned)(ble_r.ow - ble_r.orr), count_pb(0, 0x55));
        return 1;
    }
    if ((rc = expect_note_off_over_pb()) != 0) {
        printf("FAIL ble note-off-over-pb rc=%d\n", rc);
        return 2;
    }
    if ((rc = expect_cc_still_drops()) != 0) {
        printf("FAIL ble cc-drop rc=%d\n", rc);
        return 3;
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
