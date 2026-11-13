#!/usr/bin/env python3
"""SEQ chord-symbol namer: the final notation spec, host-side.

Display-only. Pitches and playback never change. Not a firmware patch.
Spelling matches put_note (ui.c 388–392, note_names.h: sharps only).
Device widths are packed Barlow advances from tools/gen_font.py (FONTSET=barlow),
the same tables felucca_font.h feeds text_w() / tw() on the device.
FONT_M folds a-z to A-Z (gfx.c 162–163).
"""
from __future__ import annotations

NOTE_N = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]

# Exact-match templates, interval bits relative to the candidate root.
# Tier 1 is required. Tier 2 is optional, in this order.
TIER1 = (
    (0x0091, ""),      # maj    {0,4,7}      bare root, e.g. C
    (0x0089, "min"),   # min    {0,3,7}
    (0x0081, "5"),     # 5      {0,7}
    (0x0491, "7"),     # 7      {0,4,7,10}
    (0x0891, "maj7"),  # maj7   {0,4,7,11}
    (0x0489, "min7"),  # min7   {0,3,7,10}
)
TIER2 = (
    (0x0049, "dim"),     # dim    {0,3,6}
    (0x00A1, "sus4"),    # sus4   {0,5,7}
    (0x0085, "sus2"),    # sus2   {0,2,7}
    (0x0111, "aug"),     # aug    {0,4,8}
    (0x0291, "6"),       # 6      {0,4,7,9}
    (0x0449, "min7b5"),  # min7b5 {0,3,6,10}
    (0x0249, "dim7"),    # dim7   {0,3,6,9}
)

NOTE_CELL_PX = 54
HELD_LINE_PX = 232  # x=4, 240-4-4

# Packed advances from a 2026-11-13 gen_font.py barlow run (FONT_*_ADV).
# FONT_M: first=32 last=95. FONT_S/B/XS: first=32 last=126.
_ADV = {
    "M": (32, 95, [
        5, 0, 0, 15, 0, 18, 0, 0, 0, 0, 0, 11, 0, 8, 6, 10,
        12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 7, 0, 0, 0, 0, 0,
        0, 13, 12, 12, 13, 12, 11, 12, 13, 6, 12, 13, 11, 14, 14, 13,
        12, 12, 12, 12, 12, 13, 12, 18, 12, 12, 11, 0, 0, 0, 0, 0,
    ]),
    "B": (32, 126, [
        3, 5, 5, 10, 8, 12, 10, 3, 5, 5, 6, 7, 4, 6, 4, 7,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 5, 4, 7, 7, 7, 7,
        12, 9, 8, 8, 8, 8, 7, 8, 8, 4, 8, 8, 7, 9, 9, 8,
        8, 8, 8, 8, 8, 8, 8, 12, 8, 8, 7, 6, 7, 6, 7, 7,
        7, 7, 8, 8, 8, 8, 7, 8, 8, 4, 8, 7, 7, 11, 7, 8,
        8, 8, 5, 7, 5, 8, 8, 12, 7, 8, 6, 7, 7, 7, 7,
    ]),
    "S": (32, 126, [
        3, 4, 4, 10, 7, 12, 9, 2, 4, 4, 6, 7, 3, 5, 4, 6,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 4, 4, 7, 7, 7, 7,
        12, 8, 8, 8, 8, 8, 7, 8, 8, 4, 8, 8, 7, 9, 9, 8,
        8, 8, 8, 8, 8, 8, 8, 11, 8, 8, 7, 5, 6, 5, 6, 7,
        7, 7, 7, 8, 7, 8, 7, 8, 8, 4, 8, 7, 7, 11, 7, 8,
        7, 8, 5, 7, 5, 8, 8, 11, 7, 8, 6, 7, 7, 7, 7,
    ]),
    "XS": (32, 126, [
        2, 4, 4, 8, 6, 10, 8, 2, 4, 4, 5, 6, 3, 4, 3, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 3, 6, 6, 6, 5,
        9, 7, 6, 6, 7, 6, 6, 6, 7, 3, 6, 7, 6, 8, 7, 7,
        6, 6, 6, 6, 6, 7, 6, 9, 6, 6, 6, 4, 5, 4, 5, 5,
        5, 6, 6, 6, 6, 6, 6, 6, 7, 3, 6, 6, 6, 9, 6, 7,
        6, 6, 4, 5, 4, 7, 6, 9, 6, 6, 5, 5, 5, 5, 5,
    ]),
}


def put_note(n: int) -> str:
    """ui.c put_note: NOTE_N[(n%12+12)%12] + (n/12-1). Sharps only, no flats."""
    n = int(n)
    return f"{NOTE_N[(n % 12 + 12) % 12]}{n // 12 - 1}"


def pc_of(n: int) -> int:
    return (int(n) % 12 + 12) % 12


def sounding_pitches(stored, transpose=24):
    """Effective-transpose map: MIDI' = stored + bp->transpose - 24 (sequencer.c 309, 358;
    already what the held line and NOTE cell pass to put_note, ui.c 5651, 8381).
    Does not change the store. Clamped 0..127 as on playback.
    """
    out = []
    for n in stored:
        p = int(n) + int(transpose) - 24
        p = 0 if p < 0 else 127 if p > 127 else p
        out.append(p)
    return out


def _glyph_index(role: str, ch: str) -> int:
    first, last, adv = _ADV[role]
    o = ord(ch)
    if "a" <= ch <= "z" and last < ord("a"):
        o -= 32
    if o < first or o > last:
        o = ord("?")
    return o - first


def device_tw(role: str, s: str) -> int:
    """Device text_w() / tw(), packed advances. FONT_M folds a-z to A-Z."""
    first, last, adv = _ADV[role]
    return sum(adv[_glyph_index(role, c)] for c in s)


def device_paint(role: str, s: str) -> str:
    """The glyphs FONT_* actually paint. FONT_M folds lowercase."""
    first, last, _ = _ADV[role]
    if last < ord("a"):
        return "".join(c.upper() if "a" <= c <= "z" else c for c in s)
    return s


def chord_symbol(notes, *, tier2: bool = True) -> str | None:
    """Name the sounding set (MIDI after effective transpose), low to high.

    Returns 'Root+suffix' or 'Root+suffix/Bass', or None (show notes).
    Never returns '?'. Octaves fold to a 12-bit pitch-class mask. The lowest
    sounding note is the bass. A bass-root exact match always wins. Otherwise
    a single non-bass exact match slashes; two or more, or none, return None.
    aug and dim7 match the bass first, so they never slash.
    """
    notes = [int(n) for n in notes]
    if len(notes) < 2:
        return None
    pcs = 0
    for n in notes:
        pcs |= 1 << pc_of(n)
    bass = pc_of(notes[0])
    templates = TIER1 + (TIER2 if tier2 else ())

    def suffix_for(root: int) -> str | None:
        rel = ((pcs >> root) | (pcs << (12 - root))) & 0xFFF
        for mask, suf in templates:
            if rel == mask:
                return suf
        return None

    suf = suffix_for(bass)
    if suf is not None:
        return NOTE_N[bass] + suf  # bass is the root: no slash

    fits: list[tuple[int, str]] = []
    for step in range(1, 12):
        root = (bass + step) % 12
        if not (pcs & (1 << root)):
            continue
        suf = suffix_for(root)
        if suf is not None:
            fits.append((root, suf))
    if len(fits) != 1:
        return None
    root, suf = fits[0]
    return NOTE_N[root] + suf + "/" + NOTE_N[bass]


def compact(notes) -> str:
    """C3x3: bass (with octave, put_note) plus the TOTAL sounding count."""
    notes = [int(n) for n in notes]
    if not notes:
        return ""
    if len(notes) == 1:
        return put_note(notes[0])
    return f"{put_note(notes[0])}x{len(notes)}"


def note_cell_label(notes, *, tier2: bool = True) -> tuple[str, str, str]:
    """(written symbol-or-compact, font role, painted glyphs).

    Recognised: the symbol, FONT_M then S then XS, then drop '/Bass' and
    retry those fonts. Never cut a name.
    Unrecognised, or an octave double of a single pitch class that is not a
    template (C4 C5): compact C4x2. Mono: C3. Compact itself is written
    C3x3 and paints C3X3 at FONT_M.
    """
    notes = [int(n) for n in notes]
    if len(notes) <= 1:
        s = compact(notes) or "—"
        return s, "M", device_paint("M", s)
    sym = chord_symbol(notes, tier2=tier2)
    if sym is None:
        s = compact(notes)
        return s, "M", device_paint("M", s)
    candidates = [sym]
    if "/" in sym:
        candidates.append(sym.split("/", 1)[0])
    for text in candidates:
        for role in ("M", "S", "XS"):
            if device_tw(role, text) <= NOTE_CELL_PX:
                return text, role, device_paint(role, text)
    # Every current template fits XS once '/Bass' is dropped. Never cut.
    text = candidates[-1]
    return text, "XS", device_paint("XS", text)


