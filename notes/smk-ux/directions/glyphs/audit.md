# Glyph audit — why the glyphs were gone (“From mocks”)

**Answer:** the drop is **from the mocks**, and it **started in Dot’s original 01–06**. The evolved A/B/C directions (and the PIL “SHIPPED” reconstructions under `directions/frames/`) never decoded the firmware glyph tables. They substituted text, bars, and lookalike drawings. The engine pictures themselves were never removed from `ui.c`.

Firmware at `94197fc8` is unchanged. This audit is of the mock **renderers**, not of a device flash.

---

## 1. Where the drop started: Dot 01–06

Unaltered crops: `/workspace/smk-ux/mockups/dot-original/01-dot-original-1x.png` … `06-…`. Source of those six: Dot’s proposal sheet, documented in `/workspace/smk-ux/mockups/README.md`.

| Dot # | What Dot drew | Shipped glyphs | Drop? |
|---|---|---|---|
| 01 “Find a part” | A proposed HOME titled “DnBOX”. “125 BPM / STOP” (or PLAY) **words**. Large “PATTERN 01”. No 9×11 `play_icon`, no `UI_G_PATTERN` from `UI_PICTOGRAM`. | First sound in source is boot **909 SEQ** (`ui_init` → `V_SEQ`), with `play_icon` (`5153–5162`, at 217,4) and the pattern pictogram (`5472`). HOME does not play a first sound. | **Yes.** Text substituted for `play_icon` and `UI_G_PATTERN`. Wrong screen for first sound. |
| 02 “Put a hit” | A 2×8 numbered one-track pad grid. “P01 / STOP”. “BD BASS DRUM”. | `draw_drum` is a 12-row kit (`5511–5549`) with `night_mark` cells (`5543`) and p-lock notches (`5546`). LENGTH/RATE use `control_glyph` (`7673–7682`). | **Yes.** Invented grid; no `night_mark`, no `control_glyph`. STOP word instead of `play_icon`. |
| 03 “Edit the drum sound” | Banner “EDITING: SOUND”. **Huge DECAY 1.0 s** as the PARA identity. “909 / SOUND”, “P01 / STOP”. Tune/Attack/Decay/P.Dpth as **large names + values only**. | PARA is the 4-row night matrix (`5740–5834`). Each cell has a **26×19** `drum_strip_glyph` at (x+3, y+22) (`7265`, `8044–8197`): `DG_BDSWEEP`, `DG_ATTACK`, `DG_DECAY`, `DG_BDSWEEP` again for P.Dpth, plus PITCH and **ACCENT** (`DG_ACCENT`, `7976`). | **Yes. This is the main SOUND-page drop.** The renderer never called `drum_strip_glyph`. Text + a huge decay readout replaced the pictures. The KIT cell was not ACCENT. |
| 04 “Edit one step” | Banner “EDITING: THIS STEP ONLY”. A DECAY slider replacing the matrix. | Same 26×19 matrix as 03, plus the SOURCE line `STEP 5  < >: CHANCE  PARA` (`5803–5806`) and p-lock notches. | **Yes.** Same missing `drum_strip_*` tables. |
| 05 “Save while playing” | Title “SAVE QUEUED”, “125 BPM / PLAY” words, “DnBOX”. | `play_icon` in phase ink; `SAVE AT STOP` / `SAVES WHEN STOPPED` (`290–300`); pattern pictogram. | **Yes.** PLAY word, no decoded `play_icon` / `UI_G_PATTERN`. |
| 06 “Saved” | “PROJECT SAVED”, green persistent panel. | Word `SAVED`, not a new glyph; `play_icon` stopped = `C_DIM`. | Text panel; transport glyph still missing. |

**The drop started here.** Dot 01–06 never decoded `drum_strip_glyph`, `orig_strip_glyph`, `UI_PICTOGRAM`, or the exact `play_icon` (9×11 `px_blend` triangle). They invented a different PARA (huge DECAY, STOP/PLAY words, “909 / SOUND”).

---

## 2. Why A/B/C and the “SHIPPED” reconstructions still dropped them

