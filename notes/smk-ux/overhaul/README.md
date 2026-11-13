# SMK-37 / DnBOX UX overhaul — phase 2

Design alternatives under `/workspace/smk-ux/overhaul/`. Native 240×240 frames drawn with **packed felucca** (device Barlow, FONT_LZ_CROP) and **decoded shipped glyphs**. No PIL TTF on a device frame. Firmware, `seq-chords/piano-roll.md`, and `presets-spec.md` are not edited.

**Baseline:** `94197fc839b00aff47b98ca02f3c6396e4a6eabf`.

## Alternatives (one line each)

- **R Instrument first** — unified 26×19 SOUND for all six engines, first play is 909 SEQ, presets on SOUND, exact chord names on pitched SEQ. Closest to the shipped instrument. **Recommended lean set.**
- **S Score first** — 909/808 still kit-first; Juno and Drone **open on the piano-roll**; preset audition in sequence context; hollow play icon when stopped.
- **L Live rail** — same SOUND and SEQ as R, plus a 2 px state rail and an `UNSAVED` that **never times out**; power-loss honesty (`SAVES WHEN STOPPED`; RAM is not `SAVED`).

## Recommendation

Ship **R’s SOUND and first play**, take **L’s unsaved-never-timeout** (0 B policy on the existing badge), and use **S’s roll** as the open page for pitched SEQ (already designed; not a new view). Do not implement the rejected §10 interval form. Measure flash at link before any factory append or unique glyph lands.

## Chord labels

Compact names **only** on exact matches (`Cmin7`, `C/E`, `Cadd9`, bare `C`). Otherwise the **actual notes**. Never `C(3)`, never `C(b2 2)`, never a forced name. `piano-roll.md` is not edited.

## Budget (headline)

Elite **248 B measured**. Chord namer Tier 1 **285 B measured** — already over. Kind-map 16+80–200 B HOST. Unique glyphs 200–300 B HOST. Recommended Juno/Drone factory subset **180 B HOST**. Interval form **0 B** (rejected). Full factory menu 276–296 B — do not ship without a new measurement. Zero new slots is acceptable. Details: `budget.md`.

## Juno / Drone factory proposals

`presets/juno-drone-factory.md`. Existing 0–3 (`JUNO POLY/PAD/PLUCK/BRASS`, `Cathedral/Cloud/Aurora/Organ`) **unchanged**. Proposed appends from index 4, original pots in real source ranges (file:line cited), **not implemented**, marked for Main Builder. Recommended first-ship subset: JUNO SOFT, STR, STAB, BASS, LEAD + Drone Horizon (~180 B). Pending render evidence and a rights record.

## Paths

| Path | What |
|---|---|
| `design.md` | Alternatives, every change with problem / evidence / impact / smallest design / HOST ESTIMATE / acceptance |
| `budget.md` | Flash table against 248 B, ranked cuts, CPU |
| `presets/juno-drone-factory.md` | Curated Juno and Drone factory **proposals** |
| `felucca.py`, `felucca_font.h` | Device font port |
| `render.py` | Frame renderer |
| `frames/before/{1x,3x}/` | Honest shipped language, 14 screens |
| `frames/R/{1x,3x}/` | Alternative R, 26 screens |
| `frames/S/{1x,3x}/` | Alternative S, 27 screens |
| `frames/L/{1x,3x}/` | Alternative L, 26 screens |
| `sheet-before.png`, `sheet-R.png`, `sheet-S.png`, `sheet-L.png` | Overview sheets |
| `SHA256SUMS` | Hashes of the artifacts in this directory |

93 device frames at 1× (and the same 93 at 3×), plus 4 sheets.