def held_line(step_n, notes, velocity=100, *, tier2=True, flags_suffix="",
              width_fn=None) -> tuple[str, str]:
    """'STEP n  <symbol>  <notes low to high>  V###' (+ TIE/ACC/SLIDE if present).

    Font by trying B, then S, then XS (more notes need the smaller face).
    If XS still overflows 232 px, the notes become C3xN and the symbol stays.
    """
    notes = [int(n) for n in notes]
    width_fn = width_fn or device_tw
    sym = chord_symbol(notes, tier2=tier2)
    names = " ".join(put_note(n) for n in notes)
    extra = flags_suffix
    vel = f"V{int(velocity)}"

    def build(name_s: str) -> str:
        mid = f"{sym}  {name_s}" if sym else name_s
        tail = f"{extra} {vel}" if extra else vel
        return f"STEP {int(step_n)}  {mid}  {tail}"

    text = build(names)
    for role in ("B", "S", "XS"):
        if width_fn(role, text) <= HELD_LINE_PX:
            return text, role
    text = build(compact(notes))
    for role in ("B", "S", "XS"):
        if width_fn(role, text) <= HELD_LINE_PX:
            return text, role
    return text, "XS"


# ---------------------------------------------------------------------------
# Acceptance test set. Pitches and playback are unchanged in every row.
# ---------------------------------------------------------------------------

def _midi(*names_or_nums):
    """'C3' / 'Eb3' / 48 → MIDI. Flats in a test prompt are input aliases
    only; output spelling is always put_note (sharps)."""
    acc = {"": 0, "#": 1, "b": -1, "bb": -2}
    out = []
    for x in names_or_nums:
        if isinstance(x, int):
            out.append(x)
            continue
        s = str(x)
        i = 1
        if i < len(s) and s[i] in "#b":
            i = 2 if not (s[i] == "b" and i + 1 < len(s) and s[i + 1] == "b") else 3
        letter = s[0]
        a = s[1:i]
        octv = int(s[i:])
        pc = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}[letter] + acc[a]
        out.append(12 * (octv + 1) + pc)
    return out


# (id, stored MIDI, transpose, tier2, expected symbol or None,
#  expected NOTE written, expected notes-spelling, source, pitches_unchanged)
TEST_SET = [
    ("maj-root",        _midi("C3", "E3", "G3"), 24, True,  "C",      "C",      "C3 E3 G3",     "recorded: before-triad / before-selected-chord, notes 48,52,55", True),
    ("maj-inv",         _midi("E3", "G3", "C4"), 24, True,  "C/E",    "C/E",    "E3 G3 C4",     "EXAMPLE SET (inversion)", True),
    ("min7-flats-in",   _midi("C3", "Eb3", "G3", "Bb3"), 24, True, "Cmin7", "Cmin7", "C3 D#3 G3 A#3", "EXAMPLE SET (C Eb G Bb in, put_note sharps out)", True),
    ("maj-spread",      _midi("C3", "G3", "C4", "E4"), 24, True, "C",    "C",      "C3 G3 C4 E4", "EXAMPLE SET (spread / octave doubles of a maj)", True),
    ("power",           _midi("C3", "G3"),       24, True,  "C5",     "C5",     "C3 G3",        "EXAMPLE SET (power chord)", True),
    ("CEGA-t2",         _midi("C3", "E3", "G3", "A3"), 24, True,  "C6",     "C6",     "C3 E3 G3 A3", "EXAMPLE SET (C E G A, Tier 2 on)", True),
    ("CEGA-t1",         _midi("C3", "E3", "G3", "A3"), 24, False, "Amin7/C","Amin7/C","C3 E3 G3 A3", "EXAMPLE SET (C E G A, Tier 1 only)", True),
    ("cluster",         _midi("C3", "C#3", "D3"), 24, True, None,    "C3x3",   "C3 C#3 D3",    "EXAMPLE SET (cluster)", True),
    ("oct-double",      _midi("C4", "C5"),       24, True,  None,    "C4x2",   "C4 C5",        "EXAMPLE SET (octave double)", True),
    ("mono",            _midi("C3"),             24, True,  None,    "C3",     "C3",           "recorded: before-mono-control, note 48", True),
    # Extra rows that lock the rules, still 0 wrong symbols.
    ("maj7-recorded",   _midi("C3", "E3", "G3", "B3"), 24, True, "Cmaj7", "Cmaj7", "C3 E3 G3 B3", "recorded: before-tetrad, notes 48,52,55,59", True),
    ("hex-recorded",    _midi("C3", "E3", "G3", "B3", "D4", "F4"), 24, True, None, "C3x6", "C3 E3 G3 B3 D4 F4", "recorded: before-hexachord, notes 48,52,55,59,62,65", True),
    ("min",             _midi("C3", "Eb3", "G3"), 24, True, "Cmin", "Cmin", "C3 D#3 G3", "EXAMPLE SET", True),
    ("dom7",            _midi("C3", "E3", "G3", "Bb3"), 24, True, "C7", "C7", "C3 E3 G3 A#3", "EXAMPLE SET", True),
    ("aug-root",        _midi("C3", "E3", "G#3"), 24, True, "Caug", "Caug", "C3 E3 G#3", "EXAMPLE SET (aug, bass is root, no slash)", True),
    ("aug-inv",         _midi("E3", "G#3", "C4"), 24, True, "Eaug", "Eaug", "E3 G#3 C4", "EXAMPLE SET (aug takes the bass as root, no slash)", True),
    ("dim7-root",       _midi("C3", "D#3", "F#3", "A3"), 24, True, "Cdim7", "Cdim7", "C3 D#3 F#3 A3", "EXAMPLE SET (dim7, bass is root, no slash)", True),
    ("min7b5-slash",    _midi("A#2", "C3", "D#3", "F#3"), 24, True, "Cmin7b5/A#", "Cmin7b5", "A#2 C3 D#3 F#3", "EXAMPLE SET (3rd inversion; NOTE drops /Bass)", True),
    ("gs-seven",        _midi("G#2", "C3", "D#3", "F#3"), 24, True, "G#7", "G#7", "G#2 C3 D#3 F#3", "EXAMPLE SET (G# C D# F# is G#7, not Cmin7b5/G#)", True),
    ("longest-produced",_midi("G#2", "A#2", "C#3", "E3"), 24, True, "A#min7b5/G#", "A#min7b5", "G#2 A#2 C#3 E3", "EXAMPLE SET (longest produced slash; NOTE drops /Bass, FONT_XS)", True),
    ("sus-ambiguous",   _midi("D3", "G3", "C4"), 24, True, None, "D3x3", "D3 G3 C4", "EXAMPLE SET (bass D: Csus2 and Gsus4 both fit, notes only)", True),
    ("dyad-third",      _midi("C3", "E3"), 24, True, None, "C3x2", "C3 E3", "recorded: before-dyad, notes 48,52 (major third, not a template)", True),
    ("transpose-plus12", _midi("C3", "E3", "G3"), 36, True, "C", "C", "C4 E4 G4", "EXAMPLE SET (bp->transpose=36, sounding is +12; store unchanged)", True),
    # Independent review of §9 (T20–T37, clamp). Same rules, extra edge cases.
    ("T20-sus2",        _midi("C3", "D3", "G3"), 24, True,  "Csus2",  "Csus2",  "C3 D3 G3",    "review T20 (bass-root sus2)", True),
    ("T21-sus4",        _midi("G3", "C4", "D4"), 24, True,  "Gsus4",  "Gsus4",  "G3 C4 D4",    "review T21 (bass-root sus4)", True),
    ("T22-sus-amb",     _midi("G3", "C4", "F4"), 24, True,  None,     "G3x3",   "G3 C4 F4",    "review T22 (two non-bass sus readings: Csus4/G and Fsus2/G → notes)", True),
    ("T23-amin7",       _midi("A2", "C3", "E3", "G3"), 24, True, "Amin7", "Amin7", "A2 C3 E3 G3", "review T23 (bass-root min7)", True),
    ("T24-CEGA-inv-t2", _midi("E3", "G3", "A3", "C4"), 24, True,  None,     "E3x4",   "E3 G3 A3 C4", "review T24 (Tier 2: C6/E and Amin7/E, notes, not first-match)", True),
    ("T25-CEGA-inv-t1", _midi("E3", "G3", "A3", "C4"), 24, False, "Amin7/E","Amin7/E","E3 G3 A3 C4", "review T25 (Tier 1 only: unique slash Amin7/E). T1 and T2 can differ.", True),
    ("T26-CEGA-g",      _midi("G3", "A3", "C4", "E4"), 24, True,  None,     "G3x4",   "G3 A3 C4 E4", "review T26 (Tier 2: Amin7/G and C6/G, notes)", True),
    ("T27-dim",         _midi("C3", "D#3", "F#3"), 24, True, "Cdim",   "Cdim",   "C3 D#3 F#3",  "review T27", True),
    ("T28-dim-inv",     _midi("D#3", "F#3", "C4"), 24, True, "Cdim/D#","Cdim/D#","D#3 F#3 C4", "review T28 (dim inversion slashes; not symmetric)", True),
    ("T29-dim7",        _midi("C3", "D#3", "F#3", "A3"), 24, True, "Cdim7", "Cdim7", "C3 D#3 F#3 A3", "review T29 (also dim7-root)", True),
    ("T30-dim7-inv",    _midi("D#3", "F#3", "A3", "C4"), 24, True, "D#dim7","D#dim7","D#3 F#3 A3 C4", "review T30 (dim7 takes the bass, no slash; symbol is not identical under inversion)", True),
    ("T31-aug-gs",      _midi("G#2", "C3", "E3"), 24, True, "G#aug",  "G#aug",  "G#2 C3 E3",   "review T31 (aug takes the bass, no slash)", True),
    ("T32-min7b5",      _midi("C3", "D#3", "F#3", "A#3"), 24, True, "Cmin7b5","Cmin7b5","C3 D#3 F#3 A#3", "review T32", True),
    ("T33-min7b5-inv",  _midi("D#3", "F#3", "A#3", "C4"), 24, True, "Cmin7b5/D#","Cmin7b5","D#3 F#3 A#3 C4", "review T33 (NOTE drops /D#; held line keeps the full slash)", True),
    ("T34-c5g",         _midi("G2", "C3"), 24, False, "C5/G", "C5/G", "G2 C3", "review T34 (Tier 1; inverted power chord). Same with Tier 2.", True),
    ("T35-foreign",     _midi("D3", "C4", "E4", "G4"), 24, True, None, "D3x4", "D3 C4 E4 G4", "review T35 (foreign bass under a C major; T1+T2 exact-match misses)", True),
    ("T37-trans-23",    _midi("C3", "E3", "G3"), 23, True, "B", "B", "B2 D#3 F#3", "review T37 (transpose 23: sounding B2 D#3 F#3 = B; store unchanged)", True),
    ("F6-clamp-merge",  [103, 107, 110], 48, True, None, "G9x3", "G9 G9 G9",
     "review F6: stored 103,107,110 (G7 B7 D8, a G major shape) at transpose=48 clamps to 127,127,127. One pitch class, not a major symbol. §9 xN counts clamped events (G9x3).", True),
]


