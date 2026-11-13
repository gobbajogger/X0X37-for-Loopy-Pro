#!/usr/bin/env python3
"""Native 240x240 SMK-37 journey mockups for firmware commit 94197fc8.

Evolves Dot's six PROPOSAL screens (journey-6-screens-proposal.png)
screen by screen. CURRENT frames are source-faithful reconstructions
(not host-UI screenshots; no firmware or host build was run).
PROPOSED frames are not implemented.

Strip layout is the real 20+69+69+82 (ui.c / gfx.c). Colours are
RGB565-safe (same RGB() quantisation as firmware/src/gfx.c:65).
Fonts: Barlow Semi Condensed TTFs from the repo, rasterised by PIL
at the source role sizes (T 10 SemiBold, XS 12 SemiBold, S 15 Medium,
B 15 Bold, M 23 SemiBold). These are not the firmware's packed
felucca bitmaps; glyph advances may differ by a few pixels.
Tabular figures (LOWER_KEEP) are not applied.
"""
from __future__ import annotations

import shutil
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageFilter

ROOT = Path("/workspace/smk-ux")
FONT_DIR = Path("/workspace/smk-ux-src/assets/fonts")
DOT_DIR = Path("/workspace/handoff-dot-20261113/firmware")
OUT = ROOT / "mockups"
OUT_1X = OUT / "1x"
OUT_2X = OUT / "2x"
OUT_SHEETS = OUT / "sheets"
OUT_DOT = OUT / "dot-original"

# ---------------------------------------------------------------------------
# RGB565-safe colours (gfx.c RGB macro, night materials, amber palette)
# ---------------------------------------------------------------------------

def rgb565(r, g, b):
    r5, g6, b5 = r >> 3, g >> 2, b >> 3
    return ((r5 << 3) | (r5 >> 2), (g6 << 2) | (g6 >> 4), (b5 << 3) | (b5 >> 2))


def RGB(r, g, b):
    return rgb565(r, g, b)


# Sampled from Dot's 94197fc8 host-UI HOME (before-after left), which matches
# the RGB565 expansion of ui_theme.h night + AMBER palette (project default).
N_BASE = RGB(5, 10, 13)          # (0, 8, 8)
N_FACE = RGB(16, 26, 31)         # (16, 24, 24)
N_EDGE = RGB(38, 56, 61)         # (32, 56, 57)
N_LEFT = RGB(28, 44, 49)         # (24, 44, 49)
N_SIDE = RGB(7, 14, 17)          # (0, 12, 16)
N_SHADOW = RGB(2, 4, 6)          # (0, 4, 0)
N_SELECT = RGB(37, 59, 45)
N_SELECT_EDGE = RGB(62, 92, 64)
N_SELECT_SIDE = RGB(21, 39, 27)
N_INK = RGB(224, 235, 214)
N_WHITE = RGB(222, 231, 223)
N_GRAY = RGB(156, 170, 168)
N_DIM = RGB(128, 146, 148)
N_LINE = RGB(40, 56, 61)
N_ACCENT = RGB(143, 191, 121)
N_AMBER = RGB(196, 155, 69)

C_BLACK = N_BASE
C_WHITE = N_WHITE
C_LINE = RGB(40, 56, 61)
C_DIM = RGB(128, 146, 148)
C_GRAY = RGB(156, 170, 168)
C_AMB = RGB(232, 136, 30)
C_HI = RGB(255, 184, 70)

PART = {
    "909": RGB(255, 150, 40),
    "808": RGB(255, 72, 64),
    "BREAK": RGB(214, 120, 255),
    "CHICAGO": RGB(130, 240, 90),
    "MIAMI": RGB(70, 205, 255),
}
# Display order is PART_ORDER: 909, 808, BREAK, 303B, 303A (ui.c 99).
# Fixture (and Dot's host HOME) uses Chicago then Miami on those slots.
LANES = [("909", "909"), ("808", "808"), ("BREAK", "BREAK"),
         ("CHICAGO", "CHICAGO"), ("MIAMI", "MIAMI")]

HDR_H, BAND_H, KNOB_H = 20, 69, 82
MAIN_Y, KNOB_Y = 20, 158
GX, DROW = 27, 11
VOICES = ["BD", "SD", "LT", "MT", "HT", "RS", "CP", "CH", "OH", "CR", "RD", "AC"]
FOOT_Y = 71  # in the knob strip
FOOT_GAP = 7

# ---------------------------------------------------------------------------
# Fonts
# ---------------------------------------------------------------------------

def load_font(fname, size):
    return ImageFont.truetype(str(FONT_DIR / fname), size)


FONT = {
    "T": load_font("BarlowSemiCondensed-SemiBold.ttf", 10),
    "XS": load_font("BarlowSemiCondensed-SemiBold.ttf", 12),
    "S": load_font("BarlowSemiCondensed-Medium.ttf", 15),
    "B": load_font("BarlowSemiCondensed-Bold.ttf", 15),
    "M": load_font("BarlowSemiCondensed-SemiBold.ttf", 23),
}


def tw(role, s):
    f = FONT[role]
    # Use the same anchor as drawing so advances match the ink.
    dummy = Image.new("L", (1, 1))
    b = ImageDraw.Draw(dummy).textbbox((0, 0), s, font=f, anchor="lt")
    return b[2] - b[0]


_PIL_ANCHOR = {
    "lt": "lt", "lm": "lm", "lb": "lb",
    "ct": "mt", "cm": "mm", "cb": "mb",
    "rt": "rt", "rm": "rm", "rb": "rb",
}


def text(img, x, y, role, s, color, anchor="lt"):
    """Draw with ImageDraw so 0/O/D are not clipped (a mask+mismatched
    anchor previously truncated capitals). anchor: lt/lm/lb/ct/cm/cb/rt/rm/rb.
    """
    if not s:
        return x
    d = ImageDraw.Draw(img)
    pil = _PIL_ANCHOR[anchor]
    d.text((int(x), int(y)), s, font=FONT[role], fill=color, anchor=pil)
    b = d.textbbox((int(x), int(y)), s, font=FONT[role], anchor=pil)
    return b[2]


def vc_top(role, y, h):
    """Centre the capital ink in [y, y+h). ImageDraw anchor=lt uses the
    top of the ink, so we must not also subtract the font's cap_top
    (that previously placed '909' / 'HOME' one pixel above the clip).
    """
    f = FONT[role]
    hb = f.getbbox("H")
    cap_h = hb[3] - hb[1]
    return int(y + (h - cap_h) / 2)


# ---------------------------------------------------------------------------
# Primitives
# ---------------------------------------------------------------------------

def new_frame(bg=C_BLACK):
    im = Image.new("RGB", (240, 240), bg)
    return im, ImageDraw.Draw(im)


def hline(draw, x, y, w, c):
    draw.line((x, y, x + w - 1, y), fill=c)


def vline(draw, x, y, h, c):
    draw.line((x, y, x, y + h - 1), fill=c)


def box(draw, x, y, w, h, c):
    draw.rectangle([x, y, x + w - 1, y + h - 1], fill=c)


def blend(a, b, t15):
    """t15 is 0..15 as in firmware blend numerators (approx)."""
    t = max(0, min(15, t15)) / 15.0
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def dim(c, t):
    """dim(c, t) in source is blend(C_BLACK, c, t) with t 0..15-ish."""
    return blend(C_BLACK, c, t)


