# SEQ chord display — host diagnosis at 94197fc8

**Date:** 2026-11-13, Asia/Vladivostok (UTC+10).
**Baseline tree:** `72d28fbc741244d18fa3d18495da16ab0fcb1d24`
(gobbajogger/X0X37-for-Loopy-Pro `94197fc839b00aff47b98ca02f3c6396e4a6eabf`).
**Branch:** `lane/fmb-seq-chord-display`.
**Method:** read-only inspection of firmware sources, PR #866, and Firmware UX's
diagnosis at `/workspace/smk-ux/seq-chords/`; then a host reproduction that
compiles the **real** `ui.c` draw path and writes native 240×240 RGB565
framebuffers. These PNGs are **OBSERVED** output, not source reconstructions.
**Firmware sources were not modified. The proposed fix was not applied.**

---

## 1. Cause (file:line)

Held-key chords are stored correctly and play back, but the SEQ view never
reads the extra pitches.

The SEQ 303 / Juno / Drone roll is a single function, `draw_303`
(`firmware/src/app/ui.c` **5572–5668**). `draw_main` (**6814**) dispatches
V_SEQ for a 303 slot with `draw_303(band, ui.view == V_GEN)` at **6888–6889**.
There is no Juno/Drone branch and **no call to `held_chord_get`** anywhere in
the draw path.

What is stored (`firmware/src/seq/held_chord.h` **21–32**): the primary pitch,
gate, accent, slide, velocity and endpoint stay in `bstep_t`. Extra pitches
(up to five, six total) live in motion-lane pool entries with
`HELD_CHORD_TARGET = 0xfc`. `held_chord_get` returns the root plus extras,
strictly increasing. Extras are written on the **onset** step only; TIE
continuations carry the root and no lanes (REC-while-playing form).

What is drawn:

| Fault | Site | Observed on host |
|---|---|---|
| **1. Hidden notes.** The only pitch mark is `night_mark` at `held_note_of(bp, s)`. `held_note_of` (**5566–5570**) returns `bp->step[note_start_of(...)].note` and never reads chord lanes. | `ui.c` **5614–5619**; whole `draw_303` **5572–5668** | A stored 2-, 3-, 4- or 6-note chord paints **one** bar, the root. Extra pitches that sit inside the 14-st window are absent. Host: dyad E3, triad E3/G3, tetrad E3/G3/B3 all `inside=1 drawn=0`. |
| **2. Pitch window from primaries only.** `lo`/`hi` use `bp->step[start].note` only. | `ui.c` **5581–5596** | A C3 hexachord (C3 E3 G3 B3 D4 F4) computes `window=[46,60]`. D4=62 and F4=65 are outside and would clip even if marks were added without expanding the window. |
| **3. Truncated labels.** The held-step / WRITE line and the NOTE knob name only the root. | `ui.c` **5645–5659** (esp. **5651**); `draw_knobs` **8370–8388** (esp. **8381**) | Held C major triad: label `STEP 1  C3 V100`, NOTE knob `C3`. Those pixels are identical to a held mono C3. Page 2: `STEP 17  C4 V100` / `C4` for a C4–E4–G4 chord. |
| **4. Sustained extras have no length.** TIE connectors and the slide preview use the single root `y`. TIE steps have no lanes, so a lookup at the TIE index would also fail. The correct onset is `note_start_of`. | `ui.c` **5617–5638**, **5566–5570** | A NOTE + three TIE run draws a four-step bar at C3 only. Playhead on the third TIE. E3 and G3 have no bar and no tie. |
| **5. False p-lock notch (host-observed, additional to the UX diagnosis).** `draw_303` asks `lock_marks(-1, -1, ...)` (**5578**), and `lock_marks` (**3082–3096**) does not skip `HELD_CHORD_TARGET`. Motion playback already excludes `0xfc` / `0xfd`. | `ui.c` **3082–3096**, **5620–5621** | A chord's roll differs from a same-root mono note by **exactly four pixels**: a 2×2 `C_WHITE` notch at `(col_x+5, note_y-3)`. 2-, 3-, 4- and 6-note chords with the same root are **byte-identical** on screen (same SHA-256). |
| **6. Chord vs single, overlap.** Successive chords and a chord next to a mono note use the same one-bar language. | `ui.c` **5615**, **5566–5570** | `before-chord-and-mono.png`: a C triad on step 1 and a mono A3 on step 8 look like two single notes (the triad also has the false notch). |

Selected vs playing vs page 2 do not have a separate bug: `step_of` and the
playhead / white-held colour already work for the **root**. The extras are
dropped in every case.

Drum (`draw_drum` **5512–5550**) and BREAK (`draw_break`) are not involved.
Chords are bass-lane data (`part == NKIT+b`). HOME mini-lanes show gate
presence, not pitches.