def run_tests() -> list[str]:
    errors = []
    for row in TEST_SET:
        rid, stored, trans, t2, exp_sym, exp_note, exp_names, _src, unchanged = row
        sounding = sounding_pitches(stored, trans)
        if unchanged and sounding == list(stored) and trans == 24:
            pass  # identity map, store == sounding
        # Pitches: the namer must not rewrite MIDI values.
        if sounding_pitches(stored, trans) != sounding:
            errors.append(f"{rid}: namer mutated sounding pitches")
        got_sym = chord_symbol(sounding, tier2=t2)
        if got_sym != exp_sym:
            errors.append(f"{rid}: symbol {got_sym!r} != {exp_sym!r}")
        written, role, painted = note_cell_label(sounding, tier2=t2)
        if written != exp_note:
            errors.append(f"{rid}: NOTE written {written!r} != {exp_note!r} (font {role}, paint {painted!r})")
        names = " ".join(put_note(n) for n in sounding)
        if names != exp_names:
            errors.append(f"{rid}: notes {names!r} != {exp_names!r}")
        if got_sym is not None and "?" in got_sym:
            errors.append(f"{rid}: '?' in symbol")
        # Playback never changes: stored in, stored still the same object values.
        if list(stored) != list(row[1]):
            errors.append(f"{rid}: stored pitches changed")
    # Longest-name widths, device packed.
    longest = "Cmin7b5/G#"
    for role, expect in (("M", 119), ("S", 75), ("XS", 61), ("B", 79)):
        w = device_tw(role, longest)
        if w != expect:
            errors.append(f"width {role} {longest} = {w}, expected {expect}")
    # F8: every NOTE cell in the harness fits 54 px (no clip). 51 px
    # fallbacks have 3 px spare; still want a real-framebuffer check.
    for row in TEST_SET:
        rid, stored, trans, t2, exp_sym, exp_note, *_ = row
        sounding = sounding_pitches(stored, trans)
        written, role, painted = note_cell_label(sounding, tier2=t2)
        w = device_tw(role, written)
        if w > NOTE_CELL_PX:
            errors.append(f"{rid}: NOTE {written!r} at FONT_{role} is {w} px > 54 (would clip)")
        got = chord_symbol(sounding, tier2=t2)
        if got and "/" in got:
            line, _lf = held_line(1, sounding, tier2=t2)
            if got not in line:
                errors.append(f"{rid}: held line dropped the full slash {got!r}: {line}")
    errors.extend(run_tests_errata())
    return errors




# ===========================================================================
# §10  Looking over ambiguous and unnamed sets.
# Add-on. chord_symbol / note_cell_label / held_line and TEST_SET are unchanged.
# Pitches and playback never change.
# ===========================================================================

# Exact-match extensions, same algorithm as TIER1/TIER2. Data-only if the
# matcher loop bound is a constant. min+9 is compact min(add9). 69 not 6/9
# (a digit after '/' is not a bass, but 69 avoids any slash-form lookalike).
TIER3 = (
    (0x0015, "add2"),     # {0,2,4}
    (0x0095, "add9"),     # {0,2,4,7}
    (0x008D, "min+9"),    # {0,2,3,7}
    (0x04A1, "7sus4"),    # {0,5,7,10}
    (0x0495, "9"),        # {0,2,4,7,10}
    (0x048D, "min9"),     # {0,2,3,7,10}
    (0x0895, "maj9"),     # {0,2,4,7,11}
    (0x04B5, "11"),       # {0,2,4,5,7,10}
    (0x04AD, "min11"),    # {0,2,3,5,7,10}
    (0x08B5, "maj11"),    # {0,2,4,5,7,11}
    (0x0295, "69"),       # {0,2,4,7,9}
    (0x0889, "minmaj7"),  # {0,3,7,11}
)

# Scale degrees from a root, sharps for the tritone and the aug 5th (put_note
# is sharps only). Used in C(b2 2)-style shorthand.
DEGREE = {
    1: "b2", 2: "2", 3: "b3", 4: "3", 5: "4", 6: "#4",
    7: "5", 8: "#5", 9: "6", 10: "b7", 11: "7",
}

# NOTE-cell last-resort shorts, only when the official suffix overflows XS
# even after '/Bass' is dropped. The held line always uses the official name.
NOTE_SHORT = {"minmaj7": "minM7"}

# Symmetric qualities: every rotation is the same name. A second reading
# would only repeat the chord (Caug / Eaug/C). Do not show one.
SYMMETRIC_SUF = frozenset({"aug", "dim7"})

STYLES = ("A", "B", "C", "D", "R")


def _pcs_bass(notes):
    notes = [int(n) for n in notes]
    pcs = 0
    for n in notes:
        pcs |= 1 << pc_of(n)
    bass = pc_of(notes[0]) if notes else 0
    return notes, pcs, bass, bin(pcs).count("1")


def _templates(*, tier2=True, tier3=False):
    t = TIER1
    if tier2:
        t = t + TIER2
    if tier3:
        t = t + TIER3
    return t


def _suffix_for(pcs, root, templates):
    rel = ((pcs >> root) | (pcs << (12 - root))) & 0xFFF
    for mask, suf in templates:
        if rel == mask:
            return suf
    return None


def _sym(root, suf, bass):
    s = NOTE_N[root] + suf
    if root != bass:
        s += "/" + NOTE_N[bass]
    return s


def exact_walk(notes, templates):
    """Exact matches, bass first then upward. (root, suffix, is_bass)."""
    notes, pcs, bass, _ = _pcs_bass(notes)
    out = []
    if not notes:
        return out, pcs, bass
    for step in range(0, 12):
        root = (bass + step) % 12
        if not (pcs & (1 << root)):
            continue
        suf = _suffix_for(pcs, root, templates)
        if suf is not None:
            out.append((root, suf, root == bass))
    return out, pcs, bass


