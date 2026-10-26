#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Full MIDI-out MQ must retain Pitch Bend under a CC flood.

Outbound sibling of midi_mq_pb_overflow_test.py. Mackie fader feedback must
not strand when the out ring is full of CCs. Note Off still displaces PB.
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


start = u.index("/* CIN 8, or CIN 9 with velocity 0")
end = u.index("void midi_send_note(")
chunk = u[start:end]
chunk = re.sub(
    r"/\* Enqueue into midi_in_q\..*?\*/\s*static int midi_in_put\(uint32_t pkt\)\s*\{[^}]*\}\s*",
    "",
    chunk,
    count=1,
    flags=re.S,
)

src = (
    "#include <stdint.h>\n#include <stdio.h>\n"
    "#define RING_PUBLISH() ((void)0)\n"
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

static int count_pb(unsigned ch, unsigned d2)
{
    unsigned n = 0;
    for (uint32_t i = mo_r; i != mo_w; i++) {
        uint32_t p = midi_out_q[i % MQ];
        if ((p & 0xFu) == 0xEu && ((p >> 8) & 0x0Fu) == ch && ((p >> 24) & 0x7Fu) == d2)
            n++;
    }
    return (int)n;
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

static int expect_pb_kept(void)
{
    usb.config = 1;
    mo_w = mo_r = 0;
    for (unsigned i = 0; i < MQ; i++)
        midi_out_event(mk(0xB, 0xB0, 1, i & 127u));
    midi_out_event(mk(0xE, 0xE0, 0, 0x55));
    if (mo_w - mo_r != MQ)
        return 14;
    if (count_pb(0, 0x55) != 1)
        return 15;
    return 0;
}

static int expect_note_off_over_pb(void)
{
    usb.config = 1;
    mo_w = mo_r = 0;
    for (unsigned i = 0; i < MQ; i++)
        midi_out_event(mk(0xE, 0xE0 + (i & 15u), 0, i & 127u));
    midi_out_event(mk(8, 0x80, 60, 0x7F));
    if (count_cin_note(8, 60) != 1)
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
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = expect_pb_kept()) != 0) {
        printf("FAIL out pb-kept rc=%d depth=%u pb=%d\n",
               rc, (unsigned)(mo_w - mo_r), count_pb(0, 0x55));
        return 1;
    }
    if ((rc = expect_note_off_over_pb()) != 0) {
        printf("FAIL out note-off-over-pb rc=%d\n", rc);
        return 2;
    }
    if ((rc = expect_cc_still_drops()) != 0) {
        printf("FAIL out cc-drop rc=%d\n", rc);
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
