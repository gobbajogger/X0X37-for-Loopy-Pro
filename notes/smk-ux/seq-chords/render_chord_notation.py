#!/usr/bin/env python3
"""PROPOSAL frames: chord symbols on the Direction B SEQ chord/note view.

Host-side Python/PIL only. Same glyph language as the earlier B frames.
Not observed output. Not a firmware patch. Pitches and playback never change.
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
import render_piano_roll as P
import chord_name as CN

OUT = Path("/workspace/smk-ux/seq-chords")
FRAMES_1X = OUT / "frames" / "1x"
FRAMES_3X = OUT / "frames" / "3x"

JUNO = S.JUNO_SLOT
DRONE = JUNO  # part colour is the slot (303B), not the engine


def ensure_part(name="JUNO B", col=JUNO):
    R.PART[name] = col
    R.PART[name.split()[0]] = col
    D.PART[name] = col
    D.PART[name.split()[0]] = col


def fill_held_line(draw, oy=S.MAIN_Y):
    R.box(draw, 0, oy + 122, 240, 16, R.C_BLACK)


def paint_held_line(img, draw, step_n, sounding, velocity=100, *,
                    tier2=True, dropped=None, ink=None):
    """Redraw the 5645 line: STEP n  <symbol>  <notes>  V###."""
    fill_held_line(draw)
    ink = ink or R.C_AMB
    line, role = CN.held_line(step_n, sounding, velocity, tier2=tier2)
    y = S.MAIN_Y + R.vc_top(role, 122, 16)
    R.text(img, 4, y, role, line, ink)
    if dropped:
        # Flag stored-but-silent Drone notes in the existing dim colour,
        # after the sounding line, if they still fit.
        extra = "  " + " ".join(CN.put_note(n) for n in dropped)
        w = CN.device_tw(role, line)
        if w + CN.device_tw(role, extra) <= CN.HELD_LINE_PX:
            # PIL advance can differ by a few px; place by PIL for the mockup.
            x = 4 + R.tw(role, line)
            R.text(img, x, y, role, extra, R.C_DIM)
    return line, role


def paint_knobs(img, draw, *, held_step=0, sounding, tier2=True,
                tag="JUNO STEP 1"):
    """Held-step NOTE cell: symbol with FONT_M → S → XS → drop /Bass."""
    ky = R.KNOB_Y
    written, role, painted = CN.note_cell_label(sounding, tier2=tier2)
    R.night_panel(draw, 3, ky + 0, 234, 11, False)
    for i in range(4):
        R.night_panel(draw, i * 60 + 3, ky + 13, 54, 55, i == 0)
    R.night_panel(draw, 0, ky + 70, 240, 12, False)
    R.text(img, 8, ky + R.vc_top("XS", 0, 11), "XS", tag, R.C_HI)
    S.page_dots(draw) if hasattr(S, "page_dots") else None
    # page dots: copy of diagnosis if present
    try:
        S.page_dots(draw)
    except Exception:
        pass
    names = ["NOTE", "GATE", "ACCENT", "SLIDE"]
    vals = [(painted, role), ("NOTE", "M"), ("OFF", "M"), ("OFF", "M")]
    for i, (n, (v, f)) in enumerate(zip(names, vals)):
        cx = i * 60 + 30
        if i:
            R.box(draw, i * 60, ky + 14, 1, 50, R.C_LINE)
        R.text(img, cx, ky + R.vc_top("XS", 13, 10), "XS", n, R.C_GRAY, anchor="ct")
        R.text(img, cx, ky + R.vc_top(f, 30, 34), f, v, R.C_WHITE, anchor="ct")
    R.footer_segs(img, ["PRESS: ON", "TAP: OFF", "HOLD + KNOBS: EDIT"], R.C_DIM)
    return written, role, painted