def interval_symbol(notes) -> str | None:
    """Bass-rooted degree shorthand: 'C(3)', 'C(b2 2)', 'C(3 5 7 9 11)'.

    One pitch class and two or more sounding notes (octave double): 'Coct'.
    A single note: None (the NOTE cell stays 'C3').
    """
    notes, pcs, bass, n_pc = _pcs_bass(notes)
    if not notes:
        return None
    if n_pc == 1:
        return (NOTE_N[bass] + "oct") if len(notes) >= 2 else None
    degs = sorted({(pc_of(n) - bass) % 12 for n in notes} - {0})
    body = " ".join(DEGREE[d] for d in degs)
    return NOTE_N[bass] + "(" + body + ")"


def _needs_safe_font(s: str) -> bool:
    """FONT_M must not paint this: it folds b→B (b2 becomes B2, a note) and
    has 0-advance for '(' and ')' (the parens vanish)."""
    if any(c in s for c in "()"):
        return True
    for i, c in enumerate(s):
        if c == "b" and i + 1 < len(s) and s[i + 1].isdigit():
            return True
    return False


def truncate_interval(sym: str, max_px: int = NOTE_CELL_PX) -> tuple[str, str]:
    """Fit a C(…) shorthand into max_px, FONT_S then XS, then '+N' trim."""
    roles = ("S", "XS")
    for role in roles:
        if device_tw(role, sym) <= max_px:
            return sym, role
    open_p = sym.find("(")
    close_p = sym.rfind(")")
    if open_p < 0 or close_p < 0:
        return sym, "XS"
    root, body = sym[:open_p], sym[open_p + 1:close_p]
    degs = body.split()
    for keep in range(len(degs) - 1, 0, -1):
        text = f"{root}({' '.join(degs[:keep])} +{len(degs) - keep})"
        for role in roles:
            if device_tw(role, text) <= max_px:
                return text, role
    text = f"{root}(+{len(degs)})"
    return text, "XS"


def nearest_tagged(notes, templates) -> str | None:
    """Unique pure extension from the bass: a template plus extra pitch
    classes, no omissions. 'C7+D#'. Two equally near templates: None
    (do not guess). Other roots are not tried (a slashed nearest is a
    guess)."""
    notes, pcs, bass, _ = _pcs_bass(notes)
    if not notes:
        return None
    rel = ((pcs >> bass) | (pcs << (12 - bass))) & 0xFFF
    cands = []
    for mask, suf in templates:
        extra = rel & ~mask
        miss = mask & ~rel
        if miss == 0 and extra != 0:
            cands.append((bin(extra).count("1"), suf, extra))
    if not cands:
        return None
    cands.sort()
    best = cands[0][0]
    tops = [c for c in cands if c[0] == best]
    if len(tops) != 1:
        return None
    _, suf, extra = tops[0]
    bits = [NOTE_N[(bass + i) % 12] for i in range(1, 12) if extra & (1 << i)]
    return NOTE_N[bass] + suf + "".join("+" + b for b in bits)


def readings(notes, *, style="R", tier2=True):
    """(primary, second, kind) for a §10 / §10r style.

    kind is 'mono' | 'unique' | 'dual' | 'amb' | 'fallback'.
    primary is a symbol (no octave) or None (caller uses the notes / compact).
    second is a different-quality exact reading, or None.

    Styles:
      A  §9 exact; dual/amb show a second reading; unrecognised stays None.
      B  §9 + TIER3 exact, nearest-tag, then interval; amb is first upward,
         no second reading. SUPERSEDED interval path; kept for §10 tests.
      C  §9 exact kept; everything else is interval shorthand; no second.
         SUPERSEDED interval path; kept for §10 tests.
      D  like A, plus a ':' marker is applied later on the NOTE cell.
      R  §10r (current): exact T1/T2/T3 name only, unique slash when the
         root differs from the bass, otherwise no name. No interval form,
         no nearest-tag, no second reading, no forced name on a true amb.
    """
    if style not in STYLES:
        raise ValueError(style)
    notes = [int(n) for n in notes]
    # One distinct MIDI after clamp (F6: 127,127,127) is a single sounding
    # pitch, not an octave double and not a chord.
    if len(set(notes)) <= 1:
        return None, None, "mono"

    use_t3 = style in ("B", "R")
    tmpls = _templates(tier2=tier2, tier3=use_t3)
    walk, pcs, bass = exact_walk(notes, tmpls)
    bass_hits = [w for w in walk if w[2]]
    other_hits = [w for w in walk if not w[2]]
    # §10r (R) never shows a second reading. A/D still do (superseded §10).
    show_second = style in ("A", "D")

    def different(src, others):
        if src[1] in SYMMETRIC_SUF:
            return []
        return [o for o in others if o[1] != src[1] and o[1] not in SYMMETRIC_SUF]

    if bass_hits:
        pri = _sym(*bass_hits[0][:2], bass)
        diffs = different(bass_hits[0], other_hits) if show_second else []
        sec = _sym(*diffs[0][:2], bass) if diffs else None
        return pri, sec, ("dual" if sec else "unique")

    if len(other_hits) == 1:
        return _sym(*other_hits[0][:2], bass), None, "unique"

    if len(other_hits) >= 2:
        if style == "R":
            # Truly uncertain: no forced name, just the notes.
            return None, None, "amb"
        if style == "C":
            return interval_symbol(notes), None, "fallback"
        pri = _sym(*other_hits[0][:2], bass)
        sec = _sym(*other_hits[1][:2], bass) if show_second else None
        return pri, sec, "amb"

    # No exact match.
    if style in ("A", "D", "R"):
        return None, None, "fallback"
    if style == "C":
        return interval_symbol(notes), None, "fallback"
    # Style B (superseded §10): nearest-tag, then interval.
    tagged = nearest_tagged(notes, tmpls)
    if tagged:
        return tagged, None, "fallback"
    return interval_symbol(notes), None, "fallback"


def _note_candidates(pri: str, *, style: str, second: str | None) -> list[str]:
    """Ordered NOTE-cell candidates: never a mid-glyph cut."""
    out = []
    def add(s):
        if s and s not in out:
            out.append(s)
    if style == "D" and second:
        add(pri + ":")
    add(pri)
    if "/" in pri:
        head = pri.split("/", 1)[0]
        if style == "D" and second:
            add(head + ":")
        add(head)
    for suf, short in NOTE_SHORT.items():
        if suf in pri:
            shortened = pri.replace(suf, short, 1)
            add(shortened)
            if "/" in shortened:
                add(shortened.split("/", 1)[0])
    return out


def _roles_for(text: str):
    return ("S", "XS") if _needs_safe_font(text) else ("M", "S", "XS")


def note_cell_10(notes, *, style="R", tier2=True):
    """(written, role, painted, primary, second, kind)."""
    notes = [int(n) for n in notes]
    pri, sec, kind = readings(notes, style=style, tier2=tier2)
    if pri is None:
        if kind == "mono" and notes:
            s = put_note(notes[0])
            return s, "M", device_paint("M", s), pri, sec, kind
        # §10r (style R): show the actual notes when they fit the 54 px
        # cell, then compact (C3xN) as last-resort overflow. Never an
        # interval form. Superseded A/D keep the compact NOTE dump.
        if style == "R":
            names = " ".join(put_note(n) for n in notes)
            if names:
                for role in ("M", "S", "XS"):
                    if device_tw(role, names) <= NOTE_CELL_PX:
                        return names, role, device_paint(role, names), pri, sec, kind
        s = compact(notes) or "—"
        for role in ("M", "S", "XS"):
            if device_tw(role, s) <= NOTE_CELL_PX:
                return s, role, device_paint(role, s), pri, sec, kind
        return s, "XS", device_paint("XS", s), pri, sec, kind
    for text in _note_candidates(pri, style=style, second=sec):
        for role in _roles_for(text):
            if device_tw(role, text) <= NOTE_CELL_PX:
                return text, role, device_paint(role, text), pri, sec, kind
    if pri and "(" in pri:
        text, role = truncate_interval(pri)
        return text, role, device_paint(role, text), pri, sec, kind
    text = _note_candidates(pri, style=style, second=sec)[-1]
    return text, "XS", device_paint("XS", text), pri, sec, kind


