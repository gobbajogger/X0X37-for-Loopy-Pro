#!/usr/bin/env python3
"""SOURCE RECONSTRUCTION of draw_303 at 94197fc8 for held-chord patterns.

Reconstructs what the draw code produces from source. Never claimed as
observed output. No firmware build, no host-UI run.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, "/workspace/smk-ux")
import render_mockups as R
from PIL import Image, ImageDraw

OUT = Path("/workspace/smk-ux/seq-chords")
FRAMES_1X = OUT / "frames" / "1x"
FRAMES_2X = OUT / "frames" / "2x"

# Slot colour: Juno on 303B (PART_303B) is cyan, ui.c PART_COLOURS[0][PART_303B]
# = RGB(70,205,255). part_col() follows the slot, not the engine.
JUNO_SLOT = R.RGB(70, 205, 255)
DRONE_SLOT = R.RGB(70, 205, 255)

G_REST, G_NOTE, G_TIE = 0, 1, 2
BS_ACCENT, BS_SLIDE, BS_END_SHIFT = 0x04, 0x08, 4
GX = 27
ROLL_TOP, ROLL_BOT = 4, 92
MAIN_Y = 20
NOTE_N = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


def put_note(n: int) -> str:
    return f"{NOTE_N[(n % 12 + 12) % 12]}{n // 12 - 1}"


def col_x(c: int) -> int:
    return GX + c * 13 + (c >> 2)


def note_start_of(steps, s, length):
    k = 0
    while k < length and (steps[s]["flags"] & 3) == G_TIE:
        s = s - 1 if s else length - 1
        k += 1
    return s if (steps[s]["flags"] & 3) == G_NOTE else -1


def held_note_of(steps, s, length):
    start = note_start_of(steps, s, length)
    return steps[start if start >= 0 else s]["note"]


def note_y(n, lo, hi):
    return ROLL_BOT - (n - lo) * (ROLL_BOT - ROLL_TOP) // (hi - lo)


def window(steps, length, gen_note=48):
    lo, hi = 127, 0
    for c in range(min(length, 64)):
        start = note_start_of(steps, c, length)
        if start >= 0:
            pitch = steps[start]["note"]
            lo = min(lo, pitch)
            hi = max(hi, pitch)
    if lo > hi:
        lo = hi = gen_note
    lo -= 2
    hi += 2
    if hi - lo < 14:
        hi = lo + 14
    return lo, hi


def night_mark(draw, x, y, w, h, ink, raised=True):
    R.night_mark(draw, x, y, w, h, ink, raised=raised)


def canvas():
    img = Image.new("RGB", (240, 240), R.C_BLACK)
    draw = ImageDraw.Draw(img)
    return img, draw


def header(img, draw, part="JUNO B", mode="SEQ", tempo=125, playing=False,
           pattern=1, rec=False, msg=None, col=JUNO_SLOT):
    """SEQ header (ui.c 5418–5498) or full-header toast (5345–5369)."""
    # Temporarily inject slot colour into PART for header_seq's chip/toast.
    saved = dict(R.PART)
    # Header chip is JUNO B on 303B when X0X_BASS_PRESET_PREVIEW=1 (ui.c 5429).
    R.PART["JUNO B"] = col
    R.PART["JUNO"] = col
    R.PART[part] = col
    try:
        chip = "JUNO B" if part in ("JUNO", "JUNO B") else part
        R.header_seq(img, draw, part=chip, mode=mode, tempo=tempo,
                     playing=playing, pattern=pattern, rec=rec, msg=msg)
    finally:
        R.PART.clear()
        R.PART.update(saved)


def page_dots(draw, n=3, cur=0):
    """pg.n > 1 page dots at the right of the tag (ui.c 8356–8369). SEQ 303 has 3 pages."""
    ky = R.KNOB_Y
    xd = 237 - n * 5
    for k in range(n):
        R.box(draw, xd + k * 5, ky + 4, 3, 3, R.C_HI if k == cur else R.C_LINE)


def dir_fwd_glyph(draw, cx, col=JUNO_SLOT):
    """R_BDIR FWD picture (ui.c 7683–7690); cell_value is skipped for R_BDIR (8435)."""
    ky = R.KNOB_Y
    left, right, mid = cx - 18, cx + 18, ky + 38
    R.box(draw, left, mid, 37, 1, col)
    # right chevron
    draw.line([(right - 6, mid - 6), (right, mid)], fill=col)
    draw.line([(right, mid), (right - 6, mid + 6)], fill=col)


def knob_strip(img, draw, held=None, held_note_label=None, gate_label="NOTE",
               tag="P1 SEQUENCE"):
    """Night panels + footer (draw_knobs / draw_footer). Held: NOTE/GATE/ACCENT/SLIDE.

    Unheld SEQ 303 page 0 (ui.c 1478–1481, 954 SEQN): LENGTH, RATE, DIRECTION, TRANSP.
    Tag is P<n> SEQUENCE when group is GR_PAT (8280–8305). Held 303 SEQ tag is
    part_name + " STEP " + n (8287–8288): "JUNO STEP 1" when 303A is not Juno.
    Header chip is still JUNO B (5429, PREVIEW=1).
    """
    ky = R.KNOB_Y
    footer = ["PRESS: ON", "TAP: OFF", "HOLD + KNOBS: EDIT"]
    if held is not None:
        R.night_panel(draw, 3, ky + 0, 234, 11, False)
        for i in range(4):
            R.night_panel(draw, i * 60 + 3, ky + 13, 54, 55, i == 0)
        R.night_panel(draw, 0, ky + 70, 240, 12, False)
        R.text(img, 8, ky + R.vc_top("XS", 0, 11), "XS", tag, R.C_HI)
        page_dots(draw)
        names = ["NOTE", "GATE", "ACCENT", "SLIDE"]
        vals = [held_note_label or "C3", gate_label, "OFF", "OFF"]
        for i, (n, v) in enumerate(zip(names, vals)):
            cx = i * 60 + 30
            if i:
                R.box(draw, i * 60, ky + 14, 1, 50, R.C_LINE)
            R.text(img, cx, ky + R.vc_top("XS", 13, 10), "XS", n, R.C_GRAY, anchor="ct")
            R.text(img, cx, ky + R.vc_top("M", 30, 34), "M", v, R.C_WHITE, anchor="ct")
        R.footer_segs(img, footer, R.C_DIM)
    else:
        R.draw_knob_strip(img, draw, "P1 SEQUENCE",
                          [("LENGTH", "16", "st"), ("RATE", "1/16", ""),
                           ("DIRECTION", "", ""), ("TRANSP", "0", "st")],
                          footer, R.C_DIM)
        dir_fwd_glyph(draw, 150)
        page_dots(draw)


def draw_303(img, draw, steps, length=16, playhead=-1, held_step=-1,
             rec=False, write_pos=-1, col=JUNO_SLOT, root=0, seed="00A3",
             transpose=24):
    """Reconstruct ui.c draw_303 5572–5668 in the main area (y starting at MAIN_Y)."""
    oy = MAIN_Y  # screen offset for main area (bands drawn as one 138-px strip)
    lo, hi = window(steps, length)

    def sy(y):
        return oy + y

    for n in range(lo, hi + 1):
        if (n - root) % 12 == 0:
            y = note_y(n, lo, hi)
            R.box(draw, GX, sy(y + 2), 213, 1, R.C_LINE)
            R.text(img, 1, sy(y - 4), "XS", put_note(n), R.C_DIM)

    for c in range(16):
        s = c  # page 0
        x = col_x(c)
        if s >= length:
            R.box(draw, x + 5, sy(ROLL_BOT + 4), 2, 2, R.C_LINE)
            continue
        st = steps[s]
        g = st["flags"] & 3
        if s == playhead:
            R.box(draw, x, sy(ROLL_TOP - 4), 12, ROLL_BOT - ROLL_TOP + 10, R.dim(col, 2))
        if g != G_REST and note_start_of(steps, s, length) >= 0:
            hn = held_note_of(steps, s, length)
            y = note_y(hn, lo, hi)
            fc = R.C_WHITE if s == held_step else (col if (st["flags"] & BS_ACCENT) else R.dim(col, 9))
            end = st["flags"] >> BS_END_SHIFT
            width = (end * 12 + 14) // 15 if end else 12
            night_mark(draw, x, sy(y), width, 5, fc, raised=True)
            if g == G_TIE:
                if c > 0:
                    prev = col_x(c - 1)
                    w = x - (prev + 6) + (width if width < 6 else 6)
                    R.box(draw, prev + 6, sy(y), w, 5, fc)
                else:
                    R.box(draw, x - 4, sy(y + 1), 4, 3, fc)
                    R.box(draw, x - 7, sy(y + 2), 3, 1, fc)
            nxt = s + 1 if s + 1 < length else 0
            if c == 15 and (steps[nxt]["flags"] & 3) == G_TIE:
                R.box(draw, x + 12, sy(y + 1), 2, 3, fc)
                R.box(draw, x + 14, sy(y + 2), 2, 1, fc)
            if (st["flags"] & BS_SLIDE) and c < 15:
                nx = s + 1 if s + 1 < length else 0
                ny = note_y(steps[nx]["note"], lo, hi)
                draw.line([(x + 11, sy(y + 2)), (x + 14, sy(ny + 2))], fill=R.C_WHITE)
        # AC / SL
        acc = g and (st["flags"] & BS_ACCENT)
        sld = g and (st["flags"] & BS_SLIDE)
        night_mark(draw, x + 1, sy(101), 10, 4, R.C_WHITE if acc else R.C_LINE, raised=bool(acc))
        night_mark(draw, x + 1, sy(113), 10, 4, col if sld else R.C_LINE, raised=bool(sld))

    R.text(img, 3, sy(R.vc_top("XS", 99, 8)), "XS", "AC", R.C_GRAY)
    R.text(img, 3, sy(R.vc_top("XS", 111, 8)), "XS", "SL", R.C_GRAY)

    if held_step >= 0 or rec:
        s = held_step if held_step >= 0 else write_pos
        st = steps[s]
        g = st["flags"] & 3
        hn = held_note_of(steps, s, length) + transpose - 24
        t = ("STEP " if held_step >= 0 else "WRITE ") + str(s + 1) + "  " + put_note(hn)
        if g == G_TIE:
            t += " TIE"
        if st["flags"] & BS_ACCENT:
            t += " ACC"
        if st["flags"] & BS_SLIDE:
            t += " SLIDE"
        vel = st["velocity"] or 100
        t += f" V{vel}"
        R.text(img, 4, sy(R.vc_top("B", 122, 16)), "B", t, R.C_WHITE)
        return t
    else:
        scale = "C MAJ"
        # segs: scale left, SEED right — approximate source segs()
        R.text(img, 4, sy(R.vc_top("S", 122, 16)), "S", scale, R.C_GRAY)
        R.text(img, 236, sy(R.vc_top("S", 122, 16)), "S", f"SEED {seed}", R.C_GRAY, anchor="rt")
        return None


def empty_steps(n=16):
    return [{"note": 48, "flags": 0, "velocity": 0, "chord": []} for _ in range(n)]


def set_onset(steps, s, notes, tie_len=0, accent=False, end=0, vel=100):
    """notes[0] is root in bstep; notes[1:] are stored in lanes (not drawn)."""
    root = notes[0]
    flags = G_NOTE | (BS_ACCENT if accent else 0) | (end << BS_END_SHIFT)
    steps[s] = {"note": root, "flags": flags, "velocity": vel, "chord": list(notes)}
    for k in range(1, tie_len + 1):
        t = s + k
        if t < len(steps):
            steps[t] = {"note": root, "flags": G_TIE | 0xF0, "velocity": vel, "chord": []}


def save_frame(img, name):
    FRAMES_1X.mkdir(parents=True, exist_ok=True)
    FRAMES_2X.mkdir(parents=True, exist_ok=True)
    p1 = FRAMES_1X / f"{name}.png"
    img.save(p1)
    img.resize((480, 480), Image.Resampling.NEAREST).save(FRAMES_2X / f"{name}.png")
    return p1


def frame_p1():
    img, draw = canvas()
    steps = empty_steps()
    set_onset(steps, 0, [48, 52, 55], tie_len=3)
    header(img, draw, playing=True, pattern=1)
    draw_303(img, draw, steps, playhead=2)
    knob_strip(img, draw)
    return save_frame(img, "SR-01-sustained-triad")


def frame_p2():
    img, draw = canvas()
    steps = empty_steps()
    set_onset(steps, 0, [48, 52, 55], tie_len=1)
    set_onset(steps, 2, [52, 55, 59], tie_len=1)
    header(img, draw, playing=True, pattern=1)
    draw_303(img, draw, steps, playhead=2)
    knob_strip(img, draw)
    return save_frame(img, "SR-02-overlapping-chords")


def frame_p3():
    img, draw = canvas()
    steps = empty_steps()
    set_onset(steps, 0, [48, 52, 55], tie_len=0)
    set_onset(steps, 7, [57], tie_len=0)
    header(img, draw, playing=False, pattern=1)
    label = draw_303(img, draw, steps, held_step=0)
    knob_strip(img, draw, held=0, held_note_label=put_note(48), gate_label="NOTE",
               tag="JUNO STEP 1")
    return save_frame(img, "SR-03-chord-and-single"), label


def frame_p4():
    img, draw = canvas()
    steps = empty_steps()
    set_onset(steps, 0, [48, 52, 55, 59, 62, 65], tie_len=0)
    header(img, draw, playing=False, pattern=1)
    draw_303(img, draw, steps)
    knob_strip(img, draw)
    lo, hi = window(steps, 16)
    return save_frame(img, "SR-04-voice-limit"), lo, hi


def frame_p5():
    img, draw = canvas()
    steps = empty_steps()
    set_onset(steps, 0, [48, 52, 55, 59, 62, 65], tie_len=0)
    header(img, draw, playing=False, pattern=1, msg="CHORD NOT SAVED", col=JUNO_SLOT)
    draw_303(img, draw, steps)
    knob_strip(img, draw)
    return save_frame(img, "SR-05-beyond-limit")


def make_sheet(paths_captions, out_name, title, subtitle):
    """Annotated sheet; captions sit outside the 240 panel."""
    n = len(paths_captions)
    pad = 24
    gap = 18
    cap_h = 86
    title_h = 86
    cell_w, cell_h = 240, 240
    cols = n
    W = pad * 2 + cols * cell_w + (cols - 1) * gap
    H = title_h + pad + cell_h + cap_h + pad
    img = Image.new("RGB", (W, H), (12, 14, 16))
    d = ImageDraw.Draw(img)
    f_title = R._sheet_font(22, "Bold")
    f_sub = R._sheet_font(13, "Medium")
    f_cap = R._sheet_font(12, "Medium")
    f_tag = R._sheet_font(11, "Bold")
    d.text((pad, 18), title, font=f_title, fill=(230, 226, 214))
    d.text((pad, 48), subtitle, font=f_sub, fill=(196, 155, 69))
    for i, (path, cap) in enumerate(paths_captions):
        x = pad + i * (cell_w + gap)
        y = title_h
        fr = Image.open(path).convert("RGB")
        img.paste(fr, (x, y))
        d.rectangle([x - 1, y - 1, x + 240, y + 240], outline=(40, 56, 61))
        d.text((x, y + 244), cap, font=f_cap, fill=(200, 204, 198), spacing=3)
    d.text((pad, H - 22),
           "SOURCE RECONSTRUCTION from ui.c draw_303 at 94197fc8. Not observed output.",
           font=f_tag, fill=(143, 191, 121))
    img.save(OUT / f"{out_name}.png")
    img.resize((img.width * 2, img.height * 2), Image.Resampling.NEAREST).save(
        OUT / f"{out_name}-2x.png")


def main():
    p1 = frame_p1()
    p2 = frame_p2()
    p3, lab = frame_p3()
    p4, lo, hi = frame_p4()
    p5 = frame_p5()
    print("P3 label:", lab, "P4 window", lo, hi)
    make_sheet(
        [
            (p1, "P1  Sustained C–E–G over four steps.\n"
                 "Only C3 is drawn; ties at the root’s y.\n"
                 "E3 and G3 hidden.  ui.c 5614–5629."),
            (p2, "P2  C triad then E-minor (shared E,G).\n"
                 "Reads as a two-note melody.\n"
                 "Overlap invisible.  ui.c 5615, 5566–5570."),
            (p3, "P3  Triad on 1, single A3 on 8, step 1 held.\n"
                 f"Label: “{lab}” — extras omitted.\n"
                 "NOTE knob is C3 only.  ui.c 5651, 8381."),
            (p4, "P4  Six Juno notes (store = 6 voices).\n"
                 f"Window lo={lo} hi={hi} from C3 only;\n"
                 "F4=65 would clip.  ui.c 5581–5619."),
            (p5, "P5  Seventh key: CHORD NOT SAVED toast\n"
                 "(ui.c 3355, 4450, 5345–5369).\n"
                 "Roll still shows one C3 bar."),
        ],
        "sheet-diagnosis",
        "Held-chord SEQ display  —  SOURCE RECONSTRUCTION",
        "Firmware 94197fc8  ·  draw_303 never calls held_chord_get  ·  not observed output",
    )
    print("wrote frames and sheet")


if __name__ == "__main__":
    main()