def dim_silent_marks(draw, steps, silent, held_step=0, length=16, col=JUNO):
    """Repaint stored-but-silent Drone notes in C_DIM (existing dim)."""
    if not silent:
        return
    oy = S.MAIN_Y
    lo, hi = P.window_proposed(steps, length)
    st = steps[held_step]
    end = st["flags"] >> S.BS_END_SHIFT
    width = (end * 12 + 14) // 15 if end else 12
    x = S.col_x(held_step)
    for pitch in silent:
        y = S.note_y(pitch, lo, hi)
        S.night_mark(draw, x, oy + y, width, 5, R.C_DIM, raised=True)


def frame_held(steps, sounding, *, name, part="JUNO B", col=JUNO,
               held_step=0, tier2=True, dropped=None, playing=False,
               velocity=100):
    ensure_part(part, col)
    img, draw = S.canvas()
    P.header(img, draw, "B", playing=playing, held_step=held_step)
    # If the header wrote JUNO B, and we want DRONE B, overpaint the chip
    # by redrawing a Direction-B header with this part name.
    if part != "JUNO B":
        D.header_proposed(img, draw, "B", part=part, mode="SEQ", tempo=125,
                          playing=playing, pattern=1, step=held_step + 1)
    P.draw_roll(img, draw, steps, style="B", length=16, held_step=held_step,
                col=col)
    if dropped:
        dim_silent_marks(draw, steps, dropped, held_step=held_step, col=col)
    line, lrole = paint_held_line(
        img, draw, held_step + 1, sounding, velocity,
        tier2=tier2, dropped=dropped, ink=R.C_AMB,
    )
    written, nrole, painted = paint_knobs(
        img, draw, held_step=held_step, sounding=sounding, tier2=tier2,
        tag=f"{part.split()[0]} STEP {held_step + 1}",
    )
    return img, {
        "line": line, "line_font": lrole,
        "note": written, "note_font": nrole, "note_paint": painted,
        "symbol": CN.chord_symbol(sounding, tier2=tier2),
        "names": " ".join(CN.put_note(n) for n in sounding),
    }


def steps_from(notes, step=0):
    st = S.empty_steps()
    S.set_onset(st, step, list(notes), tie_len=0, vel=100)
    return st


def save(img, name):
    FRAMES_1X.mkdir(parents=True, exist_ok=True)
    FRAMES_3X.mkdir(parents=True, exist_ok=True)
    p1 = FRAMES_1X / f"{name}.png"
    img.save(p1)
    img.resize((720, 720), Image.Resampling.NEAREST).save(FRAMES_3X / f"{name}.png")
    return p1


# (filename, stored notes, kwargs, caption for the sheet)
# kwargs: tier2, dropped, part, example (bool), source
CASES = []


def add(fn, notes, caption, *, source, example=False, tier2=True,
        dropped=None, part="JUNO B", extra_stored=None):
    stored = list(notes) + list(extra_stored or [])
    # For drone, stored is sounding+dropped (low to high in the store).
    CASES.append((fn, stored, notes, dict(
        tier2=tier2, dropped=dropped, part=part,
    ), caption, source, example))


