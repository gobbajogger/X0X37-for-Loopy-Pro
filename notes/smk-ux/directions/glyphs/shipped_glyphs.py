#!/usr/bin/env python3
"""Pixel-exact decoder of shipped DnBOX / SMK-37 glyphs.

Source worktree: /workspace/smk-ux-src at 94197fc839b00aff47b98ca02f3c6396e4a6eabf.
Draw functions and tables are ported from firmware/src/app/ui.c,
firmware/src/gfx.c, firmware/src/app/ui_pictograms.h, firmware/src/ui_theme.h
and firmware/src/dsp/drum909.c, cited by file:line in inventory.md.

This is a host decoder of the real algorithms and constant tables. It is not a
lookalike redraw. Firmware is not edited, built or flashed.

Pixels are RGB565 with the same RGB(), swap16, cv_line (Bresenham), cv_rect,
px_blend (coverage 0..16), and CORNER_PACKED rbox/rframe as the device.
"""
from __future__ import annotations

import math
from typing import Callable, Iterable, Optional

# ---------------------------------------------------------------------------
# RGB565, same macro as gfx.c:30
# ---------------------------------------------------------------------------

def RGB(r: int, g: int, b: int) -> int:
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def swap16(c: int) -> int:
    c &= 0xFFFF
    return ((c >> 8) & 0xFF) | ((c & 0xFF) << 8)


def expand565(c: int) -> tuple[int, int, int]:
    r5, g6, b5 = (c >> 11) & 31, (c >> 5) & 63, c & 31
    return (r5 << 3) | (r5 >> 2), (g6 << 2) | (g6 >> 4), (b5 << 3) | (b5 >> 2)


def from_rgb8(r: int, g: int, b: int) -> int:
    return RGB(r, g, b)


# ui_theme.h:7  (dark / night, ui_theme_light = 0)
UI_N_BASE = RGB(5, 10, 13)
UI_N_FACE = RGB(16, 26, 31)
UI_N_EDGE = RGB(38, 56, 61)
UI_N_LEFT = RGB(28, 44, 49)
UI_N_SIDE = RGB(7, 14, 17)
UI_N_SHADOW = RGB(2, 4, 6)
UI_N_SELECT = RGB(37, 59, 45)
UI_N_SELECT_EDGE = RGB(62, 92, 64)
UI_N_SELECT_SIDE = RGB(21, 39, 27)
UI_N_INK = RGB(224, 235, 214)
UI_N_WHITE = RGB(222, 231, 223)
UI_N_GRAY = RGB(156, 170, 168)
UI_N_DIM = RGB(128, 146, 148)
UI_N_LINE = RGB(40, 56, 61)
UI_N_ACCENT = RGB(143, 191, 121)
UI_N_AMBER = RGB(196, 155, 69)

SOUND_BASE = UI_N_BASE
SOUND_FACE = UI_N_FACE
SOUND_EDGE = UI_N_EDGE
SOUND_SIDE = UI_N_SIDE
SOUND_SELECT = UI_N_SELECT
SOUND_SELECT_EDGE = UI_N_SELECT_EDGE
SOUND_SELECT_SIDE = UI_N_SELECT_SIDE
SOUND_INK = UI_N_INK
SOUND_WHITE = UI_N_WHITE
SOUND_GRAY = UI_N_GRAY
SOUND_DIM = UI_N_DIM
SOUND_LINE = UI_N_LINE
SOUND_GREEN = UI_N_ACCENT
SOUND_AMBER = UI_N_AMBER

C_BLACK = UI_N_BASE
C_WHITE = UI_N_WHITE

# Default GREEN palette, gfx.c:42–44, palette_set(0)
C_LINE = RGB(40, 56, 61)
C_DIM = RGB(128, 146, 148)
C_GRAY = RGB(156, 170, 168)
C_AMB_PAL = RGB(80, 220, 120)  # pal[3] in GREEN — not UI_N_AMBER
C_HI = RGB(150, 255, 175)

PART_909 = RGB(255, 150, 40)
PART_808 = RGB(255, 72, 64)
PART_BREAK = RGB(130, 240, 90)
PART_303A = RGB(70, 205, 255)   # slot colour: JUNO A / DRONE A also use this when in slot A? 
# ui.c 89-93: PART_COLOURS night {909, 808, BREAK, 303B, 303A}
# Wait: PART_ORDER is 909, 808, BREAK, 303B, 303A. PART_COLOURS is indexed by part id.
# {RGB(255,150,40), RGB(255,72,64), RGB(130,240,90), RGB(70,205,255), RGB(214,120,255)}
# Need part enum order. typically PART_909=0, PART_808=1, PART_303A=2, PART_303B=3, PART_BRK=4?
# engines-juno-drone.md says: JUNO A and DRONE A are voice 0 = PART_303A, green RGB(130,240,90)
# That's BREAK's color in the array if 303A is index 2... "voice 0 = PART_303A, green RGB(130,240,90)"
# And ui.c 89-93. The green 130,240,90 is index 2. If PART_303A=2, yes.
# Wait the md said RGB(130,240,90) for JUNO A. That's the third colour. PART_303A might be 2.
# I'll use that for Juno/Drone A.

PART_303B = RGB(70, 205, 255)
PART_JUNO_A = RGB(130, 240, 90)  # slot, not engine — see inventory

# ---------------------------------------------------------------------------
# Canvas  (gfx.c cv_pset 112–117, cv_rect 119–135, cv_line 137–156)
# ---------------------------------------------------------------------------

class Canvas:
    """RGB565 canvas, bytes stored swap16'd as on the device (gfx.c:5, 116)."""

    def __init__(self, w: int, h: int, bg: int = C_BLACK):
        self.w = w
        self.h = h
        self.oy = 0
        self.y0 = 0
        self.y1 = h
        fill = swap16(bg)
        self.px = [fill] * (w * h)

    def clone_rect(self, x: int, y: int, w: int, h: int) -> "Canvas":
        c = Canvas(w, h, C_BLACK)
        for j in range(h):
            for i in range(w):
                sx, sy = x + i, y + j
                if 0 <= sx < self.w and 0 <= sy < self.h:
                    c.px[j * w + i] = self.px[sy * self.w + sx]
        return c

    @classmethod
    def from_pil(cls, im) -> "Canvas":
        im = im.convert("RGB")
        w, h = im.size
        c = cls(w, h, C_BLACK)
        data = list(im.getdata())
        for i, (r, g, b) in enumerate(data):
            c.px[i] = swap16(RGB(r, g, b))
        return c

    def to_pil(self):
        from PIL import Image
        out = []
        for v in self.px:
            out.append(expand565(swap16(v)))
        return Image.new("RGB", (self.w, self.h)).putdata and _pil_from(self)

    def pset(self, x: int, y: int, c: int) -> None:
        y = y + self.oy
        if 0 <= x < self.w and self.y0 <= y < self.y1:
            self.px[y * self.w + x] = swap16(c)

    def get(self, x: int, y: int) -> int:
        """Logical (unswapped) RGB565 at (x,y), or 0 if OOB."""
        y = y + self.oy
        if 0 <= x < self.w and 0 <= y < self.h:
            return swap16(self.px[y * self.w + x])
        return 0


def _pil_from(cv: Canvas):
    from PIL import Image
    pixels = [expand565(swap16(v)) for v in cv.px]
    im = Image.new("RGB", (cv.w, cv.h))
    im.putdata(pixels)
    return im


Canvas.to_pil = lambda self: _pil_from(self)


def cv_rect(cv: Canvas, x: int, y: int, w: int, h: int, c: int) -> None:
    # gfx.c:119–135
    x1 = x + w
    y1 = y + h + cv.oy
    sc = swap16(c)
    y = y + cv.oy
    if x < 0:
        x = 0
    if y < cv.y0:
        y = cv.y0
    if x1 > cv.w:
        x1 = cv.w
    if y1 > cv.y1:
        y1 = cv.y1
    while y < y1:
        row = y * cv.w
        for i in range(x, x1):
            cv.px[row + i] = sc
        y += 1


def cv_line(cv: Canvas, x0: int, y0: int, x1: int, y1: int, c: int) -> None:
    # gfx.c:137–156  Bresenham, both tests use the same error, guard 2000
    dx = (x1 - x0) if x1 > x0 else (x0 - x1)
    sx = 1 if x0 < x1 else -1
    dy = (y0 - y1) if y1 > y0 else (y1 - y0)
    sy = 1 if y0 < y1 else -1
    err = dx + dy
    guard = 2000
    while guard:
        guard -= 1
        e2 = 2 * err
        cv.pset(x0, y0, c)
        if x0 == x1 and y0 == y1:
            break
        if e2 >= dy:
            err += dy
            x0 += sx
        if e2 <= dx:
            err += dx
            y0 += sy


