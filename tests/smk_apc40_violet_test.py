#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""a76 (a74 owner test 1, VIOLET FAIL): violet, purple and pink clips show distinct colours.

The pad LED bytes are APC40_RGB[v] * 255 / 510 (main_fm1.c smk_pads). There is no gamma.
The channel (steady, blink, pulse) changes only the brightness, not the colour.
Rule: each APC40 mkII velocity that is violet in the protocol v1.2 (the APC colour or the
Live clip colour in the Name column) must give a violet pad hue (265-290 degrees).
Blue and magenta must not change.
Loopy Pro capture: playing clips are 0x60 (96). 96 is a Live clip colour index (60-119), so
Loopy can send the Live violets 69, 80, 81, 93, 116 for a violet clip, not only 48-51.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
src = (ROOT / "firmware/src/app/main_fm1.c").read_text()
m = re.search(r"APC40_RGB\[128\]\s*=\s*\{(.*?)\};", src, re.S)
pal = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{6})", m.group(1))]
assert len(pal) == 128


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
for v, spec in VIOLET.items():
    led = pad(pal[v])
    h = hue(led)
    ok = h is not None and 265.0 <= h <= 290.0
    fails += not ok
    print(f"{'PASS' if ok else 'FAIL'} vel {v:3d} (0x{v:02X}) spec {spec:22s} table {pal[v]:06X} pad {led} hue {h:.1f}")
for v, (spec, lo, hi) in PURPLE.items():
    led = pad(pal[v])
    h = hue(led)
    ok = h is not None and lo <= h <= hi and led[0] >= 0.55 * led[2]
    fails += not ok
    print(f"{'PASS' if ok else 'FAIL'} vel {v:3d} (0x{v:02X}) spec {spec:22s} table {pal[v]:06X} pad {led} hue {h:.1f} (want {lo:.0f}-{hi:.0f})")
# Red clearly present (R >= 0.55 x B) for each violet, purple and pink index.
for v in list(VIOLET) + list(PURPLE):
    r, g, b = pad(pal[v])
    if r < 0.55 * b:
        fails += 1
        print(f"FAIL vel {v}: red {r} < 0.55 x blue {b} (blue-dominant)")
# Distinct: violet 69/80, purple 81 and orchid 94 differ by >= 10 degrees of pad hue.
hs = {v: hue(pad(pal[v])) for v in (80, 81, 94)}
for a, b in ((80, 81), (81, 94)):
    if abs(hs[a] - hs[b]) < 10.0:
        fails += 1
        print(f"FAIL vel {a} and {b} are not distinct on the pads ({hs[a]:.1f}, {hs[b]:.1f})")
# Blink and pulse: the off phase is 0, the on phase is 255; the hue is the same at each level.
for v in VIOLET:
    assert pad(pal[v], 0) == (0, 0, 0)
# No regression: blue, magenta and 94 (already violet-magenta) stay as the protocol gives them.
assert pal[44:48] == [0x4C4CFF, 0x0000FF, 0x000059, 0x000019], "blue group changed"
assert pal[52:56] == [0xFF4CFF, 0xFF00FF, 0x590059, 0x190019], "magenta group changed"
assert pal[67] == 0x0000FF and pal[79] == 0x002AFF and pal[92] == 0x3151C6 and pal[115] == 0x9A99FF, "blue entry changed"
print(f"{'FAIL' if fails else 'PASS'} {fails} violet velocities do not show violet on the pads")
sys.exit(1 if fails else 0)
