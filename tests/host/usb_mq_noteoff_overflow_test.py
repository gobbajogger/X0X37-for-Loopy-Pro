#!/usr/bin/env python3
"""Full USB/TRS midi_in_q must retain Note Off under CC flood.

Extracts production midi_in_* helpers from usb.c. A queued Note On plus a full
ring of CCs must still accept Note Off by displacing a disposable CC entry.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
usb = (ROOT / "firmware/src/usb.c").read_text()

# MQ ring + never-drop Note Off helpers (through end of midi_in_put).
m_mq = re.search(
    r"/\* MIDI rings: 4-byte USB-MIDI event packets \*/\n"
    r"#define MQ 64u\n"
    r"static uint32_t midi_in_q\[MQ\], midi_out_q\[MQ\];\n"
    r"static volatile uint32_t mi_w, mi_r, mo_w, mo_r;\n",
    usb,
)
if not m_mq:
    raise SystemExit("usb.c: MIDI ring block missing")
start = m_mq.start()
m_put = re.search(
    r"static int midi_in_put\(uint32_t pkt\)\s*\{",
    usb[start:],
)
if not m_put:
    raise SystemExit("usb.c: midi_in_put missing")
i = start + m_put.end()
depth = 1
while depth:
    if usb[i] == "{":
        depth += 1
    elif usb[i] == "}":
        depth -= 1
    i += 1
ring = usb[start:i]
ring = ring.replace("RING_PUBLISH()", '((void)0)')
# Host extract only needs midi_in_q; silence unused midi_out_* from the production block.
ring = re.sub(
    r"static uint32_t midi_in_q\[MQ\], midi_out_q\[MQ\];\n"
    r"static volatile uint32_t mi_w, mi_r, mo_w, mo_r;",
    "static uint32_t midi_in_q[MQ];\n"
    "static volatile uint32_t mi_w, mi_r;",
    ring,
    count=1,
)

src = (
    "#include <stdint.h>\n#include <stdio.h>\n"
    + ring
    + r"""
static uint32_t note_on(uint8_t note, uint8_t vel)
{
    return 9u | (0x90u << 8) | ((uint32_t)note << 16) | ((uint32_t)vel << 24);
}
static uint32_t note_off(uint8_t note)
{
    return 8u | (0x80u << 8) | ((uint32_t)note << 16);
}
static uint32_t cc(uint8_t n, uint8_t v)
{
    return 0xBu | (0xB0u << 8) | ((uint32_t)n << 16) | ((uint32_t)v << 24);
}

static int count_note_off_pitch(uint8_t note)
{
    unsigned n = 0;
    for (uint32_t i = mi_r; i != mi_w; i++) {
        uint32_t p = midi_in_q[i % MQ];
        if (midi_in_is_note_off(p) && ((p >> 16) & 0x7Fu) == note)
            n++;
    }
    return (int)n;
}

static int count_note_on_pitch(uint8_t note)
{
    unsigned n = 0;
    for (uint32_t i = mi_r; i != mi_w; i++) {
        uint32_t p = midi_in_q[i % MQ];
        uint32_t cin = p & 0xFu;
        if (cin == 9u && ((p >> 24) & 0x7Fu) != 0u && ((p >> 16) & 0x7Fu) == note)
            n++;
    }
    return (int)n;
}

static int expect_note_off_kept(void)
{
    mi_w = mi_r = 0;
    if (!midi_in_put(note_on(60, 100)))
        return 11;
    for (unsigned i = 0; i < MQ - 1u; i++) {
        if (!midi_in_put(cc((uint8_t)(i & 0x7Fu), (uint8_t)(i & 0x7Fu))))
            return 12;
    }
    if (mi_w - mi_r != MQ)
        return 13;
    if (!midi_in_put(note_off(60)))
        return 14;
    if (mi_w - mi_r != MQ)
        return 15;
    if (count_note_on_pitch(60) != 1)
        return 16;
    if (count_note_off_pitch(60) != 1)
        return 17;
    return 0;
}

static int expect_cc_still_drops(void)
{
    mi_w = mi_r = 0;
    for (unsigned i = 0; i < MQ; i++) {
        if (!midi_in_put(cc(1, (uint8_t)(i & 0x7Fu))))
            return 21;
    }
    if (midi_in_put(cc(2, 0x55)))
        return 22; /* must drop when full and not Note Off */
    if (mi_w - mi_r != MQ)
        return 23;
    for (uint32_t i = mi_r; i != mi_w; i++) {
        uint32_t p = midi_in_q[i % MQ];
        if ((p & 0xFu) == 0xBu && ((p >> 16) & 0x7Fu) == 2u && ((p >> 24) & 0x7Fu) == 0x55u)
            return 24;
    }
    return 0;
}

static int expect_note_on_vel0_kept(void)
{
    mi_w = mi_r = 0;
    if (!midi_in_put(note_on(61, 80)))
        return 31;
    for (unsigned i = 0; i < MQ - 1u; i++) {
        if (!midi_in_put(cc(3, (uint8_t)(i & 0x7Fu))))
            return 32;
    }
    /* Note On velocity 0 is Note Off */
    if (!midi_in_put(note_on(61, 0)))
        return 33;
    if (count_note_off_pitch(61) != 1)
        return 34;
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = expect_note_off_kept()) != 0) {
        printf("FAIL note-off-kept rc=%d depth=%u on60=%d off60=%d\n",
               rc, (unsigned)(mi_w - mi_r), count_note_on_pitch(60), count_note_off_pitch(60));
        return 1;
    }
    if ((rc = expect_cc_still_drops()) != 0) {
        printf("FAIL cc-drop rc=%d\n", rc);
        return 2;
    }
    if ((rc = expect_note_on_vel0_kept()) != 0) {
        printf("FAIL vel0-off-kept rc=%d\n", rc);
        return 3;
    }
    puts("ok");
    return 0;
}
"""
)
with tempfile.TemporaryDirectory(prefix="x0x-usb-mq-noteoff-") as d:
    cpath = Path(d) / "t.c"
    cpath.write_text(src)
    exe = Path(d) / "t"
    subprocess.check_call(
        ["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", str(cpath), "-o", str(exe)]
    )
    raise SystemExit(subprocess.call([str(exe)]))