---

## 2. PR #866 relevance

[gobbajogger/X0X37-for-Loopy-Pro#866](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/866)
“Record held-key chords in sequencer steps” is **closed and unmerged as itself**.
Its implementation commit **`be48fcaed4e75f4ca9d6baf46cbfac1343e949bd`** is an
ancestor of `94197fc8` and is the held-chord work in this tree.

The PR added storage, recording (`bass_write_held_chord`, `seq_rec_bass`),
playback of extras, stream 15, and UI write-path bookkeeping (include,
clear-on-write/tap, `held_note_by` transposition of extras, generate/mutate
clears, `CHORD NOT SAVED`). **It did not change `draw_303`, `held_note_of`,
the pitch window, the held-step label, the NOTE knob, or `lock_marks`.**
That is why a recorded chord is audible and in the store, but shows as one
note. Review comments on the PR do not discuss SEQ display.

Firmware UX's read-only diagnosis (`/workspace/smk-ux/seq-chords/diagnosis.md`,
SHA-256 `eef8229736c2332a4fa9841f0f148cdb4042385d389eae44512f32b9bfa32098`)
reaches the same primary cause from source. This host run confirms it with
observed frames, and adds fault 5 (false p-lock notch) which that diagnosis
did not call out as a chord-lane false positive. Their `piano-roll.md` is a
later overhaul proposal (Direction B); the bounded fix below uses **only
existing SEQ marks and glyphs**, which is their “Direction A / bounded” map.

---

## 3. Repro

No device, no firmware source edits, no git network.

```sh
unset GIT_DIR
cd /workspace/seq-chord-001
sh tests/host/seq_chord_display_test.sh
# optional: SEQ_CHORD_EVIDENCE=/path sh tests/host/seq_chord_display_test.sh /tmp/seq-chord-build
```

The script is the existing Night UI / `smc_ui_test.sh` harness
(`tests/host/night_ui_test.sh`) with
`SMC_UI_TEST_FILE=tests/host/seq_chord_display_test.c`. That compiles the
real `ui.c` draw code into a host binary and runs a characterization test:
it **passes on this buggy baseline** (263 checks). A later draw-only fix
must invert the “extra pitch is absent” and “false p-lock notch” checks.

Patterns (Juno on 303B): 2-note, 3-note, 4-note, 6-note; a mono C3 control;
a mono E3; a triad beside a mono A3; a held (selected) triad; a playing
triad; a NOTE+TIE sustained triad with the playhead on a TIE; a 32-step
pattern with a C4 triad on step 16 (page 2), including selected and playing.

---

## 4. BEFORE PNG paths

All native 240×240, written by the host `png_write` of the real framebuffer.
SHA-256 in `evidence/before/SHA256SUMS`. Measurements:
`evidence/before/measurements.txt`.

| File | What it shows |
|---|---|
| `evidence/before/before-mono-control.png` | Mono C3, no extras. Control for identity. |
| `evidence/before/before-mono-E3.png` | Mono E3 (window follows the primary). |
| `evidence/before/before-dyad.png` | C3–E3. Only C3 is drawn. |
| `evidence/before/before-triad.png` | C3–E3–G3. Only C3. |
| `evidence/before/before-tetrad.png` | C3–E3–G3–B3. Only C3. |
| `evidence/before/before-hexachord.png` | Six notes; D4/F4 outside the primary-only window. Only C3. |
| `evidence/before/before-chord-and-mono.png` | Triad on step 1, mono A3 on step 8. |
| `evidence/before/before-selected-chord.png` | Held triad: white root, `STEP 1  C3 V100`, NOTE `C3`. |
| `evidence/before/before-selected-mono.png` | Held mono C3: label and NOTE knob identical to the triad. |
| `evidence/before/before-playing-chord.png` | Playhead on the chord step; extras still omitted. |
| `evidence/before/before-sustained-triad.png` | Four-step TIE run at C3 only; playhead on step 3. |
| `evidence/before/before-page1-with-page2-chord.png` | 32-step pattern, page 1 (steps 1–16): mono A3. |
| `evidence/before/before-page2-chord.png` | Page 2 (17–32): C4 triad shows C4 only. |
| `evidence/before/before-page2-selected-chord.png` | Held step 17: `STEP 17  C4 V100`, NOTE `C4`. |
| `evidence/before/before-page2-playing-chord.png` | Playhead on step 17; extras still omitted. |

`before-dyad.png`, `before-triad.png`, `before-tetrad.png` and
`before-hexachord.png` are byte-identical (SHA-256
`55263b56127c928943183d93de76766446725c1e12787bde4e372eeb2a9400a9`).
Two through six stored notes with the same root produce the same screen.

---

## 5. Proposed draw-only fix (NOT applied)

