#!/usr/bin/env python3
"""Phase-2 overhaul frames. Packed felucca + shipped_glyphs. Native 240x240.

Chord labels (owner, 2026-11-13): compact names ONLY on exact matches
(Cmin7, C/E, Cadd9, major triad as bare C, …). Every other case shows
the actual notes (put_note, sharps). Never the rejected §10 interval
form (C(3), C(b2 2)), and never a forced name. piano-roll.md is not
edited.

Writes under /workspace/smk-ux/overhaul/frames/{before,R,S,L}/{1x,3x}
and overview sheets. Firmware, seq-chords, and presets-spec are not
touched.
"""
from __future__ import annotations

import hashlib
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, "/workspace/smk-ux/directions/glyphs")
sys.path.insert(0, "/workspace/smk-ux/overhaul")

import shipped_glyphs as G
import felucca as F

OUT = Path("/workspace/smk-ux/overhaul")
FRAMES = OUT / "frames"

# AMBER night theme, same as the phase-1 corrected frames (gfx.c pal[4]).
C_HI = G.RGB(255, 184, 70)
C_AMB = G.RGB(232, 136, 30)
C_BASE = G.UI_N_BASE
C_FACE = G.UI_N_FACE
C_EDGE = G.UI_N_EDGE
C_INK = G.UI_N_INK
C_WHITE = G.UI_N_WHITE
C_GRAY = G.UI_N_GRAY
C_DIM = G.UI_N_DIM
C_LINE = G.UI_N_LINE
C_SELECT = G.UI_N_SELECT
C_SHADOW = G.UI_N_SHADOW

HDR_H = 20
KNOB_Y = 158
KNOB_H = 82
BAND_H = 69
CELL_W, CELL_H, CELL_STEP = 59, 46, 50
CELL_X0, SLAB0 = 4, 24
GLYPH_DX, GLYPH_DY = 3, 22
OG_W, OG_H = 26, 19

PART = {
    "909": G.PART_909,
    "808": G.PART_808,
    "CHI": G.PART_JUNO_A,   # PART_303A slot green, Chicago A
    "MIA": G.PART_303B,     # Miami in a B slot, cyan — never labelled 303B
    "JUNO": G.PART_JUNO_A,  # slot colour, engine word is JUNO
    "DRONE": G.PART_JUNO_A,
}

NOTE_N = "C C# D D# E F F# G G# A A# B".split()


def put_note(m: int) -> str:
    return f"{NOTE_N[m % 12]}{m // 12 - 1}"


def notes_text(midis) -> str:
    return " ".join(put_note(m) for m in midis)


def new_cv() -> G.Canvas:
    return G.Canvas(240, 240, C_BASE)


def blit(dst: G.Canvas, src: G.Canvas, x: int, y: int) -> None:
    for j in range(src.h):
        for i in range(src.w):
            dst.pset(x + i, y + j, src.get(i, j))


def fill(cv: G.Canvas, x: int, y: int, w: int, h: int, c: int) -> None:
    G.cv_rect(cv, x, y, w, h, c)


def save_cv(cv: G.Canvas, alt: str, name: str) -> Path:
    d1 = FRAMES / alt / "1x"
    d3 = FRAMES / alt / "3x"
    d1.mkdir(parents=True, exist_ok=True)
    d3.mkdir(parents=True, exist_ok=True)
    im = cv.to_pil()
    p1 = d1 / name
    im.save(p1)
    im.resize((720, 720), Image.Resampling.NEAREST).save(d3 / name)
    return p1


# ---------------------------------------------------------------------------
# Header + state
# ---------------------------------------------------------------------------

def _chip(cv, x, y, label, color, ink=C_SHADOW):
    # FONT_S space is only 3 px; a 5 px gap keeps "JUNO A" readable.
    parts = label.split(" ")
    if len(parts) == 2:
        tw = F.text_w("S", parts[0]) + 5 + F.text_w("S", parts[1])
    else:
        tw = F.text_w("S", label)
    w = tw + 10
    h = 14
    G.night_mark(cv, x, y, w, h, color, 1)
    if len(parts) == 2:
        F.cv_text(cv, x + 5, y + 1, "S", parts[0], ink)
        F.cv_text(cv, x + 5 + F.text_w("S", parts[0]) + 5, y + 1, "S", parts[1], ink)
    else:
        F.cv_text(cv, x + 5, y + 1, "S", label, ink)
    return w


def header(cv, *, alt, part, part_label, mode, playing=False, dirty=False,
           queued=None, tempo=125, pattern="P1", timeout_lost=False,
           cue=None):
    """Shared header. alt in {before, R, S, L}.

    L keeps a 2 px state rail and never drops UNSAVED (timeout_lost is
    ignored). Before may drop UNSAVED when timeout_lost is set — that is
    the shipped problem this overhaul is answering.
    """
    fill(cv, 0, 0, 240, HDR_H, C_BASE)
    # part chip
    col = PART[part]
    cw = _chip(cv, 2, 3, part_label, col)
    # mode word
    mx = 2 + cw + 5
    F.cv_text(cv, mx, 4, "S", mode, C_WHITE)
    # pattern badge (existing header_pattern_badge language)
    pw = F.text_w("S", pattern) + 6
    px = 126
    G.rbox(cv, px, 3, pw, 14, 2, C_FACE)
    G.rframe(cv, px, 3, pw, 14, 2, C_LINE)
    F.cv_text(cv, px + 3, 4, "S", pattern, C_GRAY)
    if queued is not None:
        q = f"Q:{queued}"
        qx = px + pw + 3
        # queued is a live cue, not a preset browser (K4 stays pattern cue)
        G.rbox(cv, qx, 3, F.text_w("T", q) + 6, 14, 2, C_SELECT)
        F.cv_text(cv, qx + 3, 5, "T", q, C_HI)
    if cue:
        F.cv_text(cv, px + pw + 3, 5, "T", cue, C_DIM)

    # dirty: L and R/S always show; before drops after a timeout
    show_dirty = dirty and not (alt == "before" and timeout_lost)
    if show_dirty:
        # existing words only — RAM capture is never the word SAVED
        tag = "UNSAVED"
        tw = F.text_w("T", tag)
        tx = 198 - tw
        if queued is not None:
            tx = 178 - tw
        F.cv_text(cv, tx, 6, "T", tag, C_AMB)

    # tempo + play
    t = str(tempo)
    F.text_r(cv, 226, 5, "T", t, C_DIM)
    if playing:
        G.play_icon(cv, 228, 5, C_HI)
    else:
        # shipped stopped: still the filled icon in dim (B hollow is a
        # documented proposal, used on S as "stopped is hollow")
        if alt == "S":
            # hollow on a BASE face
            G.play_icon_hollow(cv, 228, 5, C_GRAY, C_BASE)
        else:
            G.play_icon(cv, 228, 5, C_DIM)

    # 1 px rule
    fill(cv, 0, HDR_H - 1, 240, 1, C_LINE)

    # L: always-visible 2 px rail. Amber if waiting (dirty or queued),
    # green-accent if playing and clean, dim if idle. Never times out.
    if alt == "L":
        if dirty:
            rail = C_AMB
        elif queued is not None:
            rail = C_HI
        elif playing:
            rail = G.SOUND_GREEN
        else:
            rail = C_LINE
        fill(cv, 0, HDR_H - 2, 240, 2, rail)


