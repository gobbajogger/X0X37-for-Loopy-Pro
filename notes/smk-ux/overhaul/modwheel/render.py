#!/usr/bin/env python3
"""Mod-wheel on-screen feedback frames.

Native 240×240, packed felucca + shipped glyphs. The 224×50 box follows
ui.c draw_readout at 94197fc8 (bx=8, by=14 in band 1 → screen (8, 103)).
"""
from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
sys.path.insert(0, "/workspace/smk-ux/directions/glyphs")

import felucca as F
import shipped_glyphs as G
import render as R

OUT = HERE / "frames"
C_BLACK = G.C_BLACK

# Screen coordinates of draw_readout: MAIN_Y(20)+BAND_H(69)+by(14) = 103.
BX, BY, BW, BH = 8, 103, 224, 50


def pct(v: int) -> int:
    """Knob law, ui.c 1075 and 1110."""
    return (int(v) * 100 + 63) // 127


def _caps(role: str) -> tuple[int, int]:
    """Capital line of 'H' from the decoded glyph (intent of ui.c caps())."""
    f = F.FONTS[role]
    ch = "H" if f.last >= ord("H") else "0"
    gi = ord(ch) - f.first
    crop_x, crop_y, w, height, pixels = f.glyphs[gi]
    top, bot = -1, 0
    for y in range(height):
        for x in range(w):
            if pixels[y][x] >= 8:
                if top < 0:
                    top = crop_y + y
                bot = crop_y + y
    if top < 0:
        top = 0
    return top, bot


def vc(role: str, y0: int, h: int) -> int:
    top, bot = _caps(role)
    ch = bot - top + 1
    return y0 + (h - ch + 1) // 2 - top


def base_y(role_f: str, y: int, role_g: str) -> int:
    return y + _caps(role_f)[1] - _caps(role_g)[1]


def rad_of(h: int) -> int:
    return 6 if h >= 30 else 4 if h >= 18 else 3 if h >= 9 else 2


def draw_wheel_readout(cv, frame_color, *, kind="single", name="", live=0,
                       patch=None, lines=None):
    """kind: 'single' | 'double' | 'unassigned'.

    single: FONT_B name, FONT_M live percent, FONT_S '%'.
            If patch is an int and pct(live) != pct(patch), FONT_T PATCH line.
    double: lines = [(name, live), (name, live)], FONT_S each.
    unassigned: FONT_B 'UNASSIGNED', no number.
    """
    r = rad_of(min(BW, BH))
    G.rbox(cv, BX, BY, BW, BH, r, C_BLACK)
    G.rframe(cv, BX, BY, BW, BH, r, frame_color)

    if kind == "unassigned":
        F.cv_text(cv, BX + 12, vc("B", BY, BH), "B", "UNASSIGNED", R.C_GRAY)
        return

    if kind == "double":
        n = len(lines)
        gap = 4
        block = n * 16 + (n - 1) * gap
        y0 = BY + (BH - block) // 2
        for i, (nm, live_v) in enumerate(lines):
            ly = y0 + i * (16 + gap)
            # vc each 16-px row
            ny = vc("S", ly, 16)
            F.cv_text(cv, BX + 12, ny, "S", nm, R.C_GRAY)
            num = str(pct(live_v))
            unit = "%"
            uw = F.text_w("S", unit) + 3
            w = F.text_w("S", num)
            x = BX + BW - 12 - uw - w
            F.cv_text(cv, x, ny, "S", num, R.C_WHITE)
            F.cv_text(cv, BX + BW - 12 - uw + 3, base_y("S", ny, "S"), "S",
                      unit, R.C_GRAY)
        return

    # single
    show_patch = patch is not None and pct(live) != pct(patch)
    # When the PATCH cue is present, lift the main row so FONT_T sits under it.
    by, bh = BY, BH
    if show_patch:
        by = BY - 4
        bh = BH - 8
    nm_y = vc("B", by, bh)
    ly = vc("M", by, bh)
    F.cv_text(cv, BX + 12, nm_y, "B", name, R.C_GRAY)
    num = str(pct(live))
    unit = "%"
    uw = F.text_w("S", unit) + 3
    w = F.text_w("M", num)
    x = BX + BW - 12 - uw - w
    F.cv_text(cv, x, ly, "M", num, R.C_WHITE)
    F.cv_text(cv, BX + BW - 12 - uw + 3, base_y("M", ly, "S"), "S", unit, R.C_GRAY)
    if show_patch:
        tag = f"PATCH  {pct(patch)} %"
        ty = BY + BH - 4 - F.FONTS["T"].h
        F.text_r(cv, BX + BW - 12, ty, "T", tag, R.C_DIM)


def _knobs_idle(cv, exact, midis):
    R.knobs_seq_chord(cv, exact, midis, selected=None)


