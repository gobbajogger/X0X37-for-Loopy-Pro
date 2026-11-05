#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Full MIDI-out MQ must retain Note Off under a CC flood.

Extracts production midi_out_event (+ shared ring helper) from usb.c. A queued
Note On plus a full ring of CCs must still accept Note Off by displacing a CC.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
u = (ROOT / "firmware/src/usb.c").read_text()


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


# Shared Note Off predicate + ring put (production names).
start = u.index("/* CIN 8, or CIN 9 with velocity 0")
end = u.index("void midi_send_note(")
chunk = u[start:end]
# Drop midi_in_* (inbound harness owns those); keep out path + shared helpers.
chunk = re.sub(
    r"/\* Enqueue into midi_in_q\.[^*]*\*/\s*static int midi_in_put\(uint32_t pkt\)\s*\{[^}]*\}\s*",
    "",
    chunk,
    count=1,
    flags=re.S,
)

src = (
    "#include <stdint.h>\n#include <stdio.h>\n"
    "#define RING_PUBLISH() ((void)0)\n"
    "#define FM1_IRQ_TIMER5 63u\n"
    "static uint32_t fm1_irq_mask_save(unsigned n) { (void)n; return 0; }\n"
    "static void fm1_irq_mask_restore(unsigned n, uint32_t s) { (void)n; (void)s; }\n"
    "#define MQ 64u\n"
    "static uint32_t midi_out_q[MQ];\n"
    "static volatile uint32_t mo_w, mo_r;\n"
    "static struct { int config; } usb;\n"
    + chunk
    + r"""
static uint32_t mk(unsigned cin, unsigned st, unsigned d1, unsigned d2)
{
    return (cin & 15u) | (st << 8) | (d1 << 16) | (d2 << 24);
}

static int count_cin_note(unsigned cin, unsigned note)
{
    unsigned n = 0;
    for (uint32_t i = mo_r; i != mo_w; i++) {
        uint32_t p = midi_out_q[i % MQ];
        if ((p & 0xFu) == cin && ((p >> 16) & 0x7Fu) == note)
            n++;
    }
    return (int)n;
}

static int expect_note_off_kept(void)
{
    usb.config = 1;
    mo_w = mo_r = 0;
    midi_out_event(mk(9, 0x90, 60, 100));
    for (unsigned i = 0; i < MQ - 1u; i++)
        midi_out_event(mk(0xB, 0xB0, 1, i & 127u));
    midi_out_event(mk(8, 0x80, 60, 0x7F));
    if (mo_w - mo_r != MQ)
        return 14;
    if (count_cin_note(9, 60) != 1)
        return 15;
    if (count_cin_note(8, 60) != 1)
        return 16;
    return 0;
}

static int expect_vel0_note_on_kept(void)
{
    usb.config = 1;
    mo_w = mo_r = 0;
    midi_out_event(mk(9, 0x90, 61, 100));
    for (unsigned i = 0; i < MQ - 1u; i++)
        midi_out_event(mk(0xB, 0xB0, 2, i & 127u));
    midi_out_event(mk(9, 0x90, 61, 0));
    if (count_cin_note(9, 61) != 2)
        return 24;
    return 0;
}

static int expect_cc_still_drops(void)
{
    usb.config = 1;
    mo_w = mo_r = 0;
    for (unsigned i = 0; i < MQ; i++)
        midi_out_event(mk(0xB, 0xB0, 7, i & 127u));
    uint32_t before = mo_w;
    midi_out_event(mk(0xB, 0xB0, 7, 0x55));
    if (mo_w != before)
        return 32;
    if (mo_w - mo_r != MQ)
        return 33;
    for (uint32_t i = mo_r; i != mo_w; i++) {
        if (((midi_out_q[i % MQ] >> 24) & 0x7Fu) == 0x55u)
            return 34;
    }
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = expect_note_off_kept()) != 0) {
        printf("FAIL note-off-kept rc=%d depth=%u on=%d off=%d\n",
               rc, (unsigned)(mo_w - mo_r), count_cin_note(9, 60), count_cin_note(8, 60));
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
with tempfile.TemporaryDirectory(prefix="x0x-midi-out-noteoff-") as d:
    cpath = Path(d) / "t.c"
    cpath.write_text(src)
    exe = Path(d) / "t"
    subprocess.check_call(
        ["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", str(cpath), "-o", str(exe)]
    )
    raise SystemExit(subprocess.call([str(exe)]))
