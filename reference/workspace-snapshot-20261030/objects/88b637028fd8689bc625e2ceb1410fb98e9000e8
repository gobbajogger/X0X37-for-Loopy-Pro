#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""a76 (a74 owner test 1, VIOLET FAIL): violet, purple and pink clips. Since 2026-10-17 the owner
calibrates these colours by eye (PAD COLOURS, CLP-79), so the hue rules of a76 are information only.

The pad LED bytes are APC40_PAD_OWNER[v] * 255 / 510 (main_fm1.c smk_pads; the idle shade 51: * 255 / 255,
CLP-13). There is no gamma.
The channel (steady, blink, pulse) changes only the brightness, not the colour.
Rule now: each violet, purple and pink velocity is the owner value of the export of 2026-10-17
(tools/padcal/pads-owner-20261017.txt): the value marked ok, or the start value that the owner saw and kept.
The test prints the pad hue of each entry and the a76 range (265-290 degrees) for information.
Loopy Pro capture: playing clips are 0x60 (96). 96 is a Live clip colour index (60-119), so
Loopy can send the Live violets 69, 80, 81, 93, 116 for a violet clip, not only 48-51.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
src = (ROOT / "firmware/src/app/apc40_pad_owner.h").read_text()
m = re.search(r"APC40_PAD_OWNER\[128\]\[3\]\s*=\s*\{(.*?)\};", src, re.S)
pal = [int(r) << 16 | int(g) << 8 | int(b) for r, g, b in re.findall(r"\{(\d+),(\d+),(\d+)\}", m.group(1))]
assert len(pal) == 128
seen = {}
for line in (ROOT / "tools/padcal/pads-owner-20261017.txt").read_text().splitlines():
    f = line.split()
    if len(f) == 6 and f[0] == "padcal":
        seen[int(f[1])] = (int(f[2]) << 16 | int(f[3]) << 8 | int(f[4]), f[5])

def pad(rgb, bright=255):
    return tuple(((rgb >> s) & 0xFF) * bright // 510 for s in (16, 8, 0))


def hue(c):
    r, g, b = c
    mx, mn = max(r, g, b), min(r, g, b)
    if mx == mn:
        return None
    if mx == r:
        return (60.0 * (g - b) / (mx - mn)) % 360
    if mx == g:
        return 120.0 + 60.0 * (b - r) / (mx - mn)
    return 240.0 + 60.0 * (r - g) / (mx - mn)


# Violet in the protocol v1.2: APC colour hue 250-290, or Live colour hue 250-295 (Name column).
VIOLET = {48: "874CFF", 49: "5400FF", 50: "190064", 51: "0F0030",
          69: "2500CC (Live 624BAD violet)", 80: "3F00FF (Live 886CE4 lavender)",
          93: "877FE9 (Live B88DFF)", 116: "8E66FF (Live CDBBE4)"}
# Purple and pink-purple (owner t82u: Loopy pink looks purple): red near blue or above it.
PURPLE = {81: ("7A00FF (Live A34BAD purple)", 285.0, 300.0), 94: ("D31DFF (Live D86CE4 orchid)", 300.0, 330.0)}
fails = 0
for v, spec in list(VIOLET.items()) + [(v, sp) for v, (sp, _, _) in PURPLE.items()]:
    led = pad(pal[v])
    h = hue(led)
    lo, hi = PURPLE[v][1:] if v in PURPLE else (265.0, 290.0)
    ok = pal[v] == seen[v][0]
    fails += not ok
    hs = "-" if h is None else f"{h:.1f}"
    print(f"{'PASS' if ok else 'FAIL'} vel {v:3d} (0x{v:02X}) spec {spec:28s} table {pal[v]:06X} "
          f"owner {seen[v][0]:06X} ({'ok' if seen[v][1] == 'ok' else 'start value kept'}) pad {led} hue {hs} "
          f"(a76 range {lo:.0f}-{hi:.0f}: {'in' if h is not None and lo <= h <= hi else 'out'}, information)")
# Blink and pulse: the off phase is 0, the on phase is 255; the hue is the same at each level.
for v in list(VIOLET) + list(PURPLE):
    assert pad(pal[v], 0) == (0, 0, 0)
print(f"{'FAIL' if fails else 'PASS'} {fails} violet, purple or pink velocities differ from the owner export")
sys.exit(1 if fails else 0)
