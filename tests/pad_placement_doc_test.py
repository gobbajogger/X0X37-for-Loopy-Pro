#!/usr/bin/env python3
"""PAD_MAP.md "Physical pad placement" against the firmware pad map (the doc and the code must not drift).

Three tables (2 rows x 8, top view): the hardware pad index, the LED chain slot (PADLED_SLOT in
firmware/src/lcd.c) and the mux address (SMK_PADMUX in firmware/hal/smk_input.h; mux A = top row, mux B =
bottom row). The index table must be 0-7 on the top row and 8-15 on the bottom row, left to right: the order
that the step grid (SMK_PAD_OF_WHITE with smk_pad_grid = 1) and the orientation rule (smk_padcal_phys with
tl = 0) use. The status line must be ASSUMED or CONFIRMED (with a date and a build hash)."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
doc = (ROOT / "PAD_MAP.md").read_text()
lcd = (ROOT / "firmware/src/lcd.c").read_text()
inp = (ROOT / "firmware/hal/smk_input.h").read_text()
cal = (ROOT / "firmware/src/app/smk_padcal.h").read_text()
fails = 0


def check(ok, what):
    global fails
    print(f"{'PASS' if ok else 'FAIL'} {what}")
    fails += not ok


def table(tag):
    m = re.search(r"<!-- pad-placement:" + tag + r" -->\n```\n(.*?)```", doc, re.S)
    if not m:
        return None
    return [[int(x) for x in row.strip("|").split("|")] for row in m.group(1).splitlines() if row.startswith("|")]


def carray(src, name):
    m = re.search(name + r"\[[^\]]*\] = \{([^}]*)\}", src)
    return [int(x) for x in m.group(1).split(",")]


sec = doc.split("## Physical pad placement", 1)
check(len(sec) == 2, "PAD_MAP.md has the section \"Physical pad placement\"")
st = re.search(r"^Status: (ASSUMED|CONFIRMED)\b(.*)$", sec[-1], re.M)
check(bool(st) and (st.group(1) == "ASSUMED" or re.search(r"\d{4}-\d\d-\d\d", st.group(2))
                    and re.search(r"[0-9a-f]{8}", st.group(2))),
      "status: ASSUMED, or CONFIRMED with a date and a build hash")

idx, led, mux = table("index"), table("led-slot"), table("mux-address")
check(idx == [list(range(8)), list(range(8, 16))], f"index table: 0-7 top row, 8-15 bottom row ({idx})")
check("smk_pad_grid ? (w)" in inp and "(i ^ tl) & 15u" in cal,
      "firmware: the step grid puts white key w on pad w, the orientation rule is i XOR tl")
slot = carray(lcd, "PADLED_SLOT")
check(led is not None and len(slot) == 16 and all(led[p >> 3][p & 7] == slot[idx[p >> 3][p & 7]] for p in range(16)),
      f"LED slot table = PADLED_SLOT {slot}")
pm = carray(inp, "SMK_PADMUX")
addr = [pm.index(p & 7) for p in range(16)]
check(mux is not None and all(mux[p >> 3][p & 7] == addr[idx[p >> 3][p & 7]] for p in range(16)),
      f"mux address table = SMK_PADMUX {pm} (pad p at the address a with SMK_PADMUX[a] = p mod 8)")
if fails:
    print(f"pad_placement_doc_test: {fails} FAIL")
    sys.exit(1)
print("pad_placement_doc_test: ok")
