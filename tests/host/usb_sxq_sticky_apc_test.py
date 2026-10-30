#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Full USB SysEx ring must still deliver sticky APC handshake replies.

Mirrors production midi_send_sysex + usb_sx_answer (FELUCCA_OTA SXQ path).
A Device Inquiry refused while SXQ is full must enqueue once space frees —
same class as ble_sx_reply, opposite of a silent one-shot drop.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
u = (ROOT / "firmware/src/usb.c").read_text()
start = u.index("/* Send a complete SysEx message through the same USB-MIDI event queue")
end = u.index("/* ------------------------------------------------------- descriptors --- */")
chunk = u[start:end]
# Keep only the FELUCCA_OTA SysEx ring path used on SMK production builds.
ota = chunk.index("#if FELUCCA_OTA")
els = chunk.index("#else", ota)
endif = chunk.index("#endif", els)
chunk = chunk[:ota] + chunk[ota + len("#if FELUCCA_OTA") : els] + chunk[endif + len("#endif") :]
src = (
    "#include <stdint.h>\n#include <stdio.h>\n#include <string.h>\n"
    "#define RING_PUBLISH() ((void)0)\n"
    "#define SXQ 64u\n"
    "struct { int config; uint8_t sysex[64]; } usb;\n"
    "static volatile uint8_t sx_busy;\n"
    "static uint32_t sx_out_q[SXQ];\n"
    "static volatile uint32_t so_w, so_r;\n"
    + chunk
    + r"""
static int count_identity(void)
{
    unsigned n = 0;
    for (uint32_t i = so_r; i != so_w; i++) {
        uint32_t p = sx_out_q[i % SXQ];
        /* First packet of identity: CIN continue with F0 7E */
        if ((p & 0xFu) == 4u && ((p >> 8) & 0xFFu) == 0xF0u && ((p >> 16) & 0xFFu) == 0x7Eu)
            n++;
    }
    return (int)n;
}

static int expect_sticky_after_drain(void)
{
    uint8_t tiny[1] = {0xF7};
    unsigned i;
    usb.config = 1;
    usb_sx_reply = 0;
    so_w = so_r = 0;
    for (i = 0; i < SXQ; i++) {
        if (midi_send_sysex(tiny, 1) != 0)
            return 11;
    }
    apc40_device_inquiry(0);
    if (usb_sx_reply != 1u)
        return 12; /* must stay sticky while full */
    if (count_identity() != 0)
        return 13;
    /* Free enough slots for the 35-byte identity (12 USB-MIDI packets). */
    so_r += 12u;
    usb_sx_answer();
    if (usb_sx_reply != 0u)
        return 14;
    if (count_identity() != 1)
        return 15;
    return 0;
}

static int expect_immediate_when_room(void)
{
    usb.config = 1;
    usb_sx_reply = 0;
    so_w = so_r = 0;
    apc40_introduction_reply();
    if (usb_sx_reply != 0u)
        return 21;
    if (so_w - so_r == 0u)
        return 22;
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = expect_sticky_after_drain()) != 0) {
        printf("FAIL sticky-drain rc=%d reply=%u depth=%u id=%d\n",
               rc, (unsigned)usb_sx_reply, (unsigned)(so_w - so_r), count_identity());
        return 1;
    }
    if ((rc = expect_immediate_when_room()) != 0) {
        printf("FAIL immediate rc=%d reply=%u depth=%u\n",
               rc, (unsigned)usb_sx_reply, (unsigned)(so_w - so_r));
        return 2;
    }
    puts("ok");
    return 0;
}
"""
)
with tempfile.TemporaryDirectory(prefix="x0x-usb-sxq-sticky-") as d:
    cpath = Path(d) / "t.c"
    cpath.write_text(src)
    exe = Path(d) / "t"
    subprocess.check_call(
        ["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", str(cpath), "-o", str(exe)]
    )
    raise SystemExit(subprocess.call([str(exe)]))