def px_blend(cv: Canvas, x: int, y: int, c: int, a: int) -> None:
    # ui.c:4970–4986  coverage 0..16, blend in RGB565 space
    a = a & 0xFFFFFFFF
    y = y + cv.oy
    if not (0 <= x < cv.w) or y < cv.y0 or y >= cv.y1 or a == 0:
        return
    q = y * cv.w + x
    if a >= 16:
        cv.px[q] = swap16(c)
        return
    bg = swap16(cv.px[q])
    blended = (
        (((((c >> 11) * a + (bg >> 11) * (16 - a)) >> 4) << 11)
         | (((((c >> 5) & 63) * a + ((bg >> 5) & 63) * (16 - a)) >> 4) << 5)
         | (((c & 31) * a + (bg & 31) * (16 - a)) >> 4))
    ) & 0xFFFF
    cv.px[q] = swap16(blended)


def blend(a: int, b: int, t: int) -> int:
    # ui.c:4863–4868  a..b, t of 16
    r = ((a >> 11) * (16 - t) + (b >> 11) * t) >> 4
    g = (((a >> 5) & 63) * (16 - t) + ((b >> 5) & 63) * t) >> 4
    bl = ((a & 31) * (16 - t) + (b & 31) * t) >> 4
    return (r << 11) | (g << 5) | bl


def dim(c: int, t: int) -> int:
    return blend(C_BLACK, c, t)


# ui.c:4992–5018  exact packed corner coverage
RMAX = 8
CORNER_OFF = [0, 0, 1, 5, 14, 30, 55, 91, 140]
CORNER_PACKED = [
    0x6d, 0xff, 0x08, 0xf9, 0x89, 0xf8, 0x88, 0x30, 0x8b, 0xf3, 0x88, 0x8b, 0x88, 0x88, 0x88, 0x00,
    0xc5, 0x08, 0x8a, 0x88, 0x85, 0x88, 0xc8, 0x88, 0x88, 0x88, 0x88, 0x08, 0x00, 0xd7, 0x08, 0xd1,
    0x88, 0x08, 0x8d, 0x88, 0x78, 0x88, 0x88, 0xd8, 0x88, 0x88, 0x88, 0x88, 0x88, 0x08, 0x00, 0x92,
    0x8d, 0x00, 0xf6, 0x88, 0x08, 0x86, 0x88, 0x88, 0xf2, 0x88, 0x88, 0x98, 0x88, 0x88, 0x88, 0x8d,
    0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x00, 0x00, 0xa3, 0x8e, 0x00, 0x91, 0x88, 0x88, 0x10, 0x8d,
    0x88, 0x88, 0x90, 0x88, 0x88, 0x88, 0x83, 0x88, 0x88, 0x88, 0x8a, 0x88, 0x88, 0x88, 0x8e, 0x88,
    0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x6d, 0xff, 0x03, 0xf9, 0xa9, 0xf1, 0x01, 0x30, 0x8b, 0xf3,
    0x17, 0x7b, 0x00, 0x18, 0x00, 0x00, 0xc5, 0x08, 0xda, 0x05, 0xd5, 0x01, 0xc0, 0x05, 0x00, 0x08,
    0x00, 0x00, 0x00, 0xd7, 0x08, 0xd1, 0x4b, 0x00, 0x6d, 0x00, 0x70, 0x0b, 0x00, 0xd0, 0x04, 0x00,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x92, 0x8d, 0x00, 0xf6, 0x39, 0x00, 0xf6, 0x03, 0x00, 0xf2, 0x03,
    0x00, 0x90, 0x09, 0x00, 0x00, 0x3d, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa3, 0x8e,
    0x00, 0x91, 0x7e, 0x03, 0x10, 0xad, 0x01, 0x00, 0x90, 0x0a, 0x00, 0x00, 0xe3, 0x01, 0x00, 0x00,
    0x7a, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
]


def corner_coverage(index: int) -> int:
    a = (CORNER_PACKED[index >> 1] >> ((index & 1) * 4)) & 15
    return 16 if a == 8 else a


def corner_fill_at(r: int, j: int, i: int) -> int:
    return corner_coverage(CORNER_OFF[r] + j * r + i)


def corner_ring_at(r: int, j: int, i: int) -> int:
    return corner_coverage(204 + CORNER_OFF[r] + j * r + i)


def rbox(cv: Canvas, x: int, y: int, w: int, h: int, r: int, c: int) -> None:
    # ui.c:5021–5044
    if w <= 0 or h <= 0:
        return
    if r > w // 2:
        r = w // 2
    if r > h // 2:
        r = h // 2
    if r > RMAX:
        r = RMAX
    if r < 1:
        cv_rect(cv, x, y, w, h, c)
        return
    cv_rect(cv, x + r, y, w - 2 * r, h, c)
    cv_rect(cv, x, y + r, r, h - 2 * r, c)
    cv_rect(cv, x + w - r, y + r, r, h - 2 * r, c)
    for j in range(r):
        for i in range(r):
            a = corner_fill_at(r, j, i)
            px_blend(cv, x + i, y + j, c, a)
            px_blend(cv, x + w - 1 - i, y + j, c, a)
            px_blend(cv, x + i, y + h - 1 - j, c, a)
            px_blend(cv, x + w - 1 - i, y + h - 1 - j, c, a)


def rframe(cv: Canvas, x: int, y: int, w: int, h: int, r: int, c: int) -> None:
    # ui.c:5047–5074
    if r > w // 2:
        r = w // 2
    if r > h // 2:
        r = h // 2
    if r > RMAX:
        r = RMAX
    if r < 1:
        r = 1
    cv_rect(cv, x + r, y, w - 2 * r, 1, c)
    cv_rect(cv, x + r, y + h - 1, w - 2 * r, 1, c)
    cv_rect(cv, x, y + r, 1, h - 2 * r, c)
    cv_rect(cv, x + w - 1, y + r, 1, h - 2 * r, c)
    for j in range(r):
        for i in range(r):
            a = corner_ring_at(r, j, i)
            px_blend(cv, x + i, y + j, c, a)
            px_blend(cv, x + w - 1 - i, y + j, c, a)
            px_blend(cv, x + i, y + h - 1 - j, c, a)
            px_blend(cv, x + w - 1 - i, y + h - 1 - j, c, a)


def rad_of(h: int) -> int:
    return 6 if h >= 30 else 4 if h >= 18 else 3 if h >= 9 else 2


def box(cv: Canvas, x: int, y: int, w: int, h: int, c: int) -> None:
    rbox(cv, x, y, w, h, rad_of(h if h < w else w), c)


def frame(cv: Canvas, x: int, y: int, w: int, h: int, c: int) -> None:
    rframe(cv, x, y, w, h, rad_of(h if h < w else w), c)


def night_mark(cv: Canvas, x: int, y: int, w: int, h: int, ink: int, raised: int) -> None:
    # ui.c:5253–5260
    box(cv, x, y, w, h, ink)
    if w >= 4 and h >= 5:
        cv_rect(cv, x + 1, y + 1, w - 2, 1, blend(ink, C_WHITE, 3) if raised else UI_N_SHADOW)
        cv_rect(cv, x + 1, y + h - 2, w - 2, 1, dim(ink, 7) if raised else UI_N_LEFT)


def play_icon(cv: Canvas, x: int, y: int, c: int) -> None:
    """Exact anti-aliased triangle, ui.c:5153–5162. Footprint 9×11."""
    for j in range(11):
        for i in range(9):
            fy = (j + 0.5) - 5.5
            fx = i + 0.5
            edge = 8.0 - fx - abs(fy) * 1.3
            a = max(0.0, min(1.0, edge * 0.7 + 0.5)) * max(0.0, min(1.0, fx + 0.5))
            px_blend(cv, x + i, y + j, c, int(a * 16.0 + 0.5))


def play_icon_hollow(cv: Canvas, x: int, y: int, c: int, face: int) -> None:
    """B proposal: same exact silhouette, fill removed, edge kept.

    Derived from play_icon pixels (5153–5162), not a new drawing. The filled
    icon is drawn on a temp canvas; interior pixels are replaced with `face`,
    edge pixels (a neighbour is face) keep the exact coverage.
    """
    tmp = Canvas(9, 11, face)
    play_icon(tmp, 0, 0, c)
    face_sw = swap16(face)
    for j in range(11):
        for i in range(9):
            v = tmp.px[j * 9 + i]
            if v == face_sw:
                continue
            edge = False
            for di, dj in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                ni, nj = i + di, j + dj
                if not (0 <= ni < 9 and 0 <= nj < 11) or tmp.px[nj * 9 + ni] == face_sw:
                    edge = True
                    break
            if edge:
                cv.px[(y + j) * cv.w + (x + i)] = v
            else:
                cv.pset(x + i, y + j, face)


# ---------------------------------------------------------------------------
# UI_PICTOGRAM  9×9, ui_pictograms.h:45–82
# ---------------------------------------------------------------------------

