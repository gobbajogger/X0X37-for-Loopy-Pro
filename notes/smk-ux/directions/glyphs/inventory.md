# Glyph inventory — SMK-37 at 94197fc8

Source worktree `/workspace/smk-ux-src` at `94197fc839b00aff47b98ca02f3c6396e4a6eabf`. Firmware is read-only. Every shipped picture below is **decoded** from the named table or draw function, not redrawn. The Python port is `glyphs/shipped_glyphs.py` (RGB565 `RGB()` / `swap16`, Bresenham `cv_line`, `px_blend` coverage 0..16, `CORNER_PACKED` rbox). The sheet is `glyphs/sheet-glyphs.png` (1× and 4×).

Idle forms are shown (no live-hit envelope). Live ink (hit age, `DG_BACK_MS` 300 ms, WOBBLE scroll) is documented, not animated.

Sizes are framebuffer pixels. Colours are firmware RGB565 (expanded). Mockup frames use the **AMBER** theme (`pal[4]` C_HI = RGB(255,184,70) expands to (255,186,66); `pal[3]` C_AMB = RGB(232,136,30) expands to (239,138,24)). Neutrals `C_DIM` RGB(128,146,148), `C_GRAY`, `C_LINE`, `C_WHITE` (`UI_N_WHITE`, not pure white) are shared. SOUND-matrix ink is `SOUND_INK` / `SOUND_WHITE` (`ui.c` 7206), face `SOUND_SELECT` / `SOUND_FACE`, independent of the theme palette. Part colour `PART_COLOURS` (`ui.c:89–93`) is the **slot** colour: JUNO A / DRONE A use `PART_303A` green RGB(130,240,90), not an engine colour.

---

## 1. Header and transport

| Glyph | Size | Source | Colour / screens | Meaning |
|---|---|---|---|---|
| `play_icon` | 9×11, AA triangle, `px_blend` | `ui.c:5153–5162` | Playing: `C_HI` / `C_WHITE` by metro phase (`5497`, compact `5402`). Stopped: `C_DIM`. Compact header x=175, y=4; full clock x=217, y=4. Also song-rec (`6071`). | Transport: running or stopped. **Not** `UI_G_PLAY` (that is a 9×9 pictogram, a different bitmap). |
| `UI_G_PATTERN` | 9×9 | Table `UI_PICTOGRAM` in `ui_pictograms.h:47`; painter `ui_pictogram` `ui.c:5184–5196` | `C_WHITE`. `ui_position_badge` / `header_pattern_badge` `5283–5293`, `5472`. | Current pattern (hash of 3×3 cells) next to the number. |
| `UI_G_NEXT` | 9×9 | `ui_pictograms.h:48`; painter `5184`; call `5215`, cue path `5293–5299`, `5470–5475` | Cue ink (amber). | Queued next pattern. |
| `UI_G_SONG` | 9×9 | `ui_pictograms.h:57` | When `seq.song_on` (`5472`). | Song position badge instead of pattern. |
| `UI_G_PREV`, `UI_G_UP`, `UI_G_DOWN`, … (43 entries, `UI_G_NONE`..`UI_G_FX_EMPTY`) | 9×9, 16-bit rows, 774 B flash (`UI_GLYPHS.md`) | `ui_pictograms.h:45–82` `UI_PICTOGRAM[UI_G_COUNT][9]`; painter `5184` | Mode, FX, mute, check, etc. `ui_pictogram(168, …)` mix on/off `6616`. | Compact semantic pictures. Constant flash, no mutable state (`ui_pictograms.h:2`). |
| Unsaved mark | 2 px `dot(237, 3, 2, C_AMB)` (or red on `SAVE_FAILED`) | `ui.c:5501–5502` | AMBER pal[3]. | Project dirty. |

Direction B’s **hollow** play triangle is a **PROPOSAL** (same 9×11 footprint, empty vs full). It is not in the shipped `play_icon`. See grammar / conflicts. It is on the sheet, labelled as such.

---

## 2. SOUND / PARA matrix — 26×19 engine pictures

Placement (all engines that have a strip picture): **(x+3, y+22)** in a 59×46 cell, `draw_param_widget` `ui.c:7265–7268`. Name at **y+11** (centred, `C_GRAY`). Value via `strip_value_fit` `7181–7198`, lane **x+30..x+57**, baseline **y+28**. Night 4-row slabs: first slab y = `HDR_H+4` = 24, step 50 (`5740–5769`). Box is `OG_W=26`, `OG_H=18` (`7745–7746`) plus a 1 px `C_LINE` floor, visually 26×19.

