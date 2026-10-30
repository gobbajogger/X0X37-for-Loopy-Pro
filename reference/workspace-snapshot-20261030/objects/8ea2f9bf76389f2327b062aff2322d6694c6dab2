#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Pad calibration export round trip (BEHAVIOR.md CLP-79): the firmware words (tests/smk_padcal_test.c,
the same app/smk_padcal.h as the firmware: R | G << 8 | B << 16 | ok << 24) -> the INLOG reply bytes of args 2-5 (as main_fm1.c
smk_inlog_reply) -> tools/inlog_fmt.py parse_padcal and padcal_lines -> tools/apc_pad_colors.py
--pad-override. The entries marked ok must come out with the edited values; the others stay at the rule."""
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import apc_colour_model as m  # noqa: E402
import inlog_fmt  # noqa: E402

exe = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build/host/smk_padcal_test"
words = {}
for line in subprocess.run([str(exe), "export"], check=True, capture_output=True, text=True).stdout.split("\n"):
    if line.startswith("tl "):
        tl = int(line.split()[1])
    elif line:
        e, w = (int(x) for x in line.split())
        words[e] = w
assert sorted(words) == list(range(128))


def reply(arg):                     # main_fm1.c smk_inlog_reply: ed_b(arg), R G B, then the ok bits, each 2 x 7 bits
    w = [words[(arg - 2) * 32 + k] for k in range(32)]
    vals = [(x >> s) & 255 for x in w for s in (0, 8, 16)]
    vals += [sum(((w[8 * j + i] >> 24) & 1) << i for i in range(8)) for j in range(4)] + [tl]
    return [arg] + [p for v in vals for p in (v & 0x7F, v >> 7)]


fails = 0
ents = []
got_tl = inlog_fmt.parse_padcal_tl(reply(2))
print(f"{'PASS' if got_tl == tl == 8 else 'FAIL'} export: the orientation correction {got_tl} (the test sets 8)")
fails += not (got_tl == tl == 8)
for arg in inlog_fmt.PADCAL_ARGS:
    p = inlog_fmt.parse_padcal(reply(arg), arg)
    fails += p is None
    ents += p or []
assert inlog_fmt.parse_padcal(reply(2), 3) is None and inlog_fmt.parse_padcal(reply(2)[:50], 2) is None
ok = {v: (r, g, b) for v, r, g, b, k in ents if k}
print(f"{'PASS' if sorted(ok) == [81, 127] else 'FAIL'} export: entries marked ok {sorted(ok)} (the test marks 81 and 127)")
fails += sorted(ok) != [81, 127]
with tempfile.TemporaryDirectory() as td:
    f = Path(td) / "export.txt"
    f.write_text("\n".join(inlog_fmt.padcal_lines(ents, got_tl)) + "\n")
    h = Path(td) / "pad.h"
    r = subprocess.run([sys.executable, str(ROOT / "tools/apc_pad_colors.py"), "--pad-h", str(h), "--pad-override", str(f)],
                       check=True, capture_output=True, text=True)
    warned = "padcal_orient 8" in r.stderr
    print(f"{'PASS' if warned else 'FAIL'} generator: carries the correction and warns ({r.stderr.strip()})")
    fails += not warned
    got = [tuple(int(x) for x in t) for t in re.findall(r"\{(\d+),(\d+),(\d+)\}", h.read_text())]
want = [m.pad_level(e) for e in m.load_source()]
for v, rgb in ok.items():
    want[v] = rgb
good = got == want and got[81][2] == 0 and want[81] != m.pad_level(m.load_source()[81])
print(f"{'PASS' if good else 'FAIL'} round trip: the header has the edited 81 {got[81]} and 127 {got[127]}, "
      "the other 126 entries at the rule")
fails += not good
print("smk_padcal_export_test: " + ("ok" if not fails else f"{fails} FAIL"))
sys.exit(1 if fails else 0)