def held_line_10(step_n, notes, velocity=100, *, style="R", tier2=True,
                 flags_suffix="", width_fn=None):
    """'STEP n  <primary>  [<second>]  <notes>  V###'.

    Returns (text, role, parts) where parts is
    {prefix, main, second, notes, tail}. The painter puts `second` in
    C_DIM and the rest in the main held-line colour. Font B → S → XS.
    Overflow drops the notes to compact, then drops the notes entirely;
    the second reading is never dropped.
    """
    notes = [int(n) for n in notes]
    width_fn = width_fn or device_tw
    pri, sec, kind = readings(notes, style=style, tier2=tier2)
    names = " ".join(put_note(n) for n in notes)
    extra = flags_suffix
    vel = f"V{int(velocity)}"
    tail = f"{extra} {vel}" if extra else vel
    prefix = f"STEP {int(step_n)}  "

    def build(name_s, include_notes=True):
        mid = []
        if pri:
            mid.append(pri)
        if sec:
            mid.append(sec)
        if include_notes and name_s:
            mid.append(name_s)
        body = "  ".join(mid)
        text = prefix + (f"{body}  {tail}" if body else tail)
        parts = {
            "prefix": prefix,
            "main": pri or "",
            "second": sec or "",
            "notes": (name_s if include_notes else ""),
            "tail": tail,
            "kind": kind,
        }
        return text, parts

    # Notes stay present: full names, then compact. Never drop to an
    # empty note field (superseded §10 dropped notes to keep a second).
    attempts = [
        (names, True),
        (compact(notes), True),
    ]
    last_text, last_parts, last_role = None, None, "XS"
    for name_s, inc in attempts:
        text, parts = build(name_s, inc)
        for role in ("B", "S", "XS"):
            # A line that needs a safe font (parens, flat degrees) can
            # still use B: FONT_B has lowercase and parentheses.
            if width_fn(role, text) <= HELD_LINE_PX:
                return text, role, parts
        last_text, last_parts = text, parts
    return last_text, last_role, last_parts


# ---------------------------------------------------------------------------
# Enumeration (step 1). Bass-fixed = bass at C, 2–6 distinct pitch classes.
# ---------------------------------------------------------------------------

def enumerate_sets(*, tier2=True, tier3=False):
    from itertools import combinations
    templates = _templates(tier2=tier2, tier3=tier3)
    counts = {"bass-root": 0, "slash": 0, "ambiguous": 0, "none": 0, "total": 0}
    by_k = {k: dict(counts) for k in range(2, 7)}
    amb_shapes = []
    for k in range(2, 7):
        for others in combinations(range(1, 12), k - 1):
            pcs = 1
            for o in others:
                pcs |= 1 << o
            notes = [60 + i for i in range(12) if pcs & (1 << i)]  # bass C, mid
            walk, _, bass = exact_walk(notes, templates)
            bass_hits = [w for w in walk if w[2]]
            other_hits = [w for w in walk if not w[2]]
            if bass_hits:
                kind = "bass-root"
            elif len(other_hits) == 1:
                kind = "slash"
            elif len(other_hits) >= 2:
                kind = "ambiguous"
                amb_shapes.append((
                    tuple(i for i in range(12) if pcs & (1 << i)),
                    [_sym(r, s, 0) for r, s, _ in other_hits],
                ))
            else:
                kind = "none"
            counts[kind] += 1
            counts["total"] += 1
            by_k[k][kind] += 1
            by_k[k]["total"] += 1
    return counts, by_k, amb_shapes


# ---------------------------------------------------------------------------
# §10 acceptance tests. §9 TEST_SET is untouched and still runs first.
# ---------------------------------------------------------------------------

