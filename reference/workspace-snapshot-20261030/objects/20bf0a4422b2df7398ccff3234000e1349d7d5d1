#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The off-screen strip is one picture high, and that picture still fits.

The panel is three strips that never share the buffer: header, main area,
knob row. The strip must be exactly the taller of those, and drawing each
one must not be clamped.
"""
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
GFX = (ROOT / "firmware/src/gfx.c").read_text()
UI = (ROOT / "firmware/src/app/ui.c").read_text()
MAIN = (ROOT / "firmware/src/app/main_fm1.c").read_text()
SMC = (ROOT / "firmware/src/app/smc_mixer.c").read_text()


def define(text, name):
    m = re.search(rf"#define {name} (\d+)u?", text)
    if not m:
        raise SystemExit(f"missing {name}")
    return int(m.group(1))


def main():
    rows = define(GFX, "CV_ROWS")
    band = define(UI, "BAND_H")
    hdr = define(UI, "HDR_H")
    knob = define(UI, "KNOB_H")
    if rows != 2 * band:
        raise SystemExit(f"strip is {rows} rows, main area is {2 * band}")
    if hdr + rows + knob != 240:
        raise SystemExit(f"strips {hdr}+{rows}+{knob} do not tile 240")
    if knob > rows or hdr > rows:
        raise SystemExit("a strip is taller than the buffer")
    for label, text in ("ui", UI), ("main", MAIN), ("smc", SMC):
        for n in (int(x) for x in re.findall(r"cv_begin\(\s*240\s*,\s*(\d+)", text)):
            if n > rows:
                raise SystemExit(f"{label} draws {n} rows into a {rows}-row strip")
    block = re.search(r"#ifdef CV_ROWS\n(.*?)#endif", UI, re.S)
    if not block:
        raise SystemExit("ui.c is missing the screen-strip asserts")
    src = """
#include <stdint.h>
#include <stdio.h>
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{ (void)x; (void)y; (void)w; (void)h; (void)p; }
#include "gfx.c"
#define HDR_H %d
#define BAND_H %d
#define KNOB_H %d
%s
int main(void) {
    if (CV_ROWS != %d || CV_MAX != 240u * CV_ROWS) return 1;
    if (sizeof(cv_px) != (size_t)CV_MAX * sizeof(uint16_t)) return 2;
    cv_begin(240, %d, 0);
    if (cv_h != %du) return 3;
    cv_begin(240, %d, 0);
    if (cv_h != %du) return 4;
    cv_begin(240, %d, 0);
    if (cv_h != %du) return 5;
    printf("canvas strip %%u bytes (was %%u)\\n", (unsigned)sizeof(cv_px), 240u * 144u * 2u);
    return 0;
}
""" % (hdr, band, knob, block.group(1), rows, 2 * band, 2 * band, knob, knob, hdr, hdr)
    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        (tmp_path / "felucca_font.h").write_text("/* host stub: this check draws no glyphs */\n")
        path = tmp_path / "canvas_strip_test.c"
        path.write_text(src)
        bin_path = tmp_path / "canvas_strip_test"
        subprocess.check_call(
            ["cc", "-std=gnu99", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
             "-I", tmp, "-I", "firmware/src",
             "-o", str(bin_path), str(path)],
            cwd=ROOT,
        )
        subprocess.check_call([str(bin_path)])
    print(f"canvas strip: {rows} rows, main {2 * band}, knob {knob}, header {hdr}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
