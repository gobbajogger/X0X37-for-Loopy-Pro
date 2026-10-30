#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""CLP-13: idle clip pads. For the darkest shade v of an Akai group (v = 7, 11, ... 59, v % 4 == 3, a clip
that does not play or an empty slot, LIM-03) a lit pad sends the full owner table level
(APC40_PAD_OWNER[v] x brightness / 255): 2 x the old level / 2. All other velocities send
level x brightness / 510, byte-exact as before. Brightness 0 (the off half of a blink) sends 0.
The test builds the firmware helper (app/smk_clip_colour.h) on the host and checks all 128 x R, G, B."""
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "firmware/src/app"
fails = 0


def check(ok, msg):
    global fails
    fails += not ok
    print(("PASS " if ok else "FAIL ") + msg)


C = r'''#include <stdio.h>
#include <stdint.h>
#include "app/apc40_lcd565.h"
#include "app/apc40_lcd_idle.h"
#include "app/apc40_pad_owner.h"
#include "app/smk_clip_colour.h"
int main(void)
{
    unsigned v, k, b;
    for (b = 0; b < 2u; b++)
        for (v = 0; v < 128u; v++)
            for (k = 0; k < 3u; k++)
                printf("pad %u %u %u %u\n", b ? 255u : 0u, v, k,
                       (unsigned)smk_clip_pad_level(APC40_PAD_OWNER[v][k], v, b ? 255u : 0u));
    return 0;
}
'''
out = ""
with tempfile.TemporaryDirectory() as d:
    c, exe = Path(d) / "t.c", Path(d) / "t"
    c.write_text(C)
    r = subprocess.run(["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", f"-I{ROOT / 'firmware/src'}",
                        "-o", str(exe), str(c)], capture_output=True, text=True)
    check(r.returncode == 0, "host build of smk_clip_pad_level" + ("" if not r.returncode else ": " + r.stderr[:300]))
    if r.returncode == 0:
        out = subprocess.run([str(exe)], capture_output=True, text=True).stdout

t = re.search(r"APC40_PAD_OWNER\[128\]\[3\]\s*=\s*\{(.*?)\};", (APP / "apc40_pad_owner.h").read_text(), re.S)
owner = [tuple(map(int, x)) for x in re.findall(r"\{(\d+),(\d+),(\d+)\}", t.group(1))]
got = {(int(b), int(v), int(k)): int(x) for b, v, k, x in re.findall(r"^pad (\d+) (\d+) (\d+) (\d+)$", out, re.M)}
IDLE = set(range(7, 60, 4))
check(len(got) == 2 * 128 * 3, "128 velocities x R, G, B x brightness 255 and 0")
bad_i = [v for v in IDLE for k in range(3) if got.get((255, v, k)) != owner[v][k]]
check(not bad_i, f"idle shades 7-59: the pad sends the full table level (2 x old) {sorted(set(bad_i))}")
for v in sorted(IDLE):
    old = tuple(x * 255 // 510 for x in owner[v])
    print(f"  vel {v:2d}: table {owner[v]} old {old} new {tuple(got.get((255, v, k)) for k in range(3))}")
bad_o = [v for v in range(128) if v not in IDLE for k in range(3) if got.get((255, v, k)) != owner[v][k] * 255 // 510]
check(not bad_o, "all other velocities (0-6, other shades, 60-127, 63): level x 255 / 510, byte-exact")
check(all(got.get((0, v, k)) == 0 for v in range(128) for k in range(3)), "brightness 0: all 0")
main = (APP / "main_fm1.c").read_text()
pads = main.split("static void smk_pads(void)")[1].split("\n}\n")[0]
check("smk_clip_pad_level(rgb[0], v, brightness)" in pads and "smk_clip_pad_level(rgb[1], v, brightness)" in pads
      and "smk_clip_pad_level(rgb[2], v, brightness)" in pads and "/ 510u" not in pads,
      "smk_pads sends smk_clip_pad_level of APC40_PAD_OWNER[v] R, G, B")
print(f"smk-clip-idle-pad: {fails} fail(s)")
sys.exit(1 if fails else 0)