# (id, stored, trans, tier2, style,
#  exp_pri, exp_sec, exp_note, exp_names, source, pitches_unchanged)
TEST_SET_10 = [
    # §9 unique names are unchanged under every style (add-on, not a fork).
    ("r-maj-root",     _midi("C3", "E3", "G3"), 24, True, "R", "C", None, "C", "C3 E3 G3",
     "recorded: before-triad / before-selected-chord, notes 48,52,55", True),
    ("r-maj-inv",      _midi("E3", "G3", "C4"), 24, True, "R", "C/E", None, "C/E", "E3 G3 C4",
     "EXAMPLE SET (inversion, not ambiguous)", True),
    ("r-min7",         _midi("C3", "Eb3", "G3", "Bb3"), 24, True, "R", "Cmin7", None, "Cmin7", "C3 D#3 G3 A#3",
     "EXAMPLE SET (Cmin7 bass-root exact; §10r shows no second D#6/C)", True),
    ("r-spread",       _midi("C3", "G3", "C4", "E4"), 24, True, "R", "C", None, "C", "C3 G3 C4 E4",
     "EXAMPLE SET (spread maj, doubles fold)", True),
    ("r-power",        _midi("C3", "G3"), 24, True, "R", "C5", None, "C5", "C3 G3",
     "EXAMPLE SET (power chord, Tier 1)", True),
    ("r-CEGA-t2",      _midi("C3", "E3", "G3", "A3"), 24, True, "R", "C6", None, "C6", "C3 E3 G3 A3",
     "EXAMPLE SET (C6 bass-root exact; §10r shows no second Amin7/C)", True),
    ("r-CEGA-t1",      _midi("C3", "E3", "G3", "A3"), 24, False, "R", "Amin7/C", None, "Amin7/C", "C3 E3 G3 A3",
     "EXAMPLE SET (Tier 1 only: 6 is absent, unique slash, no second)", True),
    ("r-mono",         _midi("C3"), 24, True, "R", None, None, "C3", "C3",
     "recorded: before-mono-control, note 48. Mono rule unchanged.", True),
    ("r-maj7",         _midi("C3", "E3", "G3", "B3"), 24, True, "R", "Cmaj7", None, "Cmaj7", "C3 E3 G3 B3",
     "recorded: before-tetrad, notes 48,52,55,59", True),
    ("r-aug",          _midi("E3", "G#3", "C4"), 24, True, "R", "Eaug", None, "Eaug", "E3 G#3 C4",
     "EXAMPLE SET (symmetric: no second Eaug/C-style reading)", True),
    ("r-min7b5-slash", _midi("A#2", "C3", "D#3", "F#3"), 24, True, "R", "Cmin7b5/A#", None, "Cmin7b5",
     "A#2 C3 D#3 F#3", "EXAMPLE SET (unique slash, NOTE drops /Bass)", True),
    ("r-gs-seven",     _midi("G#2", "C3", "D#3", "F#3"), 24, True, "R", "G#7", None, "G#7", "G#2 C3 D#3 F#3",
     "EXAMPLE SET (G#7, not Cmin7b5/G#)", True),
    ("r-longest",      _midi("G#2", "A#2", "C#3", "E3"), 24, True, "R", "A#min7b5/G#", None, "A#min7b5",
     "G#2 A#2 C#3 E3", "EXAMPLE SET (longest produced §9 slash)", True),
    ("r-transpose",    _midi("C3", "E3", "G3"), 36, True, "R", "C", None, "C", "C4 E4 G4",
     "EXAMPLE SET (bp->transpose=36; store unchanged)", True),
    # The two §9 dumps the owner does not like, recommended names:
    ("r-cluster",      _midi("C3", "C#3", "D3"), 24, True, "R", None, None, "C3 C#3 D3", "C3 C#3 D3",
     "EXAMPLE SET (cluster). §10r: no interval form; NOTE shows the actual notes (FONT_XS).", True),
    ("r-dyad",         _midi("C3", "E3"), 24, True, "R", None, None, "C3 E3", "C3 E3",
     "recorded: before-dyad, notes 48,52. §10r: no C(3); NOTE is the actual notes.", True),
    ("r-hex",          _midi("C3", "E3", "G3", "B3", "D4", "F4"), 24, True, "R", "Cmaj11", None, "Cmaj11",
     "C3 E3 G3 B3 D4 F4", "recorded: before-hexachord. TIER3 exact.", True),
    ("r-sus-amb",      _midi("D3", "G3", "C4"), 24, True, "R", None, None, "D3 G3 C4", "D3 G3 C4",
     "EXAMPLE SET (true amb). §10r: no forced Gsus4/D; just the notes.", True),
    ("r-oct",          _midi("C4", "C5"), 24, True, "R", None, None, "C4 C5", "C4 C5",
     "EXAMPLE SET (octave double). §10r: the actual notes, not Coct.", True),
    # Owner-listed common unnamed sets, now named:
    ("r-add9",         _midi("C3", "D3", "E3", "G3"), 24, True, "R", "Cadd9", None, "Cadd9", "C3 D3 E3 G3",
     "EXAMPLE SET (add9)", True),
    ("r-add2",         _midi("C3", "D3", "E3"), 24, True, "R", "Cadd2", None, "Cadd2", "C3 D3 E3",
     "EXAMPLE SET (add2, no fifth)", True),
    ("r-7sus4",        _midi("C3", "F3", "G3", "Bb3"), 24, True, "R", "C7sus4", None, "C7sus4", "C3 F3 G3 A#3",
     "EXAMPLE SET (7sus4)", True),
    ("r-9",            _midi("C3", "D3", "E3", "G3", "Bb3"), 24, True, "R", "C9", None, "C9", "C3 D3 E3 G3 A#3",
     "EXAMPLE SET (dominant 9)", True),
    ("r-min9",         _midi("C3", "D3", "Eb3", "G3", "Bb3"), 24, True, "R", "Cmin9", None, "Cmin9", "C3 D3 D#3 G3 A#3",
     "EXAMPLE SET (min9)", True),
    ("r-11",           _midi("C3", "D3", "E3", "F3", "G3", "Bb3"), 24, True, "R", "C11", None, "C11",
     "C3 D3 E3 F3 G3 A#3", "EXAMPLE SET (dominant 11)", True),
    ("r-69",           _midi("C3", "D3", "E3", "G3", "A3"), 24, True, "R", "C69", None, "C69", "C3 D3 E3 G3 A3",
     "EXAMPLE SET (6/9 written C69, no slash)", True),
    ("r-minmaj7",      _midi("C3", "Eb3", "G3", "B3"), 24, True, "R", "Cminmaj7", None, "Cminmaj7", "C3 D#3 G3 B3",
     "EXAMPLE SET (min(maj7); NOTE fits XS)", True),
    ("r-min+9",        _midi("C3", "D3", "Eb3", "G3"), 24, True, "R", "Cmin+9", None, "Cmin+9", "C3 D3 D#3 G3",
     "EXAMPLE SET (min(add9), compact min+9)", True),
    ("r-maj9",         _midi("C3", "D3", "E3", "G3", "B3"), 24, True, "R", "Cmaj9", None, "Cmaj9", "C3 D3 E3 G3 B3",
     "EXAMPLE SET (maj9, TIER3 partner of 9)", True),
    # The other two true-amb shapes:
    ("r-6min7-amb",    _midi("C3", "D3", "F3", "A3"), 24, True, "R", None, None, "C3x4", "C3 D3 F3 A3",
     "EXAMPLE SET (true amb {0,2,5,9}). §10r: no forced name; notes overflow, compact last resort.", True),
    ("r-min7-6-amb",   _midi("C3", "Eb3", "F3", "Ab3"), 24, True, "R", None, None, "C3x4", "C3 D#3 F3 G#3",
     "EXAMPLE SET (true amb {0,3,5,8}). §10r: no forced name; notes overflow, compact last resort.", True),
    # Dual with bass-root (sus family):
    ("r-sus2-dual",    _midi("C3", "D3", "G3"), 24, True, "R", "Csus2", None, "Csus2", "C3 D3 G3",
     "EXAMPLE SET (Csus2 bass-root exact; §10r shows no second Gsus4/C)", True),
    # Leftover trichord (no TIER3, no unique nearest): interval.
    ("r-leftover-tri", _midi("C3", "D3", "F3"), 24, True, "R", None, None, "C3 D3 F3", "C3 D3 F3",
     "EXAMPLE SET (no exact T1/T2/T3). §10r: just the notes, no interval.", True),
    # Nearest-tag: C major plus F#, unique one-extra. (C7+D# ties Cmin7+E
    # at the same Hamming distance, so that set is interval, not a tag.)
    ("r-7plus",        _midi("C3", "E3", "F#3", "G3"), 24, True, "R", None, None, "C3x4",
     "C3 E3 F#3 G3", "EXAMPLE SET (no exact name; notes overflow 54 px, compact last resort). Not C+F#.", True),
    ("r-7sharp9-tie",  _midi("C3", "E3", "G3", "Bb3", "D#4"), 24, True, "R", None, None, "C3x5",
     "C3 E3 G3 A#3 D#4",
     "EXAMPLE SET (no exact T1/T2/T3). §10r: no interval; notes overflow, compact last resort.", True),
    # Long minmaj7 root: NOTE short minM7, official name on the held line.
    ("r-sharp-minmaj7", _midi("G#2", "B2", "D#3", "G3"), 24, True, "R", "G#minmaj7", None, "G#minM7",
     "G#2 B2 D#3 G3", "EXAMPLE SET (G#minmaj7 overflows XS at 59; NOTE short G#minM7)", True),
    # Option A: still dumps unrecognised; still shows second on amb and duals.
    ("a-cluster",      _midi("C3", "C#3", "D3"), 24, True, "A", None, None, "C3x3", "C3 C#3 D3",
     "A leaves unrecognised as compact (the A gap)", True),
    ("a-sus-amb",      _midi("D3", "G3", "C4"), 24, True, "A", "Gsus4/D", "Csus2/D", "Gsus4/D", "D3 G3 C4",
     "A shows both exact readings", True),
    ("a-C6",           _midi("C3", "E3", "G3", "A3"), 24, True, "A", "C6", "Amin7/C", "C6", "C3 E3 G3 A3",
     "A: C6 plus dim Amin7/C (the candidate example)", True),
    # Option B: names unrecognised; amb is first upward only (no second).
    ("b-add9",         _midi("C3", "D3", "E3", "G3"), 24, True, "B", "Cadd9", None, "Cadd9", "C3 D3 E3 G3",
     "B names add9", True),
    ("b-sus-amb",      _midi("D3", "G3", "C4"), 24, True, "B", "Gsus4/D", None, "Gsus4/D", "D3 G3 C4",
     "B's gap: first upward, no second reading", True),
    ("b-cluster",      _midi("C3", "C#3", "D3"), 24, True, "B", "C(b2 2)", None, "C(b2 2)", "C3 C#3 D3",
     "B interval fallback for a cluster", True),
    ("b-C6",           _midi("C3", "E3", "G3", "A3"), 24, True, "B", "C6", None, "C6", "C3 E3 G3 A3",
     "B keeps C6, no second (the R/A add-on)", True),
    # Option C: interval for unnamed; named §9 kept; no second.
    ("c-cluster",      _midi("C3", "C#3", "D3"), 24, True, "C", "C(b2 2)", None, "C(b2 2)", "C3 C#3 D3",
     "C interval for a cluster", True),
    ("c-add9",         _midi("C3", "D3", "E3", "G3"), 24, True, "C", "C(2 3 5)", None, "C(2 3 5)", "C3 D3 E3 G3",
     "C does not add TIER3: add9 is C(2 3 5)", True),
    ("c-sus-amb",      _midi("D3", "G3", "C4"), 24, True, "C", "D(4 b7)", None, "D(4 b7)", "D3 G3 C4",
     "C bass-centric interval, no second chord name", True),
    ("c-maj",          _midi("C3", "E3", "G3"), 24, True, "C", "C", None, "C", "C3 E3 G3",
     "C keeps a §9 exact name", True),
    ("c-hex",          _midi("C3", "E3", "G3", "B3", "D4", "F4"), 24, True, "C", "C(2 3 4 5 7)", None,
     "C(2 3 4 5 7)", "C3 E3 G3 B3 D4 F4",
     "C hex: within-octave degrees C(2 3 4 5 7), 52 px at FONT_XS, no trim. (Not 9/11 compound.)", True),
    # Option D: marker on amb/dual NOTE; dumps unrecognised.
    ("d-sus-amb",      _midi("D3", "G3", "C4"), 24, True, "D", "Gsus4/D", "Csus2/D", "Gsus4/D:", "D3 G3 C4",
     "D NOTE Gsus4/D: at FONT_XS (45 px); the colon marks a second reading.", True),
    ("d-cluster",      _midi("C3", "C#3", "D3"), 24, True, "D", None, None, "C3x3", "C3 C#3 D3",
     "D leaves unrecognised as compact (the D gap)", True),
    ("d-C6",           _midi("C3", "E3", "G3", "A3"), 24, True, "D", "C6", "Amin7/C", "C6:", "C3 E3 G3 A3",
     "D NOTE C6: (31 px FONT_M) marks a second reading", True),
    # Review T20–T26, T35, F6, under the recommended style. §9 exact names
    # stay; the note-dump cases get a useful name (add-on).
    ("r-T20",          _midi("C3", "D3", "G3"), 24, True, "R", "Csus2", None, "Csus2", "C3 D3 G3",
     "review T20: bass-root Csus2 exact; §10r no second Gsus4/C", True),
    ("r-T22",          _midi("G3", "C4", "F4"), 24, True, "R", None, None, "G3 C4 F4", "G3 C4 F4",
     "review T22: true amb. §10r: just the notes, no forced Csus4/G.", True),
    ("r-T24",          _midi("E3", "G3", "A3", "C4"), 24, True, "R", None, None, "E3x4", "E3 G3 A3 C4",
     "review T24: true amb. §10r: no forced Amin7/E; notes overflow 54 px, compact last resort.", True),
    ("r-T25",          _midi("E3", "G3", "A3", "C4"), 24, False, "R", "Amin7/E", None, "Amin7/E", "E3 G3 A3 C4",
     "review T25: Tier 1 only, unique slash, no second. T1 and T2 differ (see r-T24).", True),
    ("r-T26",          _midi("G3", "A3", "C4", "E4"), 24, True, "R", None, None, "G3x4", "G3 A3 C4 E4",
     "review T26: true amb. §10r: just notes; overflow, compact last resort.", True),
    ("r-T35",          _midi("D3", "C4", "E4", "G4"), 24, True, "R", "Cadd9/D", None, "Cadd9/D", "D3 C4 E4 G4",
     "review T35: §9 notes (foreign bass, no T1+T2 match); TIER3 exact slash Cadd9/D", True),
    ("r-F6-clamp",     [103, 107, 110], 48, True, "R", None, None, "G9", "G9 G9 G9",
     "review F6: clamp-merge is one sounding pitch. §10 NOTE is G9 (mono), not Goct and not a major triad. Store unchanged.", True),
    ("r-drone-stored", _midi("C3", "E3", "G3", "B3", "D4", "F4"), 24, True, "R", "Cmaj11", None, "Cmaj11",
     "C3 E3 G3 B3 D4 F4",
     "F1: no voice-state source — name the STORED six (Cmaj11), no dim marks. Not a guessed G7.", True),
    ("r-drone-voice",  _midi("G3", "B3", "D4", "F4"), 24, True, "R", "G7", None, "G7", "G3 B3 D4 F4",
     "F1: read-only voice state confirms G3 B3 D4 F4 sounding — G7. (Input is the confirmed set.)", True),
    # Recorded host-frame voicings (measurements.txt / baseline_extra.c), trans=24.
    ("r-page2-c4",     [60, 64, 67], 24, True, "R", "C", None, "C", "C4 E4 G4",
     "recorded: before-page2-chord, notes 60,64,67 (C4 E4 G4). Same quality as the C3 triad, different octave/voicing.", True),
    ("r-rec-mono-e3",  [52], 24, True, "R", None, None, "E3", "E3",
     "recorded: before-mono-E3, note 52.", True),
    ("r-rec-mono-a3",  [57], 24, True, "R", None, None, "A3", "A3",
     "recorded: before-chord-and-mono / before-page1-with-page2-chord, note 57.", True),
]


