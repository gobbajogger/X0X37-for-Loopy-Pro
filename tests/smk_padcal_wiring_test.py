#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Pad calibration wiring (BEHAVIOR.md CLP-79): the firmware text gives the mode its menu row, its keys,
knobs, pads and LCD page, and keeps the UI away meanwhile. The logic is in tests/smk_padcal_test.c."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "firmware/src/app"
main = (APP / "main_fm1.c").read_text()
ui = (APP / "ui.c").read_text()
plat = (APP / "plat_fm1.c").read_text()
fails = 0


def check(ok, what):
    global fails
    print(("PASS " if ok else "FAIL ") + what)
    fails += not ok


def body(text, head):
    i = text.index(head)
    j = text.index("\n}\n", i)
    return text[i:j]


# The menu row: GLOBAL > SYSTEM > PAD COLOURS, SMK only, not in clip mode.
glob = body(ui, "static void open_global(void)")
sysi, bti = glob.index("GR_SYSTEM"), glob.index("GR_BT")
check(sysi < glob.index("ACT_PADCAL") < bti and "if (!smk_clip_mode_on())" in glob,
      "menu: PAD COLOURS is in GLOBAL > SYSTEM, not in clip mode")
check('{"PAD COLOURS", 0, 0, 0}' in ui and re.search(r"case ACT_PADCAL: \{[^}]*ui\.overlay = O_NONE;\s*smk_padcal_enter\(\);", ui),
      "menu: the row closes the list and starts the mode")
enter = body(main, "void smk_padcal_enter(void)")
check("if (clip_grid_mode)" in enter and "smk_padcal_start(&padcal, APC40_LCD565, PAD_GAMMA_LUT, PAD_GAIN_Q8, PAD_FLOOR_LEVEL)" in enter
      and "padcal_btn = fm1_in.buttons" in enter and "padcal_pads = fm1_in.pnotes" in enter,
      "enter: not in clip mode; start values from the LCD table; held buttons and pads give no edge")
# The UI gets no buttons, knobs or keys, and the LCD shows the page.
check("if (smk_padcal_on())\n        return 0u;" in body(plat, "uint32_t plat_buttons(void)"), "UI: no buttons in the mode")
check("smk_ck_knobs_owned() || smk_padcal_on()" in body(plat, "int32_t plat_enc(int role)"), "UI: no knobs in the mode")
check("smk_clip_mode_on() || smk_padcal_on()" in body(ui, "static void key_event(int k, int down)"), "UI: no keys in the mode")
dm = body(ui, "static void draw_main(void)")
check(dm.index("smk_padcal_draw(240u, 2u * BAND_H)") < dm.index("smk_clip_lcd_draw(240u"), "LCD: the page draws before the clip grid")
# The frame: the mode takes the frame; the keys and knobs.
fr = body(main, "static void smk_frame(void)")
check(fr.index("if (padcal.on) {") < fr.index("smk_arrows();"), "frame: the mode runs before the arrows, faders and clip mode")
pf = body(main, "static void smk_padcal_frame(void)")
check("padcal_btn & ~b & (1u << panel.btn[B_HOME] | 1u << panel.btn[B_GLO])" in pf, "keys: PATCH or GLOBE ends the mode on the release")
check("down & (1u << panel.btn[B_PLAY])" in pf and "smk_padcal_mark(&padcal)" in pf, "keys: PLAY marks ok")
check("smk_padcal_page_step(padcal.page, -1)" in pf and "smk_padcal_page_step(padcal.page, 1)" in pf
      and "B_OCTDN" in pf and "B_OCTUP" in pf, "keys: OCT- / OCT+ change the page")
check("p & ~padcal_pads & (1u << SMK_WHITE_NOTE[i])" in pf and "padcal.sel = (uint8_t)i" in pf, "pads: a press selects the pad")
check("for (i = 0; i < NE; i++)" in pf and "if (d && i < 3u)" in pf and "smk_padcal_turn(&padcal, (uint8_t)i, d)" in pf,
      "knobs: K2 / K3 / K4 nudge R / G / B, the other knob turns are dropped")
check("padled_set(i, c[0] >> 1, c[1] >> 1, c[2] >> 1)" in pf and "smk_pad_grid = 1u" in pf,
      "pads: each pad shows its entry at half level (as a lit clip pad)")
dr = body(main, "uint8_t smk_padcal_draw(uint32_t w, uint32_t h)")
check("APC40_LCD565[n]" in dr and "smk_padcal_is_ok(&padcal, n)" in dr and 'put_i(put_s(t, "#"), e)' in dr,
      "LCD: the target colours, the ok dots and the entry number")
check("static smk_padcal_t padcal;" in main and "settings" not in pf + enter, "RAM only: no settings byte, no flash save")
print("smk_padcal_wiring_test: " + ("ok" if not fails else f"{fails} FAIL"))
sys.exit(1 if fails else 0)
