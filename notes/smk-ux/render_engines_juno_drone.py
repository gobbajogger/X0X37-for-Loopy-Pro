#!/usr/bin/env python3
"""Juno and Drone engine-page frames at 94197fc8, folded into directions A/B/C.

SHIPPED frames are source reconstructions (ui.c, not a device photograph and
not a host-UI run). PROPOSED frames are not implemented. Fonts are repo TTFs,
not packed felucca bitmaps; advances may differ by a few pixels.

Does not write firmware, and does not write to seq-chords/.
"""
from __future__ import annotations

import hashlib
import math
import sys
from pathlib import Path

from PIL import Image, ImageDraw

sys.path.insert(0, "/workspace/smk-ux")

from render_mockups import (
    C_AMB, C_BLACK, C_DIM, C_HI, C_LINE, C_WHITE,
    HDR_H, MAIN_Y, N_ACCENT, N_AMBER, N_BASE, N_DIM, N_EDGE, N_FACE,
    N_GRAY, N_INK, N_LINE, N_SELECT_SIDE, N_WHITE,
    PART, RGB, box, fill_header_panel, hline, new_frame, night_panel,
    pattern_badge, play_icon, text, tw, vc_top, vline,
)
from render_directions import (
    SHEET_AMB, SHEET_BG, SHEET_CARD, SHEET_DIM, SHEET_INK, SHEET_LINE,
    SHEET_MUTE, SHEET_RULE, SHEET_SHIP, footer_segs, header_proposed,
    finish_c_spine, nn2, paste_device, play_mark, save_png, sheet_font,
    to_rgb565, wrap_text,
)

ROOT = Path("/workspace/smk-ux/directions")
OUT_1X = ROOT / "frames" / "1x"
OUT_2X = ROOT / "frames" / "2x"

# Part colour is the slot, not the engine. JUNO A / DRONE A = voice 0 = PART_303A.
# ui.c 89–93: 303A green (130,240,90). Compact and chip labels are always A/B
# at 5383–5387 and 5429, even when the other slot is a different kind.
PART["JUNO A"] = RGB(130, 240, 90)
PART["DRONE A"] = RGB(130, 240, 90)

PARA_FOOT = ("BLACK: SELECT", "WHITE: SOUND", "SEQ: STEPS")
OG_W, OG_H = 26, 18


def pct(v: int) -> int:
    return (v * 100 + 63) // 127


def unit(n: int) -> float:
    return n / 127.0


def fmt_s(t: float) -> tuple[str, str]:
    """Match unit_value X0X_U_S: ms below 1 s, else one decimal and 's'."""
    if t < 0.9995:
        return str(int(t * 1000.0 + 0.5)), "ms"
    return f"{t:.1f}", "s"


def fmt_hz(x: float) -> tuple[str, str]:
    """1 decimal under 10 Hz (stock unit_value is integer and would print 0 Hz)."""
    if x < 10.0:
        return f"{x:.1f}", "Hz"
    return str(int(x + 0.5)), "Hz"


# JUNO POLY factory (original_sound.c 39), Cathedral (52).
JUNO = {
    "WAVE": 42, "SUB": 28, "CUTOFF": 70, "RES": 18, "ENV": 45,
    "ATTACK": 8, "DECAY": 40, "SUSTAIN": 92, "RELEASE": 40, "HPF": 0,
    "LFO RATE": 15, "LFO DEPTH": 8, "WIDTH": 64, "CHO RATE": 25,
    "CHORUS": 45, "LEVEL": 100,
}
DRONE = {
    "REGISTER": 57, "BRIGHTNESS": 66, "FLUTTER": 51,
    "ATTACK": 23, "RELEASE": 83, "LEVEL": 95,
}

# Proposed SI from juno6_set / juno6_chorus_set / drone_string_refresh.
# WAVE stays a continuous mix (DSP: saw*(1-macro[0])+pulse*macro[0]); the
# unused s->waveform = x*2.99 field is not a discrete SAW/PUL readout.
JUNO_PROP = {
    "WAVE": (str(pct(JUNO["WAVE"])), "%"),
    "SUB": (str(pct(JUNO["SUB"])), "%"),
    "WIDTH": (str(int((0.1 + unit(JUNO["WIDTH"]) * 0.8) * 100 + 0.5)), "%"),
    "LEVEL": (str(pct(JUNO["LEVEL"])), "%"),
    "CUTOFF": (str(pct(JUNO["CUTOFF"])), "%"),
    "RES": (str(pct(JUNO["RES"])), "%"),
    "ENV": (str(pct(JUNO["ENV"])), "%"),
    "HPF": (str(pct(JUNO["HPF"])), "%"),
    "ATTACK": fmt_s(0.002 + unit(JUNO["ATTACK"]) ** 2 * 3),
    "DECAY": fmt_s(0.01 + unit(JUNO["DECAY"]) ** 2 * 3),
    "SUSTAIN": (str(pct(JUNO["SUSTAIN"])), "%"),
    "RELEASE": fmt_s(0.01 + unit(JUNO["RELEASE"]) ** 2 * 4),
    "LFO RATE": fmt_hz(0.1 + 8 * unit(JUNO["LFO RATE"]) ** 2),
    "LFO DEPTH": (str(pct(JUNO["LFO DEPTH"])), "%"),
    "CHO RATE": fmt_hz(0.1 + 4.9 * unit(JUNO["CHO RATE"])),
    "CHORUS": (str(pct(JUNO["CHORUS"])), "%"),
}
DRONE_PROP = {
    "REGISTER": (str(pct(DRONE["REGISTER"])), "%"),
    "BRIGHT": (str(pct(DRONE["BRIGHTNESS"])), "%"),
    "FLUTTER": (str(pct(DRONE["FLUTTER"])), "%"),
    "ATTACK": fmt_s(0.004 + unit(DRONE["ATTACK"]) ** 2 * 2),
    "RELEASE": fmt_s(0.08 + unit(DRONE["RELEASE"]) ** 2 * 4),
    "LEVEL": (str(pct(DRONE["LEVEL"])), "%"),
}