def define_cases():
    CASES.clear()
    add("N-01-held-maj",
        [48, 52, 55],
        "Held C major triad.\nNOTE ‘C’ FONT_M. Held: STEP 1  C  C3 E3 G3  V100.\nPitches unchanged.",
        source="recorded: /workspace/seq-chord-001/evidence/before/before-selected-chord.png\nand before-triad.png (notes 48,52,55). Same set as SR-01 / B-03.",
        example=False)
    add("N-02-held-maj7",
        [48, 52, 55, 59],
        "Held Cmaj7 (recorded tetrad).\nNOTE ‘Cmaj7’ FONT_S (FONT_M is 58–63 px, over 54).\nHeld line FONT_B, 225 px.",
        source="recorded: before-tetrad.png (notes 48,52,55,59).",
        example=False)
    add("N-03-held-hex",
        [48, 52, 55, 59, 62, 65],
        "Held six-note set, no exact template.\nNOTE ‘C3x6’ (paints C3X6 at FONT_M). Notes only on the held line.\nNever a guessed symbol.",
        source="recorded: before-hexachord.png (notes 48,52,55,59,62,65). Same set as SR-04 / B-04.",
        example=False)
    add("N-04-held-mono",
        [48],
        "Held single note.\nNOTE stays ‘C3’. No chord symbol. Mono rule.",
        source="recorded: before-mono-control.png (note 48).",
        example=False)
    add("N-05-held-dyad",
        [48, 52],
        "Held major third, not a template.\nNOTE ‘C3x2’ (paints C3X2). Notes only.",
        source="recorded: before-dyad.png (notes 48,52).",
        example=False)
    add("N-06-inv-C-over-E",
        [52, 55, 60],
        "First inversion, not ambiguous.\nSymbol C/E. NOTE ‘C/E’ FONT_M.\nSlash because the bass is a chord tone other than the root.",
        source="EXAMPLE SET (inversion). No recorded frame at this SHA.",
        example=True)
    add("N-07-spread-maj",
        [48, 55, 60, 64],
        "Spread voicing C3 G3 C4 E4, octaves folded.\nSymbol C (maj). A doubled root is not ambiguity.\nNOTE ‘C’ FONT_M.",
        source="EXAMPLE SET (spread voicing). No recorded frame at this SHA.",
        example=True)
    add("N-08-CEGA-tier2",
        [48, 52, 55, 57],
        "C E G A, Tier 2 on (the recommended default).\nBass C matches 6 first: C6. Not Amin7/C.\nNo ‘?’.",
        source="EXAMPLE SET (C E G A). No recorded frame at this SHA.",
        example=True, tier2=True)
    add("N-09-CEGA-tier1",
        [48, 52, 55, 57],
        "C E G A, Tier 1 only (6 is not in Tier 1).\nBass C does not match; one other root does: Amin7/C.\nNOTE ‘Amin7/C’ FONT_S.",
        source="EXAMPLE SET (C E G A, Tier 1 only). No recorded frame at this SHA.",
        example=True, tier2=False)
    add("N-10-cluster",
        [48, 49, 50],
        "Cluster C C# D: no template.\nNOTE ‘C3x3’ (paints C3X3). Held line shows the notes.\nNever a guessed symbol, never ‘?’.",
        source="EXAMPLE SET (cluster). No recorded frame at this SHA.",
        example=True)
    add("N-11-power-C5",
        [48, 55],
        "Power chord C G = C5 (Tier 1).\nNOTE ‘C5’ FONT_M.",
        source="EXAMPLE SET (power chord). No recorded frame at this SHA.",
        example=True)
    add("N-12-min7",
        [48, 51, 55, 58],
        "C Eb G Bb in; put_note spells C3 D#3 G3 A#3.\nSymbol Cmin7. NOTE ‘Cmin7’ FONT_S.\nPitches unchanged — no respell of the store.",
        source="EXAMPLE SET (Cmin7). No recorded frame at this SHA.",
        example=True)
    add("N-13-oct-double",
        [60, 72],
        "Octave double C4 C5: one pitch class, not a template.\nNOTE ‘C4x2’ (paints C4X2). Notes only.",
        source="EXAMPLE SET (octave double). No recorded frame at this SHA.",
        example=True)
    add("N-14-min7b5-slash",
        [46, 48, 51, 54],
        "Cmin7b5/A# (3rd inversion). NOTE drops /A# → ‘Cmin7b5’ FONT_S\n(full form 120/75/62 px, none fit 54). Held line FONT_XS.",
        source="EXAMPLE SET (Cmin7b5/A#). No recorded frame at this SHA.",
        example=True)
    add("N-15-drone-overflow",
        [55, 59, 62, 65],  # sounding: G3 B3 D4 F4 → G7
        "Drone, six stored, four sound (cold-start INFERRED steal).\nSymbol from the sounding set: G7. C3 and E3 in C_DIM.\nName only what sounds.",
        source="EXAMPLE SET (Drone overflow). Steal is INFERRED from drone_string.h 124–156, not device-observed. Stored C3 E3 G3 B3 D4 F4; sounding G3 B3 D4 F4.",
        example=True, dropped=[48, 52], extra_stored=[48, 52], part="DRONE B")
    add("N-16-aug-bass",
        [52, 56, 60],
        "Augmented E G# C: bass E matches aug first.\nSymbol Eaug, no slash (symmetric set).\nNOTE paints EAUG at FONT_M.",
        source="EXAMPLE SET (aug takes the bass as root). No recorded frame at this SHA.",
        example=True)
    add("N-17-sus-ambiguous",
        [50, 55, 60],
        "D G C, Tier 2: bass D matches nothing;\nCsus2 and Gsus4 both fit. Notes only, NOTE ‘D3x3’.\nAn inversion is not itself ambiguous; two equal matches are.",
        source="EXAMPLE SET (two non-bass matches). No recorded frame at this SHA.",
        example=True)
    add("N-18-longest-slash",
        [44, 46, 49, 52],
        "Longest produced slash: A#min7b5/G#.\nNOTE drops /G# → ‘A#min7b5’ FONT_XS (51 px).\nCmin7b5/G# (119/75/61 px) is shown in the width table; that set is G#7.",
        source="EXAMPLE SET (longest produced name). No recorded frame at this SHA.",
        example=True)


