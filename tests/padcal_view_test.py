#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""CLP-79 (owner 2026-10-17 01:42, 01:43): in the pad calibration mode the LCD square of the SELECTED entry
shows brighter (boost 2^s, each RGB565 field << s, s 0-3, the largest s that keeps each field in range), and
the LCD shows the name of the selected entry (app/apc40_pad_names.h, tools/apc_pad_colors.py --names-h).
SEQ REC shows the real level. The pads, the other squares, the stored values and the export do not change.
The release build (X0X_DEV_PADCAL=0) has none of it: tests/padcal_flag_test.py, and the Elite x0x.bin
is byte-identical (commit report)."""
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "firmware/src/app"
sys.path.insert(0, str(ROOT / "tools"))
import apc_colour_model as m  # noqa: E402

fails = 0


def check(ok, msg):
    global fails
    fails += not ok
    print(("PASS " if ok else "FAIL ") + msg)


C = r'''#include <stdio.h>
#include <stdint.h>
#include "app/apc40_lcd565.h"
#include "app/apc40_pad_names.h"
#include "app/smk_padcal.h"
int main(void)
{
    unsigned v;
    for (v = 0; v < 128u; v++) {
        uint8_t s = smk_padcal_boost_shift(APC40_LCD565[v]);
        printf("b %u %u %u %u\n", v, (unsigned)APC40_LCD565[v], (unsigned)s,
               (unsigned)smk_padcal_boost565(APC40_LCD565[v], s));
        printf("n %u %s %s\n", v, PADCAL_HUE[PADCAL_NAME[v] >> 3], PADCAL_SHADE[PADCAL_NAME[v] & 7u]);
    }
    return 0;
}
'''
out = ""
with tempfile.TemporaryDirectory() as d:
    c, exe = Path(d) / "t.c", Path(d) / "t"
    c.write_text(C)
    r = subprocess.run(["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", f"-I{ROOT / 'firmware/src'}",
                        "-o", str(exe), str(c)], capture_output=True, text=True)
    check(r.returncode == 0, "host build of the boost helpers and the names" + ("" if not r.returncode else ": " + r.stderr[:300]))
    if r.returncode == 0:
        out = subprocess.run([str(exe)], capture_output=True, text=True).stdout
    gen = Path(d) / "names.h"
    g = subprocess.run([sys.executable, str(ROOT / "tools/apc_pad_colors.py"), "--names-h", str(gen)],
                       capture_output=True, text=True)
    nh = APP / "apc40_pad_names.h"
    check(g.returncode == 0 and nh.exists() and gen.read_text() == nh.read_text(),
          "app/apc40_pad_names.h == tools/apc_pad_colors.py --names-h")

F = lambda c: (c >> 11 & 31, c >> 5 & 63, c & 31)  # noqa: E731
bad, hist = [], {}
for v, c, s, b in ((int(a), int(b_), int(c_), int(d)) for a, b_, c_, d in re.findall(r"^b (\d+) (\d+) (\d+) (\d+)$", out, re.M)):
    f, fb = F(c), F(b)
    top = all(x << (s + 1) <= lim for x, lim in zip(f, (31, 63, 31)))
    ok = fb == tuple(x << s for x in f) and all(x <= lim for x, lim in zip(fb, (31, 63, 31))) and \
        (s == 3 or not top or c == 0) and (c != 0 or s == 0)
    bad += [] if ok else [v]
    hist[s] = hist.get(s, 0) + 1
check(sum(hist.values()) == 128 and not bad,
      f"boost: each field << s, in range, s the largest (0-3) for all 128 entries {bad} (s: {sorted(hist.items())})")
src = m.load_source()
names = {int(v): (h + " " + s).strip() for v, h, s in re.findall(r"^n (\d+) (\S+) ?(\S*)$", out, re.M)}
check(len(names) == 128 and all(names[v] == m.entry_name_text(src, v) for v in range(128)),
      "names: 128 entries, the firmware name == tools/apc_colour_model.py entry_name_text")
csv = ROOT / "docs/padcal-names.csv"
with tempfile.TemporaryDirectory() as d:
    subprocess.run([sys.executable, str(ROOT / "tools/apc_pad_colors.py"), "--names-csv", str(Path(d) / "n.csv")],
                   capture_output=True, text=True)
    want = (Path(d) / "n.csv").read_text() if (Path(d) / "n.csv").exists() else None
check(csv.exists() and csv.read_text() == want, "docs/padcal-names.csv == tools/apc_pad_colors.py --names-csv")

main = (APP / "main_fm1.c").read_text()
f0 = main.index("static void smk_padcal_frame(void)")
frame = main[f0:main.index("\n}\n", f0)]
PADS = '''    for (i = 0; i < SMK_PADCAL_PADS; i++) {      /* level / 2: as a lit clip pad (smk_pads) */
        const uint8_t *c = padcal.rgb[smk_padcal_entry(padcal.page, (uint8_t)i)];
        uint8_t ph = smk_padcal_phys(padcal.tl, (uint8_t)i), w = !i ? 127u : 0u;
        if (padcal.orient)                       /* the orientation page: only reading position 0, white */
            padled_set(ph, w, w, w);
        else
            padled_set(ph, c[0] >> 1, c[1] >> 1, c[2] >> 1);
    }'''
check(PADS in frame and frame.count("padled_set(") == 2, "pads: the calibration pad loop is the same text as before (no boost)")
check("padcal.real ^= 1u;" in frame and "B_REC" in frame, "SEQ REC toggles the real level of the selected square")
d0 = main.index("uint8_t smk_padcal_draw(uint32_t w, uint32_t h)")
draw = main[d0:main.index("\n}\n", d0)]
check("uint16_t c = i == padcal.sel ? smk_padcal_boost565(APC40_LCD565[n], s) : APC40_LCD565[n];" in draw and
      "s = padcal.real ? 0u : smk_padcal_boost_shift(APC40_LCD565[e])" in draw,
      "LCD: only the selected square is boosted, the others are APC40_LCD565[n]")
check(not re.search(r"padcal\.(rgb|ok)\[[^\]]*\]\s*(\[[^\]]*\])?\s*[-+|^&]?=[^=]", draw + frame.replace(
      "c->ok", "")), "the draw and the frame do not write padcal.rgb or padcal.ok (stored values unchanged)")
check("PADCAL_HUE[PADCAL_NAME[e] >> 3]" in draw and "PADCAL_SHADE[PADCAL_NAME[e] & 7u]" in draw and
      '"BOOST X"' in draw and '"REAL LEVEL"' in draw, "LCD: the name and the boost of the selected entry")
h = (APP / "smk_padcal.h").read_text()
w0 = h.index("static inline uint32_t smk_padcal_word")
check("c->rgb[e][0] | (uint32_t)c->rgb[e][1] << 8 | (uint32_t)c->rgb[e][2] << 16" in h[w0:w0 + 400],
      "the export word is the stored value (smk_padcal_word unchanged)")
print(f"padcal-view: {fails} fail(s)")
sys.exit(1 if fails else 0)
