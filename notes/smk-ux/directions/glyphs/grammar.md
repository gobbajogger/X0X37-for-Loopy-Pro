# Glyph grammar — foundation for phase 2

Phase 1 only. This is the visual language the six engine SOUND pages, SEQ, and the Juno/Drone additions should share. Phase 2 (coherent six-engine screens, SEQ chord / piano-roll §9/§10, presets, first-play, 2–3 alternatives) is **not** started. A short layout-problem list for that work is at the end.

Shipped pictures are **pixel-identical** to the decoded source (idle). They are not redrawn. New Juno/Drone pictures are **additions** in the same stroke, grid, and colour roles. Nothing shipped is removed.

---

## 1. Grid and placement

| Slot | Size | Position | Source |
|---|---|---|---|
| Engine / param picture | 26×19 (`OG_W=26`, `OG_H=18` + 1 px floor) | Cell (x+3, y+22) | `ui.c:7265–7268`, `7745–7746` |
| Param name | FONT role T, centred | y+11, `C_GRAY` | `draw_param_widget` |
| Param value | `strip_value_fit`, lane 27 px | x+30..x+57, y+28. Number in the cell ink; unit in `C_GRAY` | `7181–7198` |
| Cell | 59×46 in a 4-row night matrix | x = 4+col×59, slab y = 24+row×50 | `5740–5769` |
| `play_icon` | 9×11 | 217,4 (full clock) or 175,4 (compact) | `5497`, `5402` |
| Pictogram | 9×9, 16-bit rows, scale 1..3 | Pattern badge 132,5; NEXT on the cue | `5184`, `5472` |
| Knob-strip schematic | y 28..50 in the 82 px band | `KNOB_Y=158`; LENGTH cx=30, RATE cx=90 (60 px cells) | `7658–7728` |
| Screen | 240×240, strips 20+69+69+82 | Night PARA uses the body below the 20 px header | — |

Mockup 909 PARA had placed large names in the glyph band (around y+16..y+23). Corrected frames **move the name to y+11** (firmware) and put the decoded 26×19 at y+22. That is a layout correction so the shipped picture can exist, not a restyle of the picture.

---

## 2. Stroke and pixel rules (do not invent a second language)

- One-pixel stroke. Curves are firmware `cv_line` (Bresenham) and `px_blend` with coverage **0..16**.
- Rounded boxes use `CORNER_PACKED` rbox, antialiased into the **face**, never into a foreign sentinel colour.
- A `C_LINE` floor (`cv_rect` 26×1 at the bottom of the 26×19 box) is part of the engine-glyph family (`7816`, `7862`).
- RGB565 via `RGB()` (`gfx.c`) and `swap16`. Host frames are expanded 888. Do not paint pure #FFFFFF for `C_WHITE`.
- Active SOUND ink: `SOUND_INK` on `SOUND_SELECT`. Idle: `SOUND_WHITE` on `SOUND_FACE` (`7206`). SEQ strip schematics use the **part colour** (`PART_909` orange on the 909 kit). Theme C_HI / C_AMB follow the AMBER palette of these mockups (see inventory).
- Live vs idle: these frames are **idle** (no hit envelope, no WOBBLE scroll, no metro-phase flicker except the playing/stopped play-icon ink). Live behaviour stays in the firmware painters for phase 2.

---

## 3. What may change, and what may not

| Item | Rule |
|---|---|
| Shipped bitmaps and draw functions | **Restore, do not redraw.** Decode the table. |
| Meaning, muscle memory, cell order | Keep. TUNE stays TUNE, ACCENT stays Accent (the KIT **page** name stays “KIT”). |
| Juno WAVE | Continuous saw↔pulse via existing `draw_wave_glyph` shape 1. **No** discrete SAW/PUL names. |
| Chicago vs Miami | Chicago is the 303. Miami is never a 303 and is never labelled 303B. |
| Kind map | **Add-only** and kind-specific. Do **not** only flip `orig_strip_slot` — that would draw Juno as Miami. Do not rewrite Chicago or Miami arms. |
| New Juno/Drone pictures | Additions in this grammar. Unique draw functions only where reuse cannot speak (WIDTH, HPF, SUSTAIN, REGISTER, optional 9×9 identity). |
| Direction B hollow play | Same 9×11 footprint. Empty = stopped is a **proposal**, not shipped. See conflicts. |
| Fonts in mocks | Barlow TTF is a HOST ESTIMATE. Felucca on device will differ by a few px. Periods at T 10 px are unreliable; corrected values paint a 2×2 baseline dot (HOST ESTIMATE workaround). |

---

## 4. Juno and Drone additions

Factory figures from `engines-juno-drone.md` (JUNO POLY / Cathedral). Reuse is preferred. “New” means a new **draw function** in the 26×19 box, not a rewrite of a shipped table.

### Juno (JUN-60)