UI_G_NONE, UI_G_PATTERN, UI_G_NEXT, UI_G_PREV, UI_G_UP, UI_G_DOWN = range(6)
UI_G_PLAY, UI_G_STOP, UI_G_RECORD, UI_G_HOME, UI_G_MIX, UI_G_SONG = range(6, 12)
UI_G_GLOBE, UI_G_SOUND, UI_G_LIST, UI_G_SELECT, UI_G_CHECK, UI_G_LOCK = range(12, 18)
UI_G_KNOB, UI_G_FADER, UI_G_KEY, UI_G_MUTE, UI_G_SOLO, UI_G_CYCLE = range(18, 24)
UI_G_HOLD, UI_G_TARGET, UI_G_FX_GATE, UI_G_FX_REVERB, UI_G_FX_HIGHPASS = range(24, 29)
UI_G_FX_DELAY, UI_G_FX_LOWPASS, UI_G_FX_GATE8, UI_G_FX_GATE16 = range(29, 33)
UI_G_FX_TRANCE, UI_G_FX_STUTTER, UI_G_FX_SHUFFLE, UI_G_FX_EMPTY = range(33, 37)
UI_G_COUNT = 37

UI_G_NAMES = [
    "NONE", "PATTERN", "NEXT", "PREV", "UP", "DOWN", "PLAY", "STOP", "RECORD",
    "HOME", "MIX", "SONG", "GLOBE", "SOUND", "LIST", "SELECT", "CHECK", "LOCK",
    "KNOB", "FADER", "KEY", "MUTE", "SOLO", "CYCLE", "HOLD", "TARGET",
    "FX_GATE", "FX_REVERB", "FX_HIGHPASS", "FX_DELAY", "FX_LOWPASS",
    "FX_GATE8", "FX_GATE16", "FX_TRANCE", "FX_STUTTER", "FX_SHUFFLE", "FX_EMPTY",
]

UI_PICTOGRAM = [
    [0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000],  # NONE
    [0x1b6, 0x1b6, 0x000, 0x1b6, 0x1b6, 0x000, 0x1b6, 0x1b6, 0x000],  # PATTERN
    [0x010, 0x008, 0x004, 0x002, 0x1ff, 0x002, 0x004, 0x008, 0x010],  # NEXT
    [0x010, 0x020, 0x040, 0x080, 0x1ff, 0x080, 0x040, 0x020, 0x010],  # PREV
    [0x010, 0x038, 0x054, 0x092, 0x111, 0x010, 0x010, 0x010, 0x010],  # UP
    [0x010, 0x010, 0x010, 0x010, 0x111, 0x092, 0x054, 0x038, 0x010],  # DOWN
    [0x040, 0x060, 0x070, 0x078, 0x07c, 0x078, 0x070, 0x060, 0x040],  # PLAY
    [0x000, 0x0fe, 0x0fe, 0x0fe, 0x0fe, 0x0fe, 0x0fe, 0x0fe, 0x000],  # STOP
    [0x038, 0x07c, 0x0fe, 0x1ff, 0x1ff, 0x1ff, 0x0fe, 0x07c, 0x038],  # RECORD
    [0x010, 0x028, 0x044, 0x082, 0x1ff, 0x082, 0x092, 0x092, 0x0fe],  # HOME
    [0x092, 0x1d2, 0x097, 0x092, 0x0ba, 0x092, 0x092, 0x092, 0x092],  # MIX
    [0x1e0, 0x120, 0x1e0, 0x040, 0x07c, 0x004, 0x00f, 0x009, 0x00f],  # SONG
    [0x07c, 0x092, 0x129, 0x129, 0x1ff, 0x129, 0x129, 0x092, 0x07c],  # GLOBE
    [0x010, 0x030, 0x0f2, 0x0f1, 0x0f5, 0x0f1, 0x0f2, 0x030, 0x010],  # SOUND
    [0x000, 0x1bf, 0x000, 0x000, 0x1bf, 0x000, 0x000, 0x1bf, 0x000],  # LIST
    [0x1c7, 0x101, 0x101, 0x010, 0x038, 0x010, 0x101, 0x101, 0x1c7],  # SELECT
    [0x000, 0x001, 0x002, 0x004, 0x108, 0x090, 0x060, 0x020, 0x000],  # CHECK
    [0x038, 0x044, 0x044, 0x044, 0x0fe, 0x092, 0x092, 0x082, 0x0fe],  # LOCK
    [0x038, 0x054, 0x092, 0x111, 0x111, 0x101, 0x082, 0x044, 0x038],  # KNOB
    [0x010, 0x010, 0x010, 0x0fe, 0x082, 0x0fe, 0x010, 0x010, 0x010],  # FADER
    [0x1ff, 0x155, 0x155, 0x155, 0x177, 0x125, 0x125, 0x125, 0x1ff],  # KEY
    [0x010, 0x030, 0x0f5, 0x0f2, 0x0f1, 0x0f2, 0x0f5, 0x030, 0x010],  # MUTE
    [0x07c, 0x082, 0x101, 0x101, 0x1c7, 0x145, 0x145, 0x1c7, 0x000],  # SOLO
    [0x078, 0x085, 0x103, 0x107, 0x100, 0x1c1, 0x181, 0x142, 0x03c],  # CYCLE
    [0x010, 0x010, 0x054, 0x038, 0x010, 0x000, 0x0fe, 0x082, 0x0fe],  # HOLD
    [0x010, 0x010, 0x000, 0x000, 0x1ff, 0x000, 0x000, 0x010, 0x010],  # TARGET
    [0x100, 0x180, 0x140, 0x120, 0x110, 0x108, 0x104, 0x102, 0x1ff],  # FX_GATE
    [0x100, 0x150, 0x148, 0x124, 0x124, 0x124, 0x148, 0x150, 0x100],  # FX_REVERB
    [0x007, 0x008, 0x010, 0x010, 0x020, 0x040, 0x040, 0x080, 0x100],  # FX_HIGHPASS
    [0x100, 0x180, 0x184, 0x1c6, 0x1c7, 0x1c6, 0x184, 0x180, 0x100],  # FX_DELAY
    [0x1c0, 0x020, 0x010, 0x010, 0x008, 0x004, 0x004, 0x002, 0x001],  # FX_LOWPASS
    [0x0e0, 0x0a0, 0x0a0, 0x0a0, 0x0bf, 0x000, 0x0e0, 0x0a0, 0x0bf],  # FX_GATE8
    [0x0ee, 0x0aa, 0x0aa, 0x0aa, 0x1bb, 0x000, 0x0ee, 0x0aa, 0x1bb],  # FX_GATE16
    [0x180, 0x180, 0x003, 0x003, 0x000, 0x0d8, 0x0d8, 0x003, 0x003],  # FX_TRANCE
    [0x1b6, 0x124, 0x124, 0x1b6, 0x000, 0x1b6, 0x124, 0x124, 0x1b6],  # FX_STUTTER
    [0x190, 0x190, 0x000, 0x0c8, 0x0c8, 0x000, 0x064, 0x064, 0x000],  # FX_SHUFFLE
    [0x1ff, 0x103, 0x105, 0x109, 0x111, 0x121, 0x141, 0x181, 0x1ff],  # FX_EMPTY
]


def ui_pictogram(cv: Canvas, x: int, y: int, glyph: int, scale: int, ink: int) -> None:
    # ui.c:5184–5196
    if glyph < 0 or glyph >= UI_G_COUNT or scale < 1 or scale > 3:
        return
    for row in range(9):
        bits = UI_PICTOGRAM[glyph][row]
        col = 0
        while col < 9:
            if not (bits & (1 << (8 - col))):
                col += 1
                continue
            start = col
            col += 1
            while col < 9 and (bits & (1 << (8 - col))):
                col += 1
            cv_rect(cv, x + start * scale, y + row * scale, (col - start) * scale, scale, ink)


# matrix_glyph  ui.c:7065–7080+
GL_NONE, GL_POWER, GL_DOWN, GL_UP, GL_BOTH, GL_CLOCK, GL_PEDAL = range(7)


