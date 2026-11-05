#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""One owner for each press of "<" and ">" in clip mode (BEHAVIOR.md CLP-27, CLP-76, GEN-56).
A menu list open: only smk_arrows acts (ui_menu_open), and the clip keys (ck_controls) mask the arrows.
No menu: smk_arrows does nothing in clip mode, and the clip keys page step stays as it is.
The test reads the source, because the clip keys code needs the device build. It also checks that the
two hold repeats use the same times."""
import re
import sys

ROOT = sys.argv[1] if len(sys.argv) > 1 else "."
main = open(f"{ROOT}/firmware/src/app/main_fm1.c").read()
ui = open(f"{ROOT}/firmware/src/app/ui.c").read()
arw = open(f"{ROOT}/firmware/src/app/smk_arrows.h").read()
ckh = open(f"{ROOT}/firmware/src/app/smk_clip_keys.h").read()


def body(src, head):
    i = src.index(head)
    j = src.index("\n}\n", i)
    return src[i:j]


def define(src, name):
    m = re.search(rf"#define {name} (\d+)u", src)
    return int(m.group(1)) if m else None


# the clip keys own the knobs and the arrows only with no menu (no list, no question, no help card)
own = body(main, "uint8_t smk_ck_knobs_owned(void)")
assert "ui.overlay == O_NONE" in own, own
# smk_arrows takes the arrows only with a menu list open
assert "ui.overlay == O_LIST" in body(ui, "static int ui_menu_open(void)")
sa = body(main, "static void smk_arrows(void)")
assert "arw_route((uint32_t)ui_menu_open(), clip_grid_mode" in sa, sa
assert re.search(r"if \(route == ARW_MENU\) \{\s*ui_menu_arrow\(d\);", sa), sa
assert "return home ? ARW_NONE : menu ? ARW_MENU : clip ? ARW_NONE : ARW_SELECT;" in arw
# the clip keys: the arrow presses are masked when they do not own the controls
ck = body(main, "CK_FN static void ck_controls(void)")
assert re.search(r"if \(!(own|smk_ck_knobs_owned\(\))\)\s*press &= ~0x60u;", ck), "ck_controls must mask < > with a menu"
if "ck_rpt(" in ck:                      # e7989ac: the hold repeat reads the held arrows: only when owned
    arg = ck[ck.index("ck_rpt(") + 7:]
    assert re.match(r"(own|smk_ck_knobs_owned\(\)) &&", arg), "ck_rpt must get 0 with a menu open"
else:                                    # before e7989ac: a press steps the page
    assert "press & 0x60u" in ck
# no other read of the held arrows (cur bits 5-6) in ck_controls
held = [m.start() for m in re.finditer(r"cur >> 5|cur & 0x60", ck)]
assert len(held) <= ("ck_rpt(" in ck), "an unmasked read of the held < >"
# the same feel: the same hold repeat times
assert define(arw, "ARW_FIRST_MS") == 400 and define(arw, "ARW_NEXT_MS") == 80
if define(ckh, "CK_RPT_DELAY") is not None:
    assert define(ckh, "CK_RPT_DELAY") == define(arw, "ARW_FIRST_MS")
    assert define(ckh, "CK_RPT_MS") == define(arw, "ARW_NEXT_MS")
print("smk_arrows_owner: ok")
