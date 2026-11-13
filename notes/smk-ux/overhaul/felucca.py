#!/usr/bin/env python3
"""Device felucca text: packed Barlow from tools/gen_font.py (FONT_LZ_CROP),
same painter as firmware/src/gfx.c:171–253 at 94197fc8.

Not PIL TTF. Advances, coverage 0..15, and lowercase fold match the device.
"""
from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

# Reuse RGB565 blend from the glyph port
import sys
sys.path.insert(0, "/workspace/smk-ux/directions/glyphs")
import shipped_glyphs as G

HEADER = Path("/workspace/smk-ux/overhaul/felucca_font.h")


def _cv_div15(x: int) -> int:
    return (x * 0x8889) >> 19


@dataclass
class Font:
    name: str
    h: int
    pad: int
    first: int
    last: int
    adv: list
    bw: list
    off: list
    data: bytes
    # decoded (gi -> list of rows, each a list of 0..15, width=bw[gi])
    # actually cropped: we store (crop_x, crop_y, w, height, pixels[h][w])
    glyphs: dict


def _parse_header(path: Path) -> dict[str, Font]:
    text = path.read_text()
    assert "FONT_LZ_CROP" in text
    fonts = {}

    def arr_u8(name):
        m = re.search(rf"static const uint8_t {name}\[(\d+)\] = \{{(.*?)\}};", text, re.S)
        if not m:
            raise ValueError(name)
        nums = [int(x, 0) for x in re.findall(r"0x[0-9a-fA-F]+|\d+", m.group(2))]
        return nums

    def arr_u16(name):
        m = re.search(rf"static const uint16_t {name}\[(\d+)\] = \{{(.*?)\}};", text, re.S)
        if not m:
            raise ValueError(name)
        return [int(x) for x in m.group(2).replace("\n", " ").split(",") if x.strip()]

    for name in ("T", "XS", "S", "B", "M", "L"):
        m = re.search(
            rf"static const felucca_font_t FONT_{name} = \{{ (\d+), (\d+), (\d+), (\d+),",
            text,
        )
        h, pad, first, last = map(int, m.groups())
        f = Font(
            name=name, h=h, pad=pad, first=first, last=last,
            adv=arr_u8(f"FONT_{name}_ADV"),
            bw=arr_u8(f"FONT_{name}_BW"),
            off=arr_u16(f"FONT_{name}_OFF"),
            data=bytes(arr_u8(f"FONT_{name}_DATA")),
            glyphs={},
        )
        n = last - first + 1
        assert len(f.adv) == n and len(f.bw) == n and len(f.off) == n
        fonts[name] = f
        _decode_all(f)
    return fonts


def _decode_lz_crop(data: bytes, start: int) -> tuple[int, int, int, int, list]:
    """One glyph, FONT_LZ_CROP. Returns crop_x, crop_y, w, height, pixels[height][w] 0..15."""
    gd = data
    i = start
    rectangle = gd[i] | (gd[i + 1] << 8) | (gd[i + 2] << 16)
    i += 3
    crop_x = rectangle & 63
    crop_y = (rectangle >> 6) & 63
    w = (rectangle >> 12) & 63
    height = rectangle >> 18
    history = [0] * 256
    written = 0
    left = 0
    distance = 0
    byte = 0
    pixels = []
    for gy in range(height):
        row = []
        for gx in range(w):
            if (gx & 1) == 0:
                if not left:
                    token = gd[i]
                    i += 1
                    if token & 128:
                        distance = gd[i]
                        i += 1
                    else:
                        distance = 0
                    left = (token & 127) + (3 if distance else 1)
                if distance:
                    byte = history[(written - distance) & 255]
                else:
                    byte = gd[i]
                    i += 1
                history[written & 255] = byte
                written += 1
                left -= 1
            a = (byte & 15) if (gx & 1) else (byte >> 4)
            row.append(a)
        pixels.append(row)
    return crop_x, crop_y, w, height, pixels


def _decode_all(f: Font) -> None:
    for gi in range(len(f.adv)):
        f.glyphs[gi] = _decode_lz_crop(f.data, f.off[gi])


FONTS = _parse_header(HEADER)


def _gi(f: Font, ch: int) -> int:
    # gfx.c:160–168 — lowercase folds only when the font has no lowercase (last < 'a')
    if 97 <= ch <= 122 and f.last < 97:
        ch -= 32
    if ch < f.first or ch > f.last or (127 <= ch < 160):
        ch = ord("?")
    return ch - f.first


def text_w(role: str, s: str) -> int:
    f = FONTS[role]
    w = 0
    for ch in s:
        w += f.adv[_gi(f, ord(ch))]
    return w