# ---------------------------------------------------------------------------
# Glyphs: procedural reuse of existing 26×19 families (not new bitmaps).
# ---------------------------------------------------------------------------

def _line(draw, x0, y0, x1, y1, c):
    draw.line([(x0, y0), (x1, y1)], fill=c)


def glyph_wave(draw, cx, y, mix, col):
    """draw_wave_glyph shape 1: saw blended to square, mix 0..127 (ui.c 6990–6998)."""
    prev = None
    for i in range(29):
        phase = i % 14
        saw = 15 - phase * 30 // 13
        square = 15 if phase < 7 else -15
        a = (saw * (127 - mix) + square * mix) // 127
        py = y + (a + 15) * 8 // 30
        if prev is not None:
            _line(draw, cx - 14 + i - 1, prev, cx - 14 + i, py, col)
        prev = py


def glyph_sub(draw, gx, gy, q, col):
    """Miami SUB: osc squares + sub-octave squares (ui.c 7911–7915)."""
    top, bot, right = gy + 3, gy + OG_H, gx + OG_W - 1
    a = 1 + int(q * 6 + 0.5)
    for i in range(4):
        xx = gx + i * 6
        _line(draw, xx, top, xx + 3, top, C_LINE)
        _line(draw, xx + 3, top, xx + 3, top + 4, C_LINE)
        _line(draw, xx + 3, top + 4, xx + 6, top + 4, C_LINE)
        _line(draw, xx + 6, top + 4, xx + 6, top, C_LINE)
    for i in range(2):
        xx = gx + i * 12
        _line(draw, xx, bot - a, xx + 6, bot - a, col)
        _line(draw, xx + 6, bot - a, xx + 6, bot, col)
        _line(draw, xx + 6, bot, xx + 12, bot, col)
        if xx + 12 < right:
            _line(draw, xx + 12, bot, xx + 12, bot - a, col)


def glyph_lpf(draw, gx, gy, cq, pq, col):
    """Chicago CUTOFF / PEAK lowpass (ui.c 7855–7866). pq is resonance bump."""
    top, bot, right = gy + 3, gy + OG_H, gx + OG_W - 1
    pivot = gx + 4 + int(cq * 14 + 0.5)
    peak = top + 2 - int(pq * 5 + 0.5)
    box(draw, gx, bot, OG_W, 1, C_LINE)
    _line(draw, gx, top + 2, pivot - 3, top + 2, col)
    _line(draw, pivot - 3, top + 2, pivot, peak, col)
    _line(draw, pivot, peak, pivot + 6 if pivot + 6 <= right else right, bot, col)
    if pivot + 6 < right:
        _line(draw, pivot + 6, bot, right, bot, col)


def glyph_hpf(draw, gx, gy, q, col):
    """Mirror of the existing lowpass knee: a high-pass rise. Same primitive, not a new bitmap."""
    top, bot, right = gy + 3, gy + OG_H, gx + OG_W - 1
    pivot = gx + 4 + int(q * 14 + 0.5)
    box(draw, gx, bot, OG_W, 1, C_LINE)
    _line(draw, gx, bot, max(gx, pivot - 6), bot, col)
    _line(draw, max(gx, pivot - 6), bot, pivot, top + 2, col)
    _line(draw, pivot, top + 2, right, top + 2, col)


def glyph_env(draw, gx, gy, cq, eq, col):
    """Chicago ENV knee, static (no live EG). ui.c 7867–7900 family."""
    top, bot, right = gy + 3, gy + OG_H, gx + OG_W - 1
    ENV_FALL = 4

    def knee(z):
        z = (z + 0.5) * 0.5
        z = 0 if z < 0 else 1 if z > 1 else z
        return gx + 2 + int(z * (OG_W - 3 - ENV_FALL) + 0.5)

    pivot = knee(cq + eq)
    tgt = knee(cq + (2 * eq - 1) if False else cq + eq * 0.5)
    passband = top + 3
    peak = passband - 1
    box(draw, gx, bot, OG_W, 1, C_LINE)
    if tgt != pivot:
        box(draw, tgt, bot - 3, 1, 3, C_LINE)
    if pivot - 2 > gx:
        _line(draw, gx, passband, pivot - 2, passband, col)
    _line(draw, pivot - 2, passband, pivot, peak, col)
    _line(draw, pivot, peak, pivot + ENV_FALL, bot, col)
    if pivot + ENV_FALL < right:
        _line(draw, pivot + ENV_FALL, bot, right, bot, col)


