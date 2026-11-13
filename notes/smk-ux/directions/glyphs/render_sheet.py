#!/usr/bin/env python3
"""sheet-glyphs.png: every shipped glyph (decoded) and each Juno/Drone addition,
at 1× and 4×. Writes only under directions/glyphs/."""
from __future__ import annotations
import sys
from pathlib import Path
sys.path.insert(0, "/workspace/smk-ux")
sys.path.insert(0, "/workspace/smk-ux/directions/glyphs")

from PIL import Image, ImageDraw, ImageFont
import shipped_glyphs as G
from render_mockups import FONT_DIR

OUT = Path("/workspace/smk-ux/directions/glyphs/sheet-glyphs.png")
BG = (16, 24, 24)
INK = (222, 231, 222)
MUTE = (128, 146, 148)
ACC = (255, 186, 66)
FACE = G.SOUND_FACE
FACE8 = G.expand565(FACE)
INK16 = G.SOUND_WHITE
SEL = G.SOUND_SELECT
SEL8 = G.expand565(SEL)
SEL_INK = G.SOUND_INK

def font(sz, w="SemiBold"):
    name = {"SemiBold": "BarlowSemiCondensed-SemiBold.ttf",
            "Medium": "BarlowSemiCondensed-Medium.ttf",
            "Bold": "BarlowSemiCondensed-Bold.ttf"}[w]
    return ImageFont.truetype(str(FONT_DIR / name), sz)

FT = font(13)
FS = font(11)
FH = font(18, "Bold")

# Collect (section, name, source, pil_1x) 
items = []

def add(section, name, source, im):
    items.append((section, name, source, im.convert("RGB")))

def cv_img(w, h, face, drawer):
    cv = G.Canvas(w, h, face)
    drawer(cv)
    return cv.to_pil()

# --- play_icon 9×11 ---
add("Header / transport", "play_icon (filled, playing)",
    "ui.c:5153–5162 play_icon",
    cv_img(13, 15, G.C_BLACK, lambda c: G.play_icon(c, 2, 2, G.RGB(255, 184, 70))))
add("Header / transport", "play_icon (filled, stopped)",
    "ui.c:5153–5162  ink C_DIM",
    cv_img(13, 15, G.C_BLACK, lambda c: G.play_icon(c, 2, 2, G.C_DIM)))
add("Header / transport", "play_icon_hollow (B stopped, PROPOSAL)",
    "same 9×11 footprint; not shipped. Reserved outline.",
    cv_img(13, 15, G.C_BLACK, lambda c: G.play_icon_hollow(c, 2, 2, G.C_DIM, G.C_BLACK)))

# --- UI_PICTOGRAM 9×9, all 43 including NONE ---
# names from the enum
PIC_NAMES = [
    "NONE","PATTERN","NEXT","PREV","UP","DOWN","PLAY","STOP","RECORD","HOME",
    "MIX","SONG","GLOBE","SOUND","LIST","SELECT","CHECK","LOCK","KNOB","FADER",
    "KEY","MUTE","SOLO","CYCLE","HOLD","TARGET",
    "FX_GATE","FX_REVERB","FX_HIGHPASS","FX_DELAY","FX_LOWPASS",
    "FX_GATE8","FX_GATE16","FX_TRANCE","FX_STUTTER","FX_SHUFFLE","FX_EMPTY",
]
# UI_G_* values are 0..count-1 in order. Confirm COUNT.
# UI_G_COUNT is last. Enum starts at NONE=0. After TARGET come FX_*. 
# The header had UI_G_COUNT after FX_EMPTY. That's 0..41 + COUNT = 42 values? 
# NONE through FX_EMPTY inclusive: 1+26+11 = 38? Let me count:
# 0 NONE .. 25 TARGET = 26, then 11 FX = 37, COUNT=37? 
# enum: NONE=0, PATTERN=1, ... TARGET=25 (that's 26), FX_GATE=26 ... FX_EMPTY=36, COUNT=37.
# PIC_NAMES has 37 entries. Good. But wait the file listed through FX_EMPTY and COUNT.
# 26 after TARGET comment: FX starts. I listed 26 + 11 = 37. Good.

for i, name in enumerate(PIC_NAMES):
    def make(ii=i):
        return cv_img(11, 11, G.C_BLACK, lambda c, ii=ii: G.ui_pictogram(c, 1, 1, ii, 1, G.C_WHITE))
    add("UI_PICTOGRAM 9×9", f"UI_G_{name}",
        f"ui_pictograms.h:{47+i if i else 46}  table UI_PICTOGRAM, painter ui.c:5184",
        make())