def sheet(paths_captions, out_name, title, subtitle, cols=6):
    n = len(paths_captions)
    rows = (n + cols - 1) // cols
    pad, gap, cap_h, title_h = 24, 16, 118, 90
    cell_w = 240
    W = pad * 2 + cols * cell_w + (cols - 1) * gap
    H = title_h + pad + rows * (240 + cap_h + gap) + pad
    img = Image.new("RGB", (W, H), (12, 14, 16))
    d = ImageDraw.Draw(img)
    f_title = R._sheet_font(22, "Bold")
    f_sub = R._sheet_font(13, "Medium")
    f_cap = R._sheet_font(11, "Medium")
    f_tag = R._sheet_font(11, "Bold")
    d.text((pad, 16), title, font=f_title, fill=(230, 226, 214))
    d.text((pad, 46), subtitle, font=f_sub, fill=(196, 155, 69))
    for i, (path, cap, example) in enumerate(paths_captions):
        r, c = divmod(i, cols)
        x = pad + c * (cell_w + gap)
        y = title_h + r * (240 + cap_h + gap)
        fr = Image.open(path).convert("RGB")
        img.paste(fr, (x, y))
        tag = "EXAMPLE SET" if example else "RECORDED"
        tag_col = (196, 155, 69) if example else (143, 191, 121)
        d.rectangle([x - 1, y - 1, x + 240, y + 240], outline=(40, 56, 61))
        d.text((x, y + 244), tag, font=f_tag, fill=tag_col)
        d.text((x, y + 260), cap, font=f_cap, fill=(200, 204, 198), spacing=2)
    d.text((pad, H - 22),
           "PROPOSAL. Chord notation on Direction B SEQ. Not observed output. 94197fc8. 1× = native 240×240.",
           font=f_tag, fill=(143, 191, 121))
    img.save(OUT / f"{out_name}.png")
    img.resize((img.width * 3, img.height * 3), Image.Resampling.NEAREST).save(
        OUT / f"{out_name}-3x.png")


def main():
    define_cases()
    results = []
    for fn, stored, sounding, kw, cap, source, example in CASES:
        # stored on the roll: unique, low to high (held_chord_get order)
        roll_notes = sorted(set(stored))
        st = steps_from(roll_notes)
        img, meta = frame_held(
            st, sounding, name=fn, part=kw["part"], col=JUNO,
            tier2=kw["tier2"], dropped=kw["dropped"],
        )
        path = save(img, fn)
        print(f"{fn}  sym={meta['symbol']!r}  NOTE {meta['note']} "
              f"({meta['note_font']} paints {meta['note_paint']})  "
              f"held/{meta['line_font']} {meta['line']}")
        print(f"  {source}")
        # Sheet caption: short + source kind already as a tag
        results.append((path, cap, example))
    sheet(
        results,
        "sheet-notation",
        "Chord notation on the B chord/note view",
        "Native 240×240. Direction B. Final naming spec. PROPOSAL, not a firmware patch. "
        "Pitches and playback never change. 94197fc8.",
        cols=6,
    )
    print("wrote", OUT / "sheet-notation.png")


if __name__ == "__main__":
    main()