def glyph_level(draw, gx, gy, q, col):
    """LEVEL segmented meter, idle (ui.c 7909–7916)."""
    bot = gy + OG_H
    ceiling = int(q * 6.0 + 0.5)
    box(draw, gx, bot, OG_W, 1, C_LINE)
    for i in range(6):
        ink = col if i < ceiling else C_LINE
        box(draw, gx + i * 4, bot - 3 - i * 2, 3, 2, ink)


def glyph_stairs(draw, gx, gy, q, col, n=3):
    """Three octave ranks whose heights follow drone_string_refresh
    (drone_string.h 64–66): a mix, not Chicago RANGE’s discrete switch.
    Same stair vocabulary, no new bitmap."""
    bot = gy + OG_H
    box(draw, gx, bot, OG_W, 1, C_LINE)
    ranks = (0.74 - q * 0.17, 0.22 + q * 0.02, 0.04 + q * 0.15)
    sw = OG_W // 3
    for k, a in enumerate(ranks):
        h = 3 + int(max(0.0, min(1.0, a)) * (OG_H - 5) + 0.5)
        box(draw, gx + k * sw + 1, bot - h + 1, sw - 3, h, col)


def glyph_lfo(draw, gx, gy, q, col, rate=False):
    """Miami WOBBLE og_lfo_amp / og_lfo_rate (ui.c 7926–7940), idle (phase 0)."""
    bot = gy + OG_H
    mid = gy + OG_H // 2
    box(draw, gx, bot, OG_W, 1, C_LINE)
    prev = None
    cyc = (1.0 + 4.0 * q) if rate else 2.0
    amp = 1.0 if rate else (0.1 + 0.9 * q)
    for i in range(OG_W):
        t = i / (OG_W - 1)
        v = amp * math.sin(6.2831853 * t * cyc)
        yy = mid - int(v * 6.0 + (-0.5 if v < 0 else 0.5))
        if prev is not None:
            _line(draw, gx + i - 1, prev, gx + i, yy, col)
        prev = yy


def glyph_chorus(draw, gx, gy, col):
    """orig_fx_glyph type 1: chorus pair (ui.c 7752+)."""
    n = 2
    for row in range(n):
        y = gy + (5 + row * 7)
        py = y
        for k in range(1, 5):
            x = gx + k * 6
            yy = y + (2 if k & 1 else -2)
            _line(draw, gx + (k - 1) * 6, py, x, yy, col)
            py = yy


def glyph_pulse(draw, gx, gy, q, col):
    """Pulse whose duty follows Juno WIDTH (.1+x*.8). Same wave family, not a new bitmap."""
    mid = gy + OG_H // 2
    bot = gy + OG_H
    box(draw, gx, bot, OG_W, 1, C_LINE)
    duty = 0.1 + q * 0.8
    edge = gx + int(duty * (OG_W - 1) + 0.5)
    top = gy + 3
    _line(draw, gx, mid, gx, top, col)
    _line(draw, gx, top, edge, top, col)
    _line(draw, edge, top, edge, gy + OG_H - 3, col)
    _line(draw, edge, gy + OG_H - 3, gx + OG_W - 1, gy + OG_H - 3, col)


def glyph_attack(draw, gx, gy, q, col):
    """Rising ramp, length from the pot. Same line family as a tail, inverted."""
    bot = gy + OG_H
    box(draw, gx, bot, OG_W, 1, C_LINE)
    xe = 4 + int(q * 18 + 0.5)
    _line(draw, gx, bot, gx + xe, gy + 3, col)
    box(draw, gx + xe - 1, gy + 2, 3, 3, col)


def glyph_tail(draw, gx, gy, q, col):
    """Idle exponential tail, same idea as dg_tail (ui.c 8012–8040)."""
    bot = gy + OG_H
    H = OG_H - 2
    xe = 2 + int(23.0 * math.sqrt(min(1.0, max(0.0, q))) + 0.5)
    if xe < 2:
        xe = 2
    if xe > OG_W - 1:
        xe = OG_W - 1
    box(draw, gx, bot, OG_W, 1, C_LINE)
    h0 = -1
    for x in range(xe + 1):
        a = 2 ** (-9.966 * x / xe)
        h = max(1, int(a * H + 0.5))
        box(draw, gx + x, bot - h, 1, 1, col)
        if h0 > h + 1:
            box(draw, gx + x - 1, bot - h0, 1, h0 - h, col)
        h0 = h


def glyph_sustain(draw, gx, gy, q, col):
    """Hold level: a horizontal at the sustain height."""
    bot = gy + OG_H
    box(draw, gx, bot, OG_W, 1, C_LINE)
    y = bot - 2 - int(q * (OG_H - 5) + 0.5)
    _line(draw, gx + 2, gy + 4, gx + 8, y, col)
    _line(draw, gx + 8, y, gx + OG_W - 3, y, col)


# ---------------------------------------------------------------------------
# Headers and matrix
# ---------------------------------------------------------------------------