### 2.1 `drum_strip_glyph` — 909 / 808

Painter `ui.c:8044–8197`. Role map `drum_strip_role` `7968–7984`. Idle after `DG_BACK_MS` (300 ms) `8043`, `8051–8057`. Called from `7265` when `drum`.

| Role | Source | Meaning | Typical screens |
|---|---|---|---|
| `DG_BDSWEEP` | `8095–8120`; role `7977` for 909 BD **Tune**, **P.Dpth**, **Pitch** | Kick pitch drop (`drum909_bd_hz`: 49 Hz base, 16 ms hold, then τ). All three pots share this curve; live marker would differ, idle curve is the same family. | 909 SOUND, BD page |
| `DG_ATTACK` | `8121–8130` | Click spike in front of the body. Height follows Attack pot. | 909 BD Attack; also reused (add-only) for Juno/Drone Attack |
| `DG_DECAY` | `8062–8068` | Amplitude tail as long as the real time (`dg_tail_px` from ms). | 909 Decay / Tail; Juno Decay/Release and Drone Release reuse |
| `DG_NTAIL` | `8069–8072` | 909 SD TONE: snare noise decay. | 909 SD |
| `DG_TUNE` | `8073–8094` | Wave whose cycles follow pitch. 808 Tune and 909 cymbal Tune are bipolar (pot 64 = 0 st). | 808 / 909 cymbal |
| `DG_LPF` | `8131–8146` | 808 BD TONE: lowpass (300 Hz × 20^(pot/127)). | 808 BD |
| `DG_SHELLS` | `8147–8157` | 808 SD TONE: two shell resonators. | 808 SD |
| `DG_SNAPPY` | `8158–8167` | Noise burst, amount and fade. | 808/909 SD Snappy |
| `DG_SOUND` | `8168–8175` | Named switch: one pip per position. | SOUND switch |
| `DG_LEVEL` | `8176–8183`; role `7974` on MIX/PART | Ceiling; live envelope pulses segments. | MIX Level |
| `DG_ACCENT` | `8184+`; role `7976` when `r.b==NDRUM` and name Accent / Veloc | A plain hit next to an accented one; the gap is the depth. **This is the first cell of the KIT page** (`PR(R_ENG, T_909, NDRUM, 1)`), name **ACCENT**, not “KIT 0”. | 909 SOUND, group “KIT” |

Factory 909 BD (pots from `drum909.c` 80–82): Tune 34 (τ ≈ 12.96 ms, LIN 6..32), Attack 13 (~10 %), P.Dpth 0, Pitch 45. Mockup value text “14 Hz” for Tune is a unit error (firmware unit is ms) — kept as the displayed figure, flagged in the audit.

### 2.2 `orig_strip_glyph` — Chicago / Miami only on the shipped path

Painter `ui.c:7846–7959`. Slot test `orig_strip_slot` `7734–7740`: **returns 0 for JUNO and DRONE**. If that test were only flipped, Juno would draw **Miami** pictures (`chi` comes from `engine_bass_sound_bank` == `ORIGINAL_SOUND_CHICAGO`; Juno is not Chicago). Appendix: kind-specific **add-only** map. Chicago is the 303. Miami is never a 303 and is never labelled 303B.

| Kind | c | Name | Source | Picture |
|---|---|---|---|---|
| Chicago | 2 | CUTOFF | `7857–7865` | Lowpass response (`filter_glyph` family) |
| Chicago | 3 | PEAK / RES | `7857–7865` | Same family, resonance |
| Chicago | 4 | ENV | `7866+` | Lowpass knee the filter EG sweeps (`chicago_voice.c` law) |
| Chicago | 5, 6 | BITE, GNAW | `7904–7905` `og_bite` / `og_gnaw` | Soft clip / fold, `og_wave` `7813` |
| Chicago | 11 | ENV/GATE | `7907` | Toggle |
| Chicago | 15 | RANGE | `7919` | Octave stairs |
| Chicago | 12 | LEVEL | `7950` | VCA level (ramp / pips) |
| Chicago | 17 | FX TYPE | `7922`, `orig_fx_glyph` `7750–7812` | Qualitative effect identity |
| Miami | 2 | SUB | `7924–7927` | Osc square + sub-octave square |
| Miami | 3, 7 | PITCH / TUNE | `7928–7931` | Centre needle |
| Miami | 8 | XMOD | `7932` `og_fm` | FM wave |
| Miami | 10 | CRUSH | `7906` `og_crush` | Bit/rate crush |
| Miami | 12, 13 | WOBBLE depth / RATE | `7933–7941` | Live MG1 **scrolls** the wave (`og_shift`) |
| Miami | 15 | STEREO | `7942+` | L/R spread |

