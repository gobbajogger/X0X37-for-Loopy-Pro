#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Full BLE MIDI-out mirror must retain Start/Continue/Stop under a CC flood.

Sibling of ble_out_noteoff_overflow_test.py (#600) and midi_mq_rt_overflow_test.py.
Extracts production ble_out (+ keep helpers) from ble.c. Notify backpressure must
not strand realtime transport into Loopy over BLE.
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


helpers = (
    function(b, "ble_out_is_note_off")
    + "\n"
    + function(b, "ble_out_is_rt_transport")
    + "\n"
    + function(b, "ble_out_pkt_is_keep")
)

src = (
    "#include <stdint.h>\n#include <stdio.h>\n"
    "#define RING_PUBLISH() ((void)0)\n"
    "#define FM1_IRQ_TIMER5 63u\n"
    "static uint32_t fm1_irq_mask_save(unsigned n) { (void)n; return 0; }\n"
    "static void fm1_irq_mask_restore(unsigned n, uint32_t s) { (void)n; (void)s; }\n"
    "#define BLE_OQ " + re.search(r"^#define BLE_OQ (\d+)u", b, re.M).group(1) + "u\n"
    "static uint32_t ble_oq[BLE_OQ];\n"
    "static struct { volatile uint32_t ow, orr; union { int h; } host; } ble_r;\n"
    "static int ble_central_role, ble_host_on = 1, ble_want = 1;\n"
    "static int smc_ble_role_get(void) { return 0; }\n"
    "static int bleh_notifying(void *h) { (void)h; return 1; }\n"
    + helpers
    + "\n"
    + function(b, "ble_out")
    + r"""
static uint32_t mk(unsigned cin, unsigned st, unsigned d1, unsigned d2)
{
    return (cin & 15u) | (st << 8) | (d1 << 16) | (d2 << 24);
}

static int count_rt(unsigned st)
{
    unsigned n = 0;
    for (uint32_t i = ble_r.orr; i != ble_r.ow; i++) {
        uint32_t p = ble_oq[i % BLE_OQ];
        if ((p & 0xFu) == 0xFu && ((p >> 8) & 0xFFu) == st)
            n++;
    }
    return (int)n;
}

static int count_note_off(unsigned note)
{
    unsigned n = 0;
    for (uint32_t i = ble_r.orr; i != ble_r.ow; i++) {
        uint32_t p = ble_oq[i % BLE_OQ];
        if ((p & 0xFu) == 8u && ((p >> 16) & 0x7Fu) == note)
            n++;
    }
    return (int)n;
}

static int expect_rt_kept(unsigned st)
{
    ble_r.ow = ble_r.orr = 0;
    for (unsigned i = 0; i < BLE_OQ; i++)
        ble_out(mk(0xB, 0xB0, 16, i & 127u));
    ble_out(mk(0xF, st, 0, 0));
    if (ble_r.ow - ble_r.orr != BLE_OQ)
        return 13;
    if (count_rt(st) != 1)
        return 14;
    return 0;
}

static int expect_rt_does_not_displace_note_off(void)
{
    ble_r.ow = ble_r.orr = 0;
    ble_out(mk(8, 0x80, 60, 0x7F));
    for (unsigned i = 0; i < BLE_OQ - 1u; i++)
        ble_out(mk(0xB, 0xB0, 1, i & 127u));
    ble_out(mk(0xF, 0xFC, 0, 0));
    if (count_note_off(60) != 1)
        return 24;
    if (count_rt(0xFC) != 1)
        return 25;
    return 0;
}

static int expect_clock_still_drops(void)
{
    ble_r.ow = ble_r.orr = 0;
    for (unsigned i = 0; i < BLE_OQ; i++)
        ble_out(mk(0xB, 0xB0, 7, i & 127u));
    uint32_t before = ble_r.ow;
    ble_out(mk(0xF, 0xF8, 0, 0));
    if (ble_r.ow != before)
        return 32;
    if (count_rt(0xF8) != 0)
        return 33;
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = expect_rt_kept(0xFC)) != 0) {
        printf("FAIL ble stop-kept rc=%d depth=%u stops=%d\n",
               rc, (unsigned)(ble_r.ow - ble_r.orr), count_rt(0xFC));
        return 1;
    }
    if ((rc = expect_rt_kept(0xFA)) != 0) {
        printf("FAIL ble start-kept rc=%d\n", rc);
        return 2;
    }
    if ((rc = expect_rt_kept(0xFB)) != 0) {
        printf("FAIL ble continue-kept rc=%d\n", rc);
        return 3;
    }
    if ((rc = expect_rt_does_not_displace_note_off()) != 0) {
        printf("FAIL ble rt-vs-noteoff rc=%d\n", rc);
        return 4;
    }
    if ((rc = expect_clock_still_drops()) != 0) {
        printf("FAIL ble clock-drop rc=%d\n", rc);
        return 5;
    }
    puts("ok");
    return 0;
}
"""
)
with tempfile.TemporaryDirectory(prefix="x0x-ble-out-rt-") as d:
    cpath = Path(d) / "t.c"
    cpath.write_text(src)
    exe = Path(d) / "t"
    subprocess.check_call(
        ["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", str(cpath), "-o", str(exe)]
    )
    raise SystemExit(subprocess.call([str(exe)]))
