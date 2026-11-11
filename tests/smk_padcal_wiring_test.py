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
glob = body(ui, "static void global_section(int g)")
sysi = glob.index("case 5:")
check(sysi < glob.index("ACT_PADCAL") < glob.index("break;", sysi) and "if(!smk_clip_mode_on())" in glob,
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
dm = body(ui, "static UI_DRAW void draw_main(void)")
check(dm.index("smk_padcal_draw(240u, 2u * BAND_H)") < dm.index("smk_clip_lcd_draw(240u"), "LCD: the page draws before the clip grid")
# The frame: the mode takes the frame; the keys and knobs.
fr = body(main, "static void smk_frame(void)")
held = re.search(r"if \(ui_project_busy\(\)\) \{[^{}]*\}", fr)
check(held and held.start() < fr.index("if (padcal.on) {") and "return;" in held.group()
      and "smk_padcal_frame();" not in held.group(),
      "frame: a project hold returns before pad calibration dispatch")
normal = fr[:held.start()] + fr[held.end():] if held else fr
check(normal.index("if (padcal.on) {") < min(normal.index(call) for call in
      ("smk_arrows();", "smk_faders();", "if (clip_grid_mode)")),
      "frame: pad calibration runs before normal arrows, faders and clip mode")
pf = body(main, "static void smk_padcal_frame(void)")
check("padcal_btn & ~b & (1u << panel.btn[B_HOME] | 1u << panel.btn[B_GLO])" in pf, "keys: PATCH or GLOBE ends the mode on the release")
check("down & (1u << panel.btn[B_PLAY])" in pf and "smk_padcal_mark(&padcal)" in pf, "keys: PLAY marks ok")
check("smk_padcal_page_step(padcal.page, -1)" in pf and "smk_padcal_page_step(padcal.page, 1)" in pf
      and "B_OCTDN" in pf and "B_OCTUP" in pf, "keys: OCT- / OCT+ change the page")
check("p & ~padcal_pads & (1u << SMK_WHITE_NOTE[i])" in pf and "padcal.sel = smk_padcal_phys" in pf, "pads: a press selects the pad")
check("for (i = 0; i < NE; i++)" in pf and "else if (d && i < 3u && !padcal.orient)" in pf and "smk_padcal_turn(&padcal, (uint8_t)i, d)" in pf,
      "knobs: K2 / K3 / K4 nudge R / G / B, the other knob turns are dropped")
check("padled_set(ph, c[0] >> 1, c[1] >> 1, c[2] >> 1)" in pf and "smk_pad_grid = 1u" in pf
      and "uint8_t ph = smk_padcal_phys(padcal.tl, (uint8_t)i)" in pf,
      "pads: each pad shows its entry at half level (as a lit clip pad), on the corrected pad")
check("padled_set(ph, w, w, w)" in pf and "w = !i ? 127u : 0u" in pf and "padcal.sel = smk_padcal_phys(padcal.tl, (uint8_t)i)" in pf,
      "orientation: only reading position 0 lit white; a pad press goes through the correction")
check("if (padcal.orient)\n            padcal.orient = 0u;" in pf and "smk_padcal_orient_turn(&padcal, d)" in pf
      and "if (d && padcal.orient && !i)" in pf, "orientation: K2 moves the light, SEQ PLAY confirms (no mark)")
dr = body(main, "uint8_t smk_padcal_draw(uint32_t w, uint32_t h)")
check('cv_text(4, 70, &FONT_S, "pad 1 top-left?", C_WHITE)' in dr and "padcal.orient << 4" in dr,
      "orientation: the LCD asks 'pad 1 top-left?'")
check("cv_rect(x + 12, y + 12, 6, 6, smk_padcal_dot565(c))" in dr and "cv_rect(x + 3, y + 3, 24, 24, c)" in dr,
      "LCD: the ok dot has the contrast colour of its square as drawn (smk_padcal_dot565, boost included)")
check("APC40_LCD565[n]" in dr and "smk_padcal_is_ok(&padcal, n)" in dr and 'put_i(put_s(t, "#"), e)' in dr,
      "LCD: the target colours, the ok dots and the entry number")
check("static smk_padcal_t padcal;" in main and "settings" not in pf + enter, "RAM only: no settings byte, no flash save")
rp = body(main, "static void smk_inlog_reply(uint32_t arg)")
check("if (arg - 2u < 128u / SMK_PADCAL_CHUNK)" in rp and "padcal.rgb[(arg - 2u) * SMK_PADCAL_CHUNK]" in rp
      and "smk_inlog_w(c[i], 2u)" in rp and "smk_inlog_w(padcal.tl, 2u)" in rp and "smk_inlog_w(padcal.ok[(arg - 2u) * (SMK_PADCAL_CHUNK / 8u) + i], 2u)" in rp,
      "export: INLOG args 2-5 give 32 entries each (tests/smk_padcal_export_test.py decodes them)")
print("smk_padcal_wiring_test: " + ("ok" if not fails else f"{fails} FAIL"))
sys.exit(1 if fails else 0)