`orig_fx_glyph` types 0–11 (`7750–7812`): bypass, chorus pair, phaser, ring, compressor, decimator, EQ, ensemble trio, short delay, BPM delay, hall, gated reverb. Juno CHORUS reuses type 1 (add-only).

`og_wave` / Miami periodic path: the exact `ui_miami_wave_value` lives in `ui_waveform.h`. The host port is a **qualitative** periodic function of the same family, flagged for the Miami *oscillator* sheet entries. Juno WAVE does **not** use this path; it uses `draw_wave_glyph` shape 1, which is ported exactly.

### 2.3 `draw_wave_glyph`

`ui.c:6959–6998`. Shape 1 is a **continuous** saw-to-pulse mix (`mix` 0..127), the same law as `juno6.c` (`saw*(1-macro)+pulse*macro`). It is **not** a discrete SAW/PUL name. Placed at `(cx-10, y+28)` (or y+25 for shapes 2/3) from `7276`. 29-wide wave in the 26×19 visual slot.

Shipped Juno / Drone SOUND pages **do not call this**. They show name + percent + a 42 px bar (`orig_strip_slot` falls through; `wave_shape` / `original_wave_slot` skip JUNO/DRONE). That is a shipped fact, not a mock drop.

---

## 3. Knob-strip `control_glyph` and cousins

`control_glyph` `ui.c:7658–7728`, coordinates in the knob canvas (y 28..50), pasted at `KNOB_Y=158` by `draw_knobs` `8319+`, `8419`.

| Kind | Source | Picture |
|---|---|---|
| `R_DLEN` / `R_BLEN` / … | `7673–7677` | 4×2 of 7×7 `rbox`, first `j <= q*7/127` filled in part colour. For length 16, val=`len-1`=15, max=63 → 2 of 8 boxes. |
| `R_DRATE` / `R_BRATE` | `7678–7682` | Baseline + vertical bars, gaps `{12,8,6,16}` for 1/16, 1/16T, 1/32, 1/8T. |
| `R_BDIR` | `7683–7690` | Direction arrows, or a zigzag for random (val==3). |
| Pan / bipolar / DJF | `7666–7672` | Needle on a scale. |
| Neutral scale | `7718–7726` | 7 bars + a floor pip, for parameters with no schematic. |

Related (same strip y): `filter_glyph` `7517+`, `perform_glyph` `7548+` (TEMPO metronome, SWING, FILTER), `matrix_glyph` `7065+` (`GL_POWER`, `GL_CLOCK`, `GL_PEDAL`, `GL_UP`/`DOWN`/`BOTH`).

---

## 4. Step grid, p-lock, tie, slide

| Mark | Source | Meaning |
|---|---|---|
| `night_mark` | `ui.c:5253–5260` | Raised or flat rounded step cell. Hit / empty / playhead dim. `draw_drum` `5543`. |
| `night_panel` / `surface_panel` | `5246+` | Slab face. |
| P-lock notch | `5546`, `5620`, `5877` — a notch when `(lmarks >> c) & 1` | The step has its own values. |
| Accent / slide marks on 303 | `5640–5641`, `draw_303` | `BS_ACCENT`, `BS_SLIDE`. |
| Tie | 303 step flags / hold-help `4142` | Tie, not a 26×19 picture. |

---

## 5. What Juno and Drone ship today

SOUND page: **name + percent + 42 px bar**, no 26×19 picture. `orig_strip_slot` returns 0. This is correct shipped behaviour. Proposed A/B/C frames in this folder **add** pictures (see grammar). SHIPPED reconstructions of Juno/Drone in `directions/frames/` correctly have no param glyphs; the corrected copies under `glyphs/frames/` only restore header icons (`play_icon`, `UI_G_PATTERN`), and do not invent param pictures on the SHIPPED path.

---

## 6. How this inventory was used

`glyphs/render_corrected.py` composites the decoded idle glyphs onto every A/B/C (and SHIPPED-restored) 240×240 frame, written to `glyphs/frames/1x/` and `glyphs/frames/3x/` (nearest-neighbour). Existing files under `directions/frames/` are not overwritten. New Juno/Drone pictures are additions only.
