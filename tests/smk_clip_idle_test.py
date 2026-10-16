#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""CLP-05: idle clip squares on the LCD. Loopy draws a clip that does not play (and an empty slot, LIM-03)
in the darkest shade v of an Akai group (v = 7, 11, ... 59, v % 4 == 3). The LCD draws half of the full
shade v - 2: each 8-bit Akai channel / 2 (round half up), then RGB565 (tools/apc_colour_model.py to565).
All other velocities (0-6, the other shades, 60-127, also 63) draw APC40_LCD565[v], byte-exact.
The test builds the firmware helper (app/smk_clip_colour.h) on the host and checks all 128 velocities."""
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "firmware/src/app"
sys.path.insert(0, str(ROOT / "tools"))
import apc_colour_model as m  # noqa: E402

fails = 0


def check(ok, msg):
    global fails
    fails += not ok
    print(("PASS " if ok else "FAIL ") + msg)


C = r'''#include <stdio.h>
#include <stdint.h>
#include "app/apc40_lcd565.h"
#include "app/apc40_lcd_idle.h"
#include "app/smk_clip_colour.h"
int main(void)
{
    unsigned v;
    for (v = 0; v < 128u; v++)
        printf("lcd %u %u\n", v, (unsigned)smk_clip_lcd565(v));
    return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    c, exe = Path(d) / "t.c", Path(d) / "t"
    c.write_text(C)
    r = subprocess.run(["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", f"-I{ROOT / 'firmware/src'}",
                        "-o", str(exe), str(c)], capture_output=True, text=True)
    check(r.returncode == 0, "host build of app/smk_clip_colour.h" + ("" if not r.returncode else ": " + r.stderr[:300]))
    out = subprocess.run([str(exe)], capture_output=True, text=True).stdout if r.returncode == 0 else ""
    gen = Path(d) / "apc40_lcd_idle.h"
    g = subprocess.run([sys.executable, str(ROOT / "tools/apc_pad_colors.py"), "--lcd-idle-h", str(gen)],
                       capture_output=True, text=True)
    idle_h = APP / "apc40_lcd_idle.h"
    check(g.returncode == 0 and idle_h.exists() and gen.read_text() == idle_h.read_text(),
          "app/apc40_lcd_idle.h == tools/apc_pad_colors.py --lcd-idle-h")

lcd = {int(v): int(c) for v, c in re.findall(r"^lcd (\d+) (\d+)$", out, re.M)}
tab = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{4})", re.search(
    r"APC40_LCD565\[128\]\s*=\s*\{(.*?)\};", (APP / "apc40_lcd565.h").read_text(), re.S).group(1))]
src = [e["apc"] for e in m.load_source()]
IDLE = list(range(7, 60, 4))
check(len(lcd) == 128, "128 velocities")
bad = []
for v in IDLE:
    full = m.split(src[v - 2])
    half = tuple((x + 1) // 2 for x in full)
    want = m.to565(*half)
    if lcd.get(v) != want:
        bad.append(v)
    print(f"  vel {v:2d}: Akai {src[v]:06X} -> half of {v - 2:2d} ({src[v - 2]:06X}) = {''.join('%02X' % x for x in half)}"
          f" -> 0x{want:04X} (was 0x{tab[v]:04X})")
check(not bad, f"idle shades {IDLE[0]}-{IDLE[-1]}: LCD == half of the group full shade, per channel {bad}")
same = [v for v in range(128) if v not in IDLE and lcd.get(v) != tab[v]]
check(not same, "all other velocities (0-6, other shades, 60-127, 63) == APC40_LCD565[v], byte-exact")
check(src[63] != 0 and not (63 - 2 in range(4, 60)), "63 is not in an Akai shade group (groups are 4-59): no change")

main = (APP / "main_fm1.c").read_text()
draw = main.split("uint8_t smk_clip_lcd_draw(uint32_t w, uint32_t h)\n{")[1].split("\n}\n")[0]
check("uint16_t c = smk_clip_lcd565(v);" in draw and "APC40_LCD565[" not in draw,
      "smk_clip_lcd_draw draws smk_clip_lcd565(v)")
print(f"smk-clip-idle: {fails} fail(s)")
sys.exit(1 if fails else 0)