def surface_panel(draw, x, y, w, h, active=False,
                  face=N_FACE, edge=N_EDGE, side=N_SIDE):
    """Chamfered shallow panel (ui.c surface_panel 5223–5244), simplified."""
    r = 4 if h >= 18 else (2 if h >= 9 else 1)
    if w < 2 * r + 3 or h < 2 * r + 3:
        r = 1
    for row in range(h):
        inset = (r - row) if row < r else ((row - (h - r) + 1) if row >= h - r else 0)
        shade = face
        if active and h >= 18 and 0 < row < 5:
            shade = blend(face, C_WHITE, 5 - row)
        if active and h >= 18 and row >= h - 4:
            shade = blend(face, C_BLACK, (row - (h - 4) + 1) * 2)
        if w - 2 * inset > 0:
            box(draw, x + inset, y + row, w - 2 * inset, 1, shade)
    # top edge, sides, bottom shadow
    if w - 2 * r > 0:
        box(draw, x + r, y, w - 2 * r, 1, edge)
        box(draw, x + r, y + h - 1, w - 2 * r, 1, N_SHADOW)
    vline(draw, x, y + r, max(1, h - 2 * r), edge if active else N_LEFT)
    vline(draw, x + w - 1, y + r, max(1, h - 2 * r), side)
    if w > 24 and h >= 18:
        box(draw, x + r + 2, y + 1, max(1, w // 4), 1, edge if active else N_LEFT)


def night_panel(draw, x, y, w, h, active=False):
    face = N_SELECT if active else N_FACE
    edge = N_SELECT_EDGE if active else N_EDGE
    side = N_SELECT_SIDE if active else N_SIDE
    surface_panel(draw, x, y, w, h, active, face, edge, side)


def night_mark(draw, x, y, w, h, ink, raised=False):
    box(draw, x, y, w, h, ink)
    if w >= 4 and h >= 5:
        top = blend(ink, C_WHITE, 3) if raised else N_SHADOW
        bot = dim(ink, 7) if raised else N_LEFT
        box(draw, x + 1, y + 1, w - 2, 1, top)
        box(draw, x + 1, y + h - 2, w - 2, 1, bot)


def play_icon(draw, x, y, c):
    """11x9 triangle (ui.c 5153–5162), simple fill."""
    pts = [(x, y), (x, y + 10), (x + 8, y + 5)]
    draw.polygon(pts, fill=c)


def col_x(c):
    return GX + c * 13 + (c >> 2)


def pattern_badge(img, draw, x, number, ink=C_WHITE, part="909", current=True):
    """# glyph + number (ui.c header_pattern_badge / ui_position_badge)."""
    # 10x7 frame at (x, 6) + 8-px strength row; number at x+12
    frame_x, frame_y = x, 6
    # pictogram: a small hash/grid
    for i in range(3):
        vline(draw, frame_x + 2 + i * 3, frame_y, 7, ink)
    for i in range(2):
        hline(draw, frame_x, frame_y + 2 + i * 3, 10, ink)
    text(img, x + 12, vc_top("S", 1, 17), "S", str(number), ink)


def save_badge(img, draw, x, label, color=C_AMB):
    """XS amber (or fail) badge, ui.c 5450–5458."""
    w = tw("XS", label) + 10
    box(draw, x, 3, w, 13, color)
    text(img, x + w // 2, vc_top("XS", 3, 13), "XS", label, C_BLACK, anchor="ct")
    return x + w


def footer_segs(img, parts, color=C_DIM, y=None):
    """segs() at FOOT_Y in the knob strip (6810–6811)."""
    if y is None:
        y = KNOB_Y + vc_top("T", FOOT_Y, 9)
    x = 4
    for i, s in enumerate(parts):
        if not s:
            continue
        if i:
            x += FOOT_GAP
        x = text(img, x, y, "T", s, color) + 0
    return x


# ---------------------------------------------------------------------------
# Header variants
# ---------------------------------------------------------------------------

def fill_header_panel(draw):
    # night_panel(0,0,240,20,0) on C_BLACK canvas
    # leave 1px base at edges as in samples
    box(draw, 0, 0, 240, 20, N_BASE)
    night_panel(draw, 0, 0, 240, HDR_H, False)


def header_seq(img, draw, part="909", mode="SEQ", tempo=125, playing=False,
               pattern=1, unsaved=False, queued=False, msg=None,
               keep_clock=False, rec=False):
    """SEQ / HOME / SONG style header (part_view or VIEW_N path).

    If msg and not keep_clock: full-header toast (5345–5369): part-coloured
    bar, black centred text, 1300 ms. If keep_clock: pickup-style left chip
    (5410) so pattern and tempo stay.
    """
    fill_header_panel(draw)
    if msg and not keep_clock:
        box(draw, 0, 0, 240, HDR_H - 1, PART[part])
        role = "B" if tw("B", msg) <= 232 else "XS"
        text(img, 120, vc_top(role, 0, HDR_H - 1), role, msg, C_BLACK, anchor="ct")
        return
    x = 4
    if msg and keep_clock:
        # proposed / pickup: message in the left chip, FONT_XS so long
        # strings stay left of the pattern badge at 132 (5410 rule).
        role = "S" if tw("S", msg) + 8 <= 128 else "XS"
        x = text(img, 4, vc_top(role, 1, 17), role, msg, C_AMB) + 6
    elif mode == "HOME" or mode == "SONG" or mode == "LP":
        word = "HOME" if mode == "HOME" else ("SONG" if mode == "SONG" else "LP")
        col = C_HI
        x = text(img, 4, vc_top("B", 1, 17), "B", word, col) + 6
    else:
        # part chip + mode (5418–5443)
        label = part
        pf, mf = "B", "S"
        w = tw(pf, label) + 12
        box(draw, 0, 1, w, 17, PART[part])
        text(img, 6, vc_top(pf, 1, 17), pf, label, C_BLACK)
        x = w + 6
        if mode:
            ink = C_HI
            x = text(img, x, vc_top(mf, 1, 17), mf, mode, ink) + 6
    # Source 5450–5460: the badge must not cross x=130 (pattern sits at 132).
    # A long mode or a left-chip toast shrinks to Q / * or is omitted.
    if queued or unsaved:
        label = "SAVE AT STOP" if queued else "UNSAVED"
        short = "Q" if queued else "*"
        w = tw("XS", label) + 10
        if x + w <= 130:
            x = save_badge(img, draw, x, label, C_AMB) + 2
        elif x + 11 <= 130:
            x = save_badge(img, draw, x, short, C_AMB) + 2
    pattern_badge(img, draw, 132, pattern, C_WHITE, part)
    # tempo right-aligned at 212 (5495)
    t = str(tempo)
    text(img, 212, vc_top("S", 1, 17), "S", t, C_HI, anchor="rt")
    play_icon(draw, 217, 4, C_HI if playing else C_DIM)
    if rec:
        draw.ellipse([229, 7, 237, 15], fill=RGB(255, 50, 50))
    if unsaved or queued:
        draw.ellipse([235, 1, 239, 5], fill=C_AMB)


def header_para_current(img, draw, name="909", playing=False, unsaved=False, banks=1, page_bank=0):
    """Compact PARA header (5373–5408): part name only, no mode, no
    pattern, no tempo. Night/X0X_ORIGINAL_SYNTH: SOUND_GREEN name."""
    fill_header_panel(draw)
    text(img, 7, 2, "B", name, N_ACCENT)
    # bank dots at 232 - i*10, y=9
    for i in range(banks):
        dx = 232 - (banks - 1 - i) * 10
        draw.ellipse([dx - 2, 7, dx + 2, 11], fill=N_AMBER)
        if i != page_bank:
            draw.ellipse([dx - 1, 8, dx + 1, 10], fill=N_FACE)
    play_icon(draw, 175, 4, C_HI if playing else C_DIM)
    if unsaved:
        draw.ellipse([235, 1, 239, 5], fill=C_AMB)


def header_para_proposed(img, draw, part="909", mode="SOUND", tempo=125,
                         playing=False, pattern=1, unsaved=False, queued=False,
                         step=None):
    """PROPOSAL: use the part-chip + mode path (5418–5443) instead of the
    compact early return (5373). Mode word SOUND is already in source at
    5437 (currently skipped). Pattern and tempo stay (pickup rule 5410).
    Optional STEP n is a compact extra, labelled PROPOSAL if used.
    """
    # If step held, mode can stay SOUND and the STEP hint lives in the matrix.
    header_seq(img, draw, part=part, mode=mode, tempo=tempo, playing=playing,
               pattern=pattern, unsaved=unsaved, queued=queued)


# ---------------------------------------------------------------------------
# Main views
# ---------------------------------------------------------------------------

def draw_drum(img, draw, hits=None, sel=0, playing_step=None, plock=None, muted=None):
    """12-row all-track grid (draw_drum 5511–5549). hits: set of (voice, step)."""
    hits = hits or set()
    muted = muted or set()
    plock = plock or set()
    col = PART["909"]
    on_dim = dim(col, 8)
    # main area starts at MAIN_Y; band-local y = v*DROW+1
    for v, nm in enumerate(VOICES):
        y = MAIN_Y + v * DROW + 1
        is_sel = v == sel and v < 11
        is_ac = v == 11
        if is_sel:
            surface_panel(draw, 0, y - 1, 24, DROW, True, col, col, dim(col, 5))
        ink = C_BLACK if is_sel else (C_LINE if v in muted else (C_WHITE if is_ac else C_GRAY))
        text(img, 5, vc_top("XS", y - 1, DROW - 1), "XS", nm, ink)
        for c in range(16):
            x = col_x(c)
            hit = (v, c) in hits
            ph = playing_step == c
            if is_ac:
                if hit:
                    night_mark(draw, x + 2, y + 1, 8, 6, C_WHITE, True)
                else:
                    box(draw, x + 2, y + 3, 8, 2, C_LINE)
                continue
            ink_c = (C_DIM if v in muted else (col if is_sel else on_dim)) if hit else (C_DIM if ph else C_LINE)
            night_mark(draw, x, y, 12, DROW - 2, ink_c, hit)
            if hit and ph:
                box(draw, x + 2, y + 2, 8, DROW - 6, C_WHITE)
            if hit and is_sel and c in plock:
                box(draw, x + 8, y + 1, 3, 3, C_BLACK)


def draw_home_lanes(img, draw, hits_909=None, playing_step=None, pattern=1):
    """HOME: 16 pattern slots + 5 lanes (5996–6046). Keyboard-alone."""
    hits_909 = hits_909 or []
    # top plate of 16 slots
    night_panel(draw, 3, MAIN_Y + 1, 234, 33, False)
    for c in range(16):
        x = 7 + c * 14 + (4 if c >= 8 else 0)
        y = MAIN_Y + 3
        box(draw, x, y, 12, 22, N_SIDE)
        box(draw, x, y, 12, 1, N_EDGE)
        # current pattern number under slot
        if c == pattern - 1:
            t = str(c + 1)
            twid = tw("T", t)
            text(img, x + 6 - twid // 2, MAIN_Y + 25, "T", t, C_GRAY)
            # used marks for 909 if hits
            if hits_909:
                # 8-px glyph row in 909 orange
                # simplify: four ticks at 1/5/9/13 → positions in 8
                for s in hits_909:
                    # map 16 steps to 8 px
                    i = s // 2
                    box(draw, x + 2 + i, MAIN_Y + 4, 1, 3, PART["909"])
                box(draw, x, MAIN_Y + 4, 1, 3, PART["909"])
    for pos, (key, name) in enumerate(LANES):
        y = MAIN_Y + 35 + pos * 20
        night_panel(draw, 3, y, 234, 18, False)
        box(draw, 4, y + 3, 2, 11, PART[key])
        text(img, 9, vc_top("T", y + 1, 16), "T", name, PART[key])
        bits = hits_909 if key == "909" else []
        for c in range(16):
            x = 58 + c * 11 + (c >> 2)
            on = c in bits
            box(draw, x, y + 5, 9, 8, N_SIDE)
            box(draw, x, y + 5, 9, 1, PART[key] if on else N_EDGE)
            if on:
                box(draw, x + 1, y + 6, 7, 6, dim(PART[key], 11))
            box(draw, x, y + 12, 9, 1, dim(PART[key], 6) if on else N_EDGE)
            if playing_step is not None and playing_step == c:
                box(draw, x + 1, y + 14, 7, 2, C_WHITE)


def draw_song_empty(img, draw):
    """SONG view, empty song (6048–6088): heading + EMPTY line."""
    y0 = MAIN_Y
    shorts = ["909", "808", "BRK", "CHI", "MIA"]  # 303B then 303A; fixture CHI/MIA
    cols_part = [PART["909"], PART["808"], PART["BREAK"], PART["CHICAGO"], PART["MIAMI"]]
    text(img, 4, vc_top("XS", y0, 11), "XS", "BAR", C_GRAY)
    for pos, (nm, c) in enumerate(zip(shorts, cols_part)):
        text(img, 58 + pos * 36, vc_top("XS", y0, 11), "XS", nm, c, anchor="ct")
    # selected empty row
    y = y0 + 11
    surface_panel(draw, 0, y, 236, 11, True, C_LINE, N_SELECT_EDGE, N_SIDE)
    text(img, 4, vc_top("XS", y, 11), "XS", "1", C_WHITE)
    text(img, 50, vc_top("XS", y, 11), "XS", "EMPTY: WHITE KEYS ADD BARS", C_DIM)


def draw_para_matrix(img, draw, active=0, values=None, held_step=None, banks_hint="1-3"):
    """4-row sound matrix spanning main+knobs (draw_part_sound 5740–5834).

    Night materials (X0X_ORIGINAL_SYNTH=1). Page titles are the voice name.
    Row 0: BD Tune/Attack/Decay/P.Dpth (KNOBS 5-8)
    Row 1: BD Pitch (KNOB 5)
    Row 2: KIT
    Hint at y=208 (absolute from top of the 220-px matrix area starting at 20,
    i.e. screen y = 20+208- wait: off for band 2 is 138, hint 208-138=70 in
    knob canvas, screen y = 158+70=228). Absolute y in full frame: 20+208? 

    draw_part_sound y for rows: y=4+row*50-off, off=band*69.
    In full-screen coords, off=0 if we draw as one canvas: y starts at
    MAIN_Y + 4 = 24.
    Hint: 208 is in the same coordinate system as row y (from start of
    draw_part_sound canvas that is drawn in bands). For band 0, canvas
    starts at MAIN_Y, local y. So hint local 208, but main is only 138
    tall... 208 is in band 2: screen Y = MAIN_Y + 208 = 228. Yes.
    sound_slab at 207: screen 227, height 13 → 227-239.
    """
    values = values or [
        [("TUNE", "14", "Hz"), ("ATTACK", "10", "%"), ("DECAY", "0.50", "s"), ("P.DPTH", "0", "%")],
        [("PITCH", "0.40", ""), None, None, None],
        [("KIT", "—", ""), None, None, None],
        None,
    ]
    titles = ["BD", "BD", "KIT", None]
    knobs = ["KNOBS 5-8", "KNOB 5", "", ""]
    # background of matrix area
    box(draw, 0, MAIN_Y, 240, 220, N_BASE)
    for row in range(4):
        if titles[row] is None:
            continue
        y = MAIN_Y + 4 + row * 50
        height = 49
        active_row = row == active
        night_panel(draw, 3, y, 234, height, active_row)
        label = N_INK if active_row else N_GRAY
        text(img, 8, y + 2, "T", f"{row+1}  {titles[row]}", label)
        if active_row and knobs[row]:
            text(img, 232, y + 2, "T", knobs[row], label, anchor="rt")
        cells = values[row] or []
        for k, cell in enumerate(cells):
            if not cell:
                continue
            x = 4 + k * 59
            if k and cell:
                vline(draw, x, y + 14, height - 18, N_SELECT_SIDE if active_row else N_LINE)
            nm, val, unit = cell
            ink = N_INK if active_row else N_WHITE
            text(img, x + 29, y + 16, "T", nm, N_GRAY if not active_row else N_GRAY, anchor="ct")
            text(img, x + 29 - (tw("S", unit) // 2 if unit else 0), y + 28, "S", val, ink, anchor="ct")
            if unit:
                text(img, x + 29 + tw("S", val) // 2 + 2, y + 32, "T", unit, N_DIM)
    # bottom slab + hints  (screen y 227)
    night_panel(draw, 0, 227, 240, 13, False)
    gray, hint = N_GRAY, N_AMBER
    if held_step is not None:
        # SOURCE 5803–5806
        s = f"STEP {held_step}  < >: CHANCE  PARA"
        text(img, 7, 229, "T", s, C_AMB)
    else:
        # arrows hint (SOURCE 5678–5832) — simplified text, not the pictogram
        text(img, 7, 229, "T", "< >", gray)
        text(img, 234, 229, "T", banks_hint, hint, anchor="rt")


def draw_knob_strip(img, draw, tag, knobs, footer, footer_color=C_DIM, touched=None):
    """Standard knob strip (not PARA matrix). knobs: list of 4 (name, value, unit) or None."""
    box(draw, 0, KNOB_Y, 240, KNOB_H, N_BASE)
    night_panel(draw, 3, KNOB_Y + 0, 234, 11, False)
    text(img, 8, vc_top("XS", KNOB_Y + 0, 11), "XS", tag, C_HI)
    for i in range(4):
        night_panel(draw, i * 60 + 3, KNOB_Y + 13, 54, 55, touched == i)
        if i:
            box(draw, i * 60, KNOB_Y + 14, 1, 50, C_LINE)
        k = knobs[i] if i < len(knobs) else None
        if not k:
            continue
        name, val, unit = k
        cx = i * 60 + 30
        ink = C_WHITE if touched == i else C_GRAY
        text(img, cx, vc_top("XS", KNOB_Y + 13, 10), "XS", name, ink, anchor="ct")
        # simple value, no perform_glyph (reconstruction)
        text(img, cx - (tw("T", unit) // 2 if unit else 0),
             vc_top("B", KNOB_Y + 53, 13), "B", val, C_HI if touched != i else C_WHITE,
             anchor="ct")
        if unit:
            # unit to the right of value, DIM
            vw = tw("B", val)
            text(img, cx + vw // 2 + 1, vc_top("T", KNOB_Y + 56, 10), "T", unit, C_DIM)
    night_panel(draw, 0, KNOB_Y + 70, 240, 12, False)
    footer_segs(img, footer, footer_color)


# ---------------------------------------------------------------------------
# Frame builders — one per Dot screen number
# ---------------------------------------------------------------------------

BD_HITS = {0, 4, 8, 12}  # steps 1, 5, 9, 13 (Dot fixture)
BD_HIT_PAIRS = {(0, c) for c in BD_HITS}

SEQ_FOOT = ("BLACK: TRACK", "WHITE: STEP", "PATCH+BLK: MUTE")
HOME_FOOT = ("WHITE: PATTERN", "TWO: CHAIN", "BLACK: MUTE")
BOOT_FOOT = ("HOLD ANY BUTTON: WHAT IT DOES",)
SONG_FOOT = ("WHITE: THE BAR'S PATTERN", "BLACK: MUTE")
PARA_FOOT_SRC = ("BLACK: SELECT", "WHITE: SOUND", "SEQ: STEPS")  # written, currently undrawn

# Numeric PARA examples: factory-default inspired, same caveat as Dot
# (illustrative units; exact firmware strings may differ by a glyph).
PARA_VALS = [
    [("TUNE", "14", "Hz"), ("ATTACK", "10", "%"), ("DECAY", "0.50", "s"), ("P.DPTH", "0", "%")],
    [("PITCH", "0.40", ""), None, None, None],
    [("KIT", "0", ""), None, None, None],
    None,
]
PARA_VALS_STEP = [
    [("TUNE", "14", "Hz"), ("ATTACK", "10", "%"), ("DECAY", "1.50", "s"), ("P.DPTH", "0", "%")],
    [("PITCH", "0.40", ""), None, None, None],
    [("KIT", "0", ""), None, None, None],
    None,
]


def frame_01_first_sound_CURRENT():
    """Boot 909 SEQ — where first sound actually is (ui_init → V_SEQ, 8604).
    Empty pattern, BD selected, HOLD ANY BUTTON footer (any_button=0, 6738).
    """
    im, d = new_frame()
    header_seq(im, d, mode="SEQ", unsaved=False)
    draw_drum(im, d, hits=set(), sel=0)
    draw_knob_strip(im, d, "P1 SEQUENCE",
                    [("LENGTH", "16", ""), ("RATE", "1/16", ""), None, None],
                    BOOT_FOOT, C_GRAY)
    return im


def frame_01_first_sound_PROPOSED():
    """PROPOSAL: same 12-row kit grid and 909+SEQ chip (gestures unchanged).
    Show the already-written SEQ footer from boot (6783–6786) so the first
    sound (BLACK: TRACK, plays while stopped, 3193–3213) is listed without
    waiting for any_button. HOLD-help still exists (1909).
    """
    im, d = new_frame()
    header_seq(im, d, mode="SEQ", unsaved=False)
    draw_drum(im, d, hits=set(), sel=0)
    draw_knob_strip(im, d, "P1 SEQUENCE",
                    [("LENGTH", "16", ""), ("RATE", "1/16", ""), None, None],
                    SEQ_FOOT, C_DIM)
    return im


def frame_01_home_CURRENT():
    """HOME after PATCH (4051). Keyboard-alone footer (6757–6759), not SMC."""
    im, d = new_frame()
    header_seq(im, d, mode="HOME")
    draw_home_lanes(im, d, hits_909=list(BD_HITS) if False else [], pattern=1)
    # After first-sound at boot the pattern is still empty if they PATCHed
    # before programming. Empty lanes.
    draw_knob_strip(im, d, "PERFORM",
                    [("TEMPO", "125", "bpm"), ("SWING", "50", "%"),
                     ("FILTER", "OFF", ""), ("PUMP", "0.0", "dB")],
                    HOME_FOOT, C_DIM)
    return im


def frame_01_home_PROPOSED():
    """Evolved from Dot 01. Title stays HOME (VIEW_N 88), not 'DnBOX'.
    Lanes, PERFORM knobs, WHITE/TWO/BLACK footer kept (live). No always-on
    CHAIN, no 'STOP' word, no K5–K8 captions, no K3/SEQ footer (those
    would replace live pattern/chain/mute hints). Behavioural change is
    not drawn: second PATCH stays here (OWNER DECISION).
    """
    im, d = new_frame()
    header_seq(im, d, mode="HOME")
    draw_home_lanes(im, d, hits_909=[], pattern=1)
    draw_knob_strip(im, d, "PERFORM",
                    [("TEMPO", "125", "bpm"), ("SWING", "50", "%"),
                     ("FILTER", "OFF", ""), ("PUMP", "0.0", "dB")],
                    HOME_FOOT, C_DIM)
    return im


def frame_01_song_CURRENT():
    """Second PATCH from HOME opens SONG (4051). Empty song."""
    im, d = new_frame()
    header_seq(im, d, mode="SONG")
    draw_song_empty(im, d)
    draw_knob_strip(im, d, "BAR (1/2)",
                    [("BAR", "1", ""), ("909", "P1", ""), ("808", "P1", ""), ("BREAK", "P1", "")],
                    SONG_FOOT, C_DIM)
    return im


def frame_01_song_PROPOSED_stays_home():
    """PROPOSAL: second PATCH stays on HOME. Same pixels as evolved HOME.
    SONG entry is an OWNER DECISION (hold PATCH vs GLOBE vs keep tap).
    """
    return frame_01_home_PROPOSED()


def frame_02_hit_CURRENT():
    """909 SEQ after programming BD on 1/5/9/13. 12-row kit grid."""
    im, d = new_frame()
    header_seq(im, d, mode="SEQ", unsaved=True)
    draw_drum(im, d, hits=BD_HIT_PAIRS, sel=0)
    draw_knob_strip(im, d, "P1 SEQUENCE",
                    [("LENGTH", "16", ""), ("RATE", "1/16", ""), None, None],
                    SEQ_FOOT, C_DIM)
    return im


def frame_02_hit_PROPOSED():
    """Evolved from Dot 02: KEEP the 12-row kit overview (Dot README said
    a 2x8 must not delete it). DROP numbered 2x8, 'BD BASS DRUM',
    'TAP PAD: ADD/REMOVE HIT', 'P01 / STOP', K5/K6 chrome, persistent
    'SEQ PLAY / PARA / hold step' footer (already hold-help). Source
    footer and 909+SEQ chip stay. Result is deliberately close to CURRENT:
    the low-budget fix is 'do not replace the kit grid'.
    """
    return frame_02_hit_CURRENT()


def frame_03_sound_CURRENT():
    """PARA, 909 BD, compact header, matrix, no SOUND word, no tempo."""
    im, d = new_frame()
    header_para_current(im, d, name="909", unsaved=True, banks=1)
    draw_para_matrix(im, d, active=0, values=PARA_VALS)
    return im


def frame_03_sound_PROPOSED():
    """Evolved from Dot 03. KEEP the 4-row matrix (it is PARA). DROP
    'EDITING: SOUND' banner, 'Applies to this drum sound', huge DECAY
    slider as the page identity, 'BD BASS DRUM', 'P01 / STOP'.
    SHOW: 909 chip + SOUND (word already at 5437, skipped today) and
    pattern+tempo (5410). Bottom: already-written footer_text 6776–6779
    ('SEQ: STEPS' is the way back) in the existing 13 px hint slot,
    replacing only the arrow pictograms, keeping the 1-3 bank range.
    """
    im, d = new_frame()
    header_para_proposed(im, d, mode="SOUND", unsaved=True)
    draw_para_matrix(im, d, active=0, values=PARA_VALS)
    # overwrite hint with footer_text strings (SOURCE 6776–6779, currently undrawn)
    box(d, 0, 227, 240, 13, N_BASE)
    night_panel(d, 0, 227, 240, 13, False)
    footer_segs(im, PARA_FOOT_SRC, C_DIM, y=229)
    text(im, 234, 229, "T", "1-3", N_AMBER, anchor="rt")
    return im


def frame_04_step_CURRENT():
    """Held step 5 on PARA (carried from SEQ): matrix + STEP hint (5803)."""
    im, d = new_frame()
    header_para_current(im, d, name="909", unsaved=True, banks=1)
    draw_para_matrix(im, d, active=0, values=PARA_VALS_STEP, held_step=5)
    return im


def frame_04_step_PROPOSED():
    """Evolved from Dot 04. KEEP the matrix and the SOURCE hint
    'STEP 5  < >: CHANCE  PARA' (5803–5806). KEEP p-lock behaviour and
    'RELEASE STEP FIRST' when leaving (2755–2759). DROP the orange
    'EDITING: THIS STEP ONLY' banner, 'Step 05 held. Release to return.',
    persistent 'BT + knob' / 'PATCH + SEQ REC' lines (those shortcuts
    stay in hold-help; they must not be removed). Header: 909 + SOUND
    + pattern + tempo, same as 03, so the STEP line is the tell.
    """
    im, d = new_frame()
    header_para_proposed(im, d, mode="SOUND", unsaved=True)
    draw_para_matrix(im, d, active=0, values=PARA_VALS_STEP, held_step=5)
    return im


def frame_04_step_seq_CURRENT():
    """Also: held step on SEQ itself, tag BD P-LOCK STEP 5 (8284–8286), kit grid."""
    im, d = new_frame()
    header_seq(im, d, mode="SEQ", unsaved=True)
    draw_drum(im, d, hits=BD_HIT_PAIRS, sel=0, plock={4})
    draw_knob_strip(im, d, "BD P-LOCK STEP 5",
                    [("LENGTH", "16", ""), ("RATE", "1/16", ""), None, None],
                    SEQ_FOOT, C_DIM)
    return im


def frame_04_step_seq_PROPOSED():
    """PROPOSAL: keep the SEQ P-LOCK tag and kit grid. Do not replace
    them with Dot's banner. Same as CURRENT — expert muscle memory."""
    return frame_04_step_seq_CURRENT()


def _seq_playing_base(unsaved=True, queued=False, msg=None, keep_clock=False, playing=True):
    im, d = new_frame()
    header_seq(im, d, mode="SEQ", unsaved=unsaved, queued=queued, msg=msg,
               keep_clock=keep_clock, playing=playing, tempo=125)
    draw_drum(im, d, hits=BD_HIT_PAIRS, sel=0, playing_step=4 if playing else None)
    foot = SEQ_FOOT
    draw_knob_strip(im, d, "P1 SEQUENCE",
                    [("LENGTH", "16", ""), ("RATE", "1/16", ""), None, None],
                    foot, C_DIM)
    return im


def frame_05_save_playing_CURRENT_toast():
    """Tap BT while playing: full-header toast SAVES WHEN STOPPED (5345, 300, 1300 ms).
    Pattern and tempo are covered.
    """
    return _seq_playing_base(unsaved=True, msg="SAVES WHEN STOPPED", keep_clock=False, playing=True)


def frame_05_save_playing_CURRENT_badge():
    """After the 1300 ms toast: SAVE AT STOP badge, play continues (5450–5452, 294)."""
    return _seq_playing_base(unsaved=False, queued=True, msg=None, playing=True)


def frame_05_save_playing_PROPOSED():
    """Evolved from Dot 05. KEEP source string 'SAVES WHEN STOPPED'.
    Pickup-style left chip (5410) so pattern, tempo and play stay.
    Do not also stack a SAVE AT STOP badge (on a 909+SEQ header it
    shrinks to 'Q' — that squeeze is the current problem, 5458–5460).
    """
    return _seq_playing_base(unsaved=False, queued=False, msg="SAVES WHEN STOPPED",
                             keep_clock=True, playing=True)


def frame_05_save_playing_PROPOSED_badge():
    """After the 1300 ms toast: the persistent cue is the full words
    SAVE AT STOP as a left chip, not a squeezed 'Q'. Pattern and tempo stay.
    """
    return _seq_playing_base(unsaved=False, queued=False, msg="SAVE AT STOP",
                             keep_clock=True, playing=True)


def frame_06_saved_CURRENT():
    """After stop, successful save: full-header 'SAVED' 1300 ms (2262, 407)."""
    im, d = new_frame()
    header_seq(im, d, mode="SEQ", unsaved=False, msg="SAVED", keep_clock=False, playing=False)
    draw_drum(im, d, hits=BD_HIT_PAIRS, sel=0)
    draw_knob_strip(im, d, "P1 SEQUENCE",
                    [("LENGTH", "16", ""), ("RATE", "1/16", ""), None, None],
                    SEQ_FOOT, C_DIM)
    return im


def frame_06_saved_PROPOSED():
    """Evolved from Dot 06. KEEP the source word SAVED (2262). DROP
    'PROJECT SAVED', 'Your changes are stored.', 'SEQ PLAY: start again',
    persistent green panel, 'DnBOX'. Chip on the left, pattern and tempo
    stay. After 1300 ms the normal SEQ header returns (no saved badge).
    """
    im, d = new_frame()
    header_seq(im, d, mode="SEQ", unsaved=False, msg="SAVED", keep_clock=True, playing=False)
    draw_drum(im, d, hits=BD_HIT_PAIRS, sel=0)
    draw_knob_strip(im, d, "P1 SEQUENCE",
                    [("LENGTH", "16", ""), ("RATE", "1/16", ""), None, None],
                    SEQ_FOOT, C_DIM)
    return im


# ---------------------------------------------------------------------------
# Save / sheets
# ---------------------------------------------------------------------------

SHEET_BG = (16, 20, 22)  # same as Dot's sheet, already RGB-ish
SHEET_INK = (228, 233, 230)
SHEET_DIM = (140, 157, 157)
SHEET_AMB = (238, 174, 57)
SHEET_GREEN = (143, 191, 121)


def _sheet_font(size, weight="SemiBold"):
    name = {
        "SemiBold": "BarlowSemiCondensed-SemiBold.ttf",
        "Medium": "BarlowSemiCondensed-Medium.ttf",
        "Bold": "BarlowSemiCondensed-Bold.ttf",
    }[weight]
    return ImageFont.truetype(str(FONT_DIR / name), size)


def scale2(im):
    return im.resize((480, 480), Image.Resampling.NEAREST)


def save_frame(im, name):
    OUT_1X.mkdir(parents=True, exist_ok=True)
    OUT_2X.mkdir(parents=True, exist_ok=True)
    p1 = OUT_1X / f"{name}.png"
    p2 = OUT_2X / f"{name}.png"
    im.save(p1)
    scale2(im).save(p2)
    return p1, p2


def _text(draw, xy, s, font, fill, anchor="lt"):
    draw.text(xy, s, font=font, fill=fill, anchor=anchor)


def make_sheet(title, subtitle, columns, footnotes, out_name, width=None):
    """columns: list of (heading, caption, Image 240, notes_list).
    Each image shown at 2x (480). notes under the frame.
    """
    n = len(columns)
    margin, gap = 28, 36
    cell_w = 480
    top = 110
    notes_h = 220
    foot_h = 28 + 18 * (len(footnotes) + 1)
    W = margin * 2 + n * cell_w + (n - 1) * gap
    H = top + 480 + 16 + notes_h + foot_h
    im = Image.new("RGB", (W, H), SHEET_BG)
    dr = ImageDraw.Draw(im)
    f_title = _sheet_font(22, "Bold")
    f_sub = _sheet_font(13, "Medium")
    f_h = _sheet_font(16, "Bold")
    f_c = _sheet_font(12, "Medium")
    f_n = _sheet_font(11, "Medium")
    f_foot = _sheet_font(11, "Medium")
    _text(dr, (margin, 22), title, f_title, SHEET_INK)
    _text(dr, (margin, 52), subtitle, f_sub, SHEET_DIM)
    # right stamp
    _text(dr, (W - margin, 22), "94197fc8  ·  NOT IMPLEMENTED", f_sub, SHEET_DIM, anchor="rt")
    _text(dr, (W - margin, 44), "CURRENT is a source reconstruction, not a device photo", f_sub, SHEET_DIM, anchor="rt")
    for i, (heading, caption, frame, notes) in enumerate(columns):
        x = margin + i * (cell_w + gap)
        color = SHEET_AMB if "PROPOSED" in heading.upper() or "EVOLVED" in heading.upper() else (
            SHEET_DIM if "DOT" in heading.upper() or "ORIGINAL" in heading.upper() else SHEET_INK)
        _text(dr, (x, 78), heading, f_h, color)
        _text(dr, (x, 96), caption, f_c, SHEET_DIM)
        im.paste(scale2(frame) if frame.size == (240, 240) else frame.resize((480, 480), Image.Resampling.NEAREST),
                 (x, top))
        # thin rule around
        dr.rectangle([x - 1, top - 1, x + 480, top + 480], outline=(32, 40, 42))
        ny = top + 480 + 14
        for line in notes:
            # prefix tags
            fill = SHEET_INK
            if line.startswith("DROP") or line.startswith("  DROP"):
                fill = (220, 120, 110)
            elif line.startswith("KEEP") or line.startswith("  KEEP"):
                fill = SHEET_GREEN
            elif line.startswith("FLAG") or line.startswith("PROPOSAL") or line.startswith("  FLAG") or line.startswith("  PROPOSAL"):
                fill = SHEET_AMB
            elif line.startswith("SOURCE") or line.startswith("  SOURCE"):
                fill = (150, 190, 210)
            _text(dr, (x, ny), line, f_n, fill)
            ny += 15
    fy = H - foot_h + 8
    for line in footnotes:
        _text(dr, (margin, fy), line, f_foot, SHEET_DIM)
        fy += 16
    OUT_SHEETS.mkdir(parents=True, exist_ok=True)
    path = OUT_SHEETS / out_name
    im.save(path)
    return path


def load_dot_1x(n):
    p = Path(f"/tmp/dot-screens/dot-{n:02d}-1x.png")
    return Image.open(p).convert("RGB")


def main():
    for p in (OUT_1X, OUT_2X, OUT_SHEETS, OUT_DOT):
        p.mkdir(parents=True, exist_ok=True)

    # Copy Dot originals (1x + 2x) so every output maps to a screen number.
    for n in range(1, 7):
        shutil.copy(f"/tmp/dot-screens/dot-{n:02d}-1x.png", OUT_DOT / f"{n:02d}-dot-original-1x.png")
        shutil.copy(f"/tmp/dot-screens/dot-{n:02d}-2x.png", OUT_DOT / f"{n:02d}-dot-original-2x.png")

    frames = {
        "01-first-sound-CURRENT": frame_01_first_sound_CURRENT(),
        "01-first-sound-PROPOSED": frame_01_first_sound_PROPOSED(),
        "01-HOME-after-PATCH-CURRENT": frame_01_home_CURRENT(),
        "01-HOME-after-PATCH-PROPOSED": frame_01_home_PROPOSED(),
        "01-SONG-second-PATCH-CURRENT": frame_01_song_CURRENT(),
        "01-SONG-second-PATCH-PROPOSED-stays-HOME": frame_01_song_PROPOSED_stays_home(),
        "02-put-a-hit-CURRENT": frame_02_hit_CURRENT(),
        "02-put-a-hit-PROPOSED": frame_02_hit_PROPOSED(),
        "03-edit-sound-CURRENT": frame_03_sound_CURRENT(),
        "03-edit-sound-PROPOSED": frame_03_sound_PROPOSED(),
        "04-edit-one-step-PARA-CURRENT": frame_04_step_CURRENT(),
        "04-edit-one-step-PARA-PROPOSED": frame_04_step_PROPOSED(),
        "04-edit-one-step-SEQ-CURRENT": frame_04_step_seq_CURRENT(),
        "04-edit-one-step-SEQ-PROPOSED": frame_04_step_seq_PROPOSED(),
        "05-save-while-playing-CURRENT-toast": frame_05_save_playing_CURRENT_toast(),
        "05-save-while-playing-CURRENT-badge": frame_05_save_playing_CURRENT_badge(),
        "05-save-while-playing-PROPOSED": frame_05_save_playing_PROPOSED(),
        "05-save-while-playing-PROPOSED-badge": frame_05_save_playing_PROPOSED_badge(),
        "06-saved-CURRENT": frame_06_saved_CURRENT(),
        "06-saved-PROPOSED": frame_06_saved_PROPOSED(),
    }
    paths = {}
    for name, im in frames.items():
        paths[name] = save_frame(im, name)

    common_foot = [
        "Fonts: Barlow Semi Condensed TTFs from the repo, PIL at T10 / XS12 / S15 / B15 / M23. Not the packed felucca bitmaps; advances may differ by a few pixels.",
        "CURRENT is a source-faithful reconstruction at 94197fc8, not a host-UI screenshot (no firmware or host build was run). PROPOSED is not implemented.",
        "Live-note ownership, PATCH+SEQ REC / SEQ PLAY undo/redo, hold-step p-locks, chaining (live only), and all listed shortcuts are unchanged. Nothing is removed without the owner's words.",
        "Existing controls retained. Example PARA values are factory-default inspired and illustrative.",
    ]

    # Sheet 01: first sound + PATCH (evolved from Dot 01 'Find a part')
    make_sheet(
        "01  First sound  ·  evolved from Dot 01 ‘Find a part’",
        "Newcomer path starts on 909 SEQ at boot — that is where a black key plays BD. Dot 01 was HOME, which is a first-sound dead end (keys are patterns and mutes).",
        [
            ("DOT 01 ORIGINAL  ·  not implemented",
             "‘Find a part’ on HOME",
             load_dot_1x(1),
             [
                 "DROP  Title ‘DnBOX’ — header is HOME (VIEW_N, ui.c 88, 5445).",
                 "DROP  ‘125 BPM / STOP’ — tempo number + play icon, no STOP word (5495–5498).",
                 "DROP  Large ‘PATTERN 01’ and always-on ‘CHAIN 01’ — badge; CHN only when chained (5476).",
                 "DROP  Large ‘UNSAVED’ — XS badge or * , plus a 2 px dot (5450–5460).",
                 "DROP  Footer ‘K3: part  SEQ: steps  PATCH: home/song’ — would replace",
                 "      WHITE: PATTERN / TWO: CHAIN / BLACK: MUTE (6757–6759), live gestures.",
                 "DROP  K5–K8 captions on PERFORM — source titles are TEMPO/SWING/FILTER/PUMP (1470).",
                 "FLAG  HOME is the wrong first-sound screen (map problem 5, ui.c 3733–3787).",
                 "KEEP  Five-lane part overview and PERFORM knobs — reused on the HOME pair.",
             ]),
            ("CURRENT  ·  source  ·  boot 909 SEQ",
             "ui_init → V_SEQ (8604). Footer: HOLD ANY BUTTON (6738).",
             frames["01-first-sound-CURRENT"],
             [
                 "SOURCE  909 orange chip + SEQ (5418–5443). SEQ matches the printed button.",
                 "SOURCE  12-row kit grid BD…AC (5511–5549). BD selected.",
                 "SOURCE  Empty at boot. Black key 1 selects BD and, while stopped, plays it (3193–3213).",
                 "SOURCE  Pattern badge + 125 + dim play icon. LENGTH 16, RATE 1/16, tag P1 SEQUENCE.",
                 "SOURCE  Before any button: footer HOLD ANY BUTTON: WHAT IT DOES (6738–6740).",
                 "         After a button, BLACK: TRACK / WHITE: STEP / PATCH+BLK: MUTE (6783–6786).",
             ]),
            ("PROPOSED  ·  evolved  ·  first sound",
             "Same grid and gestures. SEQ footer visible from boot.",
             frames["01-first-sound-PROPOSED"],
             [
                 "PROPOSAL  Show the already-written SEQ footer (6783–6786) from boot,",
                 "          so BLACK: TRACK (first sound, stopped) and WHITE: STEP are listed.",
                 "          Hold-help is unchanged (HELP_HOLD 900 ms, 1909).",
                 "KEEP  12-row kit, 909+SEQ chip, LENGTH/RATE, pattern and tempo.",
                 "KEEP  Every live-note and shortcut. No new gesture.",
                 "This is the first newcomer step: make a sound, then (02) put a hit.",
             ]),
        ],
        common_foot,
        "01-first-sound.png",
    )

    make_sheet(
        "01b  After PATCH  ·  evolved from Dot 01 HOME chrome",
        "One PATCH from SEQ lands on HOME (4051). Keys do not play. A second PATCH opens SONG. SEQ still returns to 909 step editing (shortcut kept).",
        [
            ("CURRENT  ·  HOME after one PATCH",
             "Keyboard-alone footer (not the SMC fixture).",
             frames["01-HOME-after-PATCH-CURRENT"],
             [
                 "SOURCE  Title HOME (88, 5445). Badge # 1, 125, dim play icon (5472–5498).",
                 "SOURCE  16 pattern slots + lanes 909 / 808 / BREAK / CHICAGO / MIAMI",
                 "        (PART_ORDER 99: 909, 808, BREAK, 303B, 303A).",
                 "SOURCE  PERFORM: TEMPO, SWING, FILTER, PUMP (1470–1471).",
                 "SOURCE  WHITE: PATTERN / TWO: CHAIN / BLACK: MUTE (6757–6759).",
                 "        White keys = patterns; black keys = mutes. No sound (3733–3787).",
             ]),
            ("PROPOSED  ·  evolved HOME",
             "Chrome stripped. Live footer kept. Second PATCH: OWNER DECISION.",
             frames["01-HOME-after-PATCH-PROPOSED"],
             [
                 "KEEP  HOME title, lanes, slots, PERFORM knobs, WHITE/TWO/BLACK footer.",
                 "DROP  All Dot 01 chrome listed on sheet 01 (DnBOX, CHAIN 01, STOP, K5–K8,",
                 "      K3/SEQ footer). Those are not source and would hide live hints.",
                 "PROPOSAL (behaviour, owner decides)  PATCH already on HOME stays on HOME.",
                 "         SONG via hold PATCH or GLOBE — not a second tap. Experts who",
                 "         double-tap PATCH for SONG would need a new path. SEQ is unchanged.",
             ]),
            ("CURRENT  ·  SONG after a second PATCH",
             "set_view(V_HOME ? V_SONG : V_HOME)  (4051).",
             frames["01-SONG-second-PATCH-CURRENT"],
             [
                 "SOURCE  Title SONG (VIEW_N 88). Empty: WHITE KEYS ADD BARS (6073).",
                 "SOURCE  Footer WHITE: THE BAR'S PATTERN / BLACK: MUTE (6760–6762).",
                 "SOURCE  Badge still follows seq.song_on, which PLAY on this screen sets (4539).",
                 "         Just opening SONG does not itself start a song.",
                 "PROPOSED second PATCH (owner) is the HOME frame to the left, not this.",
             ]),
        ],
        common_foot,
        "01b-after-PATCH-home-song.png",
    )

    make_sheet(
        "02  Put a hit on a step  ·  evolved from Dot 02",
        "Second newcomer step: the pattern. White keys / pads toggle steps (3250–3253). The 12-row kit overview is kept.",
        [
            ("DOT 02 ORIGINAL  ·  not implemented",
             "2×8 numbered pads, one track",
             load_dot_1x(2),
             [
                 "DROP  2×8 numbered grid — draw_drum is 12 rows, all tracks (5511–5549).",
                 "      Dot’s README already says this must not delete the kit overview.",
                 "DROP  ‘BD BASS DRUM’ — voice name is the short ‘BD’ (engine_voice_name).",
                 "DROP  ‘PADS: STEPS 1-16’ and ‘TAP PAD: ADD / REMOVE HIT’ — pads alias",
                 "      white keys; footer already WHITE: STEP (6784–6786).",
                 "DROP  ‘909 / STEPS’ and ‘P01 / STOP’ — source is 909 chip + SEQ + badge + icon.",
                 "DROP  Persistent ‘SEQ PLAY: start  PARA: sound  Hold step + knob’ — hold-help.",
                 "KEEP  Intent (hits on 1/5/9/13, LENGTH 16, RATE 1/16) and K5/K6 meaning.",
                 "FLAG  K5/K6 captions are not drawn on the SEQ strip today (optional chrome).",
             ]),
            ("CURRENT  ·  source  ·  909 SEQ",
             "Kit grid after BD steps 1 / 5 / 9 / 13. UNSAVED badge.",
             frames["02-put-a-hit-CURRENT"],
             [
                 "SOURCE  Same 12-row grid as boot, now with hits (night_mark, 5543).",
                 "SOURCE  909 chip + SEQ, UNSAVED XS badge (5450), P1 SEQUENCE, LENGTH/RATE.",
                 "SOURCE  Footer BLACK: TRACK / WHITE: STEP / PATCH+BLK: MUTE.",
                 "SOURCE  White key: empty step on immediately; lit step off on a tap < 300 ms;",
                 "        a hold keeps it for p-locks (STEP_TAP_MS, 3250–3253).",
             ]),
            ("PROPOSED  ·  evolved  ·  keep the kit",
             "Deliberately matches CURRENT. The fix is what we did not take from Dot.",
             frames["02-put-a-hit-PROPOSED"],
             [
                 "KEEP  The 12-row kit overview — seeing the whole 909 pattern is the point.",
                 "KEEP  909+SEQ (printed button), source footer, LENGTH/RATE, live shortcuts.",
                 "DROP  Every Dot 02 chrome item listed on the left.",
                 "PROPOSAL  No gesture change. Expert step-programming is unchanged.",
                 "This is the second newcomer step, then (05–06) save. Sound/step edit is 03–04.",
             ]),
        ],
        common_foot,
        "02-put-a-hit.png",
    )

    make_sheet(
        "03  Edit the drum sound  ·  evolved from Dot 03",
        "PARA (printed) is sound edit. How the user tells: the matrix, and — in the proposal — a SOUND mode word plus SEQ: STEPS as the way back.",
        [
            ("DOT 03 ORIGINAL  ·  not implemented",
             "Banner + huge DECAY slider",
             load_dot_1x(3),
             [
                 "DROP  ‘EDITING: SOUND’ banner and ‘Applies to this drum sound’ — not in source.",
                 "DROP  Full-page DECAY 1.0 s as the PARA identity — a 900 ms draw_readout",
                 "      already exists when a knob is turned (6455–6505, 6816); it is temporary.",
                 "DROP  ‘BD BASS DRUM’, ‘P01 / STOP’, ‘909 / SOUND’ as a new header dialect.",
                 "DROP  ‘PARA: next page’ — PARA toggles V_PART; pages are K2 / SCALE list.",
                 "      Teaching ‘PARA: next page’ would be wrong.",
                 "KEEP  Intent: make SOUND vs STEPS obvious. Keep Tune/Attack/Decay/P.Dpth on K5–K8.",
                 "KEEP  ‘SEQ: steps’ as the way back (already in footer_text 6779).",
             ]),
            ("CURRENT  ·  source  ·  909 PARA",
             "Compact header: just ‘909’ in accent green. No SOUND, no pattern, no tempo.",
             frames["03-edit-sound-CURRENT"],
             [
                 "SOURCE  Compact header 5373–5408 (drums and 303). VIEW_N[V_PART] is “” (88).",
                 "        The word SOUND at 5437 is skipped by this early return.",
                 "SOURCE  4-row night matrix (5740–5834): 1 BD Tune/Attack/Decay/P.Dpth,",
                 "        2 BD Pitch, 3 KIT. KNOBS 5-8 caption (5722, UI_FIRST_KNOB 5).",
                 "SOURCE  Bottom hint: arrows / PARA / 1-3 (5678–5832). The strings",
                 "        BLACK: SELECT / WHITE: SOUND / SEQ: STEPS (6776–6779) are",
                 "        written but not drawn — draw_knobs returns before draw_footer (8337–8343).",
             ]),
            ("PROPOSED  ·  evolved  ·  SOUND, keep the matrix",
             "Existing words, existing header pattern, no new banner.",
             frames["03-edit-sound-PROPOSED"],
             [
                 "SOURCE (surfaced)  909 chip + SOUND — word already at 5437.",
                 "SOURCE (surfaced)  Pattern badge + tempo, same rule as the pickup chip (5410).",
                 "SOURCE (surfaced)  Footer BLACK: SELECT / WHITE: SOUND / SEQ: STEPS (6776–6779)",
                 "        in the existing 13 px hint. SEQ: STEPS is the way back, no new gesture.",
                 "KEEP  The 4-row matrix and K5–K8 mapping. DROP Dot’s banner and DECAY page.",
                 "PROPOSAL  Only to show strings and a header path the source already has.",
                 "How you tell: SEQ shows a kit grid and ‘SEQ’; PARA shows a matrix and ‘SOUND’.",
             ]),
        ],
        common_foot,
        "03-edit-sound.png",
    )

    make_sheet(
        "04  Edit one step only  ·  evolved from Dot 04",
        "Hold a step: p-locks. The tell versus 03 is the SOURCE line ‘STEP 5  < >: CHANCE  PARA’, not a new banner. SEQ’s ‘BD P-LOCK STEP 5’ tag is kept.",
        [
            ("DOT 04 ORIGINAL  ·  not implemented",
             "Orange ‘THIS STEP ONLY’ banner",
             load_dot_1x(4),
             [
                 "DROP  ‘EDITING: THIS STEP ONLY’ banner and ‘Step 05 held. Release to return.’",
                 "      Held-step UI is the grid, p-lock notches, and tags (8284–8288, 5803–5806).",
                 "DROP  Replacing the grid/matrix with a DECAY slider.",
                 "FLAG  ‘BT + knob: remove its step lock’ — the shortcut exists (2951–2953) but",
                 "      is hold-help text, not a persistent caption. Same for PATCH+SEQ REC undo.",
                 "      Those shortcuts are preserved, not put on the page.",
                 "KEEP  Intent: this-step vs the whole sound must be obvious.",
             ]),
            ("CURRENT  ·  source  ·  PARA, step 5 held",
             "Same matrix as 03, amber STEP hint (5803–5806).",
             frames["04-edit-one-step-PARA-CURRENT"],
             [
                 "SOURCE  SEQ→PARA can carry a held-step p-lock; PARA→SEQ is blocked with",
                 "        RELEASE STEP FIRST (2755–2759) — kept, not a bug to ‘fix’.",
                 "SOURCE  Hint: STEP 5  < >: CHANCE  PARA (5803–5806, UI_CAP_EDIT is PARA).",
                 "SOURCE  Compact header still has no SOUND word and no tempo (5373).",
                 "SOURCE  On SEQ itself the tag is ‘BD P-LOCK STEP 5’ (8284–8286); see 1x/04-…-SEQ-.",
             ]),
            ("PROPOSED  ·  evolved  ·  same hint, clearer header",
             "Header matches 03 (SOUND + clock). The STEP line is the tell.",
             frames["04-edit-one-step-PARA-PROPOSED"],
             [
                 "KEEP  Matrix, p-lock behaviour, RELEASE STEP FIRST, P-LOCK wording,",
                 "      BT+knob forget and PATCH+SEQ REC undo (hold-help, not a banner).",
                 "SOURCE (surfaced)  909 + SOUND + pattern + tempo, as in 03.",
                 "SOURCE  The amber STEP 5 line stays the difference from 03.",
                 "DROP  Dot’s banner and instructional body. No gesture change.",
             ]),
        ],
        common_foot,
        "04-edit-one-step.png",
    )

    make_sheet(
        "05  Save while playing  ·  evolved from Dot 05",
        "Third newcomer step: save. Tap BT (printed) queues a save. Flash would stop audio, so it waits (2222–2294). Work view is 909 SEQ, where the pattern was made.",
        [
            ("DOT 05 ORIGINAL  ·  not implemented",
             "HOME, ‘SAVE QUEUED’, instructional body",
             load_dot_1x(5),
             [
                 "DROP  Title ‘SAVE QUEUED’ — visible strings are SAVE AT STOP / SAVES WHEN STOPPED",
                 "      (290–300). The enum is SAVE_QUEUED; it is not a title.",
                 "DROP  ‘Playback continues. SEQ PLAY: stop and allow saving.’ — verbose, and",
                 "      would cover the music. The existing toast already says SAVES WHEN STOPPED.",
                 "DROP  ‘DnBOX’, ‘125 BPM / PLAY’, K3/SEQ/BT footer replacing the SEQ footer.",
                 "FLAG  Dot placed this on HOME; the newcomer is on 909 SEQ. Same save path",
                 "      on any view (B_SAVE → save_project, 4105).",
                 "KEEP  Intent: explain SAVE AT STOP without writing flash during play.",
             ]),
            ("CURRENT  ·  source  ·  toast, then badge",
             "Left: 1300 ms full-header toast. Right conceptually: SAVE AT STOP badge (separate file).",
             frames["05-save-while-playing-CURRENT-toast"],
             [
                 "SOURCE  say(queued_save_message()) → ‘SAVES WHEN STOPPED’ for 1300 ms (300, 407, 2294).",
                 "SOURCE  The toast paints the whole header in the part colour and hides",
                 "        pattern, tempo and the play icon (5345–5369). Pickup is the one",
                 "        exception that already keeps them (5410).",
                 "SOURCE  After the toast: amber SAVE AT STOP badge (294, 5450–5452), play continues.",
                 "        If the header is crowded the badge shrinks to ‘Q’ or a 2 px dot (5458–5460).",
                 "SOURCE  BT LED lit while queued (map §3.4). SEQ PLAY still starts/stops (4007).",
             ]),
            ("PROPOSED  ·  evolved  ·  chip, clock stays",
             "Same words. Pattern, tempo and play stay visible (pickup rule).",
             frames["05-save-while-playing-PROPOSED"],
             [
                 "SOURCE  Strings SAVES WHEN STOPPED and SAVE AT STOP — not rewritten.",
                 "SOURCE (reused)  Left-chip layout from fader pickup (5410): ‘song/pattern",
                 "        and tempo stay visible’. FONT_XS so the long toast fits left of x=132.",
                 "KEEP  Deferred save itself, timings, BT = save, copy-on-hold 250 ms, help 400 ms.",
                 "DROP  Dot’s essay, ‘SAVE QUEUED’ title, ‘DnBOX’, HOME as the work view.",
                 "PROPOSAL  Feedback only. Expert BT muscle memory is unchanged.",
             ]),
        ],
        common_foot + [
            "A second CURRENT frame, 05-save-while-playing-CURRENT-badge, shows the header after the toast: SAVE AT STOP, play icon live, playhead on step 5.",
        ],
        "05-save-while-playing.png",
    )

    make_sheet(
        "06  Saved after transport stops  ·  evolved from Dot 06",
        "When the queued save completes (rc==0), the toast is SAVED (2262). dirty is cleared only then. Confirmation is 1300 ms, then the normal SEQ header.",
        [
            ("DOT 06 ORIGINAL  ·  not implemented",
             "‘PROJECT SAVED’ green panel",
             load_dot_1x(6),
             [
                 "DROP  ‘PROJECT SAVED’ and ‘Your changes are stored.’ — the source word is SAVED",
                 "      (or AUTOSAVED). Success does not keep a persistent green panel.",
                 "DROP  ‘SEQ PLAY: start again’, ‘DnBOX’, ‘125 BPM / STOP’ as words.",
                 "KEEP  Intent: a clear confirmation after a successful write only (2260–2265).",
             ]),
            ("CURRENT  ·  source  ·  full-header SAVED",
             "1300 ms, part-coloured bar, black text (5345–5367, 2262).",
             frames["06-saved-CURRENT"],
             [
                 "SOURCE  say(“SAVED”, 0) only if project_save rc==0 (2262). Failed: SAVE FAILED.",
                 "SOURCE  The toast covers pattern and tempo for 1300 ms, then the normal",
                 "        909 SEQ header returns with no saved badge (SAVE_SAVED draws nothing).",
                 "SOURCE  Bluetooth device-save is a different path and says BLUETOOTH SAVED (8744).",
             ]),
            ("PROPOSED  ·  evolved  ·  SAVED chip, clock stays",
             "Same word. Pattern and tempo stay. No new copy.",
             frames["06-saved-PROPOSED"],
             [
                 "SOURCE  The word SAVED (2262). 1300 ms (407). Then the normal header.",
                 "SOURCE (reused)  Pickup-style left chip (5410) so the pattern just saved",
                 "        stays on screen — useful in a live demo.",
                 "DROP  ‘PROJECT SAVED’, the green panel, instructional SEQ PLAY line.",
                 "PROPOSAL  Feedback only. After the chip expires, nothing new persists.",
             ]),
        ],
        common_foot,
        "06-saved.png",
    )

    # Newcomer-path contact sheet: the six evolved proposals in order
    make_sheet(
        "Newcomer path  ·  evolved proposals  ·  first sound, then pattern, then save",
        "01 sound (black key on 909 SEQ)  →  02 hits on the kit grid  →  03/04 sound vs step, only if they edit  →  05–06 BT save. Expert gestures are the same.",
        [
            ("01  FIRST SOUND", "909 SEQ, footer from boot",
             frames["01-first-sound-PROPOSED"],
             ["BLACK: TRACK plays BD while stopped.", "WHITE: STEP is next."]),
            ("02  PATTERN", "Same kit, hits 1/5/9/13",
             frames["02-put-a-hit-PROPOSED"],
             ["12-row overview kept.", "No 2×8. UNSAVED is the source badge."]),
            ("05  SAVE (PLAYING)", "SAVES WHEN STOPPED + SAVE AT STOP",
             frames["05-save-while-playing-PROPOSED"],
             ["Tap BT. Flash waits. Clock stays.", "Source strings only."]),
            ("06  SAVED", "SAVED chip, then normal SEQ",
             frames["06-saved-PROPOSED"],
             ["Only after a successful write.", "No ‘PROJECT SAVED’ panel."]),
        ],
        common_foot,
        "00-newcomer-path-proposed.png",
    )

    # Also a compact 03 vs 04 tell sheet
    make_sheet(
        "How you tell  ·  sound edit (03) vs one-step edit (04)",
        "Same matrix, same 909 + SOUND header in the proposal. The amber STEP line (already in source) is the difference. No banners.",
        [
            ("03  SOUND  ·  PROPOSED", "Whole drum. Footer SEQ: STEPS.",
             frames["03-edit-sound-PROPOSED"],
             ["No STEP hint. Edits apply to the sound."]),
            ("04  STEP  ·  PROPOSED", "STEP 5  < >: CHANCE  PARA",
             frames["04-edit-one-step-PARA-PROPOSED"],
             ["Amber STEP line is the tell (5803).", "RELEASE STEP FIRST is kept."]),
            ("04  on SEQ  ·  CURRENT = PROPOSED", "Tag: BD P-LOCK STEP 5",
             frames["04-edit-one-step-SEQ-CURRENT"],
             ["Kit grid stays. P-LOCK wording stays.", "Dot’s banner is not used."]),
        ],
        common_foot,
        "03-04-how-you-tell.png",
    )

    print("Wrote", len(frames), "frames and sheets to", OUT)
    for n, p in paths.items():
        print(" ", n, p[0].name, p[0].stat().st_size)


if __name__ == "__main__":
    main()