def run_tests_10() -> list[str]:
    errors = []
    # Enumeration locks.
    c1, b1, a1 = enumerate_sets(tier2=False, tier3=False)
    if (c1["bass-root"], c1["slash"], c1["ambiguous"], c1["none"], c1["total"]) != (6, 14, 0, 1003, 1023):
        errors.append(f"T1 enum {c1}")
    if a1:
        errors.append(f"T1 should have 0 amb shapes, got {a1}")
    c2, b2, a2 = enumerate_sets(tier2=True, tier3=False)
    if (c2["bass-root"], c2["slash"], c2["ambiguous"], c2["none"], c2["total"]) != (13, 16, 3, 991, 1023):
        errors.append(f"T1+2 enum {c2}")
    if len(a2) != 3:
        errors.append(f"T1+2 amb shapes {a2}")
    else:
        shapes = {s[0] for s in a2}
        if shapes != {(0, 5, 10), (0, 2, 5, 9), (0, 3, 5, 8)}:
            errors.append(f"T1+2 amb shape set {shapes}")
    c3, _, a3 = enumerate_sets(tier2=True, tier3=True)
    if len(a3) != 3:
        errors.append(f"T1+2+3 must not add amb, got {a3}")

    for row in TEST_SET_10:
        rid, stored, trans, t2, style, exp_pri, exp_sec, exp_note, exp_names, _src, unchanged = row
        sounding = sounding_pitches(stored, trans)
        if sounding_pitches(stored, trans) != sounding:
            errors.append(f"{rid}: namer mutated sounding pitches")
        if list(stored) != list(row[1]):
            errors.append(f"{rid}: stored pitches changed")
        pri, sec, kind = readings(sounding, style=style, tier2=t2)
        if pri != exp_pri:
            errors.append(f"{rid}: primary {pri!r} != {exp_pri!r} (kind {kind})")
        if sec != exp_sec:
            errors.append(f"{rid}: second {sec!r} != {exp_sec!r}")
        written, role, painted, _, _, _ = note_cell_10(sounding, style=style, tier2=t2)
        if written != exp_note:
            errors.append(f"{rid}: NOTE written {written!r} != {exp_note!r} (font {role}, paint {painted!r})")
        names = " ".join(put_note(n) for n in sounding)
        if names != exp_names:
            errors.append(f"{rid}: notes {names!r} != {exp_names!r}")
        # FONT_M safety: never return M for a string that needs a safe font.
        if role == "M" and _needs_safe_font(written):
            errors.append(f"{rid}: FONT_M used for {written!r} (would paint {device_paint('M', written)!r})")
        # No '?' ever.
        for s in (pri, sec, written):
            if s and "?" in s:
                errors.append(f"{rid}: '?' in {s!r}")
        # §10r: never an interval form or nearest-tag on the recommended path.
        if style == "R":
            for s in (pri, sec, written):
                if s and ("(" in s or (s.endswith("oct") and kind != "mono")):
                    errors.append(f"{rid}: interval-like label {s!r}")
                if s and "+" in s and not any(ch.isdigit() for ch in (s.split("+")[-1] if "+" in s else "")):
                    # C+F# nearest-tag is forbidden; compact C3x4 is fine (no '+').
                    errors.append(f"{rid}: nearest-tag or plus-form {s!r}")
            if sec:
                errors.append(f"{rid}: §10r must not show a second reading ({sec!r})")
            if kind == "amb" and pri is not None:
                errors.append(f"{rid}: forced name {pri!r} on a truly ambiguous set")
            # If the actual notes fit 54 px, NOTE must show them when unnamed.
            names = " ".join(put_note(n) for n in sounding)
            if pri is None and kind != "mono" and names:
                fits = any(device_tw(r, names) <= NOTE_CELL_PX for r in ("M", "S", "XS"))
                if fits and written != names:
                    errors.append(f"{rid}: notes fit the cell but NOTE is {written!r}")
        # Every amb / dual under A, D (superseded §10) has a second reading.
        if style in ("A", "D") and kind in ("amb", "dual") and not sec:
            errors.append(f"{rid}: {kind} without a second reading")

    # §9 unique symbols stay identical under every style (add-on).
    for row in TEST_SET:
        rid, stored, trans, t2, exp_sym, exp_note, exp_names, _src, _ = row
        if exp_sym is None:
            continue
        sounding = sounding_pitches(stored, trans)
        for style in STYLES:
            pri, _sec, _k = readings(sounding, style=style, tier2=t2)
            if pri != exp_sym:
                errors.append(f"{rid}/{style}: §9 symbol changed to {pri!r}")

    # Acceptance scan, style R (§10r): no interval form; no forced name on
    # amb; no second reading; if the actual notes fit 54 px they are shown
    # when unnamed; FONT_M never paints parens or a flat degree.
    from itertools import combinations
    for k in (2, 3):
        for others in combinations(range(1, 12), k - 1):
            notes = [60] + [60 + o for o in others]
            pri, sec, kind = readings(notes, style="R", tier2=True)
            written, role, painted, *_ = note_cell_10(notes, style="R", tier2=True)
            names = " ".join(put_note(n) for n in notes)
            if pri and "(" in pri:
                errors.append(f"scan k={k} {notes}: interval primary {pri}")
            if written and "(" in written:
                errors.append(f"scan k={k} {notes}: interval NOTE {written}")
            if sec:
                errors.append(f"scan k={k} {notes}: second reading {sec}")
            if kind == "amb" and pri is not None:
                errors.append(f"scan k={k} {notes}: forced name {pri}")
            fits = any(device_tw(r, names) <= NOTE_CELL_PX for r in ("M", "S", "XS"))
            if pri is None and fits and written != names:
                errors.append(f"scan k={k} {notes}: notes fit but NOTE {written}")
            if role == "M" and _needs_safe_font(written):
                errors.append(f"scan k={k}: FONT_M for {written}")
    return errors


# Replace the original __main__ block's exclusive run: a wrapper is installed
# by wrapping run_tests usage below. The original if __name__ still only
# runs run_tests; we extend it by replacing the block at the file end.


# ---------------------------------------------------------------------------
# §9 errata helpers (F1, F5, T36). Host-side only. Not a firmware patch.
# ---------------------------------------------------------------------------

def onset_notes_of(steps, idx):
    """TIE steps do not carry lanes (sequencer.c 278–287). Walk back to the
    onset. An orphan TIE (no preceding onset) returns [] — no invented pitch.
    """
    if idx < 0 or idx >= len(steps):
        return []
    j = int(idx)
    while j >= 0 and steps[j].get("tie"):
        j -= 1
    if j < 0:
        return []
    return [int(n) for n in (steps[j].get("notes") or [])]


def drone_input(stored, transpose=24, *, voice_state=None):
    """F1. Use the sounding (voice) set only if a read-only voice-state
    source exists. Otherwise the stored set after the effective transpose.
    `voice_state` is already-sounding MIDI and is used as-is (no second
    transpose).
    """
    if voice_state is not None:
        return [int(n) for n in voice_state]
    return sounding_pitches(stored, transpose)