def footer_hint(cv, text, alt="R", y=143):
    """First-play / journey hints. Not a new gesture; names existing keys.

    Default y=143 is the 16 px gap between a shortened SEQ grid and the
    knob band. Preset lists pass y=210 so the hint does not overwrite rows.
    FONT_T caps (XS/S fold i,e,o,h).
    """
    if alt == "before":
        return
    F.cv_text(cv, 4, y, "T", text, C_DIM)


# ---------------------------------------------------------------------------
# SOUND / PARA cells
# ---------------------------------------------------------------------------

def cell_face(cv, x, y, w, h, selected=False):
    face = C_SELECT if selected else C_FACE
    G.rbox(cv, x, y, w, h, 3, face)
    return face


def value_fit(cv, x, y, s, ink, lane_right=None):
    """strip_value_fit: right-aligned in the 27 px lane ending at x+57.

    FONT_T is 32–95 (no lowercase). Values with ms/Hz/s use FONT_XS.
    """
    right = lane_right if lane_right is not None else x + 57
    lane = right - (x + 30)
    role = "XS" if any(c.islower() for c in s) else "T"
    if F.text_w(role, s) > lane:
        s2 = s.replace(" ", "")
        if F.text_w(role, s2) <= lane + 3:
            s = s2
    F.text_r(cv, right, y + 28, role, s, ink)


def sound_cell(cv, col, row, name, value, selected, painter, ink=None):
    x = CELL_X0 + col * CELL_W
    y = SLAB0 + row * CELL_STEP
    face = cell_face(cv, x, y, CELL_W - 1, CELL_H, selected)
    ink = ink or (C_INK if selected else C_WHITE)
    F.cv_text(cv, x + 4, y + 2, "T", name, C_GRAY)
    if painter:
        painter(cv, x + GLYPH_DX, y + GLYPH_DY, ink, face)
    if value is not None:
        value_fit(cv, x, y, value, ink)
    return x, y, face


def paint_matrix(cv, cells, selected):
    """Up to 16 cells in the 4-row night matrix. Unused slots stay empty faces
    so the page is a full SOUND grammar, not two rows over a black void."""
    for i, (name, val, painter) in enumerate(cells):
        col, row = i % 4, i // 4
        sound_cell(cv, col, row, name, val, name == selected, painter)
    for i in range(len(cells), 16):
        col, row = i % 4, i // 4
        x = CELL_X0 + col * CELL_W
        y = SLAB0 + row * CELL_STEP
        cell_face(cv, x, y, CELL_W - 1, CELL_H, False)


def bar_painter(pct):
    """Shipped Juno/Drone SOUND: 42 px bar, no 26x19 (honest before)."""
    def paint(cv, gx, gy, ink, face):
        G.cv_rect(cv, gx, gy + 8, 42, 4, C_LINE)
        w = max(1, int(42 * pct / 100 + 0.5))
        G.cv_rect(cv, gx, gy + 8, w, 4, ink)
    return paint


def wave_painter(mix, shape=1):
    def paint(cv, gx, gy, ink, face):
        # draw_wave_glyph wants a centre and a top; 29 px wide around cx
        G.draw_wave_glyph(cv, gx + 13, gy, shape, mix, ink, face)
    return paint


def orig_painter(chi, c, val, q, cq=0.5, pq=0.0, env_idx=0):
    def paint(cv, gx, gy, ink, face):
        G.orig_strip_glyph_idle(
            cv, gx, gy, chi=chi, c=c, val=val, q=q, col=ink,
            cq=cq, pq=pq, env_idx=env_idx,
        )
    return paint


def drum_painter(role, val, q, **kw):
    def paint(cv, gx, gy, ink, face):
        G.drum_strip_glyph_idle(cv, gx, gy, role, val, q, ink, **kw)
    return paint


def juno_hpf_p(q):
    def paint(cv, gx, gy, ink, face):
        G.glyph_juno_hpf(cv, gx, gy, q, ink)
    return paint


def juno_width_p(q):
    def paint(cv, gx, gy, ink, face):
        G.glyph_juno_width(cv, gx, gy, q, ink, face)
    return paint


def juno_sus_p(q):
    def paint(cv, gx, gy, ink, face):
        G.glyph_juno_sustain(cv, gx, gy, q, ink)
    return paint


def drone_reg_p(q):
    def paint(cv, gx, gy, ink, face):
        G.glyph_drone_register(cv, gx, gy, q, ink)
    return paint


def filter_p(mode, cutoff, res):
    def paint(cv, gx, gy, ink, face):
        G.filter_glyph(cv, gx + 13, gy, mode, cutoff, res, ink)
    return paint


# 909 BD factory, drum909.c 80–82: Tune pot 34, tau ~12.96 ms.
TUNE_POT = 34
TUNE_TAU = 6.0 + (32.0 - 6.0) * 34 / 127.0
ATTACK_POT = 13


