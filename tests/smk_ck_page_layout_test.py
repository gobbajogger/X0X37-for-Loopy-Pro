#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""CLP-75 (owner 2026-10-17 02:49, 02:51): the SCALE / CHORD / ARP page has no top bar. The KEYS strip
(smk_ck_panel) shows the stage name and ON / OFF. The help line moves to the bottom of the page, below the
37 keys, and its text agrees with the tap that pins the page (CLP-62, CLP-76). Text check of ck_page_draw."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
main = (ROOT / "firmware/src/app/main_fm1.c").read_text()
d0 = main.index("static void ck_page_draw(void)\n{")
pd = main[d0:main.index("\n}\n", d0)]
p0 = main.index("uint8_t smk_ck_panel(uint32_t *last, uint32_t force)")
panel = main[p0:main.index("\n}\n", p0)]
fails = 0


def check(ok, msg):
    global fails
    fails += not ok
    print(("PASS " if ok else "FAIL ") + msg)


check("CK_MODE_N" not in pd and '"ON"' not in pd and '"OFF"' not in pd, "page: no stage name, no ON / OFF (no top bar)")
check("cv_rect(0, 17, 240, 1, C_LINE)" not in pd, "page: no line below a top bar")
check("ck_nm(CK_MODE_N, i)" in panel and "cp->on >> i & 1u" in panel, "KEYS strip: the stage names and their ON state stay")
m = re.search(r'vc\(&FONT_T, (\d+), 12\), &FONT_T, cp->pg & CK_P_PIN \? "([^"]+)"\s*: "([^"]+)"', pd)
k = re.search(r"ck_kbd\(10, (\d+), 10, (\d+), CK_NKEY, col\)", pd)
check(bool(m and k) and int(m.group(1)) >= int(k.group(1)) + int(k.group(2)) and int(m.group(1)) + 12 <= 138,
      "help line: below the keys, inside the main area (138 px)")
check(bool(m) and m.group(2) == "KEYS PLAY  < > STEP  FADER BANK: CLOSE" and m.group(3) == "KEYS EDIT  FADER BANK: PIN",
      "help line: pinned 'KEYS PLAY  < > STEP  FADER BANK: CLOSE', held 'KEYS EDIT  FADER BANK: PIN'")
fy = re.search(r"fy = (\d+) \+ \(int32_t\)\(n % 4u\) \* 12", pd)
zn = re.search(r"vc\(&FONT_T, (\d+) \+ \(int32_t\)\(n & 1u\) \* 10, 10\)", pd)
check(bool(fy and zn and k) and int(fy.group(1)) < 17 and int(fy.group(1)) + 48 <= int(zn.group(1)) and
      int(zn.group(1)) + 20 <= int(k.group(1)), "page: fields, zone names and keys move up into the free rows, no overlap")
print(f"smk_ck_page_layout_test: {fails} fail(s)")
sys.exit(1 if fails else 0)
