#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Full MIDI-out MQ must retain Note Off under a CC flood.

Extracts production midi_out_event / midi_send_note from usb.c. A queued Note On
plus a full ring of CCs must still accept Note Off by displacing a CC; plain
Note Ons / CCs may still drop when full.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
u = (ROOT / "firmware/src/usb.c").read_text()
# Shared Note Off classifier + both rings; strip midi_in_put (unused here).
start = u.index("/* CIN 8, or CIN 9 with velocity 0")
end = u.index("/* Send a complete SysEx message")
chunk = u[start:end]
# Keep only out-path symbols for this harness.
mq_line = [ln for ln in u.splitlines() if ln.strip().startswith("#define MQ")][0]
# Drop midi_in_put body by cutting from its comment through midi_out_event.
i_in = chunk.index("/* Enqueue into midi_in_q.")
i_out = chunk.index("/* Enqueue into midi_out_q.")
chunk = chunk[:i_in] + chunk[i_out:]
src = (
    "#include <stdint.h>\n#include <stdio.h>\n"
    "#define RING_PUBLISH() ((void)0)\n"
    + mq_line
    + "\n"
    "struct { int config; } usb = {1};\n"
    "static uint32_t midi_out_q[MQ];\n"
    "static volatile uint32_t mo_w, mo_r;\n"
    + chunk
    + r"""
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
    mo_w = mo_r = 0;
    midi_send_note(60, 100);
    for (unsigned i = 0; i < MQ - 1u; i++)
        midi_out_event(0xBu | (0xB0u << 8) | (1u << 16) | ((i & 127u) << 24));
    if (mo_w - mo_r != MQ)
        return 11;
    midi_send_note(60, 0);
    if (mo_w - mo_r != MQ)
        return 12;
    if (count_cin_note(9, 60) != 1)
        return 13;
    if (count_cin_note(8, 60) != 1)
        return 14;
    return 0;
}

static int expect_vel0_note_on_kept(void)
{
    mo_w = mo_r = 0;
    midi_send_note(61, 100);
    for (unsigned i = 0; i < MQ - 1u; i++)
        midi_out_event(0xBu | (0xB0u << 8) | (2u << 16) | ((i & 127u) << 24));
    /* Note On velocity 0 is a release (midi_send_note uses CIN 8 for vel 0,
     * so also probe raw CIN 9 vel 0 via midi_out_event). */
    midi_out_event(0x9u | (0x90u << 8) | (61u << 16) | (0u << 24));
    if (count_cin_note(9, 61) != 2)
        return 21;
    return 0;
}

static int expect_cc_still_drops(void)
{
    mo_w = mo_r = 0;
    for (unsigned i = 0; i < MQ; i++)
        midi_out_event(0xBu | (0xB0u << 8) | (7u << 16) | ((i & 127u) << 24));
    uint32_t before = mo_w;
    midi_out_event(0xBu | (0xB0u << 8) | (7u << 16) | (0x55u << 24));
    if (mo_w != before)
        return 31;
    for (uint32_t i = mo_r; i != mo_w; i++) {
        if (((midi_out_q[i % MQ] >> 24) & 0x7Fu) == 0x55u)
            return 32;
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
with tempfile.TemporaryDirectory(prefix="x0x-mq-out-noteoff-") as d:
    cpath = Path(d) / "t.c"
    cpath.write_text(src)
    exe = Path(d) / "t"
    subprocess.check_call(
        ["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", str(cpath), "-o", str(exe)]
    )
    raise SystemExit(subprocess.call([str(exe)]))