# --- night_mark ---
def nm(w, h, ink, raised):
    return cv_img(w+4, h+4, G.C_BLACK, lambda c: G.night_mark(c, 2, 2, w, h, ink, raised) if hasattr(G, "night_mark") else None)

# check if night_mark exists
if hasattr(G, "night_mark"):
    add("Step / LED marks", "night_mark hit (raised)",
        "ui.c:5253–5260 night_mark",
        cv_img(16, 12, G.C_BLACK, lambda c: G.night_mark(c, 2, 2, 12, 8, G.PART_909, 1)))
    add("Step / LED marks", "night_mark empty (C_LINE)",
        "ui.c:5253–5260",
        cv_img(16, 12, G.C_BLACK, lambda c: G.night_mark(c, 2, 2, 12, 8, G.C_LINE, 0)))

# --- drum_strip 26×19 idle ---
def drum(role, pot, q, **kw):
    def d(c, role=role, pot=pot, q=q, kw=kw):
        G.drum_strip_glyph_idle(c, 0, 0, role, pot, q, INK16, **kw)
    return cv_img(26, 19, FACE, d)

TUNE_TAU = 6.0 + (32.0-6.0)*34/127.0
add("drum_strip_glyph 26×19 (idle)", "DG_BDSWEEP  909 BD TUNE / P.DPTH / PITCH",
    "ui.c:8095–8120  drum_strip_glyph, role from drum_strip_role:7977",
    drum(G.DG_BDSWEEP, 34, 34/127, tau=TUNE_TAU, m=0.0, pm=1.0))
add("drum_strip_glyph 26×19 (idle)", "DG_ATTACK  pot 13 (factory ~10%)",
    "ui.c:8121–8130",
    drum(G.DG_ATTACK, 13, 13/127))
add("drum_strip_glyph 26×19 (idle)", "DG_DECAY  0.50 s",
    "ui.c:8062–8068",
    drum(G.DG_DECAY, 0, 0, ms=500))
add("drum_strip_glyph 26×19 (idle)", "DG_DECAY  1.50 s",
    "ui.c:8062–8068",
    drum(G.DG_DECAY, 0, 0, ms=1500))
add("drum_strip_glyph 26×19 (idle)", "DG_ACCENT  127",
    "ui.c:8184+  drum_strip_role:7976 (NDRUM Accent)",
    drum(G.DG_ACCENT, 127, 1.0))
# other roles at mid
for role, name, src, pot in [
    (G.DG_TUNE, "DG_TUNE  (808 / 909 cymbal, mid)", "ui.c:8073–8094", 64),
    (G.DG_LPF, "DG_LPF  808 BD TONE", "ui.c:8131–8146", 64),
    (G.DG_SHELLS, "DG_SHELLS  808 SD TONE", "ui.c:8147–8157", 64),
    (G.DG_SNAPPY, "DG_SNAPPY", "ui.c:8158–8167", 64),
    (G.DG_SOUND, "DG_SOUND  switch pips", "ui.c:8168–8175", 0),
    (G.DG_LEVEL, "DG_LEVEL", "ui.c:8176–8183", 100),
    (G.DG_NTAIL, "DG_NTAIL  909 SD TONE", "ui.c:8069–8072", 64),
]:
    try:
        add("drum_strip_glyph 26×19 (idle)", name, src, drum(role, pot, pot/127 if pot else 0))
    except Exception as e:
        print("skip", name, e)

# --- orig_strip Chicago / Miami ---
def orig(chi, c, pot, q, **kw):
    def d(cv, chi=chi, c=c, pot=pot, q=q, kw=kw):
        G.orig_strip_glyph_idle(cv, 0, 0, chi=chi, c=c, val=pot, q=q, col=INK16, **kw)
    return cv_img(26, 19, FACE, d)

