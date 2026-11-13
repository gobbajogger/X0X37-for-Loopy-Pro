#!/usr/bin/env python3
"""PROPOSAL frames: §10r actual notes and exact names only.

Host-side Python/PIL. Same glyph language as N-01..N-18.
Not observed output. Not a firmware patch. Pitches and playback never change.
Interval forms (C(3), C(b2 2)) are not used.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, "/workspace/smk-ux")
sys.path.insert(0, "/workspace/smk-ux/seq-chords")

from PIL import Image, ImageDraw

import render_mockups as R
import render_diagnosis as S
import render_piano_roll as P
import render_chord_notation as N
import chord_name as CN

OUT = Path("/workspace/smk-ux/seq-chords")
FRAMES_1X = OUT / "frames" / "1x"
FRAMES_3X = OUT / "frames" / "3x"

JUNO = S.JUNO_SLOT

# Five hard sets. Recorded pitches reused from N-03, N-05 where those
# frames had them. cluster, sus-amb, add9 are EXAMPLE SET.
SETS = [
    dict(
        id="01-dyad",
        notes=[48, 52],
        caption_what="Held major third (recorded dyad).",
        source="recorded: before-dyad.png (notes 48,52). Same set as N-05.",
        example=False,
    ),
    dict(
        id="02-cluster",
        notes=[48, 49, 50],
        caption_what="Cluster C C# D.",
        source="EXAMPLE SET (cluster). Same set as N-10. No recorded frame at this SHA.",
        example=True,
    ),
    dict(
        id="03-susamb",
        notes=[50, 55, 60],
        caption_what="True ambiguous: D G C. No forced name.",
        source="EXAMPLE SET (two non-bass exact matches). Same set as N-17.",
        example=True,
    ),
    dict(
        id="04-add9",
        notes=[48, 50, 52, 55],
        caption_what="Cadd9, C D E G (exact TIER3).",
        source="EXAMPLE SET (add9). No recorded frame at this SHA.",
        example=True,
    ),
    dict(
        id="05-hex",
        notes=[48, 52, 55, 59, 62, 65],
        caption_what="Held six-note set (recorded hexachord).",
        source="recorded: before-hexachord.png (notes 48,52,55,59,62,65). Same set as N-03 / SR-04 / B-04.",
        example=False,
    ),
]

# Two columns: §9 compact-or-name vs §10r exact-or-notes.
STYLES = [
    ("9", "§9  exact or C3xN"),
    ("R", "§10r  exact T1–T3 or notes"),
]


def paint_held_9(img, draw, step_n, sounding, velocity=100):
    N.fill_held_line(draw)
    line, role = CN.held_line(step_n, sounding, velocity)
    y = S.MAIN_Y + R.vc_top(role, 122, 16)
    x = 4
    R.text(img, x, y, role, line, R.C_AMB)
    parts = {"prefix": f"STEP {int(step_n)}  ", "main": "", "second": "",
             "notes": "", "tail": "", "kind": "s9"}
    return line, role, parts


def paint_knobs_9(img, draw, *, sounding, tag="JUNO STEP 1"):
    ky = R.KNOB_Y
    written, role, painted = CN.note_cell_label(sounding)
    pri = CN.chord_symbol(sounding)
    R.night_panel(draw, 3, ky + 0, 234, 11, False)
    for i in range(4):
        R.night_panel(draw, i * 60 + 3, ky + 13, 54, 55, i == 0)
    R.night_panel(draw, 0, ky + 70, 240, 12, False)
    R.text(img, 8, ky + R.vc_top("XS", 0, 11), "XS", tag, R.C_HI)
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
    kind = "unique" if pri else "fallback"
    return written, role, painted, pri, None, kind


def paint_held_10(img, draw, step_n, sounding, *, style="R", velocity=100):
    N.fill_held_line(draw)
    line, role, parts = CN.held_line_10(step_n, sounding, velocity, style=style)
    y = S.MAIN_Y + R.vc_top(role, 122, 16)
    x = 4
    x = R.text(img, x, y, role, parts["prefix"], R.C_AMB)
    if parts["main"]:
        x = R.text(img, x, y, role, parts["main"], R.C_AMB)
    if parts["second"]:
        x = R.text(img, x, y, role, "  ", R.C_AMB)
        x = R.text(img, x, y, role, parts["second"], R.C_DIM)
    if parts["notes"]:
        x = R.text(img, x, y, role, "  ", R.C_AMB)
        x = R.text(img, x, y, role, parts["notes"], R.C_AMB)
    if parts["tail"]:
        x = R.text(img, x, y, role, "  ", R.C_AMB)
        x = R.text(img, x, y, role, parts["tail"], R.C_AMB)
    return line, role, parts


def paint_knobs_10(img, draw, *, sounding, style="R", tag="JUNO STEP 1"):
    ky = R.KNOB_Y
    written, role, painted, pri, sec, kind = CN.note_cell_10(sounding, style=style)
    R.night_panel(draw, 3, ky + 0, 234, 11, False)
    for i in range(4):
        R.night_panel(draw, i * 60 + 3, ky + 13, 54, 55, i == 0)
    R.night_panel(draw, 0, ky + 70, 240, 12, False)
    R.text(img, 8, ky + R.vc_top("XS", 0, 11), "XS", tag, R.C_HI)
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
    return written, role, painted, pri, sec, kind


def frame_cmp(notes, *, style, part="JUNO B"):
    N.ensure_part(part, JUNO)
    st = N.steps_from(notes)
    img, draw = S.canvas()
    P.header(img, draw, "B", playing=False, held_step=0)
    P.draw_roll(img, draw, st, style="B", length=16, held_step=0, col=JUNO)
    if style == "9":
        line, lrole, parts = paint_held_9(img, draw, 1, notes)
        written, nrole, painted, pri, sec, kind = paint_knobs_9(
            img, draw, sounding=notes, tag=f"{part.split()[0]} STEP 1",
        )
    else:
        line, lrole, parts = paint_held_10(img, draw, 1, notes, style=style)
        written, nrole, painted, pri, sec, kind = paint_knobs_10(
            img, draw, sounding=notes, style=style, tag=f"{part.split()[0]} STEP 1",
        )
    return img, {
        "line": line, "line_font": lrole, "parts": parts,
        "note": written, "note_font": nrole, "note_paint": painted,
        "pri": pri, "sec": sec, "kind": kind,
        "names": " ".join(CN.put_note(n) for n in notes),
    }


def save(img, name):
    FRAMES_1X.mkdir(parents=True, exist_ok=True)
    FRAMES_3X.mkdir(parents=True, exist_ok=True)
    p1 = FRAMES_1X / f"{name}.png"
    img.save(p1)
    img.resize((720, 720), Image.Resampling.NEAREST).save(FRAMES_3X / f"{name}.png")
    return p1


def frame_caption(meta, style, setrow):
    paint = meta["note_paint"]
    written = meta["note"]
    paint_bit = f", paints {paint}" if paint != written else ""
    sec = meta["sec"]
    sec_bit = f"  dim ‘{sec}’" if sec else ""
    return (
        f"{setrow['caption_what']}\n"
        f"NOTE ‘{written}’ FONT_{meta['note_font']}{paint_bit}.{sec_bit}\n"
        f"Held/{meta['line_font']}: {meta['line']}"
    )


def sheet(grid, out_name, title, subtitle):
    """grid: list of rows, each a list of (path, cap, example)."""
    rows = len(grid)
    cols = len(grid[0])
    pad, gap, cap_h, title_h, col_h = 24, 16, 118, 86, 28
    cell_w = 240
    W = pad * 2 + cols * cell_w + (cols - 1) * gap
    H = title_h + col_h + pad + rows * (240 + cap_h + gap) + pad
    img = Image.new("RGB", (W, H), (12, 14, 16))
    d = ImageDraw.Draw(img)
    f_title = R._sheet_font(22, "Bold")
    f_sub = R._sheet_font(13, "Medium")
    f_cap = R._sheet_font(11, "Medium")
    f_tag = R._sheet_font(11, "Bold")
    f_col = R._sheet_font(13, "Bold")
    d.text((pad, 14), title, font=f_title, fill=(230, 226, 214))
    d.text((pad, 44), subtitle, font=f_sub, fill=(196, 155, 69))
    for c, hdr in enumerate(STYLES):
        x = pad + c * (cell_w + gap)
        d.text((x, title_h), hdr[1], font=f_col, fill=(230, 226, 214))
    for r, row in enumerate(grid):
        for c, (path, cap, example) in enumerate(row):
            x = pad + c * (cell_w + gap)
            y = title_h + col_h + r * (240 + cap_h + gap)
            fr = Image.open(path).convert("RGB")
            img.paste(fr, (x, y))
            tag = "EXAMPLE SET" if example else "RECORDED"
            tag_col = (196, 155, 69) if example else (143, 191, 121)
            d.rectangle([x - 1, y - 1, x + 240, y + 240], outline=(40, 56, 61))
            d.text((x, y + 244), tag, font=f_tag, fill=tag_col)
            d.text((x, y + 260), cap, font=f_cap, fill=(200, 204, 198), spacing=2)
    d.text(
        (pad, H - 22),
        "PROPOSAL. §10r on Direction B SEQ. No interval form. Not observed output. 94197fc8. 1× = native 240×240.",
        font=f_tag, fill=(143, 191, 121),
    )
    img.save(OUT / f"{out_name}.png")
    img.resize((img.width * 3, img.height * 3), Image.Resampling.NEAREST).save(
        OUT / f"{out_name}-3x.png"
    )


def main():
    # Remove superseded A/B/C/D frames that showed interval forms.
    for folder in (FRAMES_1X, FRAMES_3X):
        for p in folder.glob("AMB-[ABCD]-*.png"):
            p.unlink()
            print("removed", p)

    grid = [[] for _ in SETS]
    for si, setrow in enumerate(SETS):
        for style, _hdr in STYLES:
            fn = f"AMB-{style}-{setrow['id']}"
            img, meta = frame_cmp(setrow["notes"], style=style)
            if "(" in (meta["note"] or "") or "(" in (meta["pri"] or "") or "(" in (meta["line"] or ""):
                raise SystemExit(f"interval form in {fn}: NOTE={meta['note']!r} pri={meta['pri']!r} line={meta['line']!r}")
            path = save(img, fn)
            cap = frame_caption(meta, style, setrow)
            print(
                f"{fn}  {meta['kind']:9s}  pri={meta['pri']!r}  sec={meta['sec']!r}  "
                f"NOTE {meta['note']} ({meta['note_font']} paints {meta['note_paint']})  "
                f"held/{meta['line_font']} {meta['line']}"
            )
            print(f"  {setrow['source']}")
            grid[si].append((path, cap, setrow["example"]))
    sheet(
        grid,
        "sheet-ambiguous",
        "§10r: actual notes, exact names only",
        "Native 240×240. Direction B. §9 compact dump vs §10r (exact T1/T2/T3 or the notes). "
        "No interval form, no ‘?’, no forced name on a truly uncertain set. PROPOSAL, not a firmware patch. 94197fc8.",
    )
    print("wrote", OUT / "sheet-ambiguous.png")


if __name__ == "__main__":
    main()