def matrix_909(cv, selected="TUNE", tail_s="0.50"):
    cells = [
        ("TUNE", "13 ms", drum_painter(G.DG_TUNE, TUNE_POT, TUNE_POT / 127.0, tau=TUNE_TAU)),
        ("ATTACK", "10 %", drum_painter(G.DG_ATTACK, ATTACK_POT, ATTACK_POT / 127.0)),
        ("DECAY", tail_s + " s", drum_painter(G.DG_DECAY, 64, 0.5, ms=500.0 if tail_s == "0.50" else 1500.0)),
        ("P.DPTH", "0", drum_painter(G.DG_BDSWEEP, 0, 0.0, tau=TUNE_TAU, m=0.0)),
        ("PITCH", "0.40", drum_painter(G.DG_BDSWEEP, 45, 45 / 127.0, tau=TUNE_TAU, m=0.0, pm=1.0)),
        ("LEVEL", "100", drum_painter(G.DG_LEVEL, 100, 100 / 127.0)),
        ("ACCENT", "127", drum_painter(G.DG_ACCENT, 127, 1.0)),
        ("ENGINE", "909", None),
    ]
    paint_matrix(cv, cells, selected)


def matrix_808(cv, selected="TUNE"):
    cells = [
        ("TUNE", "62", drum_painter(G.DG_TUNE, 62, 62 / 127.0)),
        ("TONE", "80", drum_painter(G.DG_LPF, 80, 80 / 127.0)),
        ("DECAY", "0.80 s", drum_painter(G.DG_DECAY, 80, 80 / 127.0, ms=800.0)),
        ("SNAPPY", "40", drum_painter(G.DG_SNAPPY, 40, 40 / 127.0)),
        ("LEVEL", "110", drum_painter(G.DG_LEVEL, 110, 110 / 127.0)),
        ("ACCENT", "127", drum_painter(G.DG_ACCENT, 127, 1.0)),
        ("ENGINE", "808", None),
        ("SOUND", "BD", drum_painter(G.DG_SOUND, 0, 0.0)),
    ]
    paint_matrix(cv, cells, selected)


def matrix_chicago(cv, selected="CUTOFF"):
    # Chicago is the 303. chi=True. Names from original_sound.c 9–16.
    cells = [
        ("WAVE", "SAW", wave_painter(0, 1)),
        ("GLIDE", "0", orig_painter(True, 1, 0, 0.0)),
        ("CUTOFF", "64", orig_painter(True, 2, 64, 64 / 127.0)),
        ("PEAK", "40", orig_painter(True, 3, 40, 40 / 127.0, cq=64 / 127.0, pq=40 / 127.0)),
        ("ENV", "70", orig_painter(True, 4, 70, 70 / 127.0, env_idx=4)),
        ("BITE", "20", orig_painter(True, 5, 20, 20 / 127.0)),
        ("GNAW", "10", orig_painter(True, 6, 10, 10 / 127.0)),
        ("ATTACK", "8", orig_painter(True, 7, 8, 8 / 127.0, env_idx=7)),
        ("DECAY", "50", orig_painter(True, 8, 50, 50 / 127.0, env_idx=8)),
        ("SUST", "80", orig_painter(True, 9, 80, 80 / 127.0, env_idx=9)),
        ("REL", "40", orig_painter(True, 10, 40, 40 / 127.0, env_idx=10)),
        ("AMPMOD", "0", orig_painter(True, 11, 0, 0.0)),
        ("LEVEL", "100", orig_painter(True, 12, 100, 100 / 127.0)),
        ("ARPON", "OFF", None),
        ("FXTYPE", "OFF", None),
        ("ENGINE", "CHI", None),
    ]
    paint_matrix(cv, cells, selected)


def matrix_miami(cv, selected="CUTOFF"):
    # Miami is never a 303 and is never labelled 303B. chi=False.
    # Names from original_sound_miami_params, original_sound.c 58–64.
    cells = [
        ("CRWAVE", "", orig_painter(False, 0, 40, 40 / 127.0)),
        ("CRLVL", "80", orig_painter(False, 1, 80, 80 / 127.0)),
        ("SUB", "64", orig_painter(False, 2, 64, 64 / 127.0)),
        ("CRPIT", "64", orig_painter(False, 3, 64, 64 / 127.0)),
        ("GLIDE", "0", orig_painter(False, 4, 0, 0.0)),
        ("XMLVL", "40", orig_painter(False, 5, 40, 40 / 127.0)),
        ("XMWAV", "", orig_painter(False, 6, 30, 30 / 127.0)),
        ("XMTUNE", "64", orig_painter(False, 7, 64, 64 / 127.0)),
        ("XMDEP", "20", orig_painter(False, 8, 20, 20 / 127.0)),
        ("CUTOFF", "70", orig_painter(False, 9, 70, 70 / 127.0)),
        ("CRUSH", "30", orig_painter(False, 10, 30, 30 / 127.0)),
        ("WOBSHP", "10", orig_painter(False, 11, 10, 10 / 127.0)),
        ("WOBDEP", "20", orig_painter(False, 12, 20, 20 / 127.0)),
        ("WOBRAT", "30", orig_painter(False, 13, 30, 30 / 127.0)),
        ("OUTPUT", "100", orig_painter(False, 14, 100, 100 / 127.0)),
        ("STEREO", "64", orig_painter(False, 15, 64, 64 / 127.0)),
    ]
    paint_matrix(cv, cells, selected)


# JUNO POLY factory pots, original_sound.c 39
JUNO_POLY = [42, 28, 70, 18, 45, 8, 40, 92, 40, 0, 15, 8, 64, 25, 45, 100]
# Cathedral, original_sound.c 52
DRONE_CATH = [57, 66, 51, 23, 83, 95]


