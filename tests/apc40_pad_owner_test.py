#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""CLP-13: the live pad table (app/apc40_pad_owner.h, APC40_PAD_OWNER) is the owner-calibrated table.

1. The export file is the owner export of 2026-10-17 (build 89f680e): sha256 54ecc6f8..., 128 entries, 89 ok.
2. The header is the generator output for the export (tools/apc_pad_colors.py --pad-override), byte for byte.
3. The 89 entries marked ok are the export values. The 39 other entries are the start values of 89f680e:
   the export values (89f680e wrote them), APC40_PAD_DEFAULT and the generator rule. The start-value
   inputs (apc40_pad_gamma.h, apc40_lcd565.h, smk_padcal_default) are the same as at 89f680e (sha256).
4. Render: a lit clip pad of velocity v (main_fm1.c smk_pads, brightness 255) sends the same R, G, B bytes,
   in the same order, as PAD COLOURS sent for entry v on 89f680e (level / 2, c[0] >> 1, c[1] >> 1, c[2] >> 1).
   Exception (owner 2026-10-17 01:19): the idle shades 7-59 (v % 4 == 3) send the full level (CLP-13).
   The brightness is 255 (lit) or 0 (the off half of a blink), so no other level exists.
"""
import hashlib
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "firmware/src/app"
EXPORT = ROOT / "tools/padcal/pads-owner-20261017.txt"
fails = 0


def check(ok, msg):
    global fails
    fails += not ok
    print(("PASS " if ok else "FAIL ") + msg)


def table(path, name):
    t = re.search(name + r"\[128\]\[3\]\s*=\s*\{(.*?)\};", Path(path).read_text(), re.S)
    return [tuple(map(int, x)) for x in re.findall(r"\{(\d+),(\d+),(\d+)\}", t.group(1))] if t else []


def sha(data):
    return hashlib.sha256(data).hexdigest()


raw = EXPORT.read_bytes()
check(sha(raw).startswith("54ecc6f87ff0a0da"), f"export sha256 {sha(raw)[:16]} (owner export 2026-10-17)")
exp, orient = {}, None
for line in raw.decode().splitlines():
    f = line.split()
    if len(f) == 6 and f[0] == "padcal":
        exp[int(f[1])] = (tuple(map(int, f[2:5])), f[5] == "ok")
    elif len(f) == 2 and f[0] == "padcal_orient":
        orient = int(f[1])
check(sorted(exp) == list(range(128)) and sum(k for _, k in exp.values()) == 89 and orient == 0,
      "export: 128 entries, 89 ok, padcal_orient 0")

with tempfile.TemporaryDirectory() as d:
    gen = Path(d) / "apc40_pad_owner.h"
    r = subprocess.run([sys.executable, str(ROOT / "tools/apc_pad_colors.py"), "--pad-h", str(gen),
                        "--pad-override", str(EXPORT), "--pad-name", "APC40_PAD_OWNER"], capture_output=True, text=True)
    check(r.returncode == 0 and gen.read_text() == (APP / "apc40_pad_owner.h").read_text(),
          "app/apc40_pad_owner.h == generator output for the export (all 128 entries)")

owner = table(APP / "apc40_pad_owner.h", "APC40_PAD_OWNER")
start = table(APP / "apc40_pad_default.h", "APC40_PAD_DEFAULT")
check(len(owner) == 128 and len(start) == 128, "both tables have 128 entries")
sys.path.insert(0, str(ROOT / "tools"))
import apc_colour_model as m  # noqa: E402
rule = [m.pad_level(e) for e in m.load_source()]
check(all(owner[v] == exp[v][0] for v in exp if exp[v][1]), "the 89 ok entries == the export")
others = [v for v in exp if not exp[v][1]]
check(len(others) == 39 and all(owner[v] == exp[v][0] == start[v] == rule[v] for v in others),
      "the 39 other entries == the 89f680e start values (export, APC40_PAD_DEFAULT, generator rule)")

# The start-value inputs at 89f680e (git show 89f680e:firmware/src/app/...): unchanged.
padcal = (APP / "smk_padcal.h").read_text()
i = padcal.index("static inline void smk_padcal_default")
fn = padcal[i:padcal.index("\n}\n", i) + 3]
for name, data, want in (("apc40_pad_gamma.h", (APP / "apc40_pad_gamma.h").read_bytes(), "c4dce93ddbe5cb34"),
                         ("apc40_lcd565.h", (APP / "apc40_lcd565.h").read_bytes(), "269e1b3aa2aeeea2"),
                         ("smk_padcal_default", fn.encode(), "40a61ddf5a5c515a")):
    check(sha(data).startswith(want), f"{name} is the same as at 89f680e (sha256 {want})")

# Render: the clip pad path and the PAD COLOURS path of the firmware source.
main = (APP / "main_fm1.c").read_text()
p0 = main.index("static void smk_pads(void)")
pads = main[p0:main.index("\n}\n", p0)]
CLIP = ("padled_set(pad, smk_clip_pad_level(rgb[0], v, brightness), smk_clip_pad_level(rgb[1], v, brightness),\n"
        "                              smk_clip_pad_level(rgb[2], v, brightness));")
helper = (APP / "smk_clip_colour.h").read_text()
check("const uint8_t *rgb = APC40_PAD_OWNER[v];" in pads and CLIP in pads and
      "return (uint8_t)((level * brightness) / (SMK_CLIP_IDLE_PAD_LIFT && smk_clip_idle_shade(v) ? 255u : 510u));" in helper,
      "smk_pads: a clip pad sends APC40_PAD_OWNER[v][0..2] * brightness / 510 as R, G, B (idle shades / 255)")
check("APC40_RGB" not in main, "main_fm1.c: no APC40_RGB (the old pad table is gone)")
b0 = main.index("static uint8_t smk_apc_led_brightness")
bright = main[b0:main.index("\n}\n", b0)]
levels = set(re.findall(r"\b(\d+)u;", bright)) | set(re.findall(r"\?\s*(\d+)u\s*:\s*(\d+)u", bright)[0])
check(levels <= {"0", "255"}, f"smk_apc_led_brightness returns 0 or 255 only ({sorted(levels)})")
check("padled_set(ph, c[0] >> 1, c[1] >> 1, c[2] >> 1);" in main,
      "PAD COLOURS sends c[0] >> 1, c[1] >> 1, c[2] >> 1 (as on 89f680e)")
IDLE = set(range(7, 60, 4))   # CLP-13 idle shades (owner 2026-10-17 01:19): 2 x, tests/smk_clip_idle_pad_test.py
bad = [v for v in range(128) if v not in IDLE for k in range(3) if owner[v][k] * 255 // 510 != exp[v][0][k] >> 1]
check(not bad, "render: lit clip pad of velocity v == the bytes the owner saw for entry v (114 non-idle v x R, G, B)")
bad = [v for v in IDLE for k in range(3) if owner[v][k] * 255 // 255 != exp[v][0][k]]
check(not bad, "render: idle shades 7-59 send the full export level (2 x what the owner saw at half level)")
check(all(owner[v][k] * 0 // 510 == 0 for v in range(128) for k in range(3)), "render: the off half of a blink is 0, 0, 0")
print(f"apc40-pad-owner: {fails} fail(s)")
sys.exit(1 if fails else 0)