def drone_dim_pitches(stored, transpose=24, *, voice_state=None):
    """Silent stored pitches, only when voice_state is confirmed.
    Without a voice-state source: no dim marks.
    """
    if voice_state is None:
        return []
    sounding = {int(n) for n in voice_state}
    return [p for p in sounding_pitches(stored, transpose) if p not in sounding]


def label_cache_key(*, pattern, part, step, onset_idx, notes, transpose,
                    engine, voice_state=None):
    """Cache key. Any of these changing must miss. F5."""
    vs = None if voice_state is None else tuple(int(x) for x in voice_state)
    return (
        int(pattern), str(part), int(step), int(onset_idx),
        tuple(int(n) for n in notes), int(transpose), str(engine), vs,
    )


def run_tests_errata() -> list[str]:
    """T36 (TIE / orphan / onset-edit), F1 Drone, F5 cache, F6 already in
    TEST_SET, F8 spare, T34 with Tier 2, seventh-key purity."""
    errors = []
    ceg = _midi("C3", "E3", "G3")
    cmin = _midi("C3", "Eb3", "G3")
    # T36: onset C E G, two TIE steps. C is available on both TIEs.
    steps = [
        {"notes": list(ceg), "tie": False},
        {"notes": [], "tie": True},
        {"notes": [], "tie": True},
    ]
    for idx in (0, 1, 2):
        notes = onset_notes_of(steps, idx)
        if notes != list(ceg):
            errors.append(f"T36: view {idx} got {notes}, expected onset {ceg}")
        if chord_symbol(notes, tier2=True) != "C":
            errors.append(f"T36: view {idx} symbol {chord_symbol(notes, tier2=True)!r} != 'C'")
        written, _, _ = note_cell_label(notes, tier2=True)
        if written != "C":
            errors.append(f"T36: view {idx} NOTE {written!r}")
    # Onset edit while viewing a TIE: extras still come from the (new) onset.
    steps[0]["notes"] = list(cmin)
    notes = onset_notes_of(steps, 2)
    if notes != list(cmin):
        errors.append(f"T36-edit: TIE view still {notes}")
    if chord_symbol(notes, tier2=True) != "Cmin":
        errors.append(f"T36-edit: expected Cmin, got {chord_symbol(notes, tier2=True)!r}")
    # Orphan TIE: no invented chord or pitch.
    orphan = onset_notes_of([{"notes": [], "tie": True}], 0)
    if orphan:
        errors.append(f"orphan TIE invented pitches {orphan}")
    if chord_symbol(orphan, tier2=True) is not None:
        errors.append(f"orphan TIE invented symbol {chord_symbol(orphan, tier2=True)!r}")
    if (compact(orphan) or "") != "":
        errors.append(f"orphan TIE compact {compact(orphan)!r}")

    # F1 Drone.
    six = _midi("C3", "E3", "G3", "B3", "D4", "F4")
    four_high = _midi("G3", "B3", "D4", "F4")
    four_low = _midi("C3", "E3", "G3", "B3")
    # No voice-state source: name the stored set, no dim marks. G7 is not assumed.
    inp = drone_input(six, 24, voice_state=None)
    if inp != list(six):
        errors.append(f"F1 stored input {inp}")
    if drone_dim_pitches(six, 24, voice_state=None) != []:
        errors.append("F1: dim marks without a voice-state source")
    if chord_symbol(inp, tier2=True) is not None:
        errors.append(f"F1: stored six should be unnamed under §9, got {chord_symbol(inp, tier2=True)!r}")
    if note_cell_label(inp, tier2=True)[0] != "C3x6":
        errors.append(f"F1: stored six §9 NOTE {note_cell_label(inp, tier2=True)[0]!r}")
    # Confirmed voice state: name that set. Two different active fours, two labels.
    v_g7 = drone_input(six, 24, voice_state=four_high)
    v_maj7 = drone_input(six, 24, voice_state=four_low)
    if chord_symbol(v_g7, tier2=True) != "G7":
        errors.append(f"F1 voice G7: {chord_symbol(v_g7, tier2=True)!r}")
    if chord_symbol(v_maj7, tier2=True) != "Cmaj7":
        errors.append(f"F1 voice Cmaj7: {chord_symbol(v_maj7, tier2=True)!r}")
    dim = drone_dim_pitches(six, 24, voice_state=four_high)
    if dim != [48, 52]:
        errors.append(f"F1 dim when confirmed silent: {dim}")
    # Four stored and all four confirmed active: Cmaj7.
    four = _midi("C3", "E3", "G3", "B3")
    if chord_symbol(drone_input(four, 24, voice_state=four), tier2=True) != "Cmaj7":
        errors.append("F1 four-active not Cmaj7")

    # F5 cache: every input change misses.
    base = dict(pattern=1, part="JUNO", step=0, onset_idx=0,
                notes=ceg, transpose=24, engine="juno", voice_state=None)
    k0 = label_cache_key(**base)
    tweaks = [
        ("pattern", 2),
        ("part", "DRONE"),
        ("step", 3),
        ("onset_idx", 1),
        ("notes", cmin),
        ("transpose", 36),
        ("engine", "drone"),
        ("voice_state", four_high),
    ]
    for field, val in tweaks:
        kw = dict(base)
        kw[field] = val
        if label_cache_key(**kw) == k0:
            errors.append(f"F5: changing {field} did not invalidate the cache key")
    # Same step number, different pattern: miss (review).
    if label_cache_key(**{**base, "pattern": 2}) == k0:
        errors.append("F5: pattern switch, same step, stale key")
    # Same key hits.
    if label_cache_key(**base) != k0:
        errors.append("F5: identical inputs must be the same key")

    # T34 with Tier 2 (review is Tier 1; both must agree).
    g_c = _midi("G2", "C3")
    if chord_symbol(g_c, tier2=True) != "C5/G":
        errors.append(f"T34 T2: {chord_symbol(g_c, tier2=True)!r}")

    # F8: 51 px fallbacks have 3 px spare.
    for text, role, spare in (("Cmin7b5", "S", 3), ("A#min7b5", "XS", 3)):
        w = device_tw(role, text)
        if w != NOTE_CELL_PX - spare:
            errors.append(f"F8: {text} FONT_{role} is {w}, expected {NOTE_CELL_PX - spare}")
        if w > NOTE_CELL_PX:
            errors.append(f"F8: {text} clips")

    # Seventh key refused: namer is a pure function of its input; a store
    # that did not accept a 7th note still yields the same label.
    six_sym = chord_symbol(six, tier2=True)
    six_note = note_cell_label(six, tier2=True)[0]
    # Re-name the same six (the refused key never entered the store).
    if chord_symbol(list(six), tier2=True) != six_sym or note_cell_label(list(six), tier2=True)[0] != six_note:
        errors.append("seventh-key: renaming the unchanged store drifted")
    if list(six) != _midi("C3", "E3", "G3", "B3", "D4", "F4"):
        errors.append("seventh-key: store mutated")
    return errors


if __name__ == "__main__":
    err9 = run_tests()
    err10 = run_tests_10()
    err = err9 + err10
    if err:
        print("FAIL")
        for e in err:
            print(" ", e)
        raise SystemExit(1)
    print("PASS", len(TEST_SET), "§9 rows,", len(TEST_SET_10), "§10 rows")
    c1, _, _ = enumerate_sets(tier2=False)
    c2, b2, a2 = enumerate_sets(tier2=True)
    c3, _, _ = enumerate_sets(tier2=True, tier3=True)
    print(f"  enum T1      bass-root={c1['bass-root']} slash={c1['slash']} amb={c1['ambiguous']} unrec={c1['none']}  / {c1['total']} bass-fixed")
    print(f"  enum T1+2    bass-root={c2['bass-root']} slash={c2['slash']} amb={c2['ambiguous']} unrec={c2['none']}  / {c2['total']}")
    print(f"  enum T1+2+3  bass-root={c3['bass-root']} slash={c3['slash']} amb={c3['ambiguous']} unrec={c3['none']}  / {c3['total']}")
    print("  amb shapes (T1+2, bass-fixed):")
    for shape, names in a2:
        print(f"    {shape} -> {names}")
    print()
    for row in TEST_SET_10:
        rid, stored, trans, t2, style, exp_pri, exp_sec, exp_note, exp_names, src, _ = row
        sounding = sounding_pitches(stored, trans)
        line, font, parts = held_line_10(1, sounding, style=style, tier2=t2)
        written, nrole, painted, pri, sec, kind = note_cell_10(sounding, style=style, tier2=t2)
        sec_s = f"  2nd={sec}" if sec else ""
        print(f"  {rid:20s}  {style} {kind:9s}  pri={pri!s:14s}{sec_s}  NOTE {written:12s} ({nrole} paints {painted})")
        print(f"    held/{font} {line}")
        print(f"    {src}")
