#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The clip stage page lists selectable settings and depicts keys as playable notes."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
main = (ROOT / "firmware/src/app/main_fm1.c").read_text()
header = (ROOT / "firmware/src/app/smk_clip_keys.h").read_text()
d0 = main.index("static void ck_page_draw(void)\n{")
pd = main[d0:main.index("\n}\n", d0)]
p0 = main.index("uint8_t smk_ck_panel(uint32_t *last, uint32_t force)")
panel = main[p0:main.index("\n}\n", p0)]
fails = 0


def check(ok, msg):
    global fails
    fails += not ok
    print(("PASS " if ok else "FAIL ") + msg)


check("CK_PAGE_FIELDS[pg][n]" in pd and "CK_E_ON" in header and "ck_get(f)" in pd,
      "page draws its selectable fields, including ON")
check("rbox(fx - 2, fy" in pd and "f == (uint32_t)cp->pg >> 4" in pd,
      "page marks the selected field")
check("ck_zones" not in pd and "CK_ZONE" not in header and "k - lo == v" not in pd,
      "page does not show editable key zones or value keys")
check('"PLAY KEYS"' in pd and "CK_BIT(cp->keys, k) ? c" in pd and
      "ck_kbd(10, 76, 10, 46, CK_NKEY, col)" in pd,
      "page labels its keyboard as playable and lights held keys")
check("ck_nm(CK_MODE_N, i)" in panel and "cp->on >> i & 1u" in panel,
      "KEYS strip retains stage names and ON state")
check("KNOB2 FIELD  KNOB3 / < > VALUE  KEYS PLAY" in pd,
      "help line names the knob and arrow controls")
field = re.search(r"fy = (\d+) \+ \(int32_t\)\(n % 5u\) \* (\d+)", pd)
label = re.search(r"vc\(&FONT_T, (\d+), 10\), &FONT_T, \"PLAY KEYS\"", pd)
help_y = re.search(r"vc\(&FONT_T, (\d+), 12\), &FONT_T, \"KNOB2", pd)
check(bool(field and label and help_y) and int(field.group(1)) + 4 * int(field.group(2)) + 12 <= int(label.group(1))
      and int(label.group(1)) + 10 <= 76 and 76 + 46 <= int(help_y.group(1)) and int(help_y.group(1)) + 12 <= 138,
      "fields, keyboard label, keyboard and help line do not overlap")
print(f"smk_ck_page_layout_test: {fails} fail(s)")
sys.exit(1 if fails else 0)