Evolved directions (`render_directions.py`, `render_mockups.py`, `render_engines_juno_drone.py`) **restored** some chrome the firmware already has: the 12-row kit, the night 4-row PARA, a clock, a hash-like pattern badge, a triangular play mark. They still **did not decode the tables**.

Evidence in the renderers (not in firmware):

| Lookalike | Renderer | What the firmware actually is |
|---|---|---|
| PIL filled triangle (`play_icon` in `render_mockups.py` ~220; `play_mark` / `play_icon_outline` in `render_directions.py` 72–90) | Polygon, not `px_blend` coverage 0..16 | `ui.c:5153–5162` |
| “a small hash/grid” (`render_mockups.py` ~231–234, `pattern_badge`) | Invented 10×7 hash | `UI_G_PATTERN` in `UI_PICTOGRAM` (`ui_pictograms.h:47`), painter `ui.c:5184` |
| Cue: `draw.polygon` 5 px chevron (`render_directions.py` `draw_cue` 129–146) | PIL triangle | `UI_G_NEXT` (`ui_pictograms.h:48`), call `5215` |
| SOUND cells: large name + large value, **no 26×19** (`render_mockups.py` fixtures 477, 567, 573). Comment: “simple value, no perform_glyph (reconstruction)” (~539) | Text only. Names were placed in the band where the firmware puts the glyph (mockup TUNE around y+16..y+23; firmware glyph at y+22, name at y+11). | `drum_strip_glyph` `8044`, `orig_strip_glyph` `7846`, at (x+3, y+22) |
| Juno/Drone **proposed** cells: hand-drawn lookalikes (`render_engines_juno_drone.py` `glyph_wave`, `glyph_sub`, … ~122–312) | PIL lines inspired by comments, not a byte-exact port | WAVE must be `draw_wave_glyph` shape 1 (`6959–6998`). SUB/LEVEL/CUTOFF/… must call the existing painters. |
| Juno/Drone **SHIPPED** reconstructions | Name + percent + 42 px bar, **correctly** with no 26×19 | That is what ships (`orig_strip_slot` returns 0). Not a drop. |
| SEQ LENGTH/RATE | Label and “16” / “1/16” only, no `control_glyph` | `ui.c:7673–7682` |

The `directions/frames/` “SHIPPED-\*” PNGs are **PIL reconstructions**, not device photographs (README and `mockups/README.md` say this). They drop the same 26×19 engine glyphs on 909 SOUND (`SHIPPED-03`, `SHIPPED-04`). They are marked as reconstructions in this audit.

---

## 3. Per-frame, per-direction: what was missing, and what was restored

“Missing” means the **evolved A/B/C (or SHIPPED reconstruction) frame under `directions/frames/1x/`** lacked the decoded shipped picture. “Restored” means the corrected frame under `glyphs/frames/1x/` (and 3×) now shows it, decoded from the cited source.

Legend: **M** = missing in the mock; **R** = restored in `glyphs/frames`; **P** = B hollow play (proposal, not a drop); **S** = correctly absent on the SHIPPED Juno/Drone path; **T** = SHIPPED toast covers the header (1300 ms), icons not stamped on the banner.

### 3.1 SEQ frames (01, 02, 05, 06, 07) — all four styles A, B, C, SHIPPED