def header_shipped(img, draw, name, banks=1, page_bank=0, unsaved=False, playing=False):
    """Compact PARA header, night (5373–5408): SOUND_GREEN name, bank dots,
    pattern cue at 132, play at 175, unsaved at 237. No tempo, no SOUND word."""
    fill_header_panel(draw)
    text(img, 7, 2, "B", name, N_ACCENT)
    for i in range(banks):
        dx = 232 - (banks - 1 - i) * 10
        draw.ellipse([dx - 2, 7, dx + 2, 11], fill=N_AMBER)
        if i != page_bank:
            draw.ellipse([dx - 1, 8, dx + 1, 10], fill=N_FACE)
    pattern_badge(img, draw, 132, 1, C_WHITE)
    play_icon(draw, 175, 4, C_HI if playing else C_DIM)
    if unsaved:
        draw.ellipse([235, 1, 239, 5], fill=C_AMB)


def hint_shipped(img, draw, banks_hint):
    night_panel(draw, 0, 227, 240, 13, False)
    text(img, 7, 229, "T", "< >", N_GRAY)
    text(img, 234, 229, "T", banks_hint, N_AMBER, anchor="rt")


def hint_proposed(img, draw, style, banks_hint):
    box(draw, 0, 227, 240, 13, N_BASE)
    night_panel(draw, 0, 227, 240, 13, False)
    y = 229
    if style == "C":
        x = 4
        pairs = [("BLACK:", "SELECT"), ("WHITE:", "SOUND"), ("SEQ:", "STEPS")]
        for i, (k, a) in enumerate(pairs):
            if i:
                x += 7
            ink = C_HI if k.startswith("SEQ") else N_INK
            x = text(img, x, y, "T", k, C_DIM)
            x = text(img, x + 1, y, "T", a, ink)
    else:
        footer_segs(img, PARA_FOOT, C_DIM, y=y)
    text(img, 234, y, "T", banks_hint, N_AMBER, anchor="rt")


