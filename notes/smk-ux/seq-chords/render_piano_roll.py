#!/usr/bin/env python3
"""PROPOSAL frames: B chord/note view (and A/C variants) on the SEQ roll.

Builds on the diagnosis reconstruction (same patterns, same store).
Not observed output. No firmware edit. Does not write into directions/.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, "/workspace/smk-ux")
sys.path.insert(0, "/workspace/smk-ux/seq-chords")

from PIL import Image, ImageDraw

import render_mockups as R
import render_directions as D
import render_diagnosis as S

OUT = Path("/workspace/smk-ux/seq-chords")
FRAMES_1X = OUT / "frames" / "1x"
FRAMES_2X = OUT / "frames" / "2x"

JUNO = S.JUNO_SLOT  # 303B cyan


def ensure_part():
    R.PART["JUNO B"] = JUNO
    R.PART["JUNO"] = JUNO
    D.PART["JUNO B"] = JUNO
    D.PART["JUNO"] = JUNO


def chord_of(steps, s, length):
    start = S.note_start_of(steps, s, length)
    if start < 0:
        return []
    ch = steps[start].get("chord") or [steps[start]["note"]]
    return list(ch)


def window_proposed(steps, length, gen_note=48):
    lo, hi = 127, 0
    for c in range(min(length, 64)):
        pitches = chord_of(steps, c, length)
        for pitch in pitches:
            lo = min(lo, pitch)
            hi = max(hi, pitch)
    if lo > hi:
        lo = hi = gen_note
    lo -= 2
    hi += 2
    if hi - lo < 14:
        hi = lo + 14
    return lo, hi


def names_of(pitches, transpose=24):
    return " ".join(S.put_note(n + transpose - 24) for n in pitches)


def compact_note(pitches):
    if not pitches:
        return "—"
    root = S.put_note(pitches[0])
    extra = len(pitches) - 1
    return root if extra == 0 else f"{root}+{extra}"


def fit_line(img, x, y, text, ink, max_w=232):
    for role in ("B", "S", "XS"):
        if R.tw(role, text) <= max_w:
            R.text(img, x, y, role, text, ink)
            return role
    R.text(img, x, y, "XS", text, ink)
    return "XS"


def draw_roll(img, draw, steps, style="B", length=16, playhead=-1, held_step=-1,
              rec=False, write_pos=-1, col=JUNO, root=0, seed="00A3",
              transpose=24):
    """Proposed draw_303: same helper the bounded fix is inferred to add,
    then A/B/C mark language only."""
    oy = S.MAIN_Y
    lo, hi = window_proposed(steps, length)
    label_x = 5 if style == "C" else 1  # C spine is 3 px on the left

    def sy(y):
        return oy + y

    for n in range(lo, hi + 1):
        if (n - root) % 12 == 0:
            y = S.note_y(n, lo, hi)
            R.box(draw, S.GX, sy(y + 2), 213, 1, R.C_LINE)
            R.text(img, label_x, sy(y - 4), "XS", S.put_note(n), R.C_DIM)

    for c in range(16):
        s = c
        x = S.col_x(c)
        if s >= length:
            R.box(draw, x + 5, sy(S.ROLL_BOT + 4), 2, 2, R.C_LINE)
            continue
        st = steps[s]
        g = st["flags"] & 3
        if s == playhead:
            R.box(draw, x, sy(S.ROLL_TOP - 4), 12, S.ROLL_BOT - S.ROLL_TOP + 10, R.dim(col, 2))
        pitches = chord_of(steps, s, length)
        if g != S.G_REST and pitches:
            end = st["flags"] >> S.BS_END_SHIFT
            width = (end * 12 + 14) // 15 if end else 12
            held = s == held_step
            fc = (R.C_WHITE if held else
                  (col if (st["flags"] & S.BS_ACCENT) else R.dim(col, 9)))
            for pitch in pitches:
                y = S.note_y(pitch, lo, hi)
                S.night_mark(draw, x, sy(y), width, 5, fc, raised=True)
                if style == "B" and held:
                    draw.rectangle([x, sy(y), x + width - 1, sy(y) + 4], outline=R.C_AMB)
                if g == S.G_TIE:
                    if c > 0:
                        prev = S.col_x(c - 1)
                        w = x - (prev + 6) + (width if width < 6 else 6)
                        R.box(draw, prev + 6, sy(y), w, 5, fc)
                    else:
                        R.box(draw, x - 4, sy(y + 1), 4, 3, fc)
                        R.box(draw, x - 7, sy(y + 2), 3, 1, fc)
                nxt = s + 1 if s + 1 < length else 0
                if c == 15 and (steps[nxt]["flags"] & 3) == S.G_TIE:
                    R.box(draw, x + 12, sy(y + 1), 2, 3, fc)
                    R.box(draw, x + 14, sy(y + 2), 2, 1, fc)
            if (st["flags"] & S.BS_SLIDE) and c < 15:
                nx = s + 1 if s + 1 < length else 0
                ny = S.note_y(steps[nx]["note"], lo, hi)
                hn = pitches[0]
                y = S.note_y(hn, lo, hi)
                draw.line([(x + 11, sy(y + 2)), (x + 14, sy(ny + 2))], fill=R.C_WHITE)
        acc = g and (st["flags"] & S.BS_ACCENT)
        sld = g and (st["flags"] & S.BS_SLIDE)
        S.night_mark(draw, x + 1, sy(101), 10, 4, R.C_WHITE if acc else R.C_LINE, raised=bool(acc))
        S.night_mark(draw, x + 1, sy(113), 10, 4, col if sld else R.C_LINE, raised=bool(sld))

    R.text(img, 3 if style != "C" else 5, sy(R.vc_top("XS", 99, 8)), "XS", "AC", R.C_GRAY)
    R.text(img, 3 if style != "C" else 5, sy(R.vc_top("XS", 111, 8)), "XS", "SL", R.C_GRAY)

    line = None
    line_y = sy(R.vc_top("B", 122, 16))
    if held_step >= 0 or rec:
        s = held_step if held_step >= 0 else write_pos
        st = steps[s]
        g = st["flags"] & 3
        pitches = chord_of(steps, s, length)
        t = ("STEP " if held_step >= 0 else "WRITE ") + str(s + 1) + "  " + names_of(pitches, transpose)
        if g == S.G_TIE:
            t += " TIE"
        if st["flags"] & S.BS_ACCENT:
            t += " ACC"
        if st["flags"] & S.BS_SLIDE:
            t += " SLIDE"
        vel = st["velocity"] or 100
        t += f" V{vel}"
        line = t
        if style == "C" and held_step >= 0:
            R.box(draw, 0, sy(122), 240, 16, R.C_AMB)
            R.text(img, 4, sy(R.vc_top("B", 122, 16)), "B", t, R.C_BLACK)
        elif style == "B" and held_step >= 0:
            # named line in amber — same waiting family, still the 5645 site
            fit_line(img, 4, line_y, t, R.C_AMB)
        else:
            fit_line(img, 4, line_y, t, R.C_WHITE)
    else:
        R.text(img, 4 if style != "C" else 6, sy(R.vc_top("S", 122, 16)), "S", "C MAJ", R.C_GRAY)
        R.text(img, 236, sy(R.vc_top("S", 122, 16)), "S", f"SEED {seed}", R.C_GRAY, anchor="rt")
    return line, window_proposed(steps, length)


def header(img, draw, style, playing=False, held_step=None, msg=None):
    """A/B/C headers. CHORD NOT SAVED: A keeps the source full-header toast
    (bounded fix does not change say()); B/C keep the clock."""
    ensure_part()
    if style == "A" and msg == "CHORD NOT SAVED":
        # source toast 5345–5369, as the bounded fix leaves it
        S.header(img, draw, part="JUNO B", playing=playing, pattern=1, msg=msg, col=JUNO)
        return
    if style == "A":
        S.header(img, draw, part="JUNO B", playing=playing, pattern=1, col=JUNO)
        return
    if msg == "CHORD NOT SAVED" and style == "B":
        D.fill_header_panel(draw)
        D.box(draw, 0, 19, 240, 1, R.C_AMB)  # waiting rail: a refusal is a named event
        D.box(draw, 0, 1, 3, 17, JUNO)
        D.outline_pill(img, draw, 6, 3, 13, msg, R.C_AMB, "XS")
        D.pattern_badge(img, draw, 132, 1, R.C_WHITE)
        R.text(img, 212, R.vc_top("S", 1, 17), "S", "125", R.C_HI, anchor="rt")
        D.play_mark(draw, 217, 4, playing, "B")
        return
    if msg == "CHORD NOT SAVED" and style == "C":
        D.fill_header_panel(draw)
        D.box(draw, 0, 18, 240, 2, JUNO)  # hairline; not a held step
        D.box(draw, 0, 1, 3, 17, JUNO)
        D.ticket(img, draw, 6, 1, 17, msg, R.C_AMB, R.C_BLACK, "XS")
        D.pattern_badge(img, draw, 132, 1, R.C_WHITE)
        R.text(img, 212, R.vc_top("S", 1, 17), "S", "125", R.C_HI, anchor="rt")
        D.play_mark(draw, 217, 4, playing, "C")
        return
    step = (held_step + 1) if held_step is not None and held_step >= 0 else None
    D.header_proposed(img, draw, style, part="JUNO B", mode="SEQ", tempo=125,
                      playing=playing, pattern=1, step=step)


def finish_c(draw, held=False):
    D.spine(draw, R.C_AMB if held else JUNO, w=3, y0=20)
    # hairline already in header_proposed for C


def knob(img, draw, held=None, pitches=None):
    tag = "JUNO STEP 1" if held is not None else "P1 SEQUENCE"
    note = compact_note(pitches or [48])
    S.knob_strip(img, draw, held=held, held_note_label=note, gate_label="NOTE", tag=tag)


def save_frame(img, name):
    FRAMES_1X.mkdir(parents=True, exist_ok=True)
    FRAMES_2X.mkdir(parents=True, exist_ok=True)
    p1 = FRAMES_1X / f"{name}.png"
    img.save(p1)
    img.resize((480, 480), Image.Resampling.NEAREST).save(FRAMES_2X / f"{name}.png")
    return p1


def patterns():
    p1 = S.empty_steps()
    S.set_onset(p1, 0, [48, 52, 55], tie_len=3)
    p2 = S.empty_steps()
    S.set_onset(p2, 0, [48, 52, 55], tie_len=1)
    S.set_onset(p2, 2, [52, 55, 59], tie_len=1)
    p3 = S.empty_steps()
    S.set_onset(p3, 0, [48, 52, 55], tie_len=0)
    S.set_onset(p3, 7, [57], tie_len=0)
    p4 = S.empty_steps()
    S.set_onset(p4, 0, [48, 52, 55, 59, 62, 65], tie_len=0)
    return p1, p2, p3, p4


def frame(style, kind):
    p1, p2, p3, p4 = patterns()
    img, draw = S.canvas()
    if kind == "01":
        header(img, draw, style, playing=True)
        line, win = draw_roll(img, draw, p1, style=style, playhead=2)
        knob(img, draw)
        if style == "C":
            finish_c(draw, held=False)
        return save_frame(img, f"{style}-01-sustained-triad"), line, win
    if kind == "02":
        header(img, draw, style, playing=True)
        line, win = draw_roll(img, draw, p2, style=style, playhead=2)
        knob(img, draw)
        if style == "C":
            finish_c(draw, held=False)
        return save_frame(img, f"{style}-02-overlapping-chords"), line, win
    if kind == "03":
        header(img, draw, style, playing=False, held_step=0)
        line, win = draw_roll(img, draw, p3, style=style, held_step=0)
        knob(img, draw, held=0, pitches=[48, 52, 55])
        if style == "C":
            finish_c(draw, held=True)
        return save_frame(img, f"{style}-03-chord-and-single"), line, win
    if kind == "04":
        header(img, draw, style, playing=False)
        line, win = draw_roll(img, draw, p4, style=style)
        knob(img, draw)
        if style == "C":
            finish_c(draw, held=False)
        return save_frame(img, f"{style}-04-voice-limit"), line, win
    if kind == "05":
        header(img, draw, style, playing=False, msg="CHORD NOT SAVED")
        line, win = draw_roll(img, draw, p4, style=style)
        knob(img, draw)
        if style == "C":
            finish_c(draw, held=False)
        return save_frame(img, f"{style}-05-beyond-limit"), line, win
    raise ValueError(kind)


def sheet(paths_captions, out_name, title, subtitle):
    n = len(paths_captions)
    pad, gap, cap_h, title_h = 24, 18, 96, 86
    cell_w = 240
    W = pad * 2 + n * cell_w + (n - 1) * gap
    H = title_h + pad + 240 + cap_h + pad
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
           "PROPOSAL. Direction B chord/note view, A and C variants. Not observed output. 94197fc8.",
           font=f_tag, fill=(143, 191, 121))
    img.save(OUT / f"{out_name}.png")
    img.resize((img.width * 2, img.height * 2), Image.Resampling.NEAREST).save(
        OUT / f"{out_name}-2x.png")


def main():
    ensure_part()
    results = {}
    for style, kinds in (("B", ["01", "02", "03", "04", "05"]),
                         ("A", ["01", "03"]),
                         ("C", ["01", "03"])):
        for k in kinds:
            path, line, win = frame(style, k)
            results[f"{style}-{k}"] = (path, line, win)
            print(f"{style}-{k}", line, win)
    sheet(
        [
            (results["B-01"][0],
             "P1  C–E–G fills and ties across four steps.\n"
             "Playhead on step 3. Filled triangle.\n"
             "Bounded marks; B transport.  Faults 1, 4."),
            (results["B-02"][0],
             "P2  C triad, then E-minor.\n"
             "Shared E3 and G3 visible at both onsets\n"
             "(new G_NOTE, not a TIE).  Fault 5."),
            (results["B-03"][0],
             "P3  Held triad + single A3.\n"
             f"Line: “{results['B-03'][1]}”.\n"
             "Pill STEP 1, amber rail, NOTE C3+2.  Faults 1, 3."),
            (results["B-04"][0],
             f"P4  Six Juno notes. Window lo,hi={results['B-04'][2]}.\n"
             "F4 is inside the window. Hollow triangle.\n"
             "Fault 2."),
            (results["B-05"][0],
             "P5  Seventh key refused.\n"
             "CHORD NOT SAVED outline pill; 3 px tick;\n"
             "clock and pattern stay. Six fills remain."),
        ],
        "sheet-B",
        "B chord/note view  —  PROPOSAL",
        "Direction B (Live signals)  ·  same held_chord_get path as the bounded fix  ·  not observed output",
    )
    sheet(
        [
            (results["A-01"][0],
             "A P1  Same three fills and ties.\n"
             "Source marks and play icon.\n"
             "No rail, no pill, no hollow triangle."),
            (results["B-01"][0],
             "B P1  Same roll data as A.\n"
             "Filled triangle (playing).\n"
             "Quiet rail at rest (not waiting)."),
            (results["C-01"][0],
             "C P1  Same roll.\n"
             "Cyan spine and hairline (SEQ room).\n"
             "Play icon as shipped."),
            (results["A-03"][0],
             "A P3  Chord named on the held line.\n"
             "White fills, source header JUNO B SEQ.\n"
             "No STEP pill."),
            (results["B-03"][0],
             "B P3  White fills + amber outline.\n"
             "Pill STEP 1, amber rail, amber line.\n"
             "NOTE C3+2. Hollow triangle."),
            (results["C-03"][0],
             "C P3  Amber spine and amber plate.\n"
             "Black names on the 5645 line.\n"
             "Shipped play icon. Same data path."),
        ],
        "sheet-variants",
        "A / B / C  —  same roll, different language",
        "P1 sustained triad and P3 held chord  ·  overhaul builds on the bounded fix  ·  not a second reader",
    )
    print("wrote proposal frames and sheets")


if __name__ == "__main__":
    main()