| Glyph | Source (file:line, table) | A | B | C | SHIPPED | Why the mock dropped it |
|---|---|---|---|---|---|---|
| `play_icon` 9×11 | `ui.c:5153–5162` | M→R (filled, dim when stopped, amber when playing) | M→R; **stopped uses hollow** (P, documented) | M→R filled | M→R filled, except **T** on `SHIPPED-05-save-toast` and `SHIPPED-06-saved` (amber banner over the header — left alone) | PIL polygon / “STOP” “PLAY” words (from Dot 01). Renderer never called `play_icon`. |
| `UI_G_PATTERN` 9×9 | `UI_PICTOGRAM` `ui_pictograms.h:47`; painter `5184` | M→R | M→R | M→R | M→R (not T) | `pattern_badge` drew a hash lookalike; table never decoded. |
| `UI_G_NEXT` 9×9 (07-live only) | `ui_pictograms.h:48`; `5215`, `5470–5475` | M→R at cue x=154 | M→R (inside B’s outline) | M→R (next to C’s ticket) | M→R | `draw_cue` used a 5 px PIL polygon. |
| `control_glyph` `R_DLEN` | `ui.c:7673–7677` | M→R (4×2 rboxes, 2 of 8 filled at length 16) | M→R | M→R | M→R | Text “16” only. Function never called. |
| `control_glyph` `R_DRATE` | `ui.c:7678–7682` | M→R (1/16 bars, gap 12) | M→R | M→R | M→R | Text “1/16” only. |
| `night_mark` kit cells | `5253–5260`, `5543` | Present in evolved A/B/C (restored earlier from Dot 02’s invented 2×8 grid). Not re-decoded this pass; the evolved kit is the firmware 12-row geometry. | same | same | same | Drop started in Dot 02; evolved directions already put the kit back. |

### 3.2 909 SOUND frames (03-sound, 04-step) — A, B, C, SHIPPED

Decay 0.50 s on 03, 1.50 s on 04. Same missing set on both.

| Glyph | Source | A/B/C and SHIPPED reconstruction | Why dropped |
|---|---|---|---|
| `DG_BDSWEEP` on TUNE | `drum_strip_glyph` `8095–8120`; role `7977` | M→R, idle, factory pot 34 | Text “14 Hz” only. `render_mockups.py` never called `drum_strip_glyph`. Names sat in the glyph band. |
| `DG_ATTACK` on ATTACK | `8121–8130`, pot 13 | M→R | Text “10 %” only. |
| `DG_DECAY` on DECAY | `8062–8068` | M→R (0.50 s / 1.50 s) | Huge name + “0.50 s” / “1.50 s”. Dot 03 used a huge DECAY as the PARA identity. |
| `DG_BDSWEEP` on P.DPTH | `8095`, `7977` | M→R, m=0 | Text “0 %” only. |
| `DG_BDSWEEP` on PITCH | `8095`, `7977` | M→R | Text “0.40” only. |
| `DG_ACCENT` on the KIT page cell | `8184+`; role `7976` (`r.b==NDRUM`, name Accent) | M→R. Mock label “KIT 0” **rewritten to ACCENT 127** (shipped name). Row title “3 KIT” kept. | The mock invented a “KIT 0” cell. The renderer had no `DG_ACCENT` path. |
| `play_icon`, `UI_G_PATTERN` | as in §3.1 | M→R (A/B/C play at 217; SHIPPED compact at 175) | Same PIL lookalikes. |

Value figures from the mock (“14 Hz”, “10 %”, …) were kept and moved to the shipped lane (x+30..x+57, y+28). Tune’s **unit** “Hz” is wrong (firmware is ms, LIN 6..32); flagged, not silently “fixed” as a new number, so the mock fixture stays comparable. Names were moved to the shipped y+11 so the 26×19 can sit at y+22 without clipping (the mock had placed TUNE/ATTACK/… where the glyph belongs).

### 3.3 Juno and Drone proposed frames (A/B/C only)

Shipped Juno/Drone have no 26×19 pictures — that is **not** a mock drop (`orig_strip_slot` returns 0). The A/B/C **proposed** frames already tried to add pictures, but they were **lookalikes**. Those were replaced with decoded reuse + add-only new drawings (see grammar). Header `play_icon` and `UI_G_PATTERN` were also lookalikes → restored.

