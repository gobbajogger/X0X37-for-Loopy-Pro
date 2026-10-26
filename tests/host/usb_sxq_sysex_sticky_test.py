#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Full USB SysEx out SXQ must keep APC UDI/intro via sticky retry.

When SXQ is saturated (OTA/editor SysEx flood), midi_send_sysex refuses a whole
handshake reply so framing never splits. Without a sticky pending kind the host
never sees identity / mkII introduction. Mirrors ble.c ble_sx_reply: stash the
reply kind, retry after space frees (usb_poll after ep1_tx).
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
u = (ROOT / "firmware/src/usb.c").read_text()

# Sticky state + midi_send_sysex through apc40_introduction_reply.
start = u.index("/* Sticky APC handshake reply")
end = u.index("/* ------------------------------------------------------- descriptors --- */")
chunk = u[start:end]
# Keep only the FELUCCA_OTA (SXQ) body used on SMK production builds.
chunk = re.sub(
    r"#if FELUCCA_OTA\n(.*?)\n#else\n.*?\n#endif\n",
    r"\1\n",
    chunk,
    count=1,
    flags=re.S,
)

src = (
    "#include <stdint.h>\n#include <stdio.h>\n#include <string.h>\n"
    "#define RING_PUBLISH() ((void)0)\n"
    "#define SXQ 64u\n"
    "static uint32_t sx_out_q[SXQ];\n"
    "static volatile uint32_t so_w, so_r;\n"
    "static volatile uint8_t sx_busy;\n"
    "static struct { int config; uint8_t sysex[64]; } usb;\n"
    + chunk
    + r"""
static unsigned count_f0(void)
{
    unsigned n = 0;
    for (uint32_t i = so_r; i != so_w; i++) {
        uint32_t p = sx_out_q[i % SXQ];
        if (((p >> 8) & 0xFFu) == 0xF0u)
            n++;
    }
    return n;
}

static int fill_sxq(void)
{
    uint8_t tiny[] = {0xF7};
    so_w = so_r = 0;
    sx_busy = 0;
    usb_sx_reply = 0;
    for (unsigned i = 0; i < SXQ; i++) {
        if (midi_send_sysex(tiny, 1) != 0)
            return 1;
    }
    return (so_w - so_r == SXQ) ? 0 : 2;
}

static int expect_sticky_under_full(void)
{
    uint32_t before;
    if (fill_sxq())
        return 10;
    before = so_w;
    apc40_device_inquiry(0x7F);
    if (usb_sx_reply != 1u)
        return 11;
    if (so_w != before)
        return 12;
    /* Free 12 slots (identity needs 12 packets) then retry. */
    so_r += 12u;
    usb_sx_try_send();
    if (usb_sx_reply != 0u)
        return 13;
    if (so_w - before != 12u)
        return 14;
    if (count_f0() < 1u)
        return 15;
    return 0;
}

static int expect_intro_sticky(void)
{
    uint32_t before;
    if (fill_sxq())
        return 20;
    before = so_w;
    apc40_introduction_reply();
    if (usb_sx_reply != 2u)
        return 21;
    if (so_w != before)
        return 22;
    so_r += 6u; /* 17-byte intro → 6 packets */
    usb_sx_try_send();
    if (usb_sx_reply != 0u)
        return 23;
    if (so_w - before != 6u)
        return 24;
    return 0;
}

static int expect_room_sends_now(void)
{
    so_w = so_r = 0;
    sx_busy = 0;
    usb_sx_reply = 0;
    apc40_device_inquiry(0);
    if (usb_sx_reply != 0u)
        return 30;
    if (so_w - so_r != 12u)
        return 31;
    return 0;
}

static int expect_sx_busy_keeps_sticky(void)
{
    so_w = so_r = 0;
    sx_busy = 1;
    usb_sx_reply = 0;
    apc40_device_inquiry(1);
    if (usb_sx_reply != 1u)
        return 40;
    if (so_w != 0u)
        return 41;
    sx_busy = 0;
    usb_sx_try_send();
    if (usb_sx_reply != 0u)
        return 42;
    if (so_w - so_r != 12u)
        return 43;
    return 0;
}

int main(void)
{
    int rc;
    usb.config = 1;
    if ((rc = expect_sticky_under_full()) != 0) {
        printf("FAIL sticky_full rc=%d depth=%u reply=%u\n",
               rc, (unsigned)(so_w - so_r), usb_sx_reply);
        return rc;
    }
    if ((rc = expect_intro_sticky()) != 0) {
        printf("FAIL intro_sticky rc=%d reply=%u\n", rc, usb_sx_reply);
        return rc;
    }
    if ((rc = expect_room_sends_now()) != 0) {
        printf("FAIL room_now rc=%d depth=%u\n", rc, (unsigned)(so_w - so_r));
        return rc;
    }
    if ((rc = expect_sx_busy_keeps_sticky()) != 0) {
        printf("FAIL sx_busy rc=%d reply=%u\n", rc, usb_sx_reply);
        return rc;
    }
    printf("PASS usb-sxq-sysex-sticky\n");
    return 0;
}
"""
)

d = tempfile.mkdtemp()
(Path(d) / "t.c").write_text(src)
subprocess.check_call(
    ["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", d + "/t.c", "-o", d + "/t"]
)
raise SystemExit(subprocess.call([d + "/t"]))