Use the existing SEQ visual language only: more `night_mark` bars of height 5
and the existing width law, the same 16 columns, the same colours (white when
`s == ui.held_step`, part colour on accent, `dim(col, 9)` otherwise), existing
TIE connector geometry at each pitch, existing `put_note` / fonts. No new
view, no new gesture, no piano-key sidebar, no per-note gate editing.

### Exact functions and line ranges

| Site at `94197fc8` | Change sketch |
|---|---|
| **New helper** beside `note_start_of` / `held_note_of`, `ui.c` **5556–5570** | e.g. `onset_chord_of(bp, s, out)`: `start = note_start_of(bp, s)`; if `start < 0` return 0; else `held_chord_get(proj.arr.lane, seq.ppat[ui.part], bidx(), start, bp->step[start].note, out)`. Never look up a TIE index. `held_note_of` can stay for the root. |
| **`draw_303` window**, `ui.c` **5581–5596** | Expand `lo`/`hi` with every pitch from that helper. Keep the 14-st floor and the ±2 padding. |
| **`draw_303` marks, ties, slide**, `ui.c` **5614–5638** | One `night_mark` per stored pitch, same `width` / `fc` / height 5. TIE connectors (**5622–5633**) at **each** of those y values, still from the onset chord. Slide preview (**5635–5637**) stays root-to-next-root (303 slide is into the next **step**); do not invent per-note slides. P-lock notch stays on the root bar only, and only for real p-locks (see `lock_marks`). |
| **`draw_303` held / WRITE line**, `ui.c` **5645–5659** (esp. **5651**) | After the root, `put_note` each extra in pitch order. If `FONT_B` would overflow the 232 px line, drop to `FONT_S` then `FONT_XS` — still this line, no new banner. |
| **`draw_knobs` held-step NOTE**, `ui.c` **8370–8388** (esp. **8381**) | Same helper. `FONT_M` cannot hold six names in a ~54 px cell: compact `C3+2` / `C3+5` in the NOTE cell, with the full list on the 5645 line. GATE / ACCENT / SLIDE stay per-step. |
| **`lock_marks`**, `ui.c` **3082–3096** | Skip `l->t == HELD_CHORD_TARGET` so chord lanes do not paint a false p-lock notch. This helper is draw-only (callers: `draw_303` **5578**, `draw_drum` with an explicit engine type, `draw_break`). Drum/BREAK callers pass a concrete `t` and are unaffected. |

### Do not change

- Write paths: `ui.c` **3342–3493** (`held_chord_set` on `bass_write`,
  `bass_write_held_chord`, clear-on-create/tap, `held_note_by`) and
  **4122–4132** (generate / mutate clears).
- Playback, scheduler, timing, note ownership, gestures.
- `draw_main` **6814**: the V_SEQ dispatch at **6888–6889** is already correct.
- `draw_drum`, `draw_break`.
- Engine voice counts, Chicago/Miami mono paths, streams 13/14/15.

---

## 6. Risks

- **Label width.** A six-name `FONT_B` string may not fit 240 px. The sketch
  already falls back to a smaller existing font, not a new widget.
- **NOTE knob width.** Compact `C3+N` is a compromise; the full list belongs
  on the held-step line.
- **Window expansion.** Including extras will change the vertical scale of a
  pattern that already has a wide chord. That is intended. Mono-only patterns
  are unchanged (`held_chord_get` returns 1).
- **TIE onset.** The helper must use `note_start_of`. Looking up the TIE
  index would still hide extras.
- **Slide** remains monophonic (root to next root). Honest limit; the store
  has one slide flag per step.
- **False notch.** If `lock_marks` is not filtered, adding extra bars would
  still show a fake p-lock on every chord.
- **Flash / pool.** Not measured on the JieLi toolchain. Host numbers would
  be HOST ESTIMATE only. Device delta is **MISSING** (no device build in this
  task). The code change is a short loop of at most six existing `night_mark`
  calls plus a lane-type check.
- **Playback identity.** Draw-only; audio must stay bit-identical. The test
  must keep a playback-identity check when the fix is applied.
- **Overhaul vs bounded fix.** Firmware UX Direction B (amber pills, hollow
  triangle) is a later unified design. This bound must not take those on.

---

## 7. Missing

- After frames: not produced; the fix was not applied.
- Device / JieLi flash and pool delta: **MISSING**.
- Orchestrator has not yet been given this final function list for a
  go-ahead to edit `ui.c` (required before any firmware write).
- `held_note_by` (**3472–3493**) already reads `held_chord_get` at
  `ui.held_step` and would miss extras on a TIE. That is a write-path
  issue, out of this display-only bound (also noted by Firmware UX).
- Drone five-note save vs four-voice engine is a store/engine fact, not a
  display bug. Not simulated here.