def screen_juno(playing=True):
    cv = R.new_cv()
    R.header(cv, alt="R", part="JUNO", part_label="JUNO A", mode="SEQ",
             playing=playing, dirty=False, tempo=120, pattern="P1")
    steps, exact_map, _ = R.juno_steps("cmin7")
    R.piano_roll(cv, steps, lo=48, hi=62, held=None, playhead=(0 if playing else None),
                 exact_by_step=exact_map, y=24, h=110, alt="R")
    midis = steps[0][0]
    _knobs_idle(cv, exact_map.get(0), midis)
    return cv


def screen_bass(part_key, label, midis, exact=None, playing=True, tempo=120):
    cv = R.new_cv()
    R.header(cv, alt="R", part=part_key, part_label=label, mode="SEQ",
             playing=playing, dirty=False, tempo=tempo, pattern="P1")
    rest = None

    def st(m, end=15, tie=False):
        return (list(m), end, tie)

    steps = [rest] * 16
    steps[0] = st(midis, 15)
    steps[4] = st(midis[:1], 12)
    steps[8] = st(midis, 15, True)
    steps[12] = st(midis[:1], 8)
    R.piano_roll(cv, steps, lo=min(48, min(midis)), hi=max(62, max(midis)),
                 held=None, playhead=(0 if playing else None),
                 exact_by_step={}, y=24, h=110, alt="R")
    _knobs_idle(cv, exact, midis)
    return cv


def screen_909(playing=True):
    cv = R.new_cv()
    R.header(cv, alt="R", part="909", part_label="909 A", mode="SEQ",
             playing=playing, dirty=False, tempo=125, pattern="P1")
    R.step_grid(cv, {0, 4, 8, 12, 10}, playhead=(0 if playing else None),
                y=24, h=118)
    R.knobs_909(cv, selected=None)
    return cv


def save(cv, name: str) -> Path:
    d1 = OUT / "1x"
    d3 = OUT / "3x"
    d1.mkdir(parents=True, exist_ok=True)
    d3.mkdir(parents=True, exist_ok=True)
    im = cv.to_pil()
    p1 = d1 / name
    im.save(p1)
    im.resize((720, 720), Image.Resampling.NEAREST).save(d3 / name)
    return p1


def widths():
    names = [
        "DRONE A  BRIGHTNESS",
        "CHICAGO A  CUTOFF",
        "MIAMI A  WOBBLE",
        "JUNO A  CUTOFF",
        "UNASSIGNED",
        "JUNO A  CUTOFF                MIAMI B  WOBBLE",
        "PATCH  55 %",
    ]
    for s in names:
        print(f"{s!r:50s}  B={F.text_w('B', s):4d}  S={F.text_w('S', s):4d}  "
              f"T={F.text_w('T', s):4d}  M={F.text_w('M', s) if all(ord(c)<=95 for c in s) else '-':>4}")
    for v in (0, 51, 64, 66, 70, 127):
        print(f"  live {v:3d} -> {pct(v):3d} %")


def render_all():
    widths()
    green = G.PART_JUNO_A  # slot A (Chicago/Juno/Drone/Miami A)
    red = G.PART_909

    # before: honest shipped silence (R chrome, no box)
    save(screen_juno(), "before-juno-seq.png")

    cv = screen_juno()
    # JUNO POLY cutoff pot 70 = 55 %; live equals pot, one line
    draw_wheel_readout(cv, green, kind="single", name="JUNO A  CUTOFF",
                       live=70, patch=70)
    save(cv, "after-single-juno.png")

    cv = screen_bass("CHI", "CHICAGO A", [48])
    draw_wheel_readout(cv, green, kind="single", name="CHICAGO A  CUTOFF",
                       live=64, patch=64)
    save(cv, "after-single-chicago.png")

    # Miami A: slot A green, engine word MIAMI — never 303
    cv = screen_bass("CHI", "MIAMI A", [48])
    draw_wheel_readout(cv, green, kind="single", name="MIAMI A  WOBBLE",
                       live=51, patch=51)
    save(cv, "after-single-miami.png")

    cv = screen_bass("DRONE", "DRONE A", [48, 55])
    draw_wheel_readout(cv, green, kind="single", name="DRONE A  BRIGHTNESS",
                       live=66, patch=66)
    save(cv, "after-single-drone.png")

    cv = screen_juno()
    # rest-0: live 0, pot 70 (JUNO POLY cutoff) = 55 %
    draw_wheel_readout(cv, green, kind="single", name="JUNO A  CUTOFF",
                       live=0, patch=70)
    save(cv, "after-rest-0.png")

    cv = screen_juno()
    draw_wheel_readout(cv, green, kind="double", lines=[
        ("JUNO A  CUTOFF", 64),
        ("MIAMI B  WOBBLE", 64),
    ])
    save(cv, "after-double.png")

    cv = screen_909()
    draw_wheel_readout(cv, red, kind="unassigned")
    save(cv, "after-unassigned.png")

    print("wrote", list((OUT / "1x").glob("*.png")))


if __name__ == "__main__":
    render_all()