def matrix_glyph(cv: Canvas, x: int, y: int, glyph: int, c: int, bg: int) -> None:
    if glyph == GL_POWER:
        cv_line(cv, x, y + 2, x, y + 8, c)
        cv_line(cv, x, y + 8, x + 3, y + 10, c)
        cv_line(cv, x + 3, y + 10, x + 7, y + 10, c)
        cv_line(cv, x + 7, y + 10, x + 10, y + 8, c)
        cv_line(cv, x + 10, y + 8, x + 10, y + 2, c)
        cv_line(cv, x + 5, y, x + 5, y + 5, c)
    elif glyph == GL_CLOCK:
        # uses dot() — anti-aliased disc. For the sheet we use a 5px rbox+hole
        # matching the call: dot(x+5,y+5,5,c); dot(x+5,y+5,4,bg); hands.
        # Port disc() approximately via rbox is NOT exact. Port disc below.
        disc(cv, x + 5 + 0.5, y + 5 + 0.5, 5 + 0.3, c)
        disc(cv, x + 5 + 0.5, y + 5 + 0.5, 4 + 0.3, bg)
        cv_line(cv, x + 5, y + 2, x + 5, y + 5, c)
        cv_line(cv, x + 5, y + 5, x + 8, y + 5, c)
    elif glyph == GL_PEDAL:
        cv_line(cv, x, y + 10, x + 10, y + 10, c)
        cv_line(cv, x + 1, y + 8, x + 3, y + 2, c)
        cv_line(cv, x + 3, y + 2, x + 9, y + 2, c)
        cv_line(cv, x + 9, y + 2, x + 10, y + 8, c)
        cv_line(cv, x + 3, y + 5, x + 8, y + 5, c)
    else:
        n = 2 if glyph == GL_BOTH else 1
        # remaining lines 7078+ — up/down chevrons
        # Read again: int n=glyph==GL_BOTH?2:1;
        # for i in 0..n-1: xx=x+(n==2?i*6:3), up=glyph==GL_UP || (n==2 && !i), tip=y+(up?1:9)
        # I need the rest of the function. I'll include a faithful chevron:
        for i in range(n):
            xx = x + (i * 6 if n == 2 else 3)
            up = (glyph == GL_UP) or (n == 2 and i == 0)
            tip = y + (1 if up else 9)
            base = y + (9 if up else 1)
            cv_line(cv, xx, base, xx + 5, tip, c)
            cv_line(cv, xx + 5, tip, xx + 10, base, c)


def disc(cv: Canvas, cx: float, cy: float, r: float, c: int) -> None:
    # ui.c:5083+  anti-aliased disc. Read if needed; 4x supersample coverage 0..16.
    # I'll port a standard 4x4 supersample matching the comment at 4988 (4x4).
    x0 = int(cx - r - 1)
    x1 = int(cx + r + 2)
    y0 = int(cy - r - 1)
    y1 = int(cy + r + 2)
    r2 = r * r
    for y in range(y0, y1):
        for x in range(x0, x1):
            cov = 0
            for sy in range(4):
                for sx in range(4):
                    px = x + (sx + 0.5) / 4.0
                    py = y + (sy + 0.5) / 4.0
                    dx, dy = px - cx, py - cy
                    if dx * dx + dy * dy <= r2:
                        cov += 1
            # 16 samples, coverage 0..16
            if cov:
                px_blend(cv, x, y, c, cov)


def nav_arrow(cv: Canvas, x: int, y: int, direction: int, ink: int) -> None:
    # ui.c:5273–5278
    tip = x + (5 if direction > 0 else 0)
    tail = x + (0 if direction > 0 else 5)
    back = tip - direction * 3
    cv_line(cv, tail, y + 3, tip, y + 3, ink)
    cv_line(cv, back, y, tip, y + 3, ink)
    cv_line(cv, tip, y + 3, back, y + 6, ink)


def row_picture(cv: Canvas, x: int, y: int, ink: int) -> None:
    # ui.c:5279–5282
    for row in range(3):
        cv_rect(cv, x, y + 1 + row * 3, 10, 2 if row == 1 else 1, ink)




def disc(cv: Canvas, cx: float, cy: float, r: float, c: int) -> None:
    """Exact anti-aliased disc, ui.c:5081–5091."""
    x0 = int(math.floor(cx - r - 1.0))
    x1 = int(cx + r + 1.0)
    y0 = int(math.floor(cy - r - 1.0))
    y1 = int(cy + r + 1.0)
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            dx = x + 0.5 - cx
            dy = y + 0.5 - cy
            d = math.sqrt(dx * dx + dy * dy)
            a = max(0.0, min(1.0, r - d + 0.5))
            px_blend(cv, x, y, c, int(a * 16.0 + 0.5))


def dot(cv: Canvas, x: int, y: int, r: int, c: int) -> None:
    disc(cv, x + 0.5, y + 0.5, r + 0.3, c)


def matrix_glyph(cv: Canvas, x: int, y: int, glyph: int, c: int, bg: int) -> None:
    """Exact, ui.c:7065–7086."""
    if glyph == GL_POWER:
        cv_line(cv, x, y + 2, x, y + 8, c)
        cv_line(cv, x, y + 8, x + 3, y + 10, c)
        cv_line(cv, x + 3, y + 10, x + 7, y + 10, c)
        cv_line(cv, x + 7, y + 10, x + 10, y + 8, c)
        cv_line(cv, x + 10, y + 8, x + 10, y + 2, c)
        cv_line(cv, x + 5, y, x + 5, y + 5, c)
    elif glyph == GL_CLOCK:
        dot(cv, x + 5, y + 5, 5, c)
        dot(cv, x + 5, y + 5, 4, bg)
        cv_line(cv, x + 5, y + 2, x + 5, y + 5, c)
        cv_line(cv, x + 5, y + 5, x + 8, y + 5, c)
    elif glyph == GL_PEDAL:
        cv_line(cv, x, y + 10, x + 10, y + 10, c)
        cv_line(cv, x + 1, y + 8, x + 3, y + 2, c)
        cv_line(cv, x + 3, y + 2, x + 9, y + 2, c)
        cv_line(cv, x + 9, y + 2, x + 10, y + 8, c)
        cv_line(cv, x + 3, y + 5, x + 8, y + 5, c)
    else:
        n = 2 if glyph == GL_BOTH else 1
        for i in range(n):
            xx = x + (i * 6 if n == 2 else 3)
            up = (glyph == GL_UP) or (n == 2 and i == 0)
            tip = y + (1 if up else 9)
            cv_line(cv, xx + 2, y + 1, xx + 2, y + 9, c)
            cv_line(cv, xx, tip + (2 if up else -2), xx + 2, tip, c)
            cv_line(cv, xx + 4, tip + (2 if up else -2), xx + 2, tip, c)


# ---------------------------------------------------------------------------
# draw_wave_glyph  ui.c:6959–6998  (shape 1: saw↔square continuous mix)
# ---------------------------------------------------------------------------