| Cell | What A/B/C proposed had | What the corrected frame shows | Source |
|---|---|---|---|
| WAVE | PIL `glyph_wave` lookalike | Exact `draw_wave_glyph` shape 1, mix from 33 % (pot ~42) | `ui.c:6959–6998` |
| SUB | PIL lookalike | Exact Miami SUB, `orig_strip_glyph` c=2, `chi=0` | `7924–7927` |
| WIDTH | PIL pulse lookalike | **NEW** `glyph_juno_width` (addition), 26×19, same stroke | grammar |
| LEVEL | PIL ramp | Exact Chicago LEVEL family, c=12, `chi=1` | `7950` |
| CUTOFF / RES / ENV | PIL LPF lookalikes | Exact Chicago c=2,3,4 | `7857–7866` |
| HPF | PIL lookalike | **NEW** `glyph_juno_hpf` | grammar |
| ATTACK | PIL lookalike | Exact `DG_ATTACK` | `8121` |
| DECAY / RELEASE | PIL lookalike | Exact `DG_DECAY` (308 ms / 407 ms factory) | `8062` |
| SUSTAIN | PIL trapezoid | **NEW** `glyph_juno_sustain` | grammar |
| LFO RATE / LFO DEPTH / CHO RATE | PIL LFO | Exact Miami WOBBLE c=12/13 | `7933–7941` |
| CHORUS | PIL lookalike | Exact `orig_fx_glyph` type 1 | `7750+` case 1 |
| Drone REGISTER | PIL bars | **NEW** `glyph_drone_register` | grammar |
| Drone BRIGHT | PIL LPF | Exact Chicago CUTOFF family | `7857` |
| Drone FLUTTER | PIL LFO | Exact Miami WOBBLE RATE | `7933` |
| Drone ATTACK / RELEASE / LEVEL | PIL | Exact `DG_ATTACK`, `DG_DECAY`, Chicago LEVEL | as above |

A 9×9 engine identity (`JUNO_ID` / `DRONE_ID`) was designed (same painter as `ui_pictogram`) and is on `sheet-glyphs.png`. It is **not** stamped on the A/B/C frames: the header has only a 6–7 px gap between the part chip and “SOUND” (measured). An earlier attempt at x=68 overwrote the mode word. Placement is a phase-2 layout question.

`shipped-juno.png` / `shipped-drone.png`: header icons only; **no** invented param glyphs (S).

---

## 4. PIL mockups vs source reconstructions

| Set | What it is | Glyph fidelity |
|---|---|---|
| Dot 01–06 | Unaltered proposal crops | Invented chrome. Drop starts here. |
| `mockups/1x/*-CURRENT.png` and `directions/frames/SHIPPED-*.png` | **PIL reconstructions** of 94197fc8, cited but not a host framebuffer and not a photograph | Kit and night matrix geometry are sourced; 26×19 tables and exact `play_icon` / `UI_PICTOGRAM` were **not** decoded. |
| `directions/frames/A,B,C-*.png` | Evolved direction mockups | Lookalike play, hash badge, still no decoded 26×19 on 909; Juno/Drone proposed lookalikes. |
| `glyphs/frames/1x` and `3x` | This work: existing frames composited with **decoded** painters | Shipped pictures pixel-exact from the tables (idle). New Juno/Drone pictures are additions, labelled on the sheet. |
| `docs/mockups/glyph-polish/*` in the source tree | Real **host framebuffer** sheets (e.g. `909-attack-frames-01.png`, `909-tune-values.png`, `original-controls-atlas`) | Ground truth used to visually check the Python port (attack / decay / sweep / wave). |

---

## 5. What was not a “drop”

- Shipped Juno/Drone SOUND (percents and a bar) — `orig_strip_slot` returns 0 by design.
- Direction B’s hollow play — a **documented proposal**, same footprint, not an accidental removal. Owner rule “remove none”: listed as a conflict (grammar).
- Chicago / Miami screens were not in the A/B/C six-journey set; their 26×19 tables are inventoried and on the sheet, ready for phase 2.

---

## 6. Renderer cause, in one line

`render_mockups.py`, `render_directions.py`, and `render_engines_juno_drone.py` **never decoded** `UI_PICTOGRAM`, `drum_strip_glyph`, `orig_strip_glyph`, `draw_wave_glyph`, `control_glyph`, or the exact `play_icon`. They substituted PIL polygons, a hash, large TTF names, and (for proposed Juno/Drone) lookalike line drawings. That habit begins in Dot 01–06.
