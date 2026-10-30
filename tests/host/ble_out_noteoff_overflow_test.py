#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Full BLE MIDI-out mirror must retain Note Off under a CC flood.

Extracts production ble_out (+ Note Off helper) from ble.c against a synthetic
BLE_OQ ring. Notify backpressure must not strand a release into Loopy.
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
    "#define FM1_IRQ_TIMER5 63u\n"
    "static uint32_t fm1_irq_mask_save(unsigned n) { (void)n; return 0; }\n"
    "static void fm1_irq_mask_restore(unsigned n, uint32_t s) { (void)n; (void)s; }\n"
    "#define BLE_OQ " + re.search(r"^#define BLE_OQ (\d+)u", b, re.M).group(1) + "u\n"
    "static uint32_t ble_oq[BLE_OQ];\n"
    "static struct { volatile uint32_t ow, orr; union { int h; } host; } ble_r;\n"
    "static int ble_central_role, ble_host_on = 1, ble_want = 1;\n"
    "static int smc_ble_role_get(void) { return 0; }\n"
    "static int bleh_notifying(void *h) { (void)h; return 1; }\n"
    + function(b, "ble_out_is_note_off")
    + "\n"
    + function(b, "ble_out_is_rt_transport")
    + "\n"
    + function(b, "ble_out_pkt_is_keep")
    + "\n"
    + function(b, "ble_out")
    + r"""
static uint32_t mk(unsigned cin, unsigned st, unsigned d1, unsigned d2)
{
    return (cin & 15u) | (st << 8) | (d1 << 16) | (d2 << 24);
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

static int expect_note_off_kept(void)
{
    ble_r.ow = ble_r.orr = 0;
    ble_out(mk(9, 0x90, 60, 100));
    for (unsigned i = 0; i < BLE_OQ - 1u; i++)
        ble_out(mk(0xB, 0xB0, 1, i & 127u));
    ble_out(mk(8, 0x80, 60, 0x7F));
    if (ble_r.ow - ble_r.orr != BLE_OQ)
        return 14;
    if (count_cin_note(9, 60) != 1)
        return 15;
    if (count_cin_note(8, 60) != 1)
        return 16;
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
    if ((rc = expect_note_off_kept()) != 0) {
        printf("FAIL ble note-off-kept rc=%d depth=%u on=%d off=%d\n",
               rc, (unsigned)(ble_r.ow - ble_r.orr),
               count_cin_note(9, 60), count_cin_note(8, 60));
        return 1;
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
with tempfile.TemporaryDirectory(prefix="x0x-ble-out-noteoff-") as d:
    cpath = Path(d) / "t.c"
    cpath.write_text(src)
    exe = Path(d) / "t"
    subprocess.check_call(
        ["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", str(cpath), "-o", str(exe)]
    )
    raise SystemExit(subprocess.call([str(exe)]))
