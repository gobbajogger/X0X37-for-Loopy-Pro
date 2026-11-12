#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""CLP-80: ui.c draw_main shows the waiting screen, not the clip grid, while smk_clip_wait() is 1
(tests/host/smk_clip_wait_test.c checks smk_clip_wait). Text check of the branch."""
import sys
from pathlib import Path

ui = (Path(__file__).resolve().parents[2] / "firmware/src/app/ui.c").read_text()
d0 = ui.index("static UI_DRAW void draw_main(void)")
dm = ui[d0:ui.index("\n}\n", d0)]
fails = 0


def check(ok, msg):
    global fails
    fails += not ok
    print(("PASS " if ok else "FAIL ") + msg)


w = dm.find("smk_clip_wait()")
g = dm.find("smk_clip_lcd_draw(240u")
br = dm[w:g] if 0 <= w < g else ""
check(bool(br), "draw_main: the waiting branch comes before the clip grid")
check('"WAITING FOR"' in br and '"LOOPY PRO"' in br, "waiting screen: WAITING FOR / LOOPY PRO")
check("return;" in br and "smk_clip_lcd_draw" not in br, "waiting screen: no grid (returns before smk_clip_lcd_draw)")
check("smk_clip_lcd_force();" in br, "the grid draws at once when a host comes (smk_clip_lcd_force)")
check("clip_wait_on = 0u;" in dm[g - 400:g] if g > 0 else False, "a menu or the grid resets the waiting screen flag")
check("smk_clip_mode_on() && ui.overlay == O_NONE && !ui.help && smk_clip_wait()" in br or
      "smk_clip_mode_on() && ui.overlay == O_NONE && !ui.help && smk_clip_wait()" in dm,
      "only in clip mode with no menu and no help")
print(f"smk_clip_wait_test.py: {fails} fail(s)")
sys.exit(1 if fails else 0)