CHI = [
    (2, "Chicago CUTOFF (c=2)", "ui.c:7857–7865", 70, 70/127, dict(cq=70/127, pq=18/127)),
    (3, "Chicago PEAK / RES (c=3)", "ui.c:7857–7865", 18, 18/127, dict(cq=70/127, pq=18/127)),
    (4, "Chicago ENV (c=4)", "ui.c:7866+", 45, 45/127, dict(cq=70/127, pq=18/127)),
    (5, "Chicago BITE (c=5)", "ui.c:7904 og_bite", 64, 0.5, {}),
    (6, "Chicago GNAW (c=6)", "ui.c:7905 og_gnaw", 64, 0.5, {}),
    (11, "Chicago ENV/GATE toggle (c=11)", "ui.c:7907", 0, 0, {}),
    (15, "Chicago RANGE (c=15)", "ui.c:7919", 64, 0.5, {}),
    (12, "Chicago LEVEL (c=12) — also Juno LEVEL reuse", "ui.c:7950 level pips / ramp", 100, 100/127, {}),
]
MIA = [
    (2, "Miami SUB (c=2) — also Juno SUB reuse", "ui.c:7924–7927", 28, 28/127, {}),
    (3, "Miami PITCH (c=3)", "ui.c:7928–7931", 64, 0.5, {}),
    (8, "Miami XMOD (c=8) og_fm", "ui.c:7932", 64, 0.5, {}),
    (12, "Miami WOBBLE depth (c=12)", "ui.c:7933–7941", 8, 8/127, {}),
    (13, "Miami WOBBLE RATE (c=13) — also Juno LFO / Drone FLUTTER", "ui.c:7933–7941", 15, 15/127, {}),
    (10, "Miami CRUSH (c=10)", "ui.c:7906 og_crush", 64, 0.5, {}),
    (15, "Miami STEREO (c=15)", "ui.c:7942+", 64, 0.5, {}),
]
for c, name, src, pot, q, kw in CHI:
    try:
        add("orig_strip_glyph Chicago 26×19", name, src, orig(True, c, pot, q, **kw))
    except Exception as e:
        print("skip chi", name, e)
for c, name, src, pot, q, kw in MIA:
    try:
        add("orig_strip_glyph Miami 26×19", name, src, orig(False, c, pot, q, **kw))
    except Exception as e:
        print("skip mia", name, e)

# orig_fx
for t, name in [(0,"bypass"),(1,"chorus pair (Juno CHORUS reuse)"),(2,"phaser"),
                (3,"ring"),(4,"compressor"),(5,"decimator"),(6,"EQ peak"),
                (7,"ensemble trio"),(8,"short delay"),(9,"BPM delay"),
                (10,"hall"),(11,"gated reverb")]:
    try:
        add("orig_fx_glyph 26×19", f"type {t}  {name}",
            "ui.c:7750–7812 orig_fx_glyph",
            cv_img(26, 19, FACE, lambda cv, t=t: G.orig_fx_glyph(cv, t, 0, 0, INK16)))
    except Exception as e:
        print("skip fx", t, e)

# --- waves ---
for mix, lab in [(0,"saw (mix 0)"),(42,"Juno factory WAVE 33% (mix 42)"),(64,"mid"),(127,"pulse (mix 127)")]:
    add("draw_wave_glyph shape 1 (continuous saw↔pulse)", lab,
        "ui.c:6959–6998 draw_wave_glyph  (Juno WAVE is this, not a discrete SAW/PUL name)",
        cv_img(29, 19, FACE, lambda cv, m=mix: G.draw_wave_glyph(cv, 14, 0, 1, m, INK16, FACE)))

# --- control_glyph ---
def _knob_crop(drawer, x0, y0, x1, y1):
    face = G.RGB(28, 40, 40)
    cv = G.Canvas(80, 60, face)
    drawer(cv)
    return cv.to_pil().crop((x0, y0, x1, y1))

add("control_glyph (knob strip)", "R_DLEN  length=16 (val=15, max=63) — 2 of 8 boxes",
    "ui.c:7673–7677  4×2 of 7×7 rboxes",
    _knob_crop(lambda cv: G.control_glyph_dlen(cv, 40, 15, 63, G.PART_909), 40-18, 30, 40-18+37, 47))
add("control_glyph (knob strip)", "R_DRATE  val=0 (1/16, gap=12)",
    "ui.c:7678–7682",
    _knob_crop(lambda cv: G.control_glyph_drate(cv, 40, 0, G.PART_909), 40-18, 31, 40+19, 46))

# --- NEW Juno / Drone (additions only) ---
add("Juno / Drone ADDITIONS (not in shipped firmware)", "JUNO identity 9×9",
    "NEW. Same painter as ui_pictogram. Not stamped on A/B/C frames: no 9×9 header gap.",
    cv_img(11, 11, G.C_BLACK, lambda c: G.pictogram_rows(c, 1, 1, G.JUNO_ID, 1, G.SOUND_GREEN)))
add("Juno / Drone ADDITIONS (not in shipped firmware)", "DRONE identity 9×9",
    "NEW. Same painter. Phase-2 placement.",
    cv_img(11, 11, G.C_BLACK, lambda c: G.pictogram_rows(c, 1, 1, G.DRONE_ID, 1, G.SOUND_GREEN)))