def matrix_juno(cv, selected="WAVE", bars=False):
    """All 16 audible JUNO POLY pots (original_sound.c 39). After: 26x19.
    Before: honest shipped bars, same 16 names and percents/ms."""
    p = JUNO_POLY
    # percents / times already computed for POLY: wave 33, sub 22, cut 55,
    # res 14, env 35, atk 14 ms, dcy 308 ms, sus 72, rel 407 ms, hpf 0,
    # lfo 0.2 Hz, lfod 6 %, width 50, chorate 1.1 Hz, chorus 35, level 79.
    if bars:
        cells = [
            ("WAVE", "33 %", bar_painter(33)),
            ("SUB", "22 %", bar_painter(22)),
            ("CUTOFF", "55 %", bar_painter(55)),
            ("RES", "14 %", bar_painter(14)),
            ("ENV", "35 %", bar_painter(35)),
            ("ATTACK", "14 ms", bar_painter(6)),
            ("DECAY", "308 ms", bar_painter(31)),
            ("SUSTAIN", "72 %", bar_painter(72)),
            ("RELEASE", "407 ms", bar_painter(31)),
            ("HPF", "0", bar_painter(0)),
            ("LFO", "0.2 Hz", bar_painter(12)),
            ("LFODEP", "6 %", bar_painter(6)),
            ("WIDTH", "50 %", bar_painter(50)),
            ("CHORATE", "1.1 Hz", bar_painter(20)),
            ("CHORUS", "35 %", bar_painter(35)),
            ("LEVEL", "79 %", bar_painter(79)),
        ]
    else:
        cells = [
            ("WAVE", "33 %", wave_painter(p[0], 1)),
            ("SUB", "22 %", orig_painter(False, 2, p[1], p[1] / 127.0)),
            ("CUTOFF", "55 %", orig_painter(True, 2, p[2], p[2] / 127.0)),
            ("RES", "14 %", orig_painter(True, 3, p[3], p[3] / 127.0, cq=p[2] / 127.0, pq=p[3] / 127.0)),
            ("ENV", "35 %", orig_painter(True, 4, p[4], p[4] / 127.0, env_idx=4)),
            ("ATTACK", "14 ms", drum_painter(G.DG_ATTACK, p[5], p[5] / 127.0)),
            ("DECAY", "308 ms", drum_painter(G.DG_DECAY, p[6], p[6] / 127.0, ms=308.0)),
            ("SUSTAIN", "72 %", juno_sus_p(p[7] / 127.0)),
            ("RELEASE", "407 ms", drum_painter(G.DG_DECAY, p[8], p[8] / 127.0, ms=407.0)),
            ("HPF", "0", juno_hpf_p(p[9] / 127.0)),
            ("LFO", "0.2 Hz", orig_painter(True, 11, p[10], p[10] / 127.0)),
            ("LFODEP", "6 %", orig_painter(True, 11, p[11], p[11] / 127.0)),
            ("WIDTH", "50 %", juno_width_p(p[12] / 127.0)),
            ("CHORATE", "1.1 Hz", orig_painter(True, 11, p[13], p[13] / 127.0)),
            ("CHORUS", "35 %", orig_painter(True, 12, p[14], p[14] / 127.0)),
            ("LEVEL", "79 %", orig_painter(True, 12, p[15], p[15] / 127.0)),
        ]
    paint_matrix(cv, cells, selected)


def matrix_juno_bank2(cv, selected="CHORUS"):
    """Same 16-cell Juno page, CHORUS selected — one grammar, no second bank required."""
    matrix_juno(cv, selected=selected, bars=False)


def matrix_drone(cv, selected="REGIST", bars=False):
    p = DRONE_CATH
    # Cathedral percents: reg 45, bright 52, flutter 40, atk 70 ms, rel 1.8 s, level 75.
    if bars:
        cells = [
            ("REGISTER", "45 %", bar_painter(45)),
            ("BRIGHT", "52 %", bar_painter(52)),
            ("FLUTTER", "40 %", bar_painter(40)),
            ("ATTACK", "70 ms", bar_painter(18)),
            ("RELEASE", "1.8 s", bar_painter(65)),
            ("LEVEL", "75 %", bar_painter(75)),
            ("ENGINE", "DRONE", None),
            ("PRESET", "CATH", None),
        ]
    else:
        cells = [
            ("REGISTER", "45 %", drone_reg_p(p[0] / 127.0)),
            ("BRIGHT", "52 %", orig_painter(True, 2, p[1], p[1] / 127.0)),
            ("FLUTTER", "40 %", orig_painter(True, 11, p[2], p[2] / 127.0)),
            ("ATTACK", "70 ms", drum_painter(G.DG_ATTACK, p[3], p[3] / 127.0)),
            ("RELEASE", "1.8 s", drum_painter(G.DG_DECAY, p[4], p[4] / 127.0, ms=1788.0)),
            ("LEVEL", "75 %", orig_painter(True, 12, p[5], p[5] / 127.0)),
            ("ENGINE", "DRONE", None),
            ("PRESET", "CATH", None),
        ]
    paint_matrix(cv, cells, selected)


# ---------------------------------------------------------------------------
# Knob band (y=158, h=82)
# ---------------------------------------------------------------------------