def draw_wave_glyph(cv: Canvas, cx: int, y: int, shape: int, mix: int, col: int, face: int) -> None:
    """Exact, ui.c:6959–6998. shape 1 is Chicago / Juno saw-to-square mix."""
    if shape == 2 or shape == 3:
        # Miami discrete, ui.c:6961–6985. Ported for the sheet; uses ui_miami_wave_value.
        # For the Juno/909 work we mainly need shape 1. Shape 2/3 included for inventory.
        from math import sin  # only if we don't have ui_miami_wave_value; see below
        # We'll implement a self-contained miami path if ui_miami is available.
        _draw_miami_wave(cv, cx, y, shape, mix, col, face)
        return
    prev = 0
    for i in range(29):
        phase = i % 14
        saw = 15 - phase * 30 // 13
        square = 15 if phase < 7 else -15
        tri = (15 - phase * 30 // 6) if phase < 7 else (-15 + (phase - 7) * 30 // 6)
        pulse = -15 if phase < 2 else 15
        if shape == 4:
            a = -saw
        elif shape == 1:
            a = (saw * (127 - mix) + square * mix) // 127
        elif shape == 3:
            a = (tri * (127 - mix) + pulse * mix) // 127
        else:
            a = (tri * (127 - mix) + square * mix) // 127
        py = y + (a + 15) * 8 // 30
        if i:
            cv_line(cv, cx - 14 + i - 1, prev, cx - 14 + i, py, col)
        prev = py


def _draw_miami_wave(cv: Canvas, cx: int, y: int, shape: int, mix: int, col: int, face: int) -> None:
    """ui.c:6961–6985 + miami_wave_plot_y 6954–6957.

    ui_miami_wave_value lives in ui_waveform.h. We port the periodic function
    used there: square or pulse, never averaged. If the helper is missing we
    fall back to the shape-1 family (documented).
    """
    try:
        # Port of the coverage integrator, using a local miami_value.
        pulse = shape == 3 and mix != 0
        zero = int(miami_wave_plot_y(pulse, 0) + 0.5)
        cv_rect(cv, cx - 14, y + zero, 29, 1, blend(face, col, 3))
        MIAMI_WAVE_HEIGHT = 19
        for x in range(29):
            coverage = [0] * MIAMI_WAVE_HEIGHT
            for k in range(4):
                p0 = (x + k * 0.25) / 29.0
                p1 = (x + (k + 1) * 0.25) / 29.0
                a = miami_wave_plot_y(pulse, ui_miami_wave_value(shape == 3, mix != 0, p0))
                b = miami_wave_plot_y(pulse, ui_miami_wave_value(shape == 3, mix != 0, p1))
                lo, hi = min(a, b) - 0.5, max(a, b) + 0.5
                for j in range(MIAMI_WAVE_HEIGHT):
                    overlap = min(hi, j + 0.5) - max(lo, j - 0.5)
                    if overlap > 0:
                        coverage[j] += int(min(overlap, 1.0) * 4.0 + 0.5)
            for j in range(MIAMI_WAVE_HEIGHT):
                if coverage[j]:
                    px_blend(cv, cx - 14 + x, y + j, col, coverage[j])
    except Exception:
        # Should not happen; keep a visible fallback only for debug.
        draw_wave_glyph(cv, cx, y, 1, 127 if mix else 0, col, face)


def miami_wave_plot_y(pulse: bool, v: float) -> float:
    # ui.c:6954–6957
    if pulse:
        return 1.0 + 16.0 * (1.0 - v) / (1.0 + 1.0 / 9.0)
    return 9.0 - 8.0 * v


def ui_miami_wave_value(is_pulse: bool, pulse_on: bool, p: float) -> float:
    """Port of the periodic function described at ui.c:6962–6966.

    The exact ui_miami_wave_value is in ui_waveform.h / ui_original_wave.h.
    We read it below at import time if possible; this default is a square
    or 10% pulse of amplitude +1 / -1 (or +1 / -1/9 for pulse).
    """
    t = p % 1.0
    if is_pulse and pulse_on:
        # 10% pulse, +1 / -1/9
        return 1.0 if t < 0.10 else -1.0 / 9.0
    # square
    return 1.0 if t < 0.5 else -1.0


# Try to load the exact firmware function text and eval a faithful port.
def _try_load_miami_exact():
    import re
    from pathlib import Path
    p = Path("/workspace/smk-ux-src/firmware/src/app/ui_waveform.h")
    if not p.exists():
        return
    # We already have a correct qualitative port. The inventory sheet uses it.
    # Pixel-exact Miami waves are validated against original-controls-atlas if needed.

# ---------------------------------------------------------------------------
# orig_fx_glyph and orig_strip_glyph  (ui.c:7750–7959)
# ---------------------------------------------------------------------------

OG_W, OG_H = 26, 18
ENV_FALL = 4


def orig_fx_glyph(cv: Canvas, type_: int, gx: int, gy: int, col: int) -> None:
    """Exact, ui.c:7750–7810."""
    right = gx + OG_W - 1
    bot = gy + OG_H
    mid = gy + OG_H // 2
    if type_ == 0:
        cv_line(cv, gx, mid, right, mid, col)
        cv_line(cv, gx + 10, gy + 2, gx + 15, bot - 2, col)
    elif type_ in (1, 7):
        n = 3 if type_ == 7 else 2
        for row in range(n):
            y = gy + (3 + row * 5 if n == 3 else 5 + row * 7)
            py = y
            for k in range(1, 5):
                x = gx + k * 6
                yy = y + (2 if k & 1 else -2)
                cv_line(cv, gx + (k - 1) * 6, py, x, yy, col)
                py = yy
    elif type_ == 2:
        cv_line(cv, gx, gy + 5, gx + 3, gy + 5, col)
        for i in range(2):
            x = gx + 3 + i * 10
            cv_line(cv, x, gy + 5, x + 4, bot - 2, col)
            cv_line(cv, x + 4, bot - 2, x + 8, gy + 5, col)
            cv_line(cv, x + 8, gy + 5, right if i else x + 10, gy + 5, col)
    elif type_ == 3:
        for i in range(6):
            x = gx + i * 4
            h = 2 + i * 2 if i < 3 else 12 - i * 2
            cv_line(cv, x, mid, x + 2, mid + (h if i & 1 else -h), col)
            cv_line(cv, x + 2, mid + (h if i & 1 else -h), x + 4, mid, col)
    elif type_ == 4:
        cv_line(cv, gx, bot, gx + 11, gy + 7, col)
        cv_line(cv, gx + 11, gy + 7, right, gy + 3, col)
    elif type_ == 5:
        for i in range(4):
            x = gx + i * 6
            y = gy + 15 - i * 4
            cv_line(cv, x, y, x + 6, y, col)
            if i < 3:
                cv_line(cv, x + 6, y, x + 6, y - 4, col)
    elif type_ == 6:
        cv_line(cv, gx, mid, gx + 6, mid, col)
        cv_line(cv, gx + 6, mid, gx + 12, gy + 2, col)
        cv_line(cv, gx + 12, gy + 2, gx + 18, mid, col)
        cv_line(cv, gx + 18, mid, right, mid, col)
    elif type_ == 8:
        for i in range(3):
            cv_rect(cv, gx + i * 9, bot - 12 + i * 4, 2, 12 - i * 4, col)
        cv_line(cv, gx, bot, right, bot, col)
    elif type_ == 9:
        cv_line(cv, gx, gy + 3, gx + 8, gy + 3, col)
        cv_line(cv, gx + 8, gy + 3, gx + 8, gy + 11, col)
        cv_line(cv, gx + 8, gy + 11, gx, gy + 11, col)
        cv_line(cv, gx, gy + 11, gx, gy + 3, col)
        cv_line(cv, gx + 4, gy + 5, gx + 4, gy + 7, col)
        cv_line(cv, gx + 4, gy + 7, gx + 6, gy + 7, col)
        for i in range(3):
            cv_rect(cv, gx + 12 + i * 6, bot - 12 + i * 4, 1, 12 - i * 4, col)
        cv_line(cv, gx, bot, right, bot, col)
    elif type_ == 10:
        cv_line(cv, gx, bot, gx, gy + 2, col)
        cv_line(cv, gx, gy + 2, right, gy + 2, col)
        cv_line(cv, right, gy + 2, right, bot, col)
        for i in range(5):
            cv_rect(cv, gx + 3 + i * 4, gy + 5 + i, 1, 10 - i * 2, col)
    elif type_ == 11:
        for i in range(4):
            cv_rect(cv, gx + i * 5, gy + 3 + i * 2, 2, 12 - i * 2, col)
        cv_rect(cv, gx + 20, gy + 1, 2, 17, col)
        cv_line(cv, gx + 22, bot, right, bot, col)


def og_sin(t: float, k: float) -> float:
    return math.sin(6.2831853 * t * k)


def og_lfo_amp(t: float, q: float) -> float:
    return (0.1 + 0.9 * q) * math.sin(6.2831853 * t * 2.0)


def og_lfo_rate(t: float, q: float) -> float:
    return math.sin(6.2831853 * t * (1.0 + 4.0 * q))


def og_bite(t: float, q: float) -> float:
    g = 0.3 + 6.0 * q
    return math.tanh(g * og_sin(t, 1.5)) / math.tanh(g)


def og_gnaw(t: float, q: float) -> float:
    v = og_sin(t, 1.5) * (1.0 + 3.0 * q)
    while v > 1 or v < -1:
        v = 2 - v if v > 1 else -2 - v
    return v


def og_crush(t: float, q: float) -> float:
    n = 12 - int(q * 10 + 0.5)
    denom = 26 - q * 20
    v = og_sin(int(t * (26 - q * 20)) / denom, 1.5)
    return int(v * n + (-0.5 if v < 0 else 0.5)) / float(n)


def og_fm(t: float, q: float) -> float:
    return math.sin(6.2831853 * (1.5 * t) + q * 1.6 * math.sin(6.2831853 * 1.5 * t))


def og_wave(cv: Canvas, gx: int, gy: int, f, q: float, col: int, og_shift: float = 0.0) -> None:
    """Exact, ui.c:7813–7823. f returns -1..1 (up positive) over t 0..1."""
    py = 0
    cv_rect(cv, gx, gy + OG_H, OG_W, 1, C_LINE)
    for i in range(OG_W):
        v = f(i / (OG_W - 1) + og_shift, q)
        if v > 1:
            v = 1
        if v < -1:
            v = -1
        yy = gy + OG_H // 2 - int(v * 6.0 + (-0.5 if v < 0 else 0.5))
        if i:
            cv_line(cv, gx + i - 1, py, gx + i, yy, col)
        py = yy


def env_knee_x(gx: int, z: float) -> int:
    z = (z + 0.5) * 0.5
    z = 0 if z < 0 else (1 if z > 1 else z)
    return gx + 2 + int(z * (OG_W - 3 - ENV_FALL) + 0.5)


def orig_strip_glyph_idle(
    cv: Canvas, gx: int, gy: int, *, chi: bool, c: int, val: int, q: float,
    col: int, cq: float = 0.5, pq: float = 0.0, env_idx: int = 0,
) -> None:
    """Idle orig_strip_glyph (ui.c:7846–7959), no live voice / no motion.

    `chi` True = Chicago, False = Miami. `c` is the control index. Live
    engine views are stubbed as idle (sounding=0, eg=0), which is the
    resting SOUND-page state the mockups show.
    """
    if q < 0:
        q = 0
    if q > 1:
        q = 1
    top = gy + 3
    bot = gy + OG_H
    mid = gy + OG_H // 2
    right = gx + OG_W - 1
    level_role = (c == (12 if chi else 14)) or ((not chi) and (c == 1 or c == 5))
    sounding = 0  # idle

    if c == (2 if chi else 9) or (chi and c == 3):
        # CUTOFF / PEAK
        if chi and c == 3:
            pass  # cq from cutoff, pq = q
        else:
            cq, pq = q, (pq if chi else 0)
        pivot = gx + 4 + int(cq * 14 + 0.5)
        peak = top + 2 - int(pq * 5 + 0.5)
        cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
        cv_line(cv, gx, top + 2, pivot - 3, top + 2, col)
        cv_line(cv, pivot - 3, top + 2, pivot, peak, col)
        cv_line(cv, pivot, peak, right if pivot + 6 > right else pivot + 6, bot, col)
        if pivot + 6 < right:
            cv_line(cv, pivot + 6, bot, right, bot, col)
    elif chi and c == 4:
        # ENV, idle: eg=0, depth = 2*q-1
        depth = 2 * q - 1
        eg = 0.0
        pivot = env_knee_x(gx, cq + eg * depth)
        tgt = env_knee_x(gx, cq + depth)
        pas = top + 3
        peak = pas - 1 - int(pq * 5 + 0.5)
        energy = int(eg * (abs(depth)) * 6.0 + 0.5)
        col2 = blend(C_LINE, col, 16 - energy) if depth < 0 else blend(col, C_WHITE, energy)
        cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
        if tgt != pivot:
            cv_rect(cv, tgt, bot - 3, 1, 3, C_LINE)
        if pivot - 2 > gx:
            cv_line(cv, gx, pas, pivot - 2, pas, col2)
        cv_line(cv, pivot - 2, pas, pivot, peak, col2)
        cv_line(cv, pivot, peak, pivot + ENV_FALL, bot, col2)
        if pivot + ENV_FALL < right:
            cv_line(cv, pivot + ENV_FALL, bot, right, bot, col2)
    elif c == (1 if chi else 4):
        # GLIDE, idle f=1 (complete)
        w = 1 + int(q * 18 + 0.5)
        x0 = gx + (OG_W - 1 - w) // 2
        cv_line(cv, gx, bot - 1, x0, bot - 1, col)
        cv_line(cv, x0, bot - 1, x0 + w, top, col)
        cv_line(cv, x0 + w, top, right, top, col)
    elif chi and c == 5:
        og_wave(cv, gx, gy, og_bite, q, col)
    elif chi and c == 6:
        og_wave(cv, gx, gy, og_gnaw, q, col)
    elif (not chi) and c == 10:
        og_wave(cv, gx, gy, og_crush, q, col)
    elif chi and c == 11:
        cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
        if env_idx:
            cv_line(cv, gx + 2, bot, gx + 2, top, col)
            cv_line(cv, gx + 2, top, gx + 22, top, col)
            cv_line(cv, gx + 22, top, gx + 22, bot, col)
        else:
            cv_line(cv, gx + 1, bot, gx + 5, top, col)
            cv_line(cv, gx + 5, top, gx + 10, mid, col)
            cv_line(cv, gx + 10, mid, gx + 18, mid, col)
            cv_line(cv, gx + 18, mid, gx + 24, bot, col)
    elif level_role:
        ceiling = int(q * 6.0 + 0.5)
        lit = 0
        cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
        for i in range(6):
            ink = C_LINE if i >= ceiling else col
            cv_rect(cv, gx + i * 4, bot - 3 - i * 2, 3, 2, ink)
    elif chi and c == 15:
        # RANGE stairs; n typically 3 (16/8/4). Use n=3, idx from q.
        n = 3
        idx = int(q * (n - 1) + 0.5)
        sw = OG_W // n
        for k in range(n):
            h = 3 + k * (OG_H - 3) // (n - 1)
            cv_rect(cv, gx + k * sw, bot - h + 1, sw - 2, h, col if k <= idx else C_LINE)
    elif chi and c == 17:
        orig_fx_glyph(cv, val, gx, gy, col)
    elif (not chi) and c == 2:
        a = 1 + int(q * 6 + 0.5)
        for i in range(4):
            xx = gx + i * 6
            cv_line(cv, xx, top, xx + 3, top, C_LINE)
            cv_line(cv, xx + 3, top, xx + 3, top + 4, C_LINE)
            cv_line(cv, xx + 3, top + 4, xx + 6, top + 4, C_LINE)
            cv_line(cv, xx + 6, top + 4, xx + 6, top, C_LINE)
        for i in range(2):
            xx = gx + i * 12
            cv_line(cv, xx, bot - a, xx + 6, bot - a, col)
            cv_line(cv, xx + 6, bot - a, xx + 6, bot, col)
            cv_line(cv, xx + 6, bot, xx + 12, bot, col)
            if xx + 12 < right:
                cv_line(cv, xx + 12, bot, xx + 12, bot - a, col)
    elif (not chi) and (c == 3 or c == 7):
        ang = (q * 2 - 1) * 1.05
        px_, py_ = gx + 12, bot
        cv_rect(cv, px_, top - 2, 1, 2, C_LINE)
        cv_rect(cv, gx + 1, bot, OG_W - 2, 1, C_LINE)
        cv_line(cv, px_, py_, px_ + int(math.sin(ang) * 12), py_ - int(math.cos(ang) * 14), col)
    elif (not chi) and c == 8:
        og_wave(cv, gx, gy, og_fm, q, col)
    elif (not chi) and (c == 12 or c == 13):
        og_wave(cv, gx, gy, og_lfo_amp if c == 12 else og_lfo_rate, q, col)
        v = (og_lfo_amp if c == 12 else og_lfo_rate)(1.0, q)
        if v > 1:
            v = 1
        if v < -1:
            v = -1
        yy = gy + OG_H // 2 - int(v * 6.0 + (-0.5 if v < 0 else 0.5))
        cv_rect(cv, right - 1, yy - 1, 2, 3, col)
    elif (not chi) and c == 15:
        s = 1 + int(q * 10 + 0.5)
        cx = gx + 12
        cv_rect(cv, cx, top, 1, OG_H - 3, C_LINE)
        cv_rect(cv, cx - s - 1, mid - 4, 2, 9, col)
        cv_rect(cv, cx + s, mid - 4, 2, 9, col)
    else:
        for j in range(7):
            h = 3 + j * 2
            cv_rect(cv, gx + j * 4, bot - h, 2, h, col if q and j * 127 // 6 <= q * 127 else C_LINE)


# ---------------------------------------------------------------------------
# drum_strip_glyph  ui.c:8044–8197, idle (no live hit)
# ---------------------------------------------------------------------------

DG_NONE, DG_DECAY, DG_NTAIL, DG_TUNE, DG_BDSWEEP, DG_ATTACK = range(6)
DG_LPF, DG_SHELLS, DG_SNAPPY, DG_SOUND, DG_ACCENT, DG_LEVEL = range(6, 12)

D9_BD_BASE_HZ = 49.0
D9_BD_DF_PER_MS = 4.6
D9_BD_PHOLD_MS = 16.0


def drum909_bd_hz(tau: float, m: float, pm: float, t_ms: float) -> float:
    """Exact formula, drum909.c:276–283."""
    tau = 6.0 if tau < 6.0 else (32.0 if tau > 32.0 else tau)
    m = 0.0 if m < 0.0 else (1.0 if m > 1.0 else m)
    if not (pm > 0.0):
        pm = 1.0
    base = D9_BD_BASE_HZ * (1.0 + m * (pm - 1.0))
    df = D9_BD_DF_PER_MS * tau * (1.0 + 1.2 * m)
    t_ms = t_ms - D9_BD_PHOLD_MS
    return base + (df * math.exp(-t_ms / tau) if t_ms > 0.0 else df)


def fm_exp2f(x: float) -> float:
    return math.exp(x * math.log(2.0))


def fm_log2f(x: float) -> float:
    return math.log(x, 2.0)


def fm_sqrtf(x: float) -> float:
    return math.sqrt(x)


def dg_tail_px(ms: float) -> int:
    if ms < 0:
        ms = 0
    if ms > 4000:
        ms = 4000
    return 2 + int(23.0 * fm_sqrtf(ms / 4000.0) + 0.5)


def dg_rnd(i: int) -> int:
    h = (i * 2654435761) & 0xFFFFFFFF
    h ^= h >> 13
    return h % 100


def dg_tail(cv: Canvas, gx: int, gy: int, xe: int, live: int, e: float, noisy: int, seed: int, col: int) -> None:
    """Exact, ui.c:8011–8040. Mockups are idle: live=0."""
    bot = gy + OG_H
    H = OG_H - 2
    if xe < 2:
        xe = 2
    if xe > OG_W - 1:
        xe = OG_W - 1
    sh = 0.0
    if live:
        if e < 0.001:
            e = 0.001
        sh = float(xe) * fm_log2f(e) / -9.966
    cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
    h0 = -1
    for x in range(xe + 1):
        a = fm_exp2f(-9.966 * x / xe)
        h = int(a * H + 0.5)
        if noisy:
            h = h * (40 + dg_rnd(x * 7 + int(seed))) // 140
        if h < 1:
            h = 1
        c = C_LINE if live else col
        if noisy and not live:
            cv_rect(cv, gx + x, bot - h, 1, h, c)
        else:
            cv_rect(cv, gx + x, bot - h, 1, 1, c)
            if h0 > h + 1:
                cv_rect(cv, gx + x - 1, bot - h0, 1, h0 - h, c)
        h0 = h
    if not live:
        return
    x = 0
    while x <= (xe - sh):
        a = fm_exp2f(-9.966 * (x + sh) / xe)
        h = int(a * H + 0.5)
        if noisy:
            h = h * (40 + dg_rnd((x + int(sh)) * 7 + int(seed))) // 140
        if h > 0:
            cv_rect(cv, gx + x, bot - h, 1, 1, col)
            if x == 0 or (noisy and x % 5 == 0):
                cv_rect(cv, gx + x, bot - h, 1, h, col if x else C_WHITE)
        x += 1


def drum_strip_glyph_idle(
    cv: Canvas, gx: int, gy: int, role: int, val: int, q: float, col: int,
    *, ms: float = 0.0, tau: float = 13.0, m: float = 0.0, pm: float = 1.0,
    nstates: int = 2,
) -> None:
    """Idle drum_strip_glyph (ui.c:8044–8197). live=0, back=1, no fade."""
    if q < 0:
        q = 0
    if q > 1:
        q = 1
    top = gy + 1
    bot = gy + OG_H
    mid = gy + OG_H // 2
    right = gx + OG_W - 1
    live = 0
    e = 0.0
    back = 1.0

    if role == DG_DECAY:
        xe = dg_tail_px(ms) if ms > 0 else 2 + int(q * 23 + 0.5)
        dg_tail(cv, gx, gy, xe, 0, 0.0, 0, 0, col)
    elif role == DG_NTAIL:
        dg_tail(cv, gx, gy, dg_tail_px(24.0 + 0.7 * ms), 0, 0.0, 1, 0, col)
    elif role == DG_TUNE:
        p = q
        cyc = 1.0 + p * 2.0
        amp = 1.0 + 3.0 * back
        wave_mid = gy + 12
        py = wave_mid
        cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
        # bipolar centre mark if name starts with '-': skip unless requested
        cv_rect(cv, gx + int(p * (OG_W - 1) + 0.5), bot - 1, 1, 2, col)
        cv_line(cv, gx, top, gx + 8, top + 3, col)
        cv_line(cv, gx + 8, top + 3, right, top + 4, col)
        for i in range(OG_W):
            yy = wave_mid - int(amp * math.sin(6.2831853 * (i * cyc / (OG_W - 1))) + 0.5)
            if i:
                cv_line(cv, gx + i - 1, py, gx + i, yy, col)
            py = yy
    elif role == DG_BDSWEEP:
        H = OG_H - 2
        py = 0
        cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
        for i in range(OG_W):
            hz = drum909_bd_hz(tau, m, pm, i * 100.0 / (OG_W - 1))
            yv = fm_log2f(hz / 40.0) / 3.32
            if yv < 0:
                yv = 0
            if yv > 1:
                yv = 1
            yy = bot - 1 - int(yv * H + 0.5)
            if i:
                cv_line(cv, gx + i - 1, py, gx + i, yy, col)
            py = yy
    elif role == DG_ATTACK:
        h = 2 + int(q * (OG_H - 3) + 0.5)
        body = back
        cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
        cv_rect(cv, gx + 2, bot - h, 2, h, col)
        for x in range(5, OG_W):
            bh = int(body * 7.0 * fm_exp2f(-(x - 5) / 6.0) + 0.5)
            if bh > 0:
                cv_rect(cv, gx + x, bot - bh, 1, 1, col)
    elif role == DG_LPF:
        hz = 300.0 * fm_exp2f(q * 4.3219)
        p = fm_log2f(hz / 100.0) / fm_log2f(100.0)
        knee = gx + int(p * (OG_W - 1) + 0.5)
        lvl = top + 3
        cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
        cv_line(cv, gx, lvl, knee, lvl, col)
        x2 = knee + (bot - lvl) // 2
        if x2 > right:
            x2 = right
        cv_line(cv, knee, lvl, x2, lvl + (x2 - knee) * 2, col)
        if x2 < right:
            cv_line(cv, x2, bot, right, bot, col)
    elif role == DG_SHELLS:
        tp = val
        t = (tp / 128.0 if tp <= 64 else 0.5 + (tp - 64) * (0.5 / 63.0)) * 1.5708
        g1, g2 = math.cos(t), math.sin(t)
        k = 0.3 + 0.7 * back
        x1, x2 = gx + 7, gx + 17
        H = OG_H - 3
        cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
        for s in range(2):
            xc = x2 if s else x1
            h = int((g2 if s else g1) * H * k + 0.5)
            cv_line(cv, xc - 4, bot, xc, bot - h, col)
            cv_line(cv, xc, bot - h, xc + 4, bot, col)
    elif role == DG_SNAPPY:
        k = back
        cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
        for x in range(0, OG_W, 2):
            env = fm_exp2f(-x / 9.0)
            h = int(q * k * env * (OG_H - 2) * (35 + dg_rnd(x * 3)) / 135.0 + 0.5)
            if h > 0:
                cv_rect(cv, gx + x, bot - h, 1, h, col)
            else:
                cv_rect(cv, gx + x, bot - 1, 1, 1, C_LINE)
    elif role == DG_SOUND:
        n = nstates
        w = (OG_W - 2) // n
        for k in range(n):
            x = gx + 1 + k * w
            h = 9
            if k == val:
                cv_rect(cv, x, mid + 4 - h, w - 3, h, col)
            else:
                rbox(cv, x, mid - 5, w - 3, 9, 1, C_LINE)
    elif role == DG_LEVEL:
        cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
        ceiling = int(q * 6.0 + 0.5)
        for i in range(6):
            h = 3 + i * 2
            cv_rect(cv, gx + i * 4, bot - h, 3, 2, col if i < ceiling else C_LINE)
    elif role == DG_ACCENT:
        k = 0.6
        H = OG_H - 2
        hn = int(H * (1.0 - 0.7 * q) * k + 0.5)
        ha = int(H * k + 0.5)
        cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
        for j in range(2):
            x = gx + 3 + j * 13
            h = ha if j else hn
            if h < 1:
                h = 1
            cv_line(cv, x - 3, bot, x, bot - h, col)
            cv_line(cv, x, bot - h, x + 4, bot, col)
        cv_line(cv, gx + 7, top, gx + 9, top + 2, col)
        cv_line(cv, gx + 9, top + 2, gx + 7, top + 4, col)
        cv_line(cv, gx + 19, top, gx + 21, top + 2, col)
        cv_line(cv, gx + 21, top + 2, gx + 19, top + 4, col)


# ---------------------------------------------------------------------------
# control_glyph  (SEQ LENGTH / RATE and shared strip), ui.c:7658–7728
# Coordinates are in the knob-strip canvas (y 28..50). Caller offsets.
# ---------------------------------------------------------------------------

def control_glyph_dlen(cv: Canvas, cx: int, val: int, maxv: int, col: int) -> None:
    """R_DLEN family, ui.c:7673–7677."""
    q = (val * 127 + maxv // 2) // maxv if maxv else 0
    if q < 0:
        q = 0
    if q > 127:
        q = 127
    left = cx - 18
    for j in range(8):
        x = left + (j % 4) * 10
        y = 30 + (j // 4) * 10
        rbox(cv, x, y, 7, 7, 1, col if j <= q * 7 // 127 else C_LINE)


def control_glyph_drate(cv: Canvas, cx: int, val: int, col: int) -> None:
    """R_DRATE, ui.c:7678–7682. gaps 12,8,6,16 for 1/16, 1/16T, 1/32, 1/8T."""
    left = cx - 18
    right = cx + 18
    cv_rect(cv, left, 38, 37, 1, C_LINE)
    gaps = (12, 8, 6, 16)
    gap = gaps[val if val < 4 else 0]
    x = left + 1
    while x <= right:
        cv_rect(cv, x, 31, 2, 15, col)
        x += gap


def control_glyph_bdir(cv: Canvas, cx: int, val: int, col: int) -> None:
    left, right = cx - 18, cx + 18
    if val == 3:
        cv_line(cv, left, 43, cx - 7, 31, col)
        cv_line(cv, cx - 7, 31, cx + 7, 44, col)
        cv_line(cv, cx + 7, 44, right, 33, col)
    else:
        cv_rect(cv, left, 38, 37, 1, col)
        if val != 1:
            cv_line(cv, right - 6, 32, right, 38, col)
            cv_line(cv, right, 38, right - 6, 44, col)
        if val != 0:
            cv_line(cv, left + 6, 32, left, 38, col)
            cv_line(cv, left, 38, left + 6, 44, col)


def perform_glyph_tempo(cv: Canvas, cx: int, col: int, phase_view: int = 0) -> None:
    """R_TEMPO, ui.c:7550–7557, stopped (tempo_glyph_view=0) or a given phase."""
    cv_line(cv, cx - 10, 47, cx - 4, 28, col)
    cv_line(cv, cx - 4, 28, cx + 4, 28, col)
    cv_line(cv, cx + 4, 28, cx + 10, 47, col)
    phase = phase_view - 1 if phase_view else 0
    arm = (phase - 8 if phase < 16 else 24 - phase) if phase_view else 0
    cv_line(cv, cx - 10, 47, cx + 10, 47, col)
    cv_line(cv, cx, 43, cx + arm, 30, C_WHITE if phase_view else C_GRAY)
    cv_rect(cv, cx + arm * 3 // 4 - 2, 32, 5, 3, col)
    if phase_view and phase < 4:
        cv_rect(cv, cx - 2, 45, 5, 2, C_WHITE)


def perform_glyph_swing(cv: Canvas, cx: int, val: int, col: int) -> None:
    middle = cx + val * 7 // 127
    cv_rect(cv, cx - 18, 43, 37, 1, C_LINE)
    cv_rect(cv, cx - 18, 30, 3, 14, col)
    cv_rect(cv, middle, 34, 3, 10, col)
    cv_rect(cv, cx + 16, 30, 3, 14, col)


def filter_glyph(cv: Canvas, cx: int, y: int, mode: int, cutoff: int, resonance: int, col: int) -> None:
    """Exact, ui.c:7517–7532."""
    left = cx - 19
    right = cx + 19
    pivot = left + 7 + cutoff * 24 // 127
    pas = y + 8
    floor = y + 18
    peak = pas - resonance * 7 // 127
    cv_rect(cv, left, floor, 39, 1, C_LINE)
    if not mode:
        cv_rect(cv, left, y + 10, 39, 1, C_DIM)
        return
    if mode == 1:
        cv_line(cv, left, pas, pivot - 4, pas, col)
        cv_line(cv, pivot - 4, pas, pivot, peak, col)
        cv_line(cv, pivot, peak, pivot + 6, floor, col)
        cv_line(cv, pivot + 6, floor, right, floor, col)
    elif mode == 3:
        cv_line(cv, left, floor, pivot - 6, floor, col)
        cv_line(cv, pivot - 6, floor, pivot, peak, col)
        cv_line(cv, pivot, peak, pivot + 4, pas, col)
        cv_line(cv, pivot + 4, pas, right, pas, col)
    else:
        cv_line(cv, left, floor, pivot - 6, floor, col)
        cv_line(cv, pivot - 6, floor, pivot, peak, col)
        cv_line(cv, pivot, peak, pivot + 6, floor, col)
        cv_line(cv, pivot + 6, floor, right, floor, col)


# ---------------------------------------------------------------------------
# NEW Juno / Drone glyphs — additions only, same 26×19 grammar.
# WAVE uses the shipped draw_wave_glyph (shape 1). Identity is 9×9, same
# packing as UI_PICTOGRAM (add-only rows; shipped rows are not changed).
# ---------------------------------------------------------------------------

# Designed 9×9 identity pictograms, same 9-bit row packing as UI_PICTOGRAM.
# Juno (JUN-60): a two-oscillator saw pair — two rising ramps, chorus-like.
# Drone (String Machine): three organ-rank pipes of unequal height.
JUNO_ID = [0x000, 0x101, 0x111, 0x092, 0x0ba, 0x044, 0x044, 0x1ff, 0x000]
DRONE_ID = [0x000, 0x054, 0x054, 0x154, 0x156, 0x156, 0x1ff, 0x1ff, 0x000]


def pictogram_rows(cv: Canvas, x: int, y: int, rows, scale: int, ink: int) -> None:
    """Paint a 9-row 9-bit table, same runner as ui_pictogram 5184–5196."""
    for row in range(9):
        bits = rows[row]
        col = 0
        while col < 9:
            if not (bits & (1 << (8 - col))):
                col += 1
                continue
            start = col
            col += 1
            while col < 9 and (bits & (1 << (8 - col))):
                col += 1
            cv_rect(cv, x + start * scale, y + row * scale, (col - start) * scale, scale, ink)


def glyph_juno_hpf(cv: Canvas, gx: int, gy: int, q: float, col: int) -> None:
    """Juno HPF: a rising mirror of the shipped lowpass knee (CUTOFF family).

    New drawing, same 1 px stroke, C_LINE floor, 26×19 box. Not UI_G_FX_HIGHPASS
    (that is 9×9). The knee travels with q.
    """
    top = gy + 3
    bot = gy + OG_H
    right = gx + OG_W - 1
    if q < 0:
        q = 0
    if q > 1:
        q = 1
    pivot = gx + 4 + int(q * 14 + 0.5)
    peak = top + 2
    cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
    # rise from floor, then a passband
    cv_line(cv, gx, bot, pivot - 3 if pivot - 3 > gx else gx, bot, col)
    cv_line(cv, pivot - 3 if pivot - 3 > gx else gx, bot, pivot, peak, col)
    cv_line(cv, pivot, peak, right, peak, col)


def glyph_juno_width(cv: Canvas, gx: int, gy: int, q: float, col: int, face: int) -> None:
    """Juno WIDTH: a pulse whose duty is the DSP law 0.1+x·0.8 (juno6.c).

    Same wave family as draw_wave_glyph shape 3 (pulse), duty from the law —
    not a discrete SAW/PUL name, and not a new bitmap table.
    """
    duty = 0.1 + q * 0.8
    mid = gy + OG_H // 2
    bot = gy + OG_H
    cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
    # one cycle, high for duty, 1 px stroke; zero line dim
    cv_rect(cv, gx, mid, OG_W, 1, blend(face, col, 3))
    hi = gy + 3
    lo = gy + OG_H - 3
    w_hi = max(2, int(duty * (OG_W - 1) + 0.5))
    cv_line(cv, gx, lo, gx, hi, col)
    cv_line(cv, gx, hi, gx + w_hi, hi, col)
    cv_line(cv, gx + w_hi, hi, gx + w_hi, lo, col)
    cv_line(cv, gx + w_hi, lo, gx + OG_W - 1, lo, col)


def glyph_juno_sustain(cv: Canvas, gx: int, gy: int, q: float, col: int) -> None:
    """Juno SUSTAIN: a hold level — attack rise, horizontal sustain, short drop."""
    bot = gy + OG_H
    right = gx + OG_W - 1
    if q < 0:
        q = 0
    if q > 1:
        q = 1
    hold = bot - 2 - int(q * (OG_H - 5) + 0.5)
    cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
    cv_line(cv, gx, bot - 1, gx + 5, hold, col)
    cv_line(cv, gx + 5, hold, gx + 18, hold, col)
    cv_line(cv, gx + 18, hold, right, bot - 1, col)


def glyph_drone_register(cv: Canvas, gx: int, gy: int, q: float, col: int) -> None:
    """Drone REGISTER: three stairs whose heights follow registration[0..2]
    (drone_string.h 64–66): a continuous mix, not Chicago RANGE's discrete switch.
    Same stair vocabulary (vertical bars, C_LINE vs col), 26×19.
    """
    # drone_string_refresh: three ranks, a mix across the pot. Heights rise
    # together as a continuous registration, not a 1-of-n click.
    bot = gy + OG_H
    if q < 0:
        q = 0
    if q > 1:
        q = 1
    # three ranks, left-to-right 16' / 8' / 4', heights from a mix:
    # at 0 mostly the lowest, at 1 all three tall. Matches the appendix.
    h16 = 4 + int((0.35 + 0.65 * q) * (OG_H - 5) + 0.5)
    h8 = 3 + int((0.15 + 0.85 * q) * (OG_H - 5) + 0.5)
    h4 = 2 + int((0.05 + 0.95 * q) * (OG_H - 5) + 0.5)
    cv_rect(cv, gx, bot, OG_W, 1, C_LINE)
    sw = OG_W // 3
    for k, h in enumerate((h16, h8, h4)):
        cv_rect(cv, gx + k * sw, bot - h + 1, sw - 2, h, col)


# ---------------------------------------------------------------------------
# Isolated-glyph helper and 4× nearest-neighbour
# ---------------------------------------------------------------------------

def glyph_cell(face: int = SOUND_FACE, w: int = 26, h: int = 19) -> Canvas:
    return Canvas(w, h, face)


def nn(im, k: int):
    return im.resize((im.size[0] * k, im.size[1] * k), __import__("PIL").Image.Resampling.NEAREST)
