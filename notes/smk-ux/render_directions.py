#!/usr/bin/env python3
"""Three coherent visual and interaction directions for DnBOX SMK-37.

Evolves the existing mockups in /workspace/smk-ux/mockups and Dot's six-screen
journey. SHIPPED frames are source-faithful reconstructions at commit
94197fc839b00aff47b98ca02f3c6396e4a6eabf. PROPOSED is not implemented.
Nothing here was verified on hardware. Firmware is not edited.

Strip layout is the real 20+69+69+82. Colours are RGB565-safe. Fonts are
Barlow Semi Condensed at the source roles T 10, XS 12, S 15, B 15, M 23
(repo TTFs, not packed felucca bitmaps).
"""
from __future__ import annotations

import hashlib
import sys
from pathlib import Path

sys.path.insert(0, "/workspace/smk-ux")

from PIL import Image, ImageDraw, ImageFont

from render_mockups import (
    BAND_H, BD_HIT_PAIRS, BD_HITS, BOOT_FOOT, C_AMB, C_BLACK, C_DIM, C_GRAY,
    C_HI, C_LINE, C_WHITE, FONT, FONT_DIR, FOOT_GAP, FOOT_Y, GX, HDR_H,
    HOME_FOOT, KNOB_H, KNOB_Y, MAIN_Y, N_ACCENT, N_AMBER, N_BASE, N_EDGE,
    N_FACE, N_GRAY, N_INK, N_LINE, N_SELECT, N_SHADOW, N_SIDE, N_WHITE,
    PARA_FOOT_SRC, PARA_VALS, PARA_VALS_STEP, PART, RGB, SEQ_FOOT, VOICES,
    blend, box, col_x, dim, draw_drum, draw_knob_strip, draw_para_matrix,
    fill_header_panel, footer_segs, frame_01_first_sound_CURRENT,
    frame_02_hit_CURRENT, frame_03_sound_CURRENT, frame_04_step_CURRENT,
    frame_05_save_playing_CURRENT_badge, frame_05_save_playing_CURRENT_toast,
    frame_06_saved_CURRENT, header_para_current, header_seq, hline,
    new_frame, night_mark, night_panel, pattern_badge, play_icon, rgb565,
    save_badge, surface_panel, text, tw, vc_top, vline,
)

ROOT = Path("/workspace/smk-ux")
OUT = ROOT / "directions"
OUT_1X = OUT / "frames" / "1x"
OUT_2X = OUT / "frames" / "2x"

# Sheet chrome (not on the device, not required to be RGB565).
SHEET_BG = (14, 16, 18)
SHEET_INK = (226, 230, 222)
SHEET_DIM = (132, 144, 142)
SHEET_MUTE = (92, 102, 102)
SHEET_AMB = (220, 164, 64)
SHEET_LINE = (36, 42, 44)
SHEET_RULE = (28, 32, 34)
SHEET_SHIP = (186, 196, 194)
SHEET_CARD = (10, 12, 13)


def sheet_font(size, weight="SemiBold"):
    name = {
        "SemiBold": "BarlowSemiCondensed-SemiBold.ttf",
        "Medium": "BarlowSemiCondensed-Medium.ttf",
        "Bold": "BarlowSemiCondensed-Bold.ttf",
    }[weight]
    return ImageFont.truetype(str(FONT_DIR / name), size)


def nn2(im):
    return im.resize((im.size[0] * 2, im.size[1] * 2), Image.Resampling.NEAREST)


# ---------------------------------------------------------------------------
# Shared marks
# ---------------------------------------------------------------------------

def play_icon_outline(draw, x, y, c):
    """Same 8x11 footprint as the filled play icon, drawn hollow (PROPOSAL)."""
    pts = [(x, y), (x, y + 10), (x + 8, y + 5)]
    draw.polygon(pts, fill=N_BASE)
    draw.line([pts[0], pts[1]], fill=c, width=1)
    draw.line([pts[1], pts[2]], fill=c, width=1)
    draw.line([pts[2], pts[0]], fill=c, width=1)
    for pt in pts:
        draw.point(pt, fill=c)


def play_mark(draw, x, y, playing, style):
    if style == "B":
        if playing:
            play_icon(draw, x, y, C_HI)
        else:
            play_icon_outline(draw, x, y, C_DIM)
    else:
        play_icon(draw, x, y, C_HI if playing else C_DIM)


def amber_dot(draw):
    draw.ellipse([235, 1, 239, 5], fill=C_AMB)


def part_chip(img, draw, part, x=0, y=1, h=17):
    label = part
    w = tw("B", label) + 12
    box(draw, x, y, w, h, PART[part])
    text(img, x + 6, vc_top("B", y, h), "B", label, C_BLACK)
    return x + w