| Cell | Picture | Bytes (HOST ESTIMATE) |
|---|---|---|
| WAVE 33 % | **Reuse** `draw_wave_glyph` shape 1, mix ≈ 42 | 0 (already in `ui.c`) |
| SUB 22 % | **Reuse** Miami SUB (`orig_strip` c=2, chi=0) | 0 + dispatch (kind map) |
| WIDTH 50 % | **NEW** `glyph_juno_width`: one pulse, duty = `0.1+q·0.8` (`juno6.c`) | ~40–80 B Thumb-2 |
| LEVEL 79 % | **Reuse** Chicago LEVEL (c=12) | 0 |
| CUTOFF / RES / ENV | **Reuse** Chicago c=2, 3, 4 | 0 |
| HPF 0 % | **NEW** `glyph_juno_hpf`: rising mirror of the shipped lowpass knee | ~40–60 B |
| ATTACK 14 ms | **Reuse** `DG_ATTACK` | 0 |
| DECAY 308 ms / RELEASE 407 ms | **Reuse** `DG_DECAY` | 0 |
| SUSTAIN 72 % | **NEW** `glyph_juno_sustain`: rise, hold at q, short drop | ~40 B |
| LFO RATE 0.2 Hz / LFO DEPTH 6 % / CHO RATE 1.1 Hz | **Reuse** Miami WOBBLE c=12/13 | 0 |
| CHORUS 35 % | **Reuse** `orig_fx_glyph` type 1 | 0 |
| Identity 9×9 | **NEW** `JUNO_ID`, same packing as `UI_PICTOGRAM` (two rising ramps). On the sheet only; no header slot yet | 18 B table + enum |

### Drone (String Machine)

| Cell | Picture | Bytes (HOST ESTIMATE) |
|---|---|---|
| REGISTER 45 % | **NEW** `glyph_drone_register`: three rank pipes, height follows q | ~30–50 B |
| BRIGHT 52 % | **Reuse** Chicago CUTOFF (brightness as a lowpass knee — honest to a tone control, not a new oscillator story) | 0 |
| FLUTTER 40 % | **Reuse** Miami WOBBLE RATE | 0 |
| ATTACK 70 ms | **Reuse** `DG_ATTACK` | 0 |
| RELEASE 1.8 s | **Reuse** `DG_DECAY` | 0 |
| LEVEL 75 % | **Reuse** Chicago LEVEL | 0 |
| Identity 9×9 | **NEW** `DRONE_ID` (three organ-rank pipes). Sheet only | 18 B |

### Kind map (required, add-only)

A kind-specific map in `orig_strip_slot` + `wave_shape` / `original_wave_slot` (or a small role table) must send Juno WAVE to `draw_wave_glyph`, Juno SUB to Miami SUB, Juno filter to Chicago CUTOFF/RES/ENV, etc. **Do not** only flip the `orig_strip_slot` test. ESTIMATE 16 B table + 80–200 B dispatch + 16–40 B wave (from `engines-juno-drone.md` rank 2).

### Flash vs 248 B Elite, and the chord namer

Firmware Main Builder measured **248 B Elite / 256 B Pro** above the 8192 B floor (148 B pool) at this SHA. Preset storage is already blocked on flash. The chord namer is **285 B measured** and is being added **now** on a separate path — it **already exceeds** the 248 B Elite headroom on its own.

This glyph work, if implemented:

- Reuse (WAVE, SUB, LEVEL, filter, tails, LFO, chorus): dispatch only, see kind-map ESTIMATE.
- Unique functions (WIDTH, HPF, SUSTAIN, REGISTER) + two 9×9 identities: roughly **200–300 B** HOST ESTIMATE on top of the map.
- Combined units (rank 1 in the appendix) + glyph map + unique functions: can exceed 248 B **even without** the namer. With the namer (285 B), the budget is already gone.

All figures except the 248/256/148 and the 285 B namer are **HOST ESTIMATE** or **ESTIMATE**, not a linked size. **Measure at link** before unique bitmaps or extra functions land. Ranked cuts if over: keep WAVE, SUB, filter, tails, LFO, chorus, LEVEL, and ms/Hz text; drop ENV knee extras, WIDTH pulse, REGISTER heights, or the 9×9 identities. 0 additional RAM if no new cache is declared. This work does **not** edit firmware or the chord-namer branch.

---

## 5. Corrections already applied in `glyphs/frames` (not a restyle)

- Decoded `play_icon`, `UI_G_PATTERN`, `UI_G_NEXT`, `control_glyph` DLEN/DRATE, and all 909 BD 26×19 idle pictures, composited onto every redesign frame.
- 909 PARA names moved to y+11; values to the shipped right lane. “KIT 0” → **ACCENT 127**.
- Juno/Drone lookalikes replaced by decoded reuse + the add-only drawings above.
- B stopped: hollow play (proposal, labelled).
- SHIPPED long-toast frames: header left alone (banner covers the clock).
- Identity 9×9 not forced into a 6 px gap.

Existing `directions/frames/` PNGs are **not** overwritten. SHA256SUMS gains `glyphs/` hashes only.

---

## 6. Owner-rule conflicts (do not hide)