def knob_cell(cv, i, n, name, value, painter=None, selected=False, big=True):
    w = (240 - 8) // n
    x = 4 + i * w
    y = KNOB_Y
    h = KNOB_H - 4
    if n == 4:
        # leave 10 px at the bottom of the screen for a first-play hint
        # on after frames — the cell itself is still the 82 px band.
        pass
    face = cell_face(cv, x, y + 2, w - 2, h - 4, selected)
    ink = C_INK if selected else C_WHITE
    F.cv_text(cv, x + 4, y + 6, "T", name, C_GRAY)
    if painter:
        painter(cv, x + (w - 2) // 2, y + 22, ink, face)
    if value:
        inner = w - 8
        lower = any(c.islower() for c in value)
        cands = (("S", "XS") if lower else (("M", "S", "XS") if big else ("S", "XS", "T")))
        role, shown = cands[-1], value
        for r in cands:
            if F.text_w(r, value) <= inner:
                role, shown = r, value
                break
        else:
            tight = value.replace(" ", "")
            for r in cands:
                if F.text_w(r, tight) <= inner:
                    role, shown = r, tight
                    break
        F.text_c(cv, x + (w - 2) // 2, y + 52, role, shown, ink)
    return x, w, face, ink


def knobs_909(cv, selected=None):
    knob_cell(cv, 0, 3, "TUNE", "13 ms",
              lambda c, cx, y, ink, face: G.drum_strip_glyph_idle(
                  c, cx - 13, y, G.DG_TUNE, TUNE_POT, TUNE_POT / 127.0, ink, tau=TUNE_TAU),
              selected == "TUNE")
    knob_cell(cv, 1, 3, "ATTACK", "10 %",
              lambda c, cx, y, ink, face: G.drum_strip_glyph_idle(
                  c, cx - 13, y, G.DG_ATTACK, ATTACK_POT, ATTACK_POT / 127.0, ink),
              selected == "ATTACK")
    knob_cell(cv, 2, 3, "DECAY", "0.50 s",
              lambda c, cx, y, ink, face: G.drum_strip_glyph_idle(
                  c, cx - 13, y, G.DG_DECAY, 64, 0.5, ink, ms=500.0),
              selected == "DECAY")


def knobs_seq_chord(cv, exact_name, midis, gate="12", acc="100", selected="NOTE"):
    """Held-step knobs. NOTE cell: exact compact name, else actual notes.

    54 px NOTE cell. Never interval form. Never a forced name.
    C3xN is used only as a last-resort overflow of the notes themselves,
    not as a chord name — and only if the spaced note list cannot fit
    even as FONT_XS without spaces.
    """
    if exact_name:
        note_s = exact_name
    else:
        spaced = notes_text(midis)
        tight = "".join(put_note(m) for m in midis)
        if F.text_w("XS", spaced) <= 54:
            note_s = spaced
        elif F.text_w("XS", tight) <= 54:
            note_s = tight
        else:
            # overflow of the NOTES, not a name
            note_s = f"{put_note(midis[0])}x{len(midis)}"
    knob_cell(cv, 0, 4, "NOTE", note_s, selected=(selected == "NOTE"))
    knob_cell(cv, 1, 4, "GATE", gate, selected=(selected == "GATE"))
    knob_cell(cv, 2, 4, "ACCENT", acc, selected=(selected == "ACCENT"))
    knob_cell(cv, 3, 4, "SLIDE", "OFF", selected=(selected == "SLIDE"))


# ---------------------------------------------------------------------------
# SEQ: 909 kit grid, and the 16-column roll
# ---------------------------------------------------------------------------

def step_grid(cv, hits, playhead=None, held=None, rows=2, cols=16, y=24, h=128):
    """16-step TRIG grid for 909/808 SEQ. hits is a set of step indices 0..15."""
    margin = 4
    cw = (240 - margin * 2) // cols
    ch = (h - 10) // rows if rows > 1 else h - 8
    # one row of 16 is more honest for a bar; two rows of 8 if cols=8
    if cols == 16 and rows == 2:
        cols, rows = 8, 2
        cw = (240 - margin * 2) // cols
        ch = (h - 10) // rows
    for s in range(16):
        r, c = divmod(s, 8) if rows == 2 else (0, s)
        x = margin + c * cw
        yy = y + r * ch
        on = s in hits
        face = C_SELECT if on else C_FACE
        if held == s:
            G.rbox(cv, x + 1, yy + 2, cw - 3, ch - 4, 3, C_SELECT)
            G.rframe(cv, x + 1, yy + 2, cw - 3, ch - 4, 3, C_AMB)
        else:
            G.rbox(cv, x + 1, yy + 2, cw - 3, ch - 4, 3, face)
        ink = C_INK if on else C_DIM
        F.text_c(cv, x + cw // 2, yy + ch // 2 - 6, "T", str(s + 1), ink)
        if playhead == s:
            fill(cv, x + 2, yy + ch - 5, cw - 5, 2, C_HI)


def piano_roll(cv, steps, *, lo=48, hi=None, held=None, playhead=None,
               exact_by_step=None, y=24, h=110, alt="R"):
    """16-column roll, night_mark per pitch, width law from ui.c 5617–5618.

    steps: list of 16 items, each None or (midis, end_nibble, tie).
    exact_by_step: optional dict step -> exact compact name (or None).
    Held line at y+h lists every onset pitch (FONT_B/S/XS, 232 px).
    The compact name, if exact, sits on the held line after the notes
    only as a confirmation — actually owner asked names in SEQ; we put
    the exact name in the NOTE cell and, on the held line, the notes
    always (every pitch). If exact, prefix the name then the notes.
    Ambiguous / unmatched: notes only, no forced name, no second reading.
    """
    if hi is None:
        # floor of 14 st (ui.c 5581–5596)
        pitches = [m for st in steps if st for m in st[0]]
        if pitches:
            lo = min(min(pitches), lo)
            hi = max(lo + 13, max(pitches))
        else:
            hi = lo + 13
    span = max(1, hi - lo)
    cols = 16
    x0, w = 4, 232
    cw = w // cols  # 14
    # pitch rows
    row_h = max(6, (h - 14) // (span + 1))
    fill(cv, x0, y, w, h - 12, C_BASE)
    # faint staff
    for oct_c in range(lo, hi + 1):
        if oct_c % 12 == 0:
            yy = y + (hi - oct_c) * row_h
            fill(cv, x0, yy + 4, w, 1, C_LINE)
    for s, st in enumerate(steps):
        x = x0 + s * cw
        if playhead == s:
            fill(cv, x, y, cw, h - 12, G.blend(C_BASE, C_HI, 3))
        if not st:
            continue
        midis, end, tie = st
        # width = end ? (end*12+14)/15 : 12   (ui.c 5617–5618)
        width = (end * 12 + 14) // 15 if end else 12
        width = min(width, cw - 1)
        for m in midis:
            yy = y + (hi - m) * row_h
            ink = C_WHITE if held == s else C_INK
            G.night_mark(cv, x + 1, yy, max(3, width - 1), 5, ink, 1)
            if held == s:
                G.rframe(cv, x, yy - 1, width + 1, 7, 1, C_AMB)
            if tie:
                # TIE connector toward next column, same y
                G.cv_rect(cv, x + width, yy + 2, cw - width + 1, 1, C_GRAY)
    # playhead cursor
    if playhead is not None:
        fill(cv, x0 + playhead * cw, y + h - 14, cw - 1, 2, C_HI)

    # held line, 232 px (ui.c 5645–5659)
    ly = y + h - 10
    if held is not None and steps[held]:
        midis = steps[held][0]
        exact = (exact_by_step or {}).get(held)
        # notes always, pitch order (already sorted)
        body = notes_text(midis)
        prefix = f"STEP {held + 1}  "
        # exact name first when we have a unique exact match; notes still listed
        if exact:
            line = f"{prefix}{exact}  {body}"
        else:
            line = f"{prefix}{body}"
        role = "B"
        if F.text_w(role, line) > 232:
            role = "S"
        if F.text_w(role, line) > 232:
            role = "XS"
        F.cv_text(cv, 4, ly, role, line, C_HI if alt != "before" else C_WHITE)
    else:
        F.cv_text(cv, 4, ly, "T", "P1 SEQUENCE", C_DIM)


# ---------------------------------------------------------------------------
# Preset list (browse / audition / load)
# ---------------------------------------------------------------------------

JUNO_NAMES = [
    (0, "JUNO POLY", False),
    (1, "JUNO PAD", False),
    (2, "JUNO PLUCK", False),
    (3, "JUNO BRASS", False),
    (4, "JUNO SOFT", True),
    (5, "JUNO STR", True),
    (6, "JUNO STAB", True),
    (7, "JUNO BASS", True),
]


def preset_list(cv, selected=0, y=24, audition=None, proposal_note=True):
    F.cv_text(cv, 6, y, "T", "FACTORY  JUNO", C_GRAY)
    yy = y + 14
    for idx, name, proposed in JUNO_NAMES:
        on = idx == selected
        face = C_SELECT if on else C_FACE
        G.rbox(cv, 4, yy, 232, 16, 2, face)
        ink = C_INK if on else (C_DIM if proposed else C_WHITE)
        F.cv_text(cv, 10, yy + 2, "S", f"{idx}  {name}", ink)
        if proposed and proposal_note:
            F.text_r(cv, 230, yy + 3, "T", "PROPOSAL", C_AMB if on else C_DIM)
        if audition == idx:
            F.text_r(cv, 180, yy + 3, "T", "AUD", C_HI)
        yy += 17
    F.cv_text(cv, 6, 226, "T", "0-3 SHIPPED   4+ PROPOSAL, NOT IMPLEMENTED", C_DIM)


def toast(cv, text, tick_color=C_AMB, y=2):
    """Outline pill, clock stays (no full-header takeover). Existing words."""
    w = F.text_w("S", text) + 14
    x = (240 - w) // 2
    G.rbox(cv, x, y, w, 16, 2, C_BASE)
    G.rframe(cv, x, y, w, 16, 2, tick_color)
    fill(cv, x, y + 3, 3, 10, tick_color)
    F.cv_text(cv, x + 8, y + 2, "S", text, C_HI)


# ---------------------------------------------------------------------------
# Screen composers
# ---------------------------------------------------------------------------

def screen_sound(alt, engine, **hdr):
    cv = new_cv()
    labels = {
        "909": ("909", "909 A", matrix_909),
        "808": ("808", "808 A", matrix_808),
        "chicago": ("CHI", "CHI A", matrix_chicago),
        "miami": ("MIA", "MIA B", matrix_miami),
        "juno": ("JUNO", "JUNO A", None),
        "drone": ("DRONE", "DRONE A", None),
    }
    part, plab, fn = labels[engine]
    header(cv, alt=alt, part=part, part_label=plab, mode="SOUND", **hdr)
    if engine == "juno":
        matrix_juno(cv, bars=(alt == "before"))
    elif engine == "drone":
        matrix_drone(cv, bars=(alt == "before"))
    else:
        fn(cv)
    return cv


def screen_first_play(alt, playing=False):
    cv = new_cv()
    # R/L/before: 909 SEQ is the first play (evolved 01 was right).
    # S: still 909 kit first (the box boots on 909); a companion
    #    first-play-juno opens on the roll — drawn separately.
    header(cv, alt=alt, part="909", part_label="909 A", mode="SEQ",
           playing=playing, dirty=False, tempo=125, pattern="P1")
    hits = {0, 4, 8, 12, 10}  # a simple house pattern, BD on quarters + a ghost
    step_grid(cv, hits, playhead=(0 if playing else None), y=24, h=118)
    knobs_909(cv)
    if alt != "before":
        # FONT_T (caps): XS/S fold i,e,o,h (LOWER_KEEP in gen_font.py 183)
        footer_hint(cv, "PLAY TO HEAR   K2 SOUND   K3 STEP   K4 CUE", alt)
    return cv


def screen_step(alt, playing=False):
    cv = new_cv()
    header(cv, alt=alt, part="909", part_label="909 A", mode="STEP 5",
           playing=playing, dirty=True, tempo=125, pattern="P1")
    hits = {0, 4, 8, 12, 10}
    step_grid(cv, hits, playhead=(4 if playing else None), held=4, y=24, h=118)
    knobs_909(cv, selected="DECAY")
    if alt != "before":
        footer_hint(cv, "PARAMETERS VISIBLE   NO HOLD-TO-EDIT", alt)
    return cv


def screen_dirty(alt, timeout_lost=False, playing=True):
    cv = new_cv()
    header(cv, alt=alt, part="909", part_label="909 A", mode="SEQ",
           playing=playing, dirty=True, timeout_lost=timeout_lost,
           tempo=125, pattern="P1")
    hits = {0, 4, 8, 12, 10}
    step_grid(cv, hits, playhead=8 if playing else None, y=24, h=118)
    knobs_909(cv)
    if alt != "before":
        # existing words; RAM is not SAVED
        toast(cv, "SAVE AT STOP" if playing else "UNSAVED", C_AMB, y=22)
    elif timeout_lost:
        # honest shipped problem: the dirty mark has gone, the edit has not
        F.cv_text(cv, 6, 22, "T", "UNSAVED EDIT  DISPLAY TIMED OUT", C_DIM)
    return cv


def screen_saved(alt):
    cv = new_cv()
    header(cv, alt=alt, part="909", part_label="909 A", mode="SEQ",
           playing=False, dirty=False, tempo=125, pattern="P1")
    hits = {0, 4, 8, 12, 10}
    step_grid(cv, hits, y=24, h=118)
    knobs_909(cv)
    # SAVED is only for a successful flash write
    toast(cv, "SAVED", G.SOUND_GREEN, y=22)
    return cv


def screen_queued(alt, playing=True):
    cv = new_cv()
    header(cv, alt=alt, part="909", part_label="909 A", mode="SEQ",
           playing=playing, dirty=False, queued=2, tempo=125, pattern="P1")
    hits = {0, 4, 8, 12, 10}
    step_grid(cv, hits, playhead=12 if playing else None, y=24, h=118)
    knobs_909(cv)
    if alt != "before":
        footer_hint(cv, "K4 IS PATTERN CUE   NOT A PRESET BROWSER", alt)
    return cv


# Juno SEQ patterns for chord policy demos. MIDI C3=48.
def _st(midis, end=15, tie=False):
    return (list(midis), end, tie)


# A 16-step Juno figure used across exact / notes frames; the held step varies.
def juno_steps(kind):
    rest = None
    C = [48, 52, 55]             # C major triad — exact name "C" (bare, never Cmaj)
    Cmin7 = [48, 51, 55, 58]     # Cmin7 exact
    CE = [52, 55, 60]            # C/E exact inversion
    Cadd9 = [48, 50, 52, 55]     # Cadd9 exact TIER3
    dyad = [48, 52]              # C3 E3 — NOT C(3)
    cluster = [48, 49, 50]       # C3 C#3 D3 — NOT C(b2 2)
    amb = [50, 55, 60]           # D3 G3 C4 — two readings, notes only
    table = {
        "cmin7":  (Cmin7, "Cmin7", 0),
        "inv":    (CE, "C/E", 0),
        "add9":   (Cadd9, "Cadd9", 0),
        "maj":    (C, "C", 0),
        "dyad":   (dyad, None, 0),
        "cluster":(cluster, None, 0),
        "amb":    (amb, None, 0),
    }
    midis, exact, held = table[kind]
    steps = [rest] * 16
    steps[0] = _st(midis, 15, False)
    steps[4] = _st(C, 12, False)
    # TIE: extras live on the onset (step 8). Connectors at each pitch.
    # The TIE index itself is not a new onset (sequencer.c 278–287).
    steps[8] = _st(midis, 15, True)
    steps[12] = _st([48], 8, False)
    return steps, {0: exact, 4: "C", 8: exact, 12: None}, 0


def screen_juno_seq(alt, kind, playing=False, held=True):
    cv = new_cv()
    mode = "STEP 1" if held else "SEQ"
    header(cv, alt=alt, part="JUNO", part_label="JUNO A", mode=mode,
           playing=playing, dirty=False, tempo=120, pattern="P1")
    steps, exact_map, held_s = juno_steps(kind)
    if alt == "before":
        # Honest shipped: root or C3xN dump, no compact chord name, no interval.
        exact_map = {}
    piano_roll(cv, steps, lo=48, hi=62, held=(0 if held else None),
               playhead=(0 if playing else None),
               exact_by_step=exact_map if alt != "before" else {},
               y=24, h=110, alt=alt)
    midis = steps[0][0]
    exact = exact_map.get(0) if alt != "before" else None
    if alt == "before":
        # shipped NOTE cell: one pitch FONT_M, more = C3xN overflow (not a name)
        exact = None
        # force overflow form to show what shipped does on a tetrad
        if len(midis) > 1:
            knobs_seq_chord(cv, None, midis)
            # overwrite NOTE with the shipped compact dump so the before
            # is honest (C3x3 / C3x4), not a chord name
            # Re-draw NOTE only: we already called knobs with notes — for
            # before, show C3xN which is the shipped overflow, not a name.
            # Simplest: call with a fake exact equal to the dump? That would
            # violate "never draw dump as a name" on AFTER. On BEFORE it is
            # the shipped behaviour. Use a direct redraw.
        knobs_seq_chord(cv, f"{put_note(midis[0])}x{len(midis)}" if len(midis) > 1 else put_note(midis[0]), midis)
    else:
        knobs_seq_chord(cv, exact, midis)
    return cv


def screen_preset(alt, mode="browse", selected=4, playing=False, dirty=False):
    cv = new_cv()
    header(cv, alt=alt, part="JUNO", part_label="JUNO A", mode="SOUND",
           playing=playing, dirty=dirty, tempo=120, pattern="P1")
    if alt == "before":
        # only 4 factory, no working user bank (import returns -1)
        F.cv_text(cv, 6, 28, "T", "FACTORY  JUNO", C_GRAY)
        yy = 44
        for idx, name, _ in JUNO_NAMES[:4]:
            on = idx == min(selected, 3)
            G.rbox(cv, 4, yy, 232, 20, 2, C_SELECT if on else C_FACE)
            F.cv_text(cv, 10, yy + 3, "S", f"{idx}  {name}", C_INK if on else C_WHITE)
            yy += 22
        F.cv_text(cv, 6, 150, "T", "4 FACTORY   USER BANK NOT WORKING", C_DIM)
        F.cv_text(cv, 6, 166, "T", "IMPORT RETURNS -1", C_DIM)
        return cv
    audition = selected if mode == "audition" else None
    preset_list(cv, selected=selected, y=24, audition=audition)
    if mode == "audition":
        footer_hint(cv, "AUDITION HEARS   LOAD COMMITS   PLAYING QUEUES", alt, y=210)
    elif mode == "load":
        toast(cv, "LOADED  JUNO SOFT", G.SOUND_GREEN, y=22)
    elif mode == "load-playing":
        # Q: is the pattern cue (K4). A preset waiting is named, not Q-numbered.
        toast(cv, "LOAD AT STOP  SOFT", C_HI, y=22)
        footer_hint(cv, "PRESET WAITS AT STOP   FLASH DOES NOT ERASE IN PLAY", alt, y=210)
    elif mode == "save":
        toast(cv, "SAVE AT STOP" if playing else "SAVED", C_AMB if playing else G.SOUND_GREEN, y=22)
    elif mode == "recover":
        # power-loss honesty: last successful FLASH write, RAM is not SAVED
        toast(cv, "SAVES WHEN STOPPED", C_AMB, y=22)
        F.cv_text(cv, 6, 210, "T", "RAM CAPTURE IS NOT SAVED   LAST FLASH WRITE STANDS", C_DIM)
    return cv


def screen_juno_first_s():
    """S only: pitched engines open on the piano-roll, not a kit."""
    cv = new_cv()
    header(cv, alt="S", part="JUNO", part_label="JUNO A", mode="SEQ",
           playing=False, dirty=False, tempo=120, pattern="P1")
    steps, exact_map, _ = juno_steps("maj")
    piano_roll(cv, steps, lo=48, hi=62, held=None, exact_by_step=exact_map,
               y=24, h=110, alt="S")
    knobs_seq_chord(cv, "C", [48, 52, 55])
    footer_hint(cv, "JUNO OPENS ON THE ROLL   PLAY TO HEAR   K2 SOUND", "S")
    return cv


def screen_juno_bank2(alt):
    cv = new_cv()
    header(cv, alt=alt, part="JUNO", part_label="JUNO A", mode="SOUND",
           playing=False, dirty=False, tempo=120, pattern="P1")
    if alt == "before":
        matrix_juno(cv, selected="CHORUS", bars=True)
    else:
        matrix_juno_bank2(cv, selected="CHORUS")
    return cv


# ---------------------------------------------------------------------------
# Build all frames + sheets
# ---------------------------------------------------------------------------

BEFORE_SCREENS = [
    ("01-first-play.png", lambda: screen_first_play("before", playing=False)),
    ("02-sequence.png", lambda: screen_first_play("before", playing=True)),
    ("03-sound-909.png", lambda: screen_sound("before", "909")),
    ("03-sound-808.png", lambda: screen_sound("before", "808")),
    ("03-sound-chicago.png", lambda: screen_sound("before", "chicago")),
    ("03-sound-miami.png", lambda: screen_sound("before", "miami")),
    ("03-sound-juno.png", lambda: screen_sound("before", "juno")),
    ("03-sound-drone.png", lambda: screen_sound("before", "drone")),
    ("04-step.png", lambda: screen_step("before")),
    ("05-dirty-timeout.png", lambda: screen_dirty("before", timeout_lost=True, playing=True)),
    ("06-saved.png", lambda: screen_saved("before")),
    ("07-queued.png", lambda: screen_queued("before")),
    ("seq-juno.png", lambda: screen_juno_seq("before", "cmin7", held=True)),
    ("preset-browse.png", lambda: screen_preset("before", "browse", selected=0)),
]


def after_screens(alt):
    rows = [
        ("01-first-play.png", lambda a=alt: screen_first_play(a, playing=False)),
        ("02-sequence.png", lambda a=alt: screen_first_play(a, playing=True)),
        ("03-sound-909.png", lambda a=alt: screen_sound(a, "909")),
        ("03-sound-808.png", lambda a=alt: screen_sound(a, "808")),
        ("03-sound-chicago.png", lambda a=alt: screen_sound(a, "chicago")),
        ("03-sound-miami.png", lambda a=alt: screen_sound(a, "miami")),
        ("03-sound-juno.png", lambda a=alt: screen_sound(a, "juno")),
        ("03-sound-juno-b2.png", lambda a=alt: screen_juno_bank2(a)),
        ("03-sound-drone.png", lambda a=alt: screen_sound(a, "drone")),
        ("04-step.png", lambda a=alt: screen_step(a, playing=False)),
        ("05-dirty.png", lambda a=alt: screen_dirty(a, timeout_lost=False, playing=True)),
        ("06-saved.png", lambda a=alt: screen_saved(a)),
        ("07-queued.png", lambda a=alt: screen_queued(a)),
        ("seq-juno-cmin7.png", lambda a=alt: screen_juno_seq(a, "cmin7", held=True)),
        ("seq-juno-inv.png", lambda a=alt: screen_juno_seq(a, "inv", held=True)),
        ("seq-juno-add9.png", lambda a=alt: screen_juno_seq(a, "add9", held=True)),
        ("seq-juno-maj.png", lambda a=alt: screen_juno_seq(a, "maj", held=True)),
        ("seq-juno-dyad.png", lambda a=alt: screen_juno_seq(a, "dyad", held=True)),
        ("seq-juno-cluster.png", lambda a=alt: screen_juno_seq(a, "cluster", held=True)),
        ("seq-juno-amb.png", lambda a=alt: screen_juno_seq(a, "amb", held=True)),
        ("preset-browse.png", lambda a=alt: screen_preset(a, "browse", selected=4)),
        ("preset-audition.png", lambda a=alt: screen_preset(a, "audition", selected=4)),
        ("preset-load.png", lambda a=alt: screen_preset(a, "load", selected=4)),
        ("preset-load-playing.png", lambda a=alt: screen_preset(a, "load-playing", selected=4, playing=True)),
        ("preset-save.png", lambda a=alt: screen_preset(a, "save", selected=1, playing=True, dirty=True)),
        ("preset-recover.png", lambda a=alt: screen_preset(a, "recover", selected=1)),
    ]
    if alt == "S":
        rows.insert(1, ("01-first-play-juno.png", screen_juno_first_s))
    return rows


def render_all():
    written = []
    print("before...")
    for name, fn in BEFORE_SCREENS:
        p = save_cv(fn(), "before", name)
        written.append(p)
        print(" ", p)
    for alt in ("R", "S", "L"):
        print(alt, "...")
        for name, fn in after_screens(alt):
            p = save_cv(fn(), alt, name)
            written.append(p)
            print(" ", p)
    return written


def sheet_for(alt, paths, title):
    """Overview sheet. Device frames are 1x felucca; captions are host PIL."""
    labels = [p.name.replace(".png", "") for p in paths]
    n = len(paths)
    cols = 5
    rows = (n + cols - 1) // cols
    pad, cap, gap = 16, 18, 12
    cell = 240
    W = pad * 2 + cols * cell + (cols - 1) * gap
    H = 40 + rows * (cell + cap + gap) + pad
    im = Image.new("RGB", (W, H), (8, 12, 14))
    dr = ImageDraw.Draw(im)
    try:
        font = ImageFont.truetype(
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14)
        font_s = ImageFont.truetype(
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 10)
    except Exception:
        font = ImageFont.load_default()
        font_s = font
    dr.text((pad, 12), title, fill=(255, 186, 66), font=font)
    for i, p in enumerate(paths):
        r, c = divmod(i, cols)
        x = pad + c * (cell + gap)
        y = 40 + r * (cell + cap + gap)
        fr = Image.open(p).convert("RGB")
        im.paste(fr, (x, y))
        dr.text((x, y + cell + 2), labels[i], fill=(156, 170, 168), font=font_s)
    out = OUT / f"sheet-{alt}.png"
    im.save(out)
    # a 2x sheet is huge; keep 1x and a modest 2x if width allows
    return out


def build_sheets():
    sheets = []
    before = [FRAMES / "before" / "1x" / n for n, _ in BEFORE_SCREENS]
    sheets.append(sheet_for("before", before, "BEFORE  shipped language  felucca + shipped glyphs"))
    for alt, title in (
        ("R", "R  Instrument first  — recommended lean"),
        ("S", "S  Score first  — pitched engines open on the roll"),
        ("L", "L  Live rail  — state never times out"),
    ):
        paths = [FRAMES / alt / "1x" / n for n, _ in after_screens(alt)]
        sheets.append(sheet_for(alt, title=title, paths=paths))
    return sheets


def sha256(p: Path) -> str:
    h = hashlib.sha256()
    h.update(p.read_bytes())
    return h.hexdigest()


if __name__ == "__main__":
    written = render_all()
    sheets = build_sheets()
    print("sheets:")
    for s in sheets:
        print(" ", s, sha256(s)[:16])
    print("done", len(written), "frames")