def outline_pill(img, draw, x, y, h, label, ink=C_AMB, role="XS"):
    w = tw(role, label) + 8
    # 1 px amber frame, dark inside
    box(draw, x, y, w, h, N_BASE)
    draw.rectangle([x, y, x + w - 1, y + h - 1], outline=ink)
    text(img, x + w // 2, vc_top(role, y, h), role, label, ink, anchor="ct")
    return x + w


def fill_pill(img, draw, x, y, h, label, face=C_AMB, ink=C_BLACK, role="XS"):
    w = tw(role, label) + 8
    box(draw, x, y, w, h, face)
    text(img, x + w // 2, vc_top(role, y, h), role, label, ink, anchor="ct")
    return x + w


def ticket(img, draw, x, y, h, label, face=C_AMB, ink=C_BLACK, role="XS"):
    """C: a plate in the same family as the 909 part chip."""
    w = tw(role, label) + 12
    box(draw, x, y, w, h, face)
    text(img, x + 6, vc_top(role, y, h), role, label, ink)
    return x + w


def draw_cue(img, draw, current, nxt, style="shipped"):
    """Queued next pattern. Source: NEXT glyph + number, cue ink (5470–5475)."""
    pattern_badge(img, draw, 132, current, C_WHITE)
    nx = 132 + 12 + tw("S", str(current)) + 5
    cy = 10
    if style == "C":
        # small amber ticket with the next number — same family as save tickets
        w = tw("XS", str(nxt)) + 10
        # a tiny next chevron to the left, then the ticket
        draw.polygon([(nx, 6), (nx, 14), (nx + 5, 10)], fill=C_AMB)
        fill_pill(img, draw, nx + 8, 3, 13, str(nxt), C_AMB, C_BLACK, "XS")
    elif style == "B":
        w = 8 + tw("S", str(nxt))
        draw.rectangle([nx - 3, 3, nx + w + 1, 16], outline=C_AMB)
        draw.polygon([(nx, 6), (nx, 14), (nx + 5, 10)], fill=C_AMB)
        text(img, nx + 8, vc_top("S", 1, 17), "S", str(nxt), C_AMB)
    else:
        draw.polygon([(nx, 6), (nx, 14), (nx + 5, 10)], fill=C_AMB)
        text(img, nx + 8, vc_top("S", 1, 17), "S", str(nxt), C_AMB)


def signal_bar(draw, color):
    box(draw, 0, 18, 240, 2, color)


def spine(draw, color, w=3, y0=20):
    """Full-height room edge, starting below the 20 px header so it does not
    cut a part chip or a save ticket."""
    box(draw, 0, y0, w, 240 - y0, color)


def footer_contrast(img, pairs, y=None, way_back=None):
    """C: key name dim, action word bright. Optional way-back in accent."""
    if y is None:
        y = KNOB_Y + vc_top("T", FOOT_Y, 9)
    x = 4
    for i, (key, action) in enumerate(pairs):
        if i:
            x += FOOT_GAP
        x = text(img, x, y, "T", key, C_DIM)
        x = text(img, x + 1, y, "T", action, N_INK)
    if way_back:
        key, action = way_back
        # already included in pairs when we want it inline; this is a right-side emphasis
        text(img, 236, y, "T", key + action, C_HI, anchor="rt")
    return x


def overpaint_knob_footer(img, draw, style, parts, contrast_pairs=None, highlight_last=False):
    box(draw, 0, KNOB_Y + 70, 240, 12, N_BASE)
    night_panel(draw, 0, KNOB_Y + 70, 240, 12, False)
    y = KNOB_Y + vc_top("T", FOOT_Y, 9)
    if style == "C" and contrast_pairs:
        x = 4
        last = len(contrast_pairs) - 1
        for i, (key, action) in enumerate(contrast_pairs):
            if i:
                x += FOOT_GAP
            act = C_HI if (highlight_last and i == last) else N_INK
            x = text(img, x, y, "T", key, C_DIM)
            x = text(img, x + 1, y, "T", action, act)
    else:
        col = C_DIM if style != "C" else N_INK
        footer_segs(img, parts, col, y=y)


def overpaint_para_hint(img, draw, style, held_step=None, show_footer=True):
    """Bottom 13 px of the PARA matrix: source hint, restyled per direction."""
    box(draw, 0, 227, 240, 13, N_BASE)
    if held_step is not None:
        line = f"STEP {held_step}  < >: CHANCE  PARA"
        if style == "C":
            # amber plate, black source words — the room is this step
            box(draw, 0, 227, 240, 13, C_AMB)
            text(img, 7, 229, "T", line, C_BLACK)
        elif style == "B":
            night_panel(draw, 0, 227, 240, 13, False)
            draw.rectangle([0, 227, 239, 239], outline=C_AMB)
            text(img, 7, 229, "T", line, C_AMB)
        else:
            night_panel(draw, 0, 227, 240, 13, False)
            text(img, 7, 229, "T", line, C_AMB)
        return
    night_panel(draw, 0, 227, 240, 13, False)
    if show_footer:
        y = 229
        if style == "C":
            pairs = [("BLACK:", "SELECT"), ("WHITE:", "SOUND"), ("SEQ:", "STEPS")]
            x = 4
            for i, (k, a) in enumerate(pairs):
                if i:
                    x += FOOT_GAP
                ink = C_HI if k.startswith("SEQ") else N_INK
                x = text(img, x, y, "T", k, C_DIM)
                x = text(img, x + 1, y, "T", a, ink)
            text(img, 234, y, "T", "1-3", N_AMBER, anchor="rt")
        else:
            footer_segs(img, PARA_FOOT_SRC, C_DIM, y=y)
            text(img, 234, y, "T", "1-3", N_AMBER, anchor="rt")
    else:
        text(img, 7, 229, "T", "< >", N_GRAY)
        text(img, 234, 229, "T", "1-3", N_AMBER, anchor="rt")


# ---------------------------------------------------------------------------
# Headers per style
# ---------------------------------------------------------------------------

def _clock(img, draw, tempo, playing, style, cue=None, unsaved=False, rec=False):
    if cue is None:
        pattern_badge(img, draw, 132, 1, C_WHITE)
    else:
        draw_cue(img, draw, current=1, nxt=cue, style=style if style in ("B", "C") else "shipped")
    text(img, 212, vc_top("S", 1, 17), "S", str(tempo), C_HI, anchor="rt")
    play_mark(draw, 217, 4, playing, style)
    if rec:
        draw.ellipse([229, 7, 237, 15], fill=RGB(255, 50, 50))
    if unsaved:
        amber_dot(draw)


def header_proposed(img, draw, style, part="909", mode="SEQ", tempo=125,
                    playing=False, pattern=1, unsaved=False, save_msg=None,
                    step=None, cue=None, rec=False):
    """Proposed headers for A / B / C. Pattern and tempo always stay.

    save_msg is one of the source strings: SAVES WHEN STOPPED, SAVE AT STOP,
    SAVED. step is a held-step number. cue is a queued next-pattern number.
    """
    fill_header_panel(draw)
    waiting = (
        save_msg in ("SAVES WHEN STOPPED", "SAVE AT STOP")
        or (step is not None)
        or (cue is not None)
    )

    # --- C hairline: room identity, always, in part colour (amber if step)
    if style == "C":
        hair = C_AMB if step is not None else PART[part]
        box(draw, 0, 18, 240, 2, hair)

    # --- B rail: a quiet edge at rest, amber when something is waiting.
    #     SAVED is confirmed, not waiting, so the rail stays quiet.
    if style == "B":
        if waiting:
            signal_bar(draw, C_AMB)
        else:
            box(draw, 0, 19, 240, 1, N_EDGE)

    # --- Left identity / message -----------------------------------------
    # Long toast that cannot sit next to the 909 chip:
    long_msg = save_msg == "SAVES WHEN STOPPED"

    if save_msg:
        if style == "A":
            # pickup-style amber words on the left (existing proposal)
            role = "S" if tw("S", save_msg) + 8 <= 128 else "XS"
            text(img, 4, vc_top(role, 1, 17), role, save_msg, C_AMB)
        elif style == "B":
            if long_msg:
                # 3 px part-colour tick so the 909 is not gone, then words
                box(draw, 0, 1, 3, 17, PART[part])
                outline_pill(img, draw, 6, 3, 13, save_msg, C_AMB, "XS")
            elif save_msg == "SAVED":
                # confirmed = fill
                part_chip(img, draw, part)
                fill_pill(img, draw, 34 + 6, 3, 13, save_msg, C_AMB, C_BLACK, "XS")
            else:
                # SAVE AT STOP: 909 chip stays, status takes the mode slot
                x = part_chip(img, draw, part) + 6
                outline_pill(img, draw, x, 3, 13, save_msg, C_AMB, "XS")
        else:  # C tickets
            if long_msg:
                box(draw, 0, 1, 3, 17, PART[part])
                ticket(img, draw, 6, 1, 17, save_msg, C_AMB, C_BLACK, "XS")
            elif save_msg == "SAVED":
                # brief success ticket in the part colour (same family as the
                # 909 chip and as the shipped toast), not a persistent panel
                # and not a new 'success green'.
                ticket(img, draw, 0, 1, 17, save_msg, PART[part], C_BLACK, "S")
            else:
                x = part_chip(img, draw, part) + 4
                ticket(img, draw, x, 1, 17, save_msg, C_AMB, C_BLACK, "XS")
    else:
        # Normal identity: part chip + mode word (SOUND or SEQ or HOME…)
        if mode in ("HOME", "SONG", "LP"):
            word = mode
            text(img, 4, vc_top("B", 1, 17), "B", word, C_HI)
            x = 4 + tw("B", word) + 6
        else:
            x = part_chip(img, draw, part) + 6
            if mode:
                mf = "S"
                text(img, x, vc_top(mf, 1, 17), mf, mode, C_HI)
                x = x + tw(mf, mode) + 6
        # Held-step pill in B: fits with SOUND at XS-sized pill
        if step is not None and style == "B":
            outline_pill(img, draw, x, 3, 13, f"STEP {step}", C_AMB, "XS")
        # Held-step in C: the spine and plate carry it; keep SOUND in the header
        # Held-step in A: the STEP line in the matrix is the tell (no extra pill)

        # UNSAVED as a full word only when it fits left of the badge (x=132).
        # B uses an outline (waiting / unresolved). C uses a ticket.
        # A uses the source fill badge. A squeezed * or Q is never drawn;
        # the 2 px amber dot (source 5501) remains.
        if unsaved and not save_msg:
            w = tw("XS", "UNSAVED") + 10
            if x + w <= 130:
                if style == "B":
                    outline_pill(img, draw, x, 3, 13, "UNSAVED", C_AMB, "XS")
                elif style == "C":
                    ticket(img, draw, x, 3, 13, "UNSAVED", C_AMB, C_BLACK, "XS")
                else:
                    save_badge(img, draw, x, "UNSAVED", C_AMB)

    # Clock, cue, play, unsaved dot. For A/B/C the clock ALWAYS stays.
    # If a cue is drawn it replaces the plain pattern badge.
    if cue is not None:
        draw_cue(img, draw, current=pattern, nxt=cue, style=style)
    else:
        pattern_badge(img, draw, 132, pattern, C_WHITE)
    text(img, 212, vc_top("S", 1, 17), "S", str(tempo), C_HI, anchor="rt")
    play_mark(draw, 217, 4, playing, style)
    if rec:
        draw.ellipse([229, 7, 237, 15], fill=RGB(255, 50, 50))
    # 2 px unsaved / queued-save dot (source 5501–5502) — not for SAVED
    if unsaved or (save_msg in ("SAVES WHEN STOPPED", "SAVE AT STOP")):
        amber_dot(draw)


# ---------------------------------------------------------------------------
# Knob / grid helpers
# ---------------------------------------------------------------------------

SEQ_KNOBS = [("LENGTH", "16", ""), ("RATE", "1/16", ""), None, None]
PERF_KNOBS = [("TEMPO", "125", "bpm"), ("SWING", "50", "%"),
              ("FILTER", "OFF", ""), ("PUMP", "0.0", "dB")]

SEQ_CONTRAST = [("BLACK:", "TRACK"), ("WHITE:", "STEP"), ("PATCH+BLK:", "MUTE")]
PARA_CONTRAST = [("BLACK:", "SELECT"), ("WHITE:", "SOUND"), ("SEQ:", "STEPS")]


def seq_body(img, draw, style, hits=None, sel=0, playing_step=None, plock=None,
             tag="P1 SEQUENCE", footer=SEQ_FOOT, unsaved=False):
    draw_drum(img, draw, hits=hits or set(), sel=sel,
              playing_step=playing_step, plock=plock)
    draw_knob_strip(img, d := draw, tag, SEQ_KNOBS, footer, C_DIM)
    overpaint_knob_footer(img, draw, style, footer,
                          contrast_pairs=SEQ_CONTRAST, highlight_last=False)


def finish_c_spine(draw, part="909", step=None):
    spine(draw, C_AMB if step is not None else PART[part], w=3)


def to_rgb565(im):
    """Quantise every pixel to the firmware RGB() expansion (gfx.c:65).
    PIL text anti-aliasing otherwise leaves illegal intermediate colours.
    """
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            px[x, y] = rgb565(*px[x, y])
    return im


# ---------------------------------------------------------------------------
# SHIPPED frames (reuse existing reconstructions; add a live frame)
# ---------------------------------------------------------------------------

def shipped_live():
    """909 SEQ, playing, playhead on step 5, next pattern queued, unsaved."""
    im, d = new_frame()
    header_seq(im, d, mode="SEQ", unsaved=True, playing=True, tempo=125, pattern=1)
    # cue is not in header_seq — draw it over, in the free mid-header gap
    draw_cue(im, d, current=1, nxt=2, style="shipped")
    draw_drum(im, d, hits=BD_HIT_PAIRS, sel=0, playing_step=4)
    draw_knob_strip(im, d, "P1 SEQUENCE", SEQ_KNOBS, SEQ_FOOT, C_DIM)
    return im


def _ship(fn):
    return lambda: to_rgb565(fn())


SHIPPED = {
    "01-first-sound": _ship(frame_01_first_sound_CURRENT),
    "02-pattern": _ship(frame_02_hit_CURRENT),
    "03-sound": _ship(frame_03_sound_CURRENT),
    "04-step": _ship(frame_04_step_CURRENT),
    "05-save-playing": _ship(frame_05_save_playing_CURRENT_badge),  # persistent Q
    "05-save-toast": _ship(frame_05_save_playing_CURRENT_toast),    # 1300 ms cover
    "06-saved": _ship(frame_06_saved_CURRENT),
    "07-live": _ship(shipped_live),
}


# ---------------------------------------------------------------------------
# Proposed frames per direction
# ---------------------------------------------------------------------------

def _proposed(style, kind):
    im, d = new_frame()

    if kind == "01-first-sound":
        header_proposed(im, d, style, mode="SEQ", playing=False, unsaved=False)
        seq_body(im, d, style, hits=set(), sel=0, footer=SEQ_FOOT)
        if style == "C":
            finish_c_spine(d, "909")

    elif kind == "02-pattern":
        header_proposed(im, d, style, mode="SEQ", playing=False, unsaved=True)
        seq_body(im, d, style, hits=BD_HIT_PAIRS, sel=0, footer=SEQ_FOOT)
        if style == "C":
            finish_c_spine(d, "909")

    elif kind == "03-sound":
        header_proposed(im, d, style, mode="SOUND", playing=False, unsaved=True)
        draw_para_matrix(im, d, active=0, values=PARA_VALS)
        overpaint_para_hint(im, d, style, held_step=None, show_footer=True)
        if style == "C":
            finish_c_spine(d, "909")

    elif kind == "04-step":
        header_proposed(im, d, style, mode="SOUND", playing=False, unsaved=True,
                        step=5)
        draw_para_matrix(im, d, active=0, values=PARA_VALS_STEP, held_step=5)
        overpaint_para_hint(im, d, style, held_step=5)
        if style == "C":
            finish_c_spine(d, "909", step=5)

    elif kind == "05-save-playing":
        # persistent queued-save state (the live-demo moment, after the toast)
        header_proposed(im, d, style, mode="SEQ", playing=True, unsaved=False,
                        save_msg="SAVE AT STOP")
        seq_body(im, d, style, hits=BD_HIT_PAIRS, sel=0, playing_step=4)
        if style == "C":
            finish_c_spine(d, "909")

    elif kind == "05-save-toast":
        # the moment of the BT tap, same words, clock stays
        header_proposed(im, d, style, mode="SEQ", playing=True, unsaved=False,
                        save_msg="SAVES WHEN STOPPED")
        seq_body(im, d, style, hits=BD_HIT_PAIRS, sel=0, playing_step=4)
        if style == "C":
            finish_c_spine(d, "909")

    elif kind == "06-saved":
        header_proposed(im, d, style, mode="SEQ", playing=False, unsaved=False,
                        save_msg="SAVED")
        seq_body(im, d, style, hits=BD_HIT_PAIRS, sel=0)
        if style == "C":
            finish_c_spine(d, "909")

    elif kind == "07-live":
        header_proposed(im, d, style, mode="SEQ", playing=True, unsaved=True,
                        cue=2, pattern=1)
        seq_body(im, d, style, hits=BD_HIT_PAIRS, sel=0, playing_step=4)
        if style == "C":
            finish_c_spine(d, "909")

    else:
        raise ValueError(kind)

    return to_rgb565(im)


KINDS = [
    "01-first-sound",
    "02-pattern",
    "03-sound",
    "04-step",
    "05-save-playing",
    "05-save-toast",
    "06-saved",
    "07-live",
]

# Journey rows shown on the direction sheet (toast is kept as a frame, not a row)
SHEET_ROWS = [
    "01-first-sound",
    "02-pattern",
    "03-sound",
    "04-step",
    "05-save-playing",
    "06-saved",
    "07-live",
]


# ---------------------------------------------------------------------------
# Captions (plain, no code, no line numbers)
# ---------------------------------------------------------------------------

ROW_TITLE = {
    "01-first-sound": "1    First sound",
    "02-pattern":     "2    A pattern",
    "03-sound":       "3    The sound",
    "04-step":        "4    One step",
    "05-save-playing":"5    Save, while playing",
    "06-saved":       "6    Saved",
    "07-live":        "7    Live play",
}

CAPTION_SHIPPED = {
    "01-first-sound": "The keys are not named until you press something. The dim triangle can look as if the pattern is already running.",
    "02-pattern": "The whole 909 kit, with a kick on every beat. This overview is kept in every direction.",
    "03-sound": "A 909, but is this the sound or the steps? The pattern and the tempo are gone.",
    "04-step": "A STEP line is already here, but the header still does not say SOUND, and the clock is still gone.",
    "05-save-playing": "A queued save shrinks to Q. The first toast also covers the clock.",
    "06-saved": "SAVED covers the pattern and the tempo for a moment.",
    "07-live": "Playing, with the next pattern waiting. Selected, playing and queued each use a different kind of mark.",
}

CAPTION_A = {
    "01-first-sound": "The keys are named from the first moment: black plays a sound, white writes a step.",
    "02-pattern": "The same kit, kept in full. Nothing on this screen needed inventing.",
    "03-sound": "SOUND, with the pattern and the tempo still in view. SEQ: STEPS is the way back.",
    "04-step": "SOUND is in the header, with the pattern and the tempo. The amber STEP line is how you tell this edit is one step.",
    "05-save-playing": "SAVE AT STOP, in full, and the clock stays. These are the words the firmware already uses.",
    "06-saved": "SAVED, and you can still see the pattern that was just written.",
    "07-live": "The same sentence as every other screen: where you are, what is selected, what is playing, what is waiting.",
}

CAPTION_B = {
    "01-first-sound": "The keys are named from the first moment. An empty triangle means stopped.",
    "02-pattern": "The same kit, kept in full. A filled row is the selected drum.",
    "03-sound": "SOUND, and the clock stays. Waiting marks are reserved for a held step or a queued save.",
    "04-step": "SOUND stays. STEP 5 is an amber outline — the same waiting mark as a queued save, named so the scope is clear.",
    "05-save-playing": "SAVE AT STOP as an amber outline, the 909 still visible, the clock still there. Never a Q.",
    "06-saved": "SAVED is a brief filled mark: waiting became written. The clock stays.",
    "07-live": "A filled triangle is playing. An amber outline is the next pattern waiting. One family of marks.",
}

CAPTION_C = {
    "01-first-sound": "You are in the 909. The keys are named, and the action words are the bright ones.",
    "02-pattern": "The same kit, kept in full, inside the 909 room.",
    "03-sound": "The SOUND room. The orange edge is the part. SEQ: STEPS, in accent, is the way back.",
    "04-step": "The edge turns amber, and the STEP line is a plate. You are in this step. The words are the firmware’s.",
    "05-save-playing": "A SAVE AT STOP ticket, in the same family as the 909 chip. The edge of the room is still there. The clock stays.",
    "06-saved": "A brief SAVED ticket, then the room returns. No persistent panel.",
    "07-live": "Playing in the 909 room. The next pattern is a small ticket — waiting, named, not a save.",
}

CAPTIONS = {"SHIPPED": CAPTION_SHIPPED, "A": CAPTION_A, "B": CAPTION_B, "C": CAPTION_C}

DIR_META = {
    "A": {
        "name": "Plain speech",
        "kicker": "Direction A",
        "deck": "The firmware already wrote the right words. This direction shows them, on every screen, as one sentence: where you are, what the keys do, and whether a save is waiting.",
        "legend": "Mode words and key names are the language. Selected, playing and queued keep the marks the firmware already draws.",
    },
    "B": {
        "name": "Live signals",
        "kicker": "Direction B",
        "deck": "One family of marks you can read at a glance, even mid-performance. A fill is selected or playing. An amber outline is waiting. The clock never goes away.",
        "legend": "Filled row: selected.    Filled triangle: playing.    Empty triangle: stopped.    Amber outline: waiting, always named.",
    },
    "C": {
        "name": "Two rooms",
        "kicker": "Direction C",
        "deck": "Sound and steps are different rooms, with the same furniture. A coloured edge tells you which room you are in. Waiting and saving use tickets in the same family as the part chip.",
        "legend": "Orange edge: the 909.    Amber edge: this step.    Tickets: a save or a waiting pattern.    Bright footer words: what the keys do.",
    },
}

SHEET_FOOT = [
    "SHIPPED 94197fc8 is a source reconstruction, not a device photograph. PROPOSED is not implemented, and was not verified on hardware.",
    "Existing controls and live meanings are unchanged. The 12-row kit overview is kept. Evolved from the existing 94197fc8 screens and Dot’s journey proposals.",
    "DnBOX. Chicago is the 303 engine; Miami is never called a 303.",
]


def wrap_text(s, font, max_w, draw):
    words = s.split()
    lines, cur = [], ""
    for w in words:
        trial = (cur + " " + w).strip()
        if draw.textlength(trial, font=font) <= max_w:
            cur = trial
        else:
            if cur:
                lines.append(cur)
            cur = w
    if cur:
        lines.append(cur)
    return lines


def paste_device(sheet, frame, x, y, bezel, line):
    """Paste a 240×240 (or 480×480) frame with a quiet bezel. The frame itself is native pixels."""
    fw, fh = frame.size
    # outer fill + 1 px line
    dr = ImageDraw.Draw(sheet)
    dr.rectangle([x, y, x + fw + bezel * 2 - 1, y + fh + bezel * 2 - 1], fill=SHEET_CARD)
    dr.rectangle([x, y, x + fw + bezel * 2 - 1, y + fh + bezel * 2 - 1], outline=line)
    sheet.paste(frame, (x + bezel, y + bezel))
    return fw + bezel * 2, fh + bezel * 2


def make_direction_sheet(style, frames_shipped, frames_prop, scale=1):
    """User-facing before/after. Frames are shown at real 240×240 (or 480 at 2×)."""
    meta = DIR_META[style]
    S = scale
    margin = 64 * S
    gap = 72 * S
    bezel = 6 * S
    frame_s = 240 * S
    col_w = frame_s + bezel * 2
    top = 176 * S
    row_title_h = 26 * S
    col_lab_h = 22 * S
    cap_h = 76 * S
    row_gap = 36 * S
    foot_h = 118 * S
    n = len(SHEET_ROWS)
    W = margin * 2 + col_w * 2 + gap
    H = top + n * (row_title_h + col_lab_h + col_w + cap_h + row_gap) + foot_h

    im = Image.new("RGB", (W, H), SHEET_BG)
    dr = ImageDraw.Draw(im)
    f_kicker = sheet_font(13 * S, "Medium")
    f_title = sheet_font(32 * S, "Bold")
    f_deck = sheet_font(15 * S, "Medium")
    f_leg = sheet_font(13 * S, "Medium")
    f_row = sheet_font(15 * S, "Bold")
    f_col = sheet_font(13 * S, "Bold")
    f_cap = sheet_font(13 * S, "Medium")
    f_foot = sheet_font(12 * S, "Medium")

    dr.text((margin, 28 * S), meta["kicker"], font=f_kicker, fill=SHEET_AMB)
    dr.text((margin, 46 * S), meta["name"], font=f_title, fill=SHEET_INK)
    # deck, wrapped
    deck_w = W - margin * 2
    dy = 88 * S
    for line in wrap_text(meta["deck"], f_deck, deck_w, dr):
        dr.text((margin, dy), line, font=f_deck, fill=SHEET_DIM)
        dy += 20 * S
    dr.text((margin, dy + 6 * S), meta["legend"], font=f_leg, fill=SHEET_MUTE)

    # column headings, right aligned to each column — drawn per row's x
    # stamp
    dr.text((W - margin, 28 * S), "94197fc8   ·   NOT IMPLEMENTED",
            font=f_kicker, fill=SHEET_MUTE, anchor="rt")
    dr.text((W - margin, 46 * S), "Not a device photograph",
            font=f_kicker, fill=SHEET_MUTE, anchor="rt")

    y = top
    for kind in SHEET_ROWS:
        dr.text((margin, y), ROW_TITLE[kind], font=f_row, fill=SHEET_INK)
        y += row_title_h
        xs = [margin, margin + col_w + gap]
        labels = [("SHIPPED  94197fc8", SHEET_SHIP, frames_shipped[kind], "SHIPPED"),
                  ("PROPOSED", SHEET_AMB, frames_prop[kind], style)]
        for x, (lab, col, frame, cap_key) in zip(xs, labels):
            dr.text((x, y), lab, font=f_col, fill=col)
        y += col_lab_h
        for x, (lab, col, frame, cap_key) in zip(xs, labels):
            fr = frame if S == 1 else nn2(frame)
            paste_device(im, fr, x, y, bezel, SHEET_LINE)
            cap = CAPTIONS[cap_key][kind]
            cy = y + col_w + 10 * S
            for line in wrap_text(cap, f_cap, col_w, dr):
                dr.text((x, cy), line, font=f_cap, fill=SHEET_DIM)
                cy += 16 * S
        y += col_w + cap_h + row_gap

    # rule + footer
    fy = H - foot_h + 10 * S
    dr.line([margin, fy, W - margin, fy], fill=SHEET_RULE)
    fy += 12 * S
    for para in SHEET_FOOT:
        for line in wrap_text(para, f_foot, W - margin * 2, dr):
            dr.text((margin, fy), line, font=f_foot, fill=SHEET_MUTE)
            fy += 15 * S
    return im


OVERVIEW_MOMENTS = [
    "01-first-sound",
    "04-step",
    "05-save-playing",
    "07-live",
]

OVERVIEW_COL = [
    ("SHIPPED  94197fc8", "SHIPPED", SHEET_SHIP),
    ("A   Plain speech", "A", SHEET_INK),
    ("B   Live signals", "B", SHEET_AMB),
    ("C   Two rooms", "C", (186, 176, 140)),
]


def make_overview(all_frames, scale=1):
    S = scale
    margin = 56 * S
    gap = 36 * S
    bezel = 5 * S
    frame_s = 240 * S
    col_w = frame_s + bezel * 2
    n = len(OVERVIEW_COL)
    top = 160 * S
    row_title_h = 26 * S
    col_lab_h = 22 * S
    cap_h = 72 * S
    row_gap = 32 * S
    foot_h = 110 * S
    W = margin * 2 + n * col_w + (n - 1) * gap
    H = (top + col_lab_h + 8 * S
         + len(OVERVIEW_MOMENTS) * (row_title_h + col_w + cap_h + row_gap)
         + foot_h)

    im = Image.new("RGB", (W, H), SHEET_BG)
    dr = ImageDraw.Draw(im)
    f_kicker = sheet_font(13 * S, "Medium")
    f_title = sheet_font(30 * S, "Bold")
    f_deck = sheet_font(15 * S, "Medium")
    f_row = sheet_font(15 * S, "Bold")
    f_col = sheet_font(13 * S, "Bold")
    f_cap = sheet_font(12 * S, "Medium")
    f_foot = sheet_font(12 * S, "Medium")

    dr.text((margin, 26 * S), "DnBOX  ·  SMK-37", font=f_kicker, fill=SHEET_AMB)
    dr.text((margin, 44 * S), "Three directions, the same four moments", font=f_title, fill=SHEET_INK)
    deck = ("Frames are the real 240×240 screen. SHIPPED 94197fc8 is a reconstruction, "
            "not a device photograph. PROPOSED is not implemented, and was not verified on hardware.")
    dy = 88 * S
    for line in wrap_text(deck, f_deck, W - margin * 2, dr):
        dr.text((margin, dy), line, font=f_deck, fill=SHEET_DIM)
        dy += 20 * S
    note = ("Existing controls are unchanged. The kit overview is kept. "
            "A speaks in the firmware’s own words. B is one family of marks. C is two rooms.")
    for line in wrap_text(note, f_cap, W - margin * 2, dr):
        dr.text((margin, dy + 2 * S), line, font=f_cap, fill=SHEET_MUTE)
        dy += 16 * S

    # column heads at top of first row, also repeated? just first, aligned
    y = top
    for i, (lab, key, col) in enumerate(OVERVIEW_COL):
        x = margin + i * (col_w + gap)
        dr.text((x, y), lab, font=f_col, fill=col)
    y += col_lab_h + 8 * S
    for mi, kind in enumerate(OVERVIEW_MOMENTS):
        dr.text((margin, y), ROW_TITLE[kind], font=f_row, fill=SHEET_INK)
        y += row_title_h
        for i, (lab, key, col) in enumerate(OVERVIEW_COL):
            x = margin + i * (col_w + gap)
            fr = all_frames[key][kind]
            if S == 2:
                fr = nn2(fr)
            paste_device(im, fr, x, y, bezel, SHEET_LINE)
            cap = CAPTIONS[key][kind]
            cy = y + col_w + 8 * S
            for line in wrap_text(cap, f_cap, col_w, dr):
                dr.text((x, cy), line, font=f_cap, fill=SHEET_DIM)
                cy += 15 * S
        y += col_w + cap_h + row_gap

    fy = H - foot_h + 10 * S
    dr.line([margin, fy, W - margin, fy], fill=SHEET_RULE)
    fy += 12 * S
    for para in SHEET_FOOT:
        for line in wrap_text(para, f_foot, W - margin * 2, dr):
            dr.text((margin, fy), line, font=f_foot, fill=SHEET_MUTE)
            fy += 15 * S
    return im


# ---------------------------------------------------------------------------
# QA
# ---------------------------------------------------------------------------

def qa_frame(name, im, report):
    if im.size != (240, 240):
        report.append(f"FAIL {name}: size {im.size}, want 240x240")
        return
    # RGB565: every pixel must already be a legal expansion
    pix = im.getdata()
    bad = 0
    sample_bad = None
    for i, (r, g, b) in enumerate(pix):
        rr, gg, bb = rgb565(r, g, b)
        if (r, g, b) != (rr, gg, bb):
            bad += 1
            if sample_bad is None:
                sample_bad = ((i % 240, i // 240), (r, g, b), (rr, gg, bb))
    if bad:
        report.append(f"FAIL {name}: {bad} pixels not RGB565, e.g. {sample_bad}")
    # emptiness / all-black check
    colors = set(pix)
    if len(colors) < 4:
        report.append(f"WARN {name}: only {len(colors)} colours")


def header_overlap_check(name, im, report):
    """Flag a nearly-solid collision between left chrome and the pattern badge zone.
    Heuristic: we just record the rightmost non-base pixel in y=2..16, x=0..131.
    """
    # informational, not a fail
    pass


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def save_png(im, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    im.save(path)
    return path


def main():
    OUT_1X.mkdir(parents=True, exist_ok=True)
    OUT_2X.mkdir(parents=True, exist_ok=True)

    report = []
    shipped = {k: fn() for k, fn in SHIPPED.items()}
    proposed = {s: {k: _proposed(s, k) for k in KINDS} for s in ("A", "B", "C")}

    # Save every frame at 1x and 2x
    all_1x = {}
    for k, im in shipped.items():
        name = f"SHIPPED-{k}"
        save_png(im, OUT_1X / f"{name}.png")
        save_png(nn2(im), OUT_2X / f"{name}.png")
        qa_frame(name, im, report)
        all_1x[("SHIPPED", k)] = im
    for s in ("A", "B", "C"):
        for k, im in proposed[s].items():
            name = f"{s}-{k}"
            save_png(im, OUT_1X / f"{name}.png")
            save_png(nn2(im), OUT_2X / f"{name}.png")
            qa_frame(name, im, report)
            all_1x[(s, k)] = im

    # Direction sheets
    for s in ("A", "B", "C"):
        # SHIPPED dict uses the same keys
        sheet = make_direction_sheet(s, shipped, proposed[s], scale=1)
        sheet2 = make_direction_sheet(s, shipped, proposed[s], scale=2)
        ddir = OUT / s
        ddir.mkdir(parents=True, exist_ok=True)
        save_png(sheet, ddir / "sheet.png")
        save_png(sheet2, ddir / "sheet-2x.png")
        print(f"sheet {s}: {sheet.size}  2x {sheet2.size}")

    # Overview
    ov_frames = {"SHIPPED": shipped, "A": proposed["A"], "B": proposed["B"], "C": proposed["C"]}
    ov = make_overview(ov_frames, scale=1)
    ov2 = make_overview(ov_frames, scale=2)
    save_png(ov, OUT / "overview.png")
    save_png(ov2, OUT / "overview-2x.png")
    print(f"overview: {ov.size}  2x {ov2.size}")

    # QA contact of every 1x proposed + shipped (for visual review, not a deliverable)
    tmp = Path("/tmp/directions-qa")
    tmp.mkdir(exist_ok=True)
    kinds_show = SHEET_ROWS + ["05-save-toast"]
    cols = 1 + 3  # shipped + A B C
    pad, lh, gap = 10, 16, 8
    W = pad * 2 + cols * 240 + (cols - 1) * gap
    H = pad * 2 + len(kinds_show) * (240 + lh + 4 + gap)
    contact = Image.new("RGB", (W, H), (16, 18, 18))
    cdr = ImageDraw.Draw(contact)
    cf = sheet_font(11, "Medium")
    heads = [("SHIPPED", shipped), ("A", proposed["A"]), ("B", proposed["B"]), ("C", proposed["C"])]
    for r, kind in enumerate(kinds_show):
        for c, (lab, dct) in enumerate(heads):
            x = pad + c * (240 + gap)
            y = pad + r * (240 + lh + 4 + gap)
            cdr.text((x, y), f"{lab}  {kind}", font=cf, fill=(200, 205, 198))
            contact.paste(dct[kind], (x, y + lh))
    contact.save(tmp / "contact-1x.png")
    nn2(contact).save(tmp / "contact-1x-2x.png")

    # 6x header/footer strip for QA
    crops = []
    for r, kind in enumerate(kinds_show):
        for lab, dct in heads:
            imf = dct[kind]
            hdr = imf.crop((0, 0, 240, 20)).resize((240 * 5, 100), Image.Resampling.NEAREST)
            ftr = imf.crop((0, 227, 240, 240)).resize((240 * 5, 65), Image.Resampling.NEAREST)
            labim = Image.new("RGB", (240 * 5, 18), (24, 26, 26))
            ImageDraw.Draw(labim).text((4, 3), f"{lab}  {kind}", font=cf, fill=(210, 214, 208))
            col = Image.new("RGB", (240 * 5, 18 + 100 + 4 + 65), (8, 8, 8))
            col.paste(labim, (0, 0))
            col.paste(hdr, (0, 18))
            col.paste(ftr, (0, 18 + 100 + 4))
            crops.append(col)
    ch = sum(c.size[1] for c in crops)
    strip = Image.new("RGB", (240 * 5, ch), (8, 8, 8))
    yy = 0
    for c in crops:
        strip.paste(c, (0, yy))
        yy += c.size[1]
    strip.save(tmp / "headers-5x.png")

    print("QA report:")
    if report:
        for line in report:
            print(" ", line)
    else:
        print("  all frames 240x240, RGB565 clean")
    print("frames 1x:", len(list(OUT_1X.glob('*.png'))))
    print("wrote contact", tmp / "contact-1x.png")


if __name__ == "__main__":
    main()