1. **“Remove none” vs B’s hollow play.** B replaces the filled stopped triangle with an empty one of the same size. This is a reserved outline language (empty = stopped, filled = playing), documented in direction B, not a silent deletion. If the owner wants every frame to show the **shipped** filled `play_icon` even when stopped, use the A/C treatment.
2. **Tune “14 Hz”.** The mock fixture printed Hz; the firmware Tune axis is milliseconds (LIN 6..32, pot 34 ≈ 13 ms). The corrected frames keep “14 Hz” so the journey figure is comparable, and this audit flags the unit.
3. **Flash.** 248 B Elite headroom vs 285 B chord namer (measured, in progress) vs this add-only map (ESTIMATE). Combined units+glyphs+namer cannot all fit without cuts. Measure at link. No live-device claims.
4. **Identity 9×9 vs “every frame must show them.”** Param pictures are on every Juno/Drone A/B/C frame. The identity pictogram has no honest header slot without overlapping “SOUND”. It lives on the sheet until phase 2 makes a slot.
5. **A/B/C are not final** (steering). The owner is not asked to pick a direction in this phase. The grammar is the foundation.

---

## 7. Layout-level problems across the six engine screens (seeds for phase 2)

Do not start these now; they are the brief for a later coherent pass.

1. **Two SOUND grammars.** 909 / 808 / Chicago / Miami already have 26×19 pictures in a 4-row night matrix. Shipped Juno / Drone are name + percent + a 42 px bar. A single “flip `orig_strip_slot`” would paint Juno with Miami’s pictures. Phase 2 needs one kind-specific page that *looks* like one instrument family, not six templates with a bar or a glyph bolted on.
2. **WAVE is a mix, not a switch.** Any Juno screen that names SAW and PUL as discrete positions is wrong. The picture must move continuously (`draw_wave_glyph` shape 1 / `juno6.c`).
3. **Mock PARA vs firmware PARA.** Evolved 909 cells put a large name where the glyph belongs, and a large value under it, with no 27 px value lane. Restoring the picture forced the name up. A phase-2 909 page should be designed *around* the 26×19 from the start, and the same cell should work for Chicago, Miami, Juno, and Drone.
4. **Wrong units and wrong cell names.** Tune as “Hz”; KIT page cell as “KIT 0” instead of ACCENT. Phase 2 should follow `pref_desc` names and the engine’s own axis (ms, Hz, %, st) — HOST ESTIMATE text until felucca is used.
5. **Header overcrowding.** Part chip + mode word + UNSAVED ticket + pattern badge + cue + tempo + `play_icon` + unsaved dot already fill 240 px. A 9×9 engine identity does not fit today (measured 6–7 px gap). Phase 2 must *make a slot* or drop the identity, not overlap “SOUND”.
6. **Slot colour is not engine identity.** JUNO A and DRONE A use `PART_303A` green. Six engines in one header language will read as “the 303’s colour” unless phase 2 decides how engine identity and slot colour share the chip.
7. **SEQ LENGTH/RATE vs PERFORM.** The 82 px strip already has `control_glyph` and `perform_glyph`. Journey frames show LENGTH/RATE as text-first. Phase 2 should treat the schematic as first-class on first play, consistent with SOUND’s 26×19.
8. **SEQ chords / piano-roll** must follow `seq-chords/piano-roll.md` §9 / §10 (read, do not edit). The current A/B/C SEQ grid is the 12-row kit, not that roll. Do not invent a third chord picture.
9. **First-play vs HOME.** Dot 01 was a “DnBOX” HOME that cannot play a first sound. The evolved 01 is 909 SEQ, which is right, but the six-engine first-play (how you meet Juno or Drone on a new machine, presets from `presets-spec.md`) is still unsolved.
10. **TTF vs felucca.** Barlow at 10 px drops periods and is wider than the 27 px value lane (“308 ms”). Phase 2 device frames should use packed felucca, or a measured HOST ESTIMATE that does not invent dots.
11. **Miami is not a 303.** Any six-engine sheet that labels Miami “303B” or draws it with Chicago’s bass-filter story is wrong. Chicago owns the 303 pictures; Miami owns SUB, XMOD, WOBBLE, CRUSH, STEREO.
12. **Budget.** Phase 2 layouts that assume new bitmaps will lose to the 248 B Elite floor and the 285 B namer. Design first with **reuse and the kind map**; unique functions are a measured last resort.

---

## 8. Deliverable map

| Path | What |
|---|---|
| `glyphs/inventory.md` | Every shipped glyph, file:line, table, size, colour, screens |
| `glyphs/audit.md` | What was missing, per frame and direction; why; Dot 01–06 as the start of the drop |
| `glyphs/grammar.md` | This file |
| `glyphs/shipped_glyphs.py` | Pixel-exact (idle) Python port of the painters |
| `glyphs/render_corrected.py` | Compositor |
| `glyphs/frames/1x/`, `glyphs/frames/3x/` | Corrected A/B/C (and SHIPPED-restored) 240×240, Juno and Drone included |
| `glyphs/sheet-glyphs.png` | Every glyph at 1× and 4×, shipped and additions, with citations |
| `glyphs/render_sheet.py` | Sheet generator |