def cv_text(cv: G.Canvas, x: int, y: int, role: str, s: str, color: int,
            clip_left: int = 0, clip_right: int | None = None) -> int:
    """gfx.c cv_text_clip. colour is RGB565 (unswapped, same as Canvas). Returns end x."""
    f = FONTS[role]
    if clip_right is None:
        clip_right = cv.w
    # unpack ink
    r = color >> 11
    g = (color >> 5) & 63
    b = color & 31
    px = x
    for ch in s:
        gi = _gi(f, ord(ch))
        crop_x, crop_y, w, height, pixels = f.glyphs[gi]
        for gy in range(height):
            row = pixels[gy]
            for gx in range(w):
                a = row[gx]
                if not a:
                    continue
                dx = px - f.pad + gx + crop_x
                dy = y + gy + crop_y
                if dx < clip_left or dx >= clip_right or dx < 0 or dx >= cv.w or dy < 0 or dy >= cv.h:
                    continue
                if a == 15:
                    cv.pset(dx, dy, color)
                else:
                    bg = cv.get(dx, dy) if hasattr(cv, "get") else None
                    if bg is None:
                        # Canvas stores unswapped RGB565
                        bg = cv.px[dy * cv.w + dx]
                    br, bgg, bb = bg >> 11, (bg >> 5) & 63, bg & 31
                    nr = _cv_div15(r * a + br * (15 - a))
                    ng = _cv_div15(g * a + bgg * (15 - a))
                    nb = _cv_div15(b * a + bb * (15 - a))
                    cv.pset(dx, dy, (nr << 11) | (ng << 5) | nb)
        px += f.adv[gi]
    return px


def text_c(cv, cx, y, role, s, color):
    w = text_w(role, s)
    return cv_text(cv, cx - w // 2, y, role, s, color)


def text_r(cv, rx, y, role, s, color):
    w = text_w(role, s)
    return cv_text(cv, rx - w, y, role, s, color)


# NOTE-cell fallback: FONT_M → S → XS, then drop /Bass, then short forms. 54 px. §9.7, §10.2
NOTE_W = 54


def note_font_and_text(s: str) -> tuple[str, str]:
    """Pick a role and a string that fits the 54 px NOTE cell. Rec / §9 rules."""
    # Parentheses or a flat degree must start at FONT_S (FONT_M folds/drops them).
    needs_s = ("(" in s) or (")" in s) or ("b" in s and any(c.isdigit() for c in s))
    # lowercase in a name that isn't only a slash bass: FONT_M would paint CMIN7
    if any(c.islower() for c in s):
        needs_s = True
    order = (("S", "XS") if needs_s else ("M", "S", "XS"))
    if text_w(order[0], s) <= NOTE_W:
        return order[0], s
    for role in order:
        if text_w(role, s) <= NOTE_W:
            return role, s
    # drop /Bass
    if "/" in s:
        s2 = s.split("/", 1)[0]
        for role in ("S", "XS"):
            if text_w(role, s2) <= NOTE_W:
                return role, s2
    # registered short
    short = s.replace("minmaj7", "minM7")
    if "/" in short:
        short = short.split("/", 1)[0]
    for role in ("S", "XS"):
        if text_w(role, short) <= NOTE_W:
            return role, short
    return "XS", short


if __name__ == "__main__":
    samples = [
        ("T", "TUNE"), ("T", "ACCENT"), ("T", "14"), ("T", "0.50"),
        ("S", "Cmin7"), ("S", "Cmaj7"), ("S", "Cadd9"), ("S", "C(b2 2)"),
        ("S", "Gsus4/D"), ("S", "Cmin7b5"), ("XS", "A#min7b5"),
        ("M", "C"), ("M", "C5"), ("M", "C6"), ("M", "125"),
        ("B", "SOUND"), ("B", "SEQ"), ("B", "JUNO A"),
        ("XS", "FACTORY"),
    ]
    for role, s in samples:
        print(f"{role:2s} {s!r:16s} w={text_w(role,s):3d}  note={note_font_and_text(s)}")
    # visual smoke
    cv = G.Canvas(240, 40, G.C_BLACK)
    x = 4
    x = cv_text(cv, x, 4, "B", "SOUND  ", G.RGB(255, 184, 70))
    x = cv_text(cv, x, 4, "S", "Cmin7  ", G.C_WHITE)
    x = cv_text(cv, x, 4, "S", "C(b2 2)", G.C_WHITE)
    cv.to_pil().resize((720, 120), __import__("PIL").Image.Resampling.NEAREST).save(
        "/tmp/felucca-smoke.png"
    )
    print("smoke /tmp/felucca-smoke.png")