def strip_value(img, cx, y, num, unit, ink, cell_x=None):
    """ui.c 7172–7179. Right-align in the cell so a 3-digit ms is not clipped
    (TTF '308'+'ms' is 30 px HOST ESTIMATE; the drum 27 px zone is tight)."""
    if not unit or unit == "%":
        s = num + (unit or "")
        w = tw("T", s)
        if cell_x is not None:
            x = min(cx - w // 2, cell_x + 57 - w)
            x = max(cell_x + 30, x) if w <= 27 else (cell_x + 57 - w)
        else:
            x = cx - w // 2
        text(img, x, y, "T", s, ink)
        return
    w = tw("T", num) + tw("XS", unit) + 1
    x = (cell_x + 57 - w) if cell_x is not None else (cx - w // 2)
    x2 = text(img, x, y, "T", num, ink)
    text(img, x2 + 1, y + 1, "XS", unit, N_GRAY)



def draw_cell(img, draw, x, y, name, val, unit, ink, label, proposed, kind, q=0.0,
              extra=None, wave=False):
    """One 59×49 cell. SHIPPED: name + 'NN %' + 42 px bar, no glyph.
    PROPOSED: existing 26×19 family on the left (or a wave at cx-10), readout right, no bar."""
    cx = x + 29
    text(img, cx, y + 11, "T", name, label, anchor="ct")
    if not proposed:
        value = f"{val} {unit}" if unit else val
        text(img, cx, y + 26, "B", value, ink, anchor="ct")
        box(draw, x + 8, y + 45, 42, 1, N_LINE)
        if q > 0:
            box(draw, x + 8, y + 45, 1 + int(41 * q), 1, ink)
        return
    gy, gx = y + 22, x + 3
    if wave:
        glyph_wave(draw, cx - 10, y + 28, int(q * 127 + 0.5), ink)
        strip_value(img, cx + 17, y + 28, val, unit, ink, cell_x=x)
        return
    drawers = {
        "sub": lambda: glyph_sub(draw, gx, gy, q, ink),
        "lpf": lambda: glyph_lpf(draw, gx, gy, q, (extra or 0), ink),
        "hpf": lambda: glyph_hpf(draw, gx, gy, q, ink),
        "env": lambda: glyph_env(draw, gx, gy, extra or 0.5, q, ink),
        "level": lambda: glyph_level(draw, gx, gy, q, ink),
        "stairs": lambda: glyph_stairs(draw, gx, gy, q, ink, 3),
        "lfo": lambda: glyph_lfo(draw, gx, gy, q, ink, False),
        "lfo_rate": lambda: glyph_lfo(draw, gx, gy, q, ink, True),
        "chorus": lambda: glyph_chorus(draw, gx, gy, ink),
        "pulse": lambda: glyph_pulse(draw, gx, gy, q, ink),
        "attack": lambda: glyph_attack(draw, gx, gy, q, ink),
        "tail": lambda: glyph_tail(draw, gx, gy, q, ink),
        "sustain": lambda: glyph_sustain(draw, gx, gy, q, ink),
    }
    if kind in drawers:
        drawers[kind]()
        strip_value(img, x + 45, y + 28, val, unit, ink, cell_x=x)
    else:
        value = f"{val} {unit}" if unit else val
        text(img, cx, y + 26, "B", value, ink, anchor="ct")
        box(draw, x + 8, y + 45, 42, 1, N_LINE)
        if q > 0:
            box(draw, x + 8, y + 45, 1 + int(41 * q), 1, ink)


def draw_page_row(img, draw, row, title, knobs, cells, active, proposed):
    """One 49 px sound slab. cells: list of dicts or None (empty slot)."""
    y = MAIN_Y + 4 + row * 50
    height = 49
    night_panel(draw, 3, y, 234, height, active)
    label = N_INK if active else N_GRAY
    ink = N_INK if active else N_WHITE
    text(img, 8, y, "T", f"{row + 1}  {title}", label)
    if active and knobs:
        text(img, 232, y, "T", knobs, label, anchor="rt")
    for k, cell in enumerate(cells):
        if not cell:
            continue
        x = 4 + k * 59
        if k:
            vline(draw, x, y + 14, height - 18, N_SELECT_SIDE if active else N_LINE)
        draw_cell(img, draw, x, y, cell["name"], cell["val"], cell.get("unit", ""),
                  ink, N_GRAY, proposed, cell.get("glyph"), q=cell.get("q", 0),
                  extra=cell.get("extra"), wave=cell.get("wave", False))


def juno_cells(proposed: bool):
    def pot(name, glyph=None, wave=False, extra=None, prop_name=None):
        v = JUNO[name]
        q = v / 127.0
        if proposed:
            shown = prop_name or name
            num, unit_s = JUNO_PROP[shown if shown in JUNO_PROP else name]
            return {"name": shown, "val": num, "unit": unit_s, "q": q,
                    "glyph": glyph, "wave": wave, "extra": extra}
        return {"name": name, "val": str(pct(v)), "unit": "%", "q": q}

    osc = [
        pot("WAVE", wave=True),
        pot("SUB", "sub"),
        pot("WIDTH", "pulse"),
        pot("LEVEL", "level"),
    ]
    filt = [
        pot("CUTOFF", "lpf", extra=unit(JUNO["RES"])),
        pot("RES", "lpf", extra=unit(JUNO["RES"])),  # bump follows RES
        pot("ENV", "env", extra=unit(JUNO["CUTOFF"])),
        pot("HPF", "hpf"),
    ]
    # For RES, show a lowpass with this resonance as the bump, cutoff mid.
    filt[1]["extra"] = unit(JUNO["RES"])
    filt[1]["q"] = unit(JUNO["CUTOFF"])  # pivot from cutoff; bump from extra
    # wait: glyph_lpf(cq, pq) uses q as cq and extra as pq. For CUTOFF, q=cutoff, extra=res.
    # For RES, we want cq=cutoff, pq=res. So q should be cutoff and extra=res — but
    # then the "q" used is also for... only the glyph. Value still from JUNO_PROP["RES"].
    filt[1]["q"] = unit(JUNO["CUTOFF"])
    filt[1]["extra"] = unit(JUNO["RES"])
    env = [
        pot("ATTACK", "attack"),
        pot("DECAY", "tail"),
        pot("SUSTAIN", "sustain"),
        pot("RELEASE", "tail"),
    ]
    lfo = [
        pot("LFO RATE", "lfo_rate"),
        pot("LFO DEPTH", "lfo"),
        pot("CHO RATE", "lfo_rate"),
        pot("CHORUS", "chorus"),
    ]
    return [
        ("OSCILLATOR", "KNOBS 5-8", osc),
        ("FILTER", "", filt),
        ("ENVELOPE", "", env),
        ("LFO/CHORUS", "", lfo),
    ]


def drone_cells(proposed: bool):
    def pot(src, shown, glyph=None):
        v = DRONE[src]
        q = v / 127.0
        if proposed:
            num, unit_s = DRONE_PROP[shown if shown in DRONE_PROP else src]
            return {"name": shown, "val": num, "unit": unit_s, "q": q, "glyph": glyph}
        return {"name": src, "val": str(pct(v)), "unit": "%", "q": q}

    str_row = [
        pot("REGISTER", "REGISTER", "stairs"),
        pot("BRIGHTNESS", "BRIGHT" if proposed else "BRIGHTNESS", "lpf"),
        pot("FLUTTER", "FLUTTER", "lfo"),
        None,
    ]
    env = [
        pot("ATTACK", "ATTACK", "attack"),
        pot("RELEASE", "RELEASE", "tail"),
        pot("LEVEL", "LEVEL", "level"),
        None,
    ]
    # SOUND row: ENGINE / PRESET name / STATUS. Unchanged in PROPOSED (Main Builder).
    sound = [
        {"name": "ENGINE", "val": "DRONE", "unit": "", "q": 0},
        {"name": "PRESET", "val": "Cathedral", "unit": "", "q": 0, "preset": True},
        {"name": "STATUS", "val": "READY", "unit": "", "q": 0},
        None,
    ]
    return [
        ("STRING", "KNOBS 5-7", str_row),
        ("ENVELOPE", "", env),
        ("SOUND", "", sound),
    ]


def draw_sound_preset_cell(img, draw, x, y, active):
    """R_BPRESET: draw_preset_name (7158) + bar. Not a proposed change."""
    cx = x + 29
    ink = N_INK if active else N_WHITE
    label = N_GRAY
    text(img, cx, y + 11, "T", "PRESET", label, anchor="ct")
    # "Cathedral" fits FONT_T (~50?); use S if needed.
    name = "Cathedral"
    role = "S" if tw("S", name) <= 54 else "T"
    if tw(role, name) > 54:
        role = "T"
    text(img, cx, y + 26, role if tw("B", name) > 54 else "B", name, ink, anchor="ct")
    box(draw, x + 8, y + 45, 42, 1, N_LINE)


def draw_engine_matrix(img, draw, engine, proposed, active=0):
    box(draw, 0, MAIN_Y, 240, 220, N_BASE)
    pages = juno_cells(proposed) if engine == "juno" else drone_cells(proposed)
    for row, (title, knobs, cells) in enumerate(pages):
        y = MAIN_Y + 4 + row * 50
        is_active = row == active
        # For SOUND, PRESET uses the name path; others are plain value cells
        # with no glyph (even in proposed).
        if engine == "drone" and title == "SOUND":
            night_panel(draw, 3, y, 234, 49, is_active)
            label = N_INK if is_active else N_GRAY
            ink = N_INK if is_active else N_WHITE
            text(img, 8, y, "T", f"{row + 1}  {title}", label)
            for k, cell in enumerate(cells):
                if not cell:
                    continue
                x = 4 + k * 59
                if k:
                    vline(draw, x, y + 14, 31, N_SELECT_SIDE if is_active else N_LINE)
                if cell["name"] == "PRESET":
                    draw_sound_preset_cell(img, draw, x, y, is_active)
                else:
                    draw_cell(img, draw, x, y, cell["name"], cell["val"], "",
                              ink, N_GRAY, False, None, q=0)
            continue
        draw_page_row(img, draw, row, title, knobs if is_active else "", cells,
                      is_active, proposed)


def frame_engine(engine, style, proposed, unsaved=False, playing=False):
    im, d = new_frame()
    part = "JUNO A" if engine == "juno" else "DRONE A"
    banks_hint = "1-4" if engine == "juno" else "1-3"
    banks = 2 if engine == "juno" else 1
    if not proposed:
        header_shipped(im, d, part, banks=banks, page_bank=0,
                       unsaved=unsaved, playing=playing)
        draw_engine_matrix(im, d, engine, False, active=0)
        hint_shipped(im, d, banks_hint)
    else:
        header_proposed(im, d, style, part=part, mode="SOUND", tempo=125,
                        playing=playing, pattern=1, unsaved=unsaved)
        draw_engine_matrix(im, d, engine, True, active=0)
        hint_proposed(im, d, style, banks_hint)
        if style == "C":
            finish_c_spine(d, part=part)
    return to_rgb565(im)


# ---------------------------------------------------------------------------
# Sheets
# ---------------------------------------------------------------------------

CAPTION = {
    "shipped-juno": "Names, percents, and a thin bar. No picture of the wave, the filter, the envelope, or the chorus. Times are a percent.",
    "shipped-drone": "The same bare percent cells. BRIGHTNESS is a tight fit in the cell. The SOUND row is the engine and the preset, unchanged.",
    "A-juno": "Pictures the other engines already use: a wave, a sub, a filter, a tail, an LFO, a chorus. Times in milliseconds, rates in hertz. SOUND and the clock stay.",
    "A-drone": "Register as octave stairs, brightness as a tone filter, flutter as an LFO, tails for the envelope. BRIGHT is the shorter name. The SOUND row is not changed.",
    "B-juno": "The same pictures and units as A. The quiet rail and the hollow play triangle are B’s marks; they are not waiting signals here.",
    "B-drone": "The same pictures and units as A, in B’s marks. Waiting outlines stay reserved for a held step or a queued save.",
    "C-juno": "The same pictures and units as A. The green edge is this part’s room. SEQ: STEPS, in accent, is the way back.",
    "C-drone": "The same pictures and units as A, with the green room edge. The SOUND row is still the engine and the preset, not a new control.",
}

ROW_TITLE = {
    "juno": "Juno  ·  JUNO POLY, all four parameter pages",
    "drone": "Drone  ·  Cathedral, with the SOUND row as it ships",
}


def make_direction_sheet(style, frames, scale=1):
    S = scale
    margin = 64 * S
    gap = 72 * S
    bezel = 6 * S
    frame_s = 240 * S
    col_w = frame_s + bezel * 2
    top = 176 * S
    row_title_h = 28 * S
    col_lab_h = 22 * S
    cap_h = 80 * S
    row_gap = 36 * S
    foot_h = 140 * S
    names = {"A": "A  ·  Plain speech", "B": "B  ·  Live signals", "C": "C  ·  Two rooms"}
    decks = {
        "A": "Juno and Drone parameter pages, in A’s language: already-written words, SOUND and the clock, no new marks.",
        "B": "Juno and Drone parameter pages, in B’s language: fill is confirmed, a hollow triangle is stopped, the quiet rail is not a wait.",
        "C": "Juno and Drone parameter pages, in C’s language: a part-colour room edge, contrast footer, play icon as shipped.",
    }
    n = 2
    W = margin * 2 + col_w * 2 + gap
    H = top + n * (row_title_h + col_lab_h + col_w + cap_h + row_gap) + foot_h
    im = Image.new("RGB", (W, H), SHEET_BG)
    dr = ImageDraw.Draw(im)
    f_kicker = sheet_font(13 * S, "Medium")
    f_title = sheet_font(32 * S, "Bold")
    f_deck = sheet_font(15 * S, "Medium")
    f_row = sheet_font(15 * S, "Bold")
    f_col = sheet_font(13 * S, "Bold")
    f_cap = sheet_font(13 * S, "Medium")
    f_foot = sheet_font(12 * S, "Medium")
    dr.text((margin, 28 * S), "APPENDIX  ·  ENGINE PAGES", font=f_kicker, fill=SHEET_AMB)
    dr.text((margin, 46 * S), names[style], font=f_title, fill=SHEET_INK)
    dy = 88 * S
    for line in wrap_text(decks[style], f_deck, W - margin * 2, dr):
        dr.text((margin, dy), line, font=f_deck, fill=SHEET_DIM)
        dy += 20 * S
    dr.text((W - margin, 28 * S), "94197fc8   ·   NOT IMPLEMENTED",
            font=f_kicker, fill=SHEET_MUTE, anchor="rt")
    dr.text((W - margin, 46 * S), "Not a device photograph",
            font=f_kicker, fill=SHEET_MUTE, anchor="rt")
    y = top
    for eng in ("juno", "drone"):
        dr.text((margin, y), ROW_TITLE[eng], font=f_row, fill=SHEET_INK)
        y += row_title_h
        xs = [margin, margin + col_w + gap]
        labs = [("SHIPPED  94197fc8", SHEET_SHIP, frames[f"shipped-{eng}"], f"shipped-{eng}"),
                ("PROPOSED", SHEET_AMB, frames[f"{style}-{eng}"], f"{style}-{eng}")]
        for x, (lab, col, fr, key) in zip(xs, labs):
            dr.text((x, y), lab, font=f_col, fill=col)
        y += col_lab_h
        for x, (lab, col, fr, key) in zip(xs, labs):
            paste_device(im, fr if S == 1 else nn2(fr), x, y, bezel, SHEET_LINE)
            cy = y + col_w + 10 * S
            for line in wrap_text(CAPTION[key], f_cap, col_w, dr):
                dr.text((x, cy), line, font=f_cap, fill=SHEET_DIM)
                cy += 16 * S
        y += col_w + cap_h + row_gap
    fy = H - foot_h + 10 * S
    dr.line([margin, fy, W - margin, fy], fill=SHEET_RULE)
    foot = (
        "Pictures are reused from Chicago, Miami, and the 909 tails — no new glyph bitmaps. "
        "WAVE is a continuous saw-to-pulse mix, not three named shapes. "
        "The SOUND row is owned with presets and is not redesigned here. "
        "Chord and note drawing on SEQ is a separate task (seq-chords/). "
        "Fonts are repo TTFs, not packed felucca bitmaps. Not a device photograph. PROPOSED is not implemented."
    )
    cy = fy + 12 * S
    for line in wrap_text(foot, f_foot, W - margin * 2, dr):
        dr.text((margin, cy), line, font=f_foot, fill=SHEET_MUTE)
        cy += 16 * S
    return im


def make_combined_sheet(frames, scale=1):
    S = scale
    margin = 48 * S
    gap = 40 * S
    bezel = 5 * S
    frame_s = 240 * S
    col_w = frame_s + bezel * 2
    top = 168 * S
    row_title_h = 28 * S
    col_lab_h = 22 * S
    cap_h = 86 * S
    row_gap = 28 * S
    foot_h = 150 * S
    cols = ["shipped", "A", "B", "C"]
    col_labs = ["SHIPPED  94197fc8", "A  ·  Plain speech", "B  ·  Live signals", "C  ·  Two rooms"]
    n = 2
    W = margin * 2 + col_w * 4 + gap * 3
    H = top + n * (row_title_h + col_lab_h + col_w + cap_h + row_gap) + foot_h
    im = Image.new("RGB", (W, H), SHEET_BG)
    dr = ImageDraw.Draw(im)
    f_kicker = sheet_font(13 * S, "Medium")
    f_title = sheet_font(32 * S, "Bold")
    f_deck = sheet_font(15 * S, "Medium")
    f_row = sheet_font(15 * S, "Bold")
    f_col = sheet_font(13 * S, "Bold")
    f_cap = sheet_font(12 * S, "Medium")
    f_foot = sheet_font(12 * S, "Medium")
    dr.text((margin, 24 * S), "APPENDIX  ·  ENGINE PAGES", font=f_kicker, fill=SHEET_AMB)
    dr.text((margin, 42 * S), "Juno and Drone, in A, B, and C", font=f_title, fill=SHEET_INK)
    deck = (
        "The pictures and the units are the same in every direction. "
        "What changes is only that direction’s header, footer, and marks."
    )
    dy = 88 * S
    for line in wrap_text(deck, f_deck, W - margin * 2, dr):
        dr.text((margin, dy), line, font=f_deck, fill=SHEET_DIM)
        dy += 20 * S
    dr.text((W - margin, 24 * S), "94197fc8   ·   NOT IMPLEMENTED",
            font=f_kicker, fill=SHEET_MUTE, anchor="rt")
    dr.text((W - margin, 42 * S), "Not a device photograph",
            font=f_kicker, fill=SHEET_MUTE, anchor="rt")
    y = top
    for eng in ("juno", "drone"):
        dr.text((margin, y), ROW_TITLE[eng], font=f_row, fill=SHEET_INK)
        y += row_title_h
        xs = [margin + i * (col_w + gap) for i in range(4)]
        for x, lab, colk in zip(xs, col_labs, cols):
            color = SHEET_SHIP if colk == "shipped" else SHEET_AMB
            dr.text((x, y), lab, font=f_col, fill=color)
        y += col_lab_h
        for x, colk in zip(xs, cols):
            key = f"{colk}-{eng}"
            fr = frames[key]
            paste_device(im, fr if S == 1 else nn2(fr), x, y, bezel, SHEET_LINE)
            cy = y + col_w + 8 * S
            for line in wrap_text(CAPTION[key], f_cap, col_w, dr):
                dr.text((x, cy), line, font=f_cap, fill=SHEET_DIM)
                cy += 15 * S
        y += col_w + cap_h + row_gap
    fy = H - foot_h + 10 * S
    dr.line([margin, fy, W - margin, fy], fill=SHEET_RULE)
    foot = (
        "Pictures are reused from Chicago, Miami, and the 909 tails — no new glyph bitmaps. "
        "WAVE is a continuous saw-to-pulse mix, matching the mixer, not three named shapes. "
        "Just flipping the existing Chicago/Miami slot test would draw the wrong pictures (Juno’s indices are not Miami’s). "
        "The SOUND row is owned with presets and is not redesigned. "
        "Chord and note drawing on SEQ is a separate task (seq-chords/). "
        "Fonts are repo TTFs, not packed felucca bitmaps. Not a device photograph. PROPOSED is not implemented."
    )
    cy = fy + 12 * S
    for line in wrap_text(foot, f_foot, W - margin * 2, dr):
        dr.text((margin, cy), line, font=f_foot, fill=SHEET_MUTE)
        cy += 16 * S
    return im


def qa_frame(name, im, report):
    if im.size != (240, 240):
        report.append(f"FAIL {name}: size {im.size}")
        return
    px = im.load()
    bad = 0
    for y in range(240):
        for x in range(240):
            r, g, b = px[x, y]
            if (r, g, b) != RGB(r, g, b):
                bad += 1
    if bad:
        report.append(f"FAIL {name}: {bad} non-RGB565 pixels")
    else:
        report.append(f"OK   {name}: 240×240 RGB565")


def label_fit_report():
    rows = []
    for name in ("WAVE", "SUB", "CUTOFF", "RES", "ENV", "ATTACK", "DECAY",
                 "SUSTAIN", "RELEASE", "HPF", "LFO RATE", "LFO DEPTH", "WIDTH",
                 "CHO RATE", "CHORUS", "LEVEL", "REGISTER", "BRIGHTNESS",
                 "BRIGHT", "FLUTTER", "OSCILLATOR", "FILTER", "ENVELOPE",
                 "LFO/CHORUS", "STRING", "KNOBS 5-8", "KNOBS 5-7", "JUNO A",
                 "DRONE A", "Cathedral", "JUNO POLY", "ENGINE", "PRESET",
                 "STATUS", "READY", "SOUND"):
        wt = tw("T", name)
        rows.append((name, wt, "fits 59" if wt <= 57 else ("tight 59" if wt <= 59 else "OVERFLOW 59")))
    return rows


def main():
    OUT_1X.mkdir(parents=True, exist_ok=True)
    OUT_2X.mkdir(parents=True, exist_ok=True)
    report = []
    frames = {}
    # SHIPPED once; copied conceptually into each direction's comparison.
    for eng in ("juno", "drone"):
        im = frame_engine(eng, "A", proposed=False)
        frames[f"shipped-{eng}"] = im
        qa_frame(f"shipped-{eng}", im, report)
        p = save_png(im, OUT_1X / f"shipped-{eng}.png")
        save_png(nn2(im), OUT_2X / f"shipped-{eng}.png")
        print("wrote", p)
        for style in ("A", "B", "C"):
            im2 = frame_engine(eng, style, proposed=True)
            frames[f"{style}-{eng}"] = im2
            qa_frame(f"{style}-{eng}", im2, report)
            p = save_png(im2, OUT_1X / f"{style}-{eng}.png")
            save_png(nn2(im2), OUT_2X / f"{style}-{eng}.png")
            print("wrote", p)

    for style in ("A", "B", "C"):
        sh = make_direction_sheet(style, frames, 1)
        save_png(sh, ROOT / style / "engines-juno-drone.png")
        save_png(make_direction_sheet(style, frames, 2), ROOT / style / "engines-juno-drone-2x.png")
        print("wrote", ROOT / style / "engines-juno-drone.png")

    comb = make_combined_sheet(frames, 1)
    save_png(comb, ROOT / "engines-juno-drone.png")
    save_png(make_combined_sheet(frames, 2), ROOT / "engines-juno-drone-2x.png")
    print("wrote", ROOT / "engines-juno-drone.png")

    print("---- QA ----")
    for line in report:
        print(line)
    print("---- label widths (TTF HOST ESTIMATE) ----")
    for name, w, note in label_fit_report():
        print(f"  FONT_T {w:3d}  {note:12}  {name}")
    print("---- proposed values ----")
    print("JUNO", JUNO_PROP)
    print("DRONE", DRONE_PROP)


if __name__ == "__main__":
    main()
