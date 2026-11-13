#!/usr/bin/env python3
"""Compose corrected A/B/C frames: shipped glyphs decoded from source,
Juno/Drone glyphs as additions. Writes only under directions/glyphs/.

Existing direction frames are read, not overwritten. Firmware is not touched.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, "/workspace/smk-ux")
sys.path.insert(0, "/workspace/smk-ux/directions/glyphs")

from PIL import Image, ImageDraw, ImageFont

import shipped_glyphs as G
from render_mockups import FONT, FONT_DIR, KNOB_Y, text, tw, vc_top

DIR = Path("/workspace/smk-ux/directions")
SRC_1X = DIR / "frames" / "1x"
OUT = DIR / "glyphs"
OUT_1X = OUT / "frames" / "1x"
OUT_3X = OUT / "frames" / "3x"

# SOUND matrix geometry, draw_part_sound 5740–5769 + draw_param_widget 7265–7268
# Screen y = HDR_H(20) + slab y. First bank, 4-row night: slab y = 4+row*50.
HDR = 20
SLAB0 = HDR + 4
STEP = 50
CELL_W = 59
CELL_X0 = 4
GLYPH_DX, GLYPH_DY = 3, 22  # x+3, y+22
# ink: night active SOUND_INK, inactive SOUND_WHITE  (7206)

# 909 BD fixture pots (drum909.c 80–82 defaults), SI as displayed:
# Tune LIN 6..32 ms, def 34 → ~13 (mockup wrote "14 Hz"; unit is not Hz — see audit)
# Attack LIN 0..1, def 13 → 10%
# Decay EXP, 0.50 s / 1.50 s on the two PARA fixtures
# P.Dpth LIN 0, Pitch EXP def 45 → "0.40"
TUNE_POT, ATTACK_POT = 34, 13
TUNE_TAU = 6.0 + (32.0 - 6.0) * 34 / 127.0  # ≈12.96 ms
PDEPTH_M = 0.0
PITCH_PM = 1.0  # P.Dpth 0 → m=0, pm does not affect the idle curve (base = 49 Hz)


def exp565(c: int):
    return G.expand565(c)


INK_ACTIVE = exp565(G.SOUND_INK)
INK_IDLE = exp565(G.SOUND_WHITE)
FACE_ACTIVE = exp565(G.SOUND_SELECT)
FACE_IDLE = exp565(G.SOUND_FACE)
C_GRAY8 = exp565(G.C_GRAY)
C_DIM8 = exp565(G.C_DIM)
C_HI8 = exp565(G.RGB(255, 184, 70))  # AMBER pal[4], the mockup theme
PART909 = exp565(G.PART_909)


def load(name: str) -> Image.Image:
    return Image.open(SRC_1X / name).convert("RGB")


def save(im: Image.Image, name: str) -> None:
    OUT_1X.mkdir(parents=True, exist_ok=True)
    OUT_3X.mkdir(parents=True, exist_ok=True)
    im = im.convert("RGB")
    im.save(OUT_1X / name)
    im.resize((720, 720), Image.Resampling.NEAREST).save(OUT_3X / name)


def to_cv(im: Image.Image) -> G.Canvas:
    return G.Canvas.from_pil(im)


def sample(im: Image.Image, x: int, y: int):
    return im.getpixel((x, y))


def fill(im: Image.Image, box, color) -> None:
    d = ImageDraw.Draw(im)
    x0, y0, x1, y1 = box
    d.rectangle([x0, y0, x1 - 1, y1 - 1], fill=color)


def paste_cv(im: Image.Image, cv: G.Canvas, x: int, y: int) -> None:
    piece = cv.to_pil()
    im.paste(piece, (x, y))


def cell_origin(row: int, col: int):
    x = CELL_X0 + col * CELL_W
    y = SLAB0 + row * STEP
    return x, y


def draw_sound_glyph(im, row, col, drawer, active=False, name=None, move_name=False):
    """Paint a 26×19 glyph at the firmware slot (x+3, y+22).

    If move_name, the mockup put a large name where the glyph belongs (y+16..);
    clear that band and rewrite the name at the shipped y+11 (ends y+19, above
    the glyph). If not, only the glyph+value band (y+22..) is cleared, so an
    already-correct name at y+11 is left alone.
    """
    x, y = cell_origin(row, col)
    face = FACE_ACTIVE if active else FACE_IDLE
    if move_name:
        # Below the row title ("1 BD", "KNOBS 5-8" occupy y+2..y+9) and
        # through the old large name and value. Name will be rewritten at y+11.
        fill(im, (x + 1, y + 16, x + CELL_W - 1, y + 47), face)
        if name:
            text(im, x + CELL_W // 2, y + 11, "T", name, C_GRAY8, anchor="ct")
    else:
        # Names already sit at y+11..y+19 (Juno/Drone proposed). Clear only
        # the shipped glyph+value band, so lookalike leftovers below y+40 go.
        fill(im, (x + 1, y + 22, x + CELL_W - 1, y + 47), face)
    cv = G.Canvas(26, 19, G.SOUND_SELECT if active else G.SOUND_FACE)
    ink = G.SOUND_INK if active else G.SOUND_WHITE
    drawer(cv, 0, 0, ink)
    paste_cv(im, cv, x + GLYPH_DX, y + GLYPH_DY)
    return x, y, ink


def _num_with_dot(im, x, y, s, ink8):
    """Barlow T at 10 px drops or misplaces the period (HOST ESTIMATE).
    Split on '.' and paint a 2×2 baseline square so 1.1 / 0.2 / 0.50 stay readable.
    """
    from PIL import ImageDraw
    if "." not in s:
        return text(im, x, y, "T", s, ink8, anchor="lt")
    left, right = s.split(".", 1)
    x = text(im, x, y, "T", left, ink8, anchor="lt")
    d = ImageDraw.Draw(im)
    # 2 px gap so the period is clear of a "1" foot (T "1" inks 4 px at the baseline
    # but advances only 3). Then a 2×2 square and 1 px before the fraction.
    x += 2
    d.rectangle([x, y + 6, x + 1, y + 7], fill=ink8)
    x += 3
    if right:
        x = text(im, x, y, "T", right, ink8, anchor="lt")
    return x


def _num_width(s):
    if "." not in s:
        return tw("T", s)
    left, right = s.split(".", 1)
    return tw("T", left) + 5 + (tw("T", right) if right else 0)


def strip_value(im, x, y, num, unit, ink8):
    """TTF HOST ESTIMATE of strip_value_fit, ui.c:7181–7198.

    Felucca lane is x+30..x+57 (27 px). Barlow TTF is wider for some unit
    pairs (e.g. "308"+"ms" = 30). Use FONT T (narrower than XS here), a 0-px
    gap if needed, and at most 1 px of the glyph's right margin (x+29..x+58).
    The value area was already cleared by draw_sound_glyph.
    """
    vy = y + 28
    if unit == "%":
        num = num + unit
        unit = ""
    gap = 1 if unit else 0
    w = _num_width(num) + (tw("XS", unit) + gap if unit else 0)
    if unit and w > 29:
        gap = 0
        w = _num_width(num) + tw("XS", unit)
    lane_l, lane_r = x + 29, x + 58
    l = x + 44 - w // 2
    if l < lane_l:
        l = lane_l
    if l + w > lane_r:
        l = lane_r - w
    end = _num_with_dot(im, l, vy, num, ink8)
    if unit:
        text(im, end + gap, vy + 2, "XS", unit, C_GRAY8, anchor="lt")


def restore_909_sound(im: Image.Image, *, decay_ms: float, active_row: int = 0, accent: int = 127) -> None:
    """Paint exact idle 909 BD glyphs into a PARA matrix frame."""
    q_atk = ATTACK_POT / 127.0
    q_pd = 0.0
    # Decay q is unused when ms is given.

    def sweep(cv, gx, gy, ink):
        G.drum_strip_glyph_idle(
            cv, gx, gy, G.DG_BDSWEEP, TUNE_POT, TUNE_POT / 127.0, ink,
            tau=TUNE_TAU, m=PDEPTH_M, pm=PITCH_PM,
        )

    def attack(cv, gx, gy, ink):
        G.drum_strip_glyph_idle(cv, gx, gy, G.DG_ATTACK, ATTACK_POT, q_atk, ink)

    def decay(cv, gx, gy, ink):
        G.drum_strip_glyph_idle(cv, gx, gy, G.DG_DECAY, 0, 0, ink, ms=decay_ms)

    def pdepth(cv, gx, gy, ink):
        G.drum_strip_glyph_idle(
            cv, gx, gy, G.DG_BDSWEEP, 0, 0, ink, tau=TUNE_TAU, m=0.0, pm=PITCH_PM,
        )

    def pitch(cv, gx, gy, ink):
        G.drum_strip_glyph_idle(
            cv, gx, gy, G.DG_BDSWEEP, 45, 45 / 127.0, ink,
            tau=TUNE_TAU, m=PDEPTH_M, pm=PITCH_PM,
        )

    def acc(cv, gx, gy, ink):
        G.drum_strip_glyph_idle(cv, gx, gy, G.DG_ACCENT, accent, accent / 127.0, ink)

    # Row 0: TUNE, ATTACK, DECAY, P.DPTH (active). Names moved to shipped y+11.
    # "14 Hz" is the figure the mockup already showed; Tune's real unit is ms (audit).
    kw = dict(move_name=True)
    x, y, ink = draw_sound_glyph(im, 0, 0, sweep, active=True, name="TUNE", **kw)
    strip_value(im, x, y, "14", "Hz", INK_ACTIVE)
    x, y, ink = draw_sound_glyph(im, 0, 1, attack, active=True, name="ATTACK", **kw)
    strip_value(im, x, y, "10", "%", INK_ACTIVE)
    x, y, ink = draw_sound_glyph(im, 0, 2, decay, active=True, name="DECAY", **kw)
    sec = f"{decay_ms/1000:.2f}"
    strip_value(im, x, y, sec, "s", INK_ACTIVE)
    x, y, ink = draw_sound_glyph(im, 0, 3, pdepth, active=True, name="P.DPTH", **kw)
    strip_value(im, x, y, "0", "%", INK_ACTIVE)
    # Row 1: PITCH
    x, y, ink = draw_sound_glyph(im, 1, 0, pitch, active=False, name="PITCH", **kw)
    strip_value(im, x, y, "0.40", "", INK_IDLE)
    # Row 2: shipped cell is ACCENT (param 1 of NDRUM), not the mockup's "KIT 0".
    # Row title "3 KIT" is left in the slab top.
    x, y, ink = draw_sound_glyph(im, 2, 0, acc, active=False, name="ACCENT", **kw)
    strip_value(im, x, y, "127", "", INK_IDLE)


def restore_play(im: Image.Image, x: int, y: int, playing: bool, style: str) -> None:
    """Replace the PIL lookalike with the exact play_icon (5153–5162)."""
    # Sample the header face just left of the icon.
    sx = max(0, x - 3)
    face8 = sample(im, sx, y + 5)
    face = G.RGB(*face8)
    fill(im, (x - 1, y - 1, x + 10, y + 12), face8)
    cv = G.Canvas.from_pil(im)
    ink = G.RGB(255, 184, 70) if playing else G.C_DIM  # AMBER C_HI when playing, else dim
    if style == "B" and not playing:
        G.play_icon_hollow(cv, x, y, G.C_DIM, face)
    else:
        G.play_icon(cv, x, y, ink)
    # Only paste the 9×11 footprint back
    piece = cv.to_pil().crop((x, y, x + 9, y + 11))
    im.paste(piece, (x, y))


def restore_pattern(im: Image.Image, x: int = 132, y: int = 5, ink=None) -> None:
    """Replace the hash lookalike with exact UI_G_PATTERN (ui_pictograms.h:47, painter 5184)."""
    if ink is None:
        ink = G.C_WHITE
    # Clear 9×9, keep surrounding (the number starts at 144)
    face8 = sample(im, x - 2, y + 4)
    fill(im, (x, y, x + 9, y + 9), face8)
    cv = G.Canvas.from_pil(im)
    G.ui_pictogram(cv, x, y, G.UI_G_PATTERN, 1, ink)
    piece = cv.to_pil().crop((x, y, x + 9, y + 9))
    im.paste(piece, (x, y))


def restore_next(im: Image.Image, x: int, y: int = 5, ink=None) -> None:
    if ink is None:
        ink = G.RGB(255, 184, 70)
    face8 = sample(im, x - 1, y + 4)
    fill(im, (x, y, x + 9, y + 9), face8)
    cv = G.Canvas.from_pil(im)
    G.ui_pictogram(cv, x, y, G.UI_G_NEXT, 1, ink)
    piece = cv.to_pil().crop((x, y, x + 9, y + 9))
    im.paste(piece, (x, y))


def restore_seq_knobs(im: Image.Image, length: int = 16, rate: int = 0) -> None:
    """LENGTH / RATE control_glyph in the knob strip (ui.c:7673–7682).

    Draw on a canvas filled with the cell face so rbox antialiasing blends
    into the real face (a magenta sentinel previously fringed the boxes).
    Paste only the glyph footprints; the rest of the band is untouched.
    """
    face8 = sample(im, 30, KNOB_Y + 26)
    face = G.RGB(*face8)
    cv = G.Canvas(240, 82, face)
    col = G.PART_909
    # NSTEPS=64, firmware val = len-1, max = 63. 4×2 of 7×7 rboxes.
    G.control_glyph_dlen(cv, 30, length - 1, 63, col)
    G.control_glyph_drate(cv, 90, rate, col)
    gly = cv.to_pil()
    # Footprints: DLEN left=cx-18, 4 cols × 10, 7×7, two rows at y=30 and 40.
    # DRATE: left=cx-18, y=31..45, width 37+ a little.
    boxes = (
        (30 - 18, 30, 30 - 18 + 37, 30 + 17),
        (90 - 18, 31, 90 + 19, 31 + 15),
    )
    for (x0, y0, x1, y1) in boxes:
        piece = gly.crop((x0, y0, x1, y1))
        im.paste(piece, (x0, KNOB_Y + y0))


def _header_is_toast(im: Image.Image) -> bool:
    """SHIPPED long-toast frames paint an amber banner over the header (1300 ms).
    Do not stamp play/pattern on top of that banner."""
    # Sample the mid-header; a toast is vividly amber, a normal header is dark.
    r, g, b = sample(im, 160, 8)
    return r > 180 and g > 80 and b < 80


def restore_header_icons(im, style, playing, compact=False, cue=False):
    if _header_is_toast(im):
        return
    play_x = 175 if compact else 217
    restore_play(im, play_x, 4, playing, style)
    restore_pattern(im)
    if cue:
        # render_directions.draw_cue: nx = 132+12+tw("S", current)+5 = 154
        # for current=1. The mockup drew a 5 px PIL triangle there; replace
        # it with exact UI_G_NEXT (9×9). Shift the next-number 3 px right if
        # it would collide (number was at nx+8=162; glyph ends at 163).
        nx = 132 + 12 + tw("S", "1") + 5  # 154
        # Clear the lookalike triangle (nx..nx+6, y 4..15) with header face.
        face8 = sample(im, nx - 2, 4)
        fill(im, (nx, 4, nx + 7, 16), face8)
        restore_next(im, nx, 5, ink=G.RGB(239, 138, 24))  # C_AMB exact


# --- Juno / Drone proposed cells (factory JUNO POLY / Cathedral) ---
# Percents from engines-juno-drone.md; WAVE is a continuous mix.

JUNO_Q = {
    "WAVE": 42 / 127, "SUB": 28 / 127, "WIDTH": 64 / 127, "LEVEL": 100 / 127,
    "CUTOFF": 70 / 127, "RES": 18 / 127, "ENV": 45 / 127, "HPF": 0 / 127,
    "ATTACK": 8 / 127, "DECAY": 40 / 127, "SUSTAIN": 91 / 127, "RELEASE": 40 / 127,
    "LFO RATE": 15 / 127, "LFO DEPTH": 8 / 127, "CHO RATE": 25 / 127, "CHORUS": 45 / 127,
}
# The md listed percents (WAVE 33% → pot ~42). q from those percents:
JUNO_PCT = {
    "WAVE": 0.33, "SUB": 0.22, "WIDTH": 0.50, "LEVEL": 0.79,
    "CUTOFF": 0.55, "RES": 0.14, "ENV": 0.35, "HPF": 0.0,
    "ATTACK": 0.06, "DECAY": 0.31, "SUSTAIN": 0.72, "RELEASE": 0.31,
    "LFO RATE": 0.12, "LFO DEPTH": 0.06, "CHO RATE": 0.20, "CHORUS": 0.35,
}
DRONE_PCT = {
    "REGISTER": 0.45, "BRIGHT": 0.52, "FLUTTER": 0.40,
    "ATTACK": 0.18, "RELEASE": 0.65, "LEVEL": 0.75,
}


def juno_drawer(name):
    q = JUNO_PCT[name]

    def wave(cv, gx, gy, ink):
        # shipped draw_wave_glyph shape 1, mix = pot. cx is mid of 29-wide wave at gx+14.
        G.draw_wave_glyph(cv, 14, 0, 1, int(q * 127 + 0.5), ink, G.SOUND_SELECT)

    def sub(cv, gx, gy, ink):
        G.orig_strip_glyph_idle(cv, gx, gy, chi=False, c=2, val=int(q * 127), q=q, col=ink)

    def width(cv, gx, gy, ink):
        G.glyph_juno_width(cv, gx, gy, q, ink, G.SOUND_SELECT)

    def level(cv, gx, gy, ink):
        G.orig_strip_glyph_idle(cv, gx, gy, chi=True, c=12, val=int(q * 127), q=q, col=ink)

    def lpf(cv, gx, gy, ink):
        G.orig_strip_glyph_idle(
            cv, gx, gy, chi=True, c=2, val=int(q * 127), q=q, col=ink,
            cq=q, pq=JUNO_PCT["RES"] if name == "CUTOFF" else 0,
        )

    def res(cv, gx, gy, ink):
        G.orig_strip_glyph_idle(
            cv, gx, gy, chi=True, c=3, val=int(q * 127), q=q, col=ink,
            cq=JUNO_PCT["CUTOFF"], pq=q,
        )

    def env(cv, gx, gy, ink):
        G.orig_strip_glyph_idle(
            cv, gx, gy, chi=True, c=4, val=int(q * 127), q=q, col=ink,
            cq=JUNO_PCT["CUTOFF"], pq=JUNO_PCT["RES"],
        )

    def hpf(cv, gx, gy, ink):
        G.glyph_juno_hpf(cv, gx, gy, q, ink)

    def attack(cv, gx, gy, ink):
        G.drum_strip_glyph_idle(cv, gx, gy, G.DG_ATTACK, int(q * 127), q, ink)

    def tail(cv, gx, gy, ink):
        # 308 ms decay / 407 ms release at factory (md)
        ms = 308 if name == "DECAY" else 407
        G.drum_strip_glyph_idle(cv, gx, gy, G.DG_DECAY, 0, q, ink, ms=ms)

    def sus(cv, gx, gy, ink):
        G.glyph_juno_sustain(cv, gx, gy, q, ink)

    def lfo(cv, gx, gy, ink):
        c = 13 if "RATE" in name or "CHO RATE" == name else 12
        G.orig_strip_glyph_idle(cv, gx, gy, chi=False, c=c, val=int(q * 127), q=q, col=ink)

    def chorus(cv, gx, gy, ink):
        G.orig_fx_glyph(cv, 1, gx, gy, ink)

    return {
        "WAVE": wave, "SUB": sub, "WIDTH": width, "LEVEL": level,
        "CUTOFF": lpf, "RES": res, "ENV": env, "HPF": hpf,
        "ATTACK": attack, "DECAY": tail, "SUSTAIN": sus, "RELEASE": tail,
        "LFO RATE": lfo, "LFO DEPTH": lfo, "CHO RATE": lfo, "CHORUS": chorus,
    }[name]


def drone_drawer(name):
    q = DRONE_PCT[name]

    def reg(cv, gx, gy, ink):
        G.glyph_drone_register(cv, gx, gy, q, ink)

    def bright(cv, gx, gy, ink):
        G.orig_strip_glyph_idle(cv, gx, gy, chi=True, c=2, val=int(q * 127), q=q, col=ink)

    def flut(cv, gx, gy, ink):
        G.orig_strip_glyph_idle(cv, gx, gy, chi=False, c=13, val=int(q * 127), q=q, col=ink)

    def atk(cv, gx, gy, ink):
        G.drum_strip_glyph_idle(cv, gx, gy, G.DG_ATTACK, int(q * 127), q, ink)

    def rel(cv, gx, gy, ink):
        G.drum_strip_glyph_idle(cv, gx, gy, G.DG_DECAY, 0, q, ink, ms=1800)

    def lvl(cv, gx, gy, ink):
        G.orig_strip_glyph_idle(cv, gx, gy, chi=True, c=12, val=int(q * 127), q=q, col=ink)

    return {
        "REGISTER": reg, "BRIGHT": bright, "FLUTTER": flut,
        "ATTACK": atk, "RELEASE": rel, "LEVEL": lvl,
    }[name]


# Juno four pages, 4 cells each, first bank visible (rows 0–3 = pages 1–4)
JUNO_CELLS = [
    ["WAVE", "SUB", "WIDTH", "LEVEL"],
    ["CUTOFF", "RES", "ENV", "HPF"],
    ["ATTACK", "DECAY", "SUSTAIN", "RELEASE"],
    ["LFO RATE", "LFO DEPTH", "CHO RATE", "CHORUS"],
]
DRONE_CELLS = [
    ["REGISTER", "BRIGHT", "FLUTTER", None],
    ["ATTACK", "RELEASE", "LEVEL", None],
    # SOUND row unchanged — no param glyph
]


def restore_juno(im, style, playing=False):
    restore_header_icons(im, style, playing, compact=False)
    # identity glyph to the left of the name, 9×9, add-only
    # Place at (4, 5) if the chip has room; else just after the name area.
    face8 = sample(im, 4, 5)
    # Don't overwrite the 909-style chip; Juno header is a green name. Put at x=2,y=5
    # only if that pixel is dark. We'll paint at (78, 5) — after a short name — later if needed.
    # Safer: top of the first cell is the engine page, identity goes in the header
    # at a free 9×9 just left of "JUNO A". We'll detect a dark gap around x=4–20.
    for row, names in enumerate(JUNO_CELLS):
        for col, name in enumerate(names):
            if not name:
                continue
            draw_sound_glyph(im, row, col, juno_drawer(name), active=(row == 0))
            # values already on the frame (right side may overlap). The previous
            # proposed renderer already put a glyph+value; we cleared and redrew
            # the glyph. Re-place the existing displayed text from the md.
            # We'll leave values if they already sit on the right; draw_sound_glyph
            # cleared the lower cell, so we must rewrite values.
    # rewrite values (proposed text from engines-juno-drone.md)
    vals = {
        (0, 0): ("33", "%"), (0, 1): ("22", "%"), (0, 2): ("50", "%"), (0, 3): ("79", "%"),
        (1, 0): ("55", "%"), (1, 1): ("14", "%"), (1, 2): ("35", "%"), (1, 3): ("0", "%"),
        (2, 0): ("14", "ms"), (2, 1): ("308", "ms"), (2, 2): ("72", "%"), (2, 3): ("407", "ms"),
        (3, 0): ("0.2", "Hz"), (3, 1): ("6", "%"), (3, 2): ("1.1", "Hz"), (3, 3): ("35", "%"),
    }
    for (r, c), (n, u) in vals.items():
        x, y = cell_origin(r, c)
        ink = INK_ACTIVE if r == 0 else INK_IDLE
        strip_value(im, x, y, n, u, ink)


def restore_drone(im, style, playing=False):
    restore_header_icons(im, style, playing, compact=False)
    for row, names in enumerate(DRONE_CELLS):
        for col, name in enumerate(names):
            if not name:
                continue
            # Drone first row of params may be page 0; SOUND is row 2 — skip.
            draw_sound_glyph(im, row, col, drone_drawer(name), active=(row == 0))
    vals = {
        (0, 0): ("45", "%"), (0, 1): ("52", "%"), (0, 2): ("40", "%"),
        (1, 0): ("70", "ms"), (1, 1): ("1.8", "s"), (1, 2): ("75", "%"),
    }
    for (r, c), (n, u) in vals.items():
        x, y = cell_origin(r, c)
        ink = INK_ACTIVE if r == 0 else INK_IDLE
        # BRIGHT name already shortened in proposed frames
        strip_value(im, x, y, n, u, ink)


def _place_identity(im, rows, ink, x=68, y=5):
    """9×9 engine identity. NOT used on the A/B/C frames: the header has no
    9×9 gap between the part chip and "SOUND" (measured 6–7 px). Stamping at
    x=68 overwrote the mode word. The identity lives on sheet-glyphs.png;
    placing it is a phase-2 layout question. Kept here so a later layout can
    call it once a slot exists.
    """
    face8 = sample(im, x - 1, y + 4)
    fill(im, (x, y, x + 9, y + 9), face8)
    cv = G.Canvas.from_pil(im)
    G.pictogram_rows(cv, x, y, rows, 1, ink)
    piece = cv.to_pil().crop((x, y, x + 9, y + 9))
    im.paste(piece, (x, y))


# ---------------------------------------------------------------------------
# Frame table
# ---------------------------------------------------------------------------

SEQ_KINDS = {
    "01-first-sound": dict(playing=False, knobs=True, compact=False, cue=False),
    "02-pattern": dict(playing=False, knobs=True, compact=False, cue=False),
    "05-save-playing": dict(playing=True, knobs=True, compact=False, cue=False),
    "05-save-toast": dict(playing=True, knobs=True, compact=False, cue=False),
    "06-saved": dict(playing=False, knobs=True, compact=False, cue=False),
    "07-live": dict(playing=True, knobs=True, compact=False, cue=True),
}
SOUND_KINDS = {
    "03-sound": dict(playing=False, decay=500, compact_shipped=True),
    "04-step": dict(playing=False, decay=1500, compact_shipped=True),
}


def correct_one(name: str, style: str, kind: str) -> Image.Image:
    im = load(name)
    if kind in SEQ_KINDS:
        meta = SEQ_KINDS[kind]
        restore_header_icons(im, style, meta["playing"], compact=False, cue=meta["cue"])
        if meta["knobs"]:
            restore_seq_knobs(im)
    elif kind in SOUND_KINDS:
        meta = SOUND_KINDS[kind]
        # A/B/C SOUND keeps the clock (proposed), play at 217. SHIPPED uses 175.
        compact = style == "SHIPPED"
        restore_header_icons(im, style, meta["playing"], compact=compact, cue=False)
        restore_909_sound(im, decay_ms=meta["decay"])
    elif kind == "juno":
        restore_juno(im, style)
    elif kind == "drone":
        restore_drone(im, style)
    return im


def all_frame_jobs():
    jobs = []
    for style in ("A", "B", "C", "SHIPPED"):
        for kind in list(SEQ_KINDS) + list(SOUND_KINDS):
            fname = f"{style}-{kind}.png"
            if (SRC_1X / fname).exists():
                jobs.append((fname, style, kind))
        for eng in ("juno", "drone"):
            # shipped uses lowercase 'shipped-juno'
            if style == "SHIPPED":
                fname = f"shipped-{eng}.png"
                # Shipped Juno/Drone correctly have NO param glyphs. We do not
                # invent them on the SHIPPED reconstruction. Header icons only.
                if (SRC_1X / fname).exists():
                    jobs.append((fname, "SHIPPED", "seq-header-only"))
            else:
                fname = f"{style}-{eng}.png"
                if (SRC_1X / fname).exists():
                    jobs.append((fname, style, eng))
    return jobs


def correct_header_only(im, style):
    restore_header_icons(im, style, playing=False, compact=True, cue=False)
    return im


def main():
    OUT_1X.mkdir(parents=True, exist_ok=True)
    OUT_3X.mkdir(parents=True, exist_ok=True)
    done = []
    for fname, style, kind in all_frame_jobs():
        if kind == "seq-header-only":
            im = correct_header_only(load(fname), style)
        else:
            im = correct_one(fname, style, kind)
        save(im, fname)
        done.append(fname)
    print(f"wrote {len(done)} frames × (1x+3x)")
    for n in done:
        print(" ", n)
    return done


if __name__ == "__main__":
    main()