add("Juno / Drone ADDITIONS (not in shipped firmware)", "Juno WIDTH (pulse width, q=0.50)",
    "NEW draw fn glyph_juno_width. 26×19. Add-only.",
    cv_img(26, 19, FACE, lambda c: G.glyph_juno_width(c, 0, 0, 0.50, INK16, FACE)))
add("Juno / Drone ADDITIONS (not in shipped firmware)", "Juno HPF (q=0)",
    "NEW draw fn glyph_juno_hpf. 26×19. Add-only.",
    cv_img(26, 19, FACE, lambda c: G.glyph_juno_hpf(c, 0, 0, 0.0, INK16)))
add("Juno / Drone ADDITIONS (not in shipped firmware)", "Juno SUSTAIN (q=0.72)",
    "NEW draw fn glyph_juno_sustain. 26×19. Add-only.",
    cv_img(26, 19, FACE, lambda c: G.glyph_juno_sustain(c, 0, 0, 0.72, INK16)))
add("Juno / Drone ADDITIONS (not in shipped firmware)", "Drone REGISTER (q=0.45)",
    "NEW draw fn glyph_drone_register. 26×19. Add-only.",
    cv_img(26, 19, FACE, lambda c: G.glyph_drone_register(c, 0, 0, 0.45, INK16)))

print(f"collected {len(items)} glyphs")

# ---- layout ----
# Each cell: 4× bitmap, 1× to its left, label under. Columns: 3. 
PAD = 16
COL_W = 360
ROW_H_BASE = 28  # label
# We'll compute row heights from the 4x size.

# Group by section
from collections import OrderedDict
secs = OrderedDict()
for s,n,src,im in items:
    secs.setdefault(s, []).append((n,src,im))

rows = []  # (section_header or None, items in this visual row)
for s, lst in secs.items():
    rows.append(("H", s, None))
    for trip in lst:
        rows.append(("I", trip, None))

# Measure
W = PAD + 2*COL_W + PAD  # 2 columns of (1x + 4x + text)
# Actually 2 cols is tighter. Let's do 2.

# Precompute heights
def cell_h(im):
    return 8 + im.height*4 + 4 + 28 + 16  # 4x, gap, 2 lines of label

# Build a list of flow items with heights
flow = []
for s, lst in secs.items():
    flow.append(("H", s, 36))
    for trip in lst:
        flow.append(("I", trip, cell_h(trip[2])))

# 2-col pack within a section (headers full width)
Htot = PAD + 40  # title
col_y = [0, 0]
# We'll just use a simple sequential 2-col: for each section, header, then items packed 2-wide.

# First pass: compute total height
h = PAD + 48
for kind, payload, hh in flow:
    if kind == "H":
        h += 36
        # pad to even
    else:
        h += hh  # we'll pack 2-wide so this overestimates; compute properly:

h = PAD + 48
for s, lst in secs.items():
    h += 36
    # pairs
    i = 0
    while i < len(lst):
        h1 = cell_h(lst[i][2])
        h2 = cell_h(lst[i+1][2]) if i+1 < len(lst) else 0
        h += max(h1, h2)
        i += 2
h += PAD + 20

sheet = Image.new("RGB", (W, h), BG)
d = ImageDraw.Draw(sheet)
title = "SMK-37 glyphs  ·  decoded from 94197fc8  ·  1× and 4×  ·  shipped tables, not lookalikes"
d.text((PAD, 12), title, font=FH, fill=INK)
d.text((PAD, 34), "New Juno/Drone glyphs are additions only. Shipped bitmaps and draw functions are used as-is.", font=FS, fill=MUTE)

y = PAD + 52
for s, lst in secs.items():
    d.rectangle([PAD, y, W-PAD, y+1], fill=(40,52,52))
    d.text((PAD, y+8), s, font=FT, fill=ACC)
    y += 32
    i = 0
    while i < len(lst):
        pair = lst[i:i+2]
        row_h = max(cell_h(t[2]) for t in pair)
        for ci, (name, src, im) in enumerate(pair):
            x = PAD + ci*COL_W
            # 1x
            sheet.paste(im, (x, y))
            # 4x
            im4 = im.resize((im.width*4, im.height*4), Image.Resampling.NEAREST)
            sheet.paste(im4, (x + im.width + 10, y))
            d.text((x, y + im4.height + 6), name, font=FS, fill=INK)
            d.text((x, y + im4.height + 18), src, font=font(10), fill=MUTE)
        y += row_h
        i += 2

# Crop to actual used height
sheet = sheet.crop((0, 0, W, min(y+PAD, sheet.height)))
sheet.save(OUT, optimize=True)
print("wrote", OUT, sheet.size, OUT.stat().st_size)
