#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""U1 design-only 240x240 frame studies using DnBOX's actual font source.
Run from repository root: python3 tools/ux_prototypes/render_u1_frames.py
Requires Pillow and existing assets/fonts. Does not compile/run firmware.
Outputs untracked PNGs under build/u1-design-frames. NOT baseline UI captures.
"""
from __future__ import annotations
import importlib.util
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
GEN = ROOT / "tools/gen_font.py"
spec = importlib.util.spec_from_file_location("dnbox_gen_font_u1", GEN)
assert spec and spec.loader
fontgen = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fontgen)
FONTSET = fontgen.FONTSET
OUT = ROOT / "build/u1-design-frames"
W = H = 240
# Renderer source: header 20, main 138, knob strip 82.
HEADER, MAIN_END = 20, 158
BLACK = (0, 0, 0)
WHITE = (244, 245, 245)
DIM = (128, 146, 148)
GRAY = (156, 170, 168)
LINE = (40, 56, 61)
HI = (150, 225, 255)
CYAN = (70, 205, 255)
GREEN = (130, 240, 90)
COLORS = {"HOME": CYAN, "SEQ": GREEN, "HELD": CYAN, "PRESETS": CYAN, "READOUT": CYAN}

def rgb565(rgb):
    r, g, b = rgb
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

def quant(rgb):
    c = rgb565(rgb)
    return (((c >> 11) & 31) << 3, ((c >> 5) & 63) << 2, (c & 31) << 3)

def load_fonts():
    result = {}
    for face, name, px, scale, pixel, first, last in fontgen.SIZES:
        if face not in ("T", "XS", "S", "B", "M"):
            continue
        h, glyphs = fontgen.render(name, px, scale, pixel, first, last)
        result[face] = (first, last, h, glyphs)
    return result

FONTS = load_fonts()

def label(img, x, y, text, face="S", color=WHITE, limit=238):
    """Use the same glyph advance and alpha quantization source as gen_font.py.
    Not an implementation of packed firmware renderer/compositing."""
    first, last, h, glyphs = FONTS[face]
    pix = img.load()
    color = quant(color)
    for char in text:
        code = ord(char)
        if char.islower() and code > last:
            code = ord(char.upper())
        if not first <= code <= last:
            code = ord("?")
        advance, bw, alpha = glyphs[code-first]
        for yy in range(h):
            ty = y + yy
            if ty < 0 or ty >= H:
                continue
            for xx in range(bw):
                tx = x + xx - fontgen.PAD
                if tx < 0 or tx >= W or tx >= limit:
                    continue
                a = alpha[yy*bw+xx]
                if not a:
                    continue
                base = pix[tx, ty]
                pix[tx, ty] = quant(tuple((base[k]*(15-a)+color[k]*a+7)//15 for k in range(3)))
        x += advance
    return x

def line(img, xy, color=LINE, width=1):
    ImageDraw.Draw(img).line(xy, fill=quant(color), width=width)

def rectangle(img, box, color=LINE, fill=None):
    ImageDraw.Draw(img).rectangle(box, outline=quant(color), fill=quant(fill) if fill else None)

def page(title, part, engine, pattern="P03"):
    img = Image.new("RGB", (W, H), BLACK)
    label(img, 5, 3, "DnBOX", "B", WHITE)
    label(img, 62, 4, part, "XS", CYAN)
    label(img, 155, 4, pattern, "XS", GRAY)
    label(img, 196, 4, title, "XS", HI)
    line(img, (0, 19, 239, 19))
    line(img, (0, 157, 239, 157))
    return img

def strip(img, cells):
    for i, (name, val) in enumerate(cells):
        x = i*60
        if i:
            line(img, (x, 160, x, 239))
        label(img, x+4, 165, name, "XS", DIM, x+59)
        label(img, x+4, 194, val, "S", WHITE, x+59)
        line(img, (x+6, 225, x+52, 225), LINE)
        line(img, (x+6, 225, x+33, 225), HI, 2)

def home():
    img = page("HOME", "BASS B", "CHICAGO")
    label(img, 8, 30, "CHICAGO", "B", HI)
    label(img, 8, 58, "ACID LINE 03", "S", WHITE)
    label(img, 8, 87, "SOUND READY", "XS", GRAY)
    label(img, 8, 113, "PATCH: PATTERN", "XS", DIM)
    label(img, 8, 133, "PARA: EDIT SOUND", "XS", DIM)
    strip(img, [("CUTOFF", "87"), ("PEAK", "54"), ("ENV", "68"), ("LEVEL", "102")])
    return img

def seq():
    img = page("SEQ", "BASS B", "CHICAGO")
    label(img, 7, 29, "STEP 07 / 16", "B", HI)
    label(img, 7, 56, "Cmin7", "B", WHITE)
    label(img, 7, 84, "C4 Eb4 G4 Bb4", "S", WHITE)
    for i in range(16):
        x = 8+i*14
        rectangle(img, (x, 119, x+10, 135), HI if i == 6 else LINE, HI if i == 6 else None)
    strip(img, [("STEP", "07"), ("VEL", "110"), ("GATE", "80%"), ("RATE", "1/16")])
    return img

def held():
    img = page("HELD", "BASS A", "JUNO")
    label(img, 7, 28, "JUNO  LIVE KEYS", "B", HI)
    label(img, 7, 52, "HELD: C4 E4 G4", "S", WHITE)
    # 37-key window: distinct from a recorded note lane.
    x0 = 8
    for k in range(37):
        x = x0+k*6
        if x > 230:
            break
        active = k in (8, 12, 15)
        rectangle(img, (x, 82, x+4, 101), CYAN if active else LINE, CYAN if active else None)
    label(img, 7, 110, "RECORDED STEP 07", "XS", DIM)
    for k in (8, 12, 15):
        rectangle(img, (8+k*6, 130, 8+k*6+4, 143), GREEN, GREEN)
    strip(img, [("CUTOFF", "76"), ("RES", "20"), ("SUB", "40"), ("CHORUS", "ON")])
    return img

def presets():
    img = page("PRESETS", "BASS A", "JUNO")
    label(img, 7, 29, "JUNO  SOUND", "B", HI)
    for y, text in [(56, "> FACTORY  01"), (80, "  FACTORY  02"), (104, "  USER     01")]:
        label(img, 8, y, text, "S", WHITE if y==56 else GRAY)
    label(img, 8, 135, "IMPORT: VERIFY FORMAT", "XS", DIM)
    strip(img, [("SOURCE", "FACTORY"), ("BANK", "01"), ("APPLY", "SELECT"), ("BACK", "PATCH")])
    return img

def readout():
    img = seq()
    # For proposed transient feedback only: never an assertion of implemented behavior.
    rectangle(img, (8, 103, 231, 153), CYAN, BLACK)
    label(img, 15, 108, "CHICAGO B  CUTOFF", "S", HI)
    label(img, 15, 131, "87 / 127", "S", WHITE)
    return img

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    for name, fn in [("01-home",home), ("02-seq",seq), ("03-held",held), ("04-presets",presets), ("05-readout",readout)]:
        im = fn()
        assert im.size == (240, 240)
        path = OUT / (name + ".png")
        im.save(path)
        print(path.relative_to(ROOT))
    print("DESIGN STUDIES ONLY: original firmware framebuffer BEFORE not captured.")

if __name__ == "__main__":
    main()
