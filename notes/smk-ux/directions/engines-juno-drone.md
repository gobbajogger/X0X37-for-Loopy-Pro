# Appendix: Juno and Drone engine pages

An update to the A / B / C directions at `/workspace/smk-ux/directions/`, not a fourth visual language. The pictures and the units proposed here are the same in every direction; only that direction’s header, footer, and marks change.

Firmware at `94197fc839b00aff47b98ca02f3c6396e4a6eabf` is read-only. Frames are reconstructions and proposals, not a device photograph and not a host-UI run. Fonts are repo TTFs, not packed felucca bitmaps; advances may differ by a few pixels (HOST ESTIMATE). PROPOSED is not implemented. Elite and Pro interaction is identical in source.

Chord or note drawing on the SEQ piano roll is a separate design task; see `/workspace/smk-ux/seq-chords/`. This file covers only Juno and Drone engine sound and parameter pages.

## What ships (reconstruction)

Night 4-row PART matrix (`draw_part_sound` `ui.c` 5740–5848; `matrix_three_rows` is 0 at 4877–4878; height 49, step 50). One screen is a bank of four pages (`first = page/4*4`). Juno’s four parameter pages therefore appear together; Drone’s two parameter pages share the first bank with the SOUND row (ENGINE / PRESET / STATUS, added at 1524).

Header is the compact night path (5373–5408): `SOUND_GREEN` name, bank dots, pattern cue at 132, play icon at 175, no tempo, no `SOUND` word. The name is always `JUNO A` / `JUNO B` / `DRONE A` / `DRONE B` from `bidx()` (5383–5387), even when `part_display` / `bass_display_name` (117–124) would shorten to `JUNO` or `DRONE` because the other slot is a different kind.

Every Juno and Drone parameter is a name, a percent, and a 42 px bar. There is no 26×19 picture.

| Cause | Where | What it does |
|---|---|---|
| `original_value_text` early-return | `ui.c` 1075 | For every Juno and Drone id, `(value*100+63)/127` and `"%"`. Times, rates, and pulse width never reach `unit_value` (1121–1142). |
| `ui_original_states` returns 0 | `ui_original_controls.h` 56–68 | No discrete names (no SAW/PUL, no ON/OFF). |
| `ui_original_controls_project` returns 1 at once | `ui_original_controls.h` 106 | Empty view; “no Chicago/Miami fields.” |
| `engine_param_si` default | `engine.c` 512–521 | `X0X_U_NONE` for T_303. |
| `wave_shape` / `original_wave_slot` | `ui.c` 6913–6951 | Only Chicago WAVE (`c==0`) and Miami WAVE 0 / 6 and SHAPE 11. Slot is −1 for JUNO and DRONE. |
| `original_wave_view` | `ui.c` 2911–2915 | Returns 0 for JUNO and DRONE (“drone has no Miami oscillator UI”). |
| `orig_strip_slot` | `ui.c` 7734–7740 | Returns 1 only for listed Chicago and Miami ids. JUNO and DRONE fall through to 0. |
| `adsr_row` | `ui.c` 7369–7374 | Chicago ENVELOPE slots 7–10 only. Juno’s ATTACK/DECAY/SUSTAIN/RELEASE (ids 5–8, one page at `ui_original_juno_rows` `{5,6,7,8}`) do not get the graph. `adsr_values` (7312) uses Chicago VCA time laws; they must not be reused as-is. |
| `draw_param_widget` fallback | `ui.c` 7204–7305, 7280–7300 | Name at `y+11` (`FONT_T`), value at `y+26`, bar at `height-4`. The glyph path (7261–7279) is never taken. |

Factory fixture, JUNO POLY and Cathedral (`original_sound.c` 39, 52), displayed with the shipped percent law:

- Juno: WAVE 33 %, SUB 22 %, WIDTH 50 %, LEVEL 79 %, CUTOFF 55 %, RES 14 %, ENV 35 %, HPF 0 %, ATTACK 6 %, DECAY 31 %, SUSTAIN 72 %, RELEASE 31 %, LFO RATE 12 %, LFO DEPTH 6 %, CHO RATE 20 %, CHORUS 35 %.
- Drone: REGISTER 45 %, BRIGHTNESS 52 %, FLUTTER 40 %, ATTACK 18 %, RELEASE 65 %, LEVEL 75 %.

Those percents hide the DSP laws (`juno6_set` `juno6.c` 70–86, chorus in `original_runtime.c` 101, `drone_string_refresh` `drone_string.h` 61–72): Juno attack `0.002+x²·3` s (14 ms at factory 8), decay `0.01+x²·3` s (308 ms), release `0.01+x²·4` s (407 ms), LFO rate `0.1+8·x²` Hz (0.2 Hz), chorus rate `0.1+4.9·x` Hz (1.1 Hz), pulse width `0.1+x·0.8` (50 %). Drone attack `0.004+x²·2` s (70 ms), release `0.08+x²·4` s (1.8 s).

WAVE is a continuous saw-to-pulse mix in the renderer (`juno6.c` 41: `saw*(1-macro[0])+pulse*macro[0]`). `s->waveform=(uint8_t)(x*2.99f)` is set and unused. A discrete SAW/PUL/MIX readout would be invented. The percent is the honest text; the missing piece is the existing saw-to-square wave picture (`draw_wave_glyph` shape 1, `ui.c` 6990–6998), already used for Chicago WAVE.

### Labels (TTF HOST ESTIMATE)

Cell 59 px, `FONT_T` 10 SemiBold, centred at `x+29`. Repo TTF advances, not felucca:

| Label | Width | Slack in 59 |
|---|---|---|
| BRIGHTNESS | 51 | 4 px each side of centre (left at +4). Tight. Felucca may clip. |
| LFO DEPTH | 44 | 7–8 px. Fits. |
| REGISTER | 40 | Fits. |
| LFO RATE, CHO RATE | 38, 39 | Fit. |
| LFO/CHORUS (page title, not a cell) | 52; `"4  LFO/CHORUS"` is 61, ends at 69 | `"KNOBS 5-8"` starts at 189. No collision. |

`x0x_param_t` comments ask for names of six characters or fewer. `BRIGHTNESS` is the outlier (ten). Chicago already uses ATTACK/RELEASE at seven and eight.

### What the other engines have, that Juno and Drone lack

Chicago and Miami take `orig_strip_glyph` (7846–7960) and `draw_wave_glyph` (6959–6998) in the same 26×19 box: WAVE (saw/square mix or Miami discrete), SUB squares, CUTOFF/PEAK lowpass, ENV knee, LEVEL meter, RANGE octave stairs, FX TYPE including a chorus pair (`orig_fx_glyph` 1 and 7), Miami WOBBLE `og_lfo_amp` / `og_lfo_rate`, GLIDE, BITE, GNAW. 909/808 use `drum_strip_glyph` (7961+) with live `dg_tail` attack and decay. Juno and Drone have none of these on the page.

## What is proposed

Same 20+69+69+82 strip, same 4×49 matrix, same gestures, same write path. No new glyph bitmaps. Each proposed picture is an existing primitive, reached by a kind-specific mapping — **not** by flipping `orig_strip_slot` alone.

If `orig_strip_slot` merely returned 1 for JUNO, `orig_strip_glyph` would treat the engine as Miami (`chi==0`) and map by control index. Juno SUB (id 1) would be Miami’s LEVEL meter, CUTOFF (2) would be SUB squares, ATTACK (5) a LEVEL meter, LFO RATE (10) CRUSH, CHORUS (14) LEVEL, LEVEL (15) STEREO. That is worse than a bare percent. A small role table, or `else if (kind==JUNO)` branches that call the existing helpers and leave the Chicago and Miami arms unchanged, is required.

| Control | Proposed picture (reuse) | Proposed text |
|---|---|---|
| Juno WAVE | `draw_wave_glyph` shape 1, `mix=value` (Chicago WAVE) | 33 % (continuous mix; no invented SAW/PUL) |
| Juno SUB | Miami SUB squares (`!chi && c==2`) | pot % |
| Juno WIDTH | Pulse whose duty is `0.1+x·0.8` (same wave family) | actual width, 50 % at factory |
| Juno LEVEL, Drone LEVEL | LEVEL meter (`level_role`) | pot % |
| Juno CUTOFF / RES | Lowpass knee + optional peak bump (`c==(chi?2:9)` / PEAK) | pot % |
| Juno ENV | Chicago ENV knee, static, reading Juno CUTOFF (2) and this ENV | pot % |
| Juno HPF | Mirror of the same lowpass primitive (a rise). Not `UI_G_FX_HIGHPASS` (9×9). 0 new bytes of bitmap. | pot % |
| Juno ATTACK / Drone ATTACK | Rising ramp, length from the pot (tail family, inverted) | `unit_value` `X0X_U_S` → 14 ms, 70 ms |
| Juno DECAY / RELEASE, Drone RELEASE | `dg_tail` (already shared, 8012–8040) | 308 ms, 407 ms, 1.8 s |
| Juno SUSTAIN | Hold level (horizontal) | pot % |
| Juno LFO RATE / DEPTH, CHO RATE; Drone FLUTTER | Miami `og_lfo_rate` / `og_lfo_amp` | rates in hertz, one decimal under 10 Hz (stock `unit_value` `X0X_U_HZ` is integer and would print `0 Hz` for 0.2); depth and flutter as % |
| Juno CHORUS | `orig_fx_glyph` type 1 (chorus pair) | pot % |
| Drone REGISTER | Three stairs whose heights follow `registration[0..2]` (`drone_string.h` 64–66). A mix, not Chicago RANGE’s discrete switch. Same stair vocabulary. | pot % |
| Drone BRIGHTNESS | Lowpass / tone (CUTOFF family). Label **BRIGHT**. | pot % |

The SOUND row on Drone’s first bank (ENGINE `DRONE`, PRESET factory name `Cathedral` via `draw_preset_name` 7158, STATUS `READY` from `original_status_name` 1047–1052) is **unchanged**. It is preset-owned. Juno’s SOUND page sits on the second bank (`5-5`) and is not redesigned.

A / B / C chrome is the already-agreed language: `SOUND` + clock; A reveals `BLACK: SELECT` / `WHITE: SOUND` / `SEQ: STEPS`; B adds the quiet rail and the hollow play triangle while stopped; C adds the part-colour spine (y=20–239) and hairline, and puts `SEQ: STEPS` in accent. Part colour is the **slot**, not the engine: JUNO A and DRONE A are voice 0 = `PART_303A`, green `RGB(130,240,90)` (`ui.c` 89–93). Bank hints stay `1-4` (Juno, `pg.n=5`) and `1-3` (Drone, `pg.n=3`).

A 3-digit millisecond plus a letter unit is 30 px on the repo TTF (HOST ESTIMATE) against the 27 px drum `strip_value_fit` zone (7181–). The frames right-align so `407 ms` is not clipped. An implementation should measure packed felucca advances and use the existing fit helper.

## Resource budget

Firmware Main Builder measured flash headroom on this SHA at **248 B Elite / 256 B Pro** above the 8192 B floor, and **148 B** pool headroom. Preset storage is already blocked on flash. Figures below are **ESTIMATE** (Thumb-2, from similar existing branches, not a linked size) or **HOST ESTIMATE** (struct layout and TTF). Main Builder must measure at link. Remove no feature.

### RAM (HOST ESTIMATE) — 0 additional, if we do not add buffers

| Asset | Where | Size | New? |
|---|---|---|---|
| `cv_px` screen strip | `gfx.c` 22–25, `.pool` | 240×138×2 = 66 240 B | No. All draws use this canvas. |
| `original_controls` | `ui.c` 420, `X0X_POOL` | view 32 B + cache 92 B | No. Already allocated. Juno/Drone do not need new fields if we only extend draw. |
| `original_wave` | `ui.c` 421, `X0X_POOL` | 154 B | No. Already allocated. Prefer `draw_wave_glyph` (no project path) so this cache stays unused for Juno; enabling `original_wave_view` would use it and still add 0 RAM. |
| `UI_PICTOGRAM` | `ui_pictograms.h` 45, 43×9 `uint16` | 774 B **flash**, 0 RAM | No new row. One new pictogram would be +18 B flash, 0 RAM. Not proposed. |
| Role table (proposed) | new `const uint8_t[16+6]` | 22 B **flash**, 0 RAM | Flash only. |
| Time / rate locals | stack in `original_value_text` | a few floats, same class as existing `unit_value` | 0 static RAM. |

`X0X_POOL` is empty on the host (`plat.h` 14) and `.pool` on the device (16). Device `.pool` free must stay ≥ 8192. These proposals do not grow the pool if no new cache is declared.

### Flash, ranked

| Rank | Change | Flash ESTIMATE | RAM | Class | Notes |
|---|---|---|---|---|---|
| 1 | Replace the 1075 early-return with `unit_value` and the DSP laws above. Reuse `"ms"`, `"s"`, `"Hz"`, `"%"`. One-decimal hertz under 10 Hz (a small branch; stock integer `X0X_U_HZ` prints `0 Hz` at 0.2). | 80–140 B | 0 | Needs a flash decision if stacked with 2 | Highest readability per byte. |
| 2 | Kind-specific mapping in `orig_strip_slot` + `orig_strip_glyph` (or a 22 B role table) and `wave_shape` / `original_wave_slot` for Juno WAVE. Call existing helpers only. Do **not** only flip the slot test. Do **not** rewrite Chicago or Miami arms. | 16 B table + 80–200 B dispatch + 16–40 B wave | 0 | Tight against 248 B when added to 1 | Combined 1+2 ESTIMATE 192–380 B. The high end **exceeds** 248 B. Implement, then measure. If over, keep WAVE / SUB / CUTOFF / LEVEL / LFO / CHORUS / tails / units first. |
| 3 | `BRIGHTNESS` → `BRIGHT` in `ui_original_name` (drone table, `ui_original_controls.h` 46–47) | **−4 B** (`"BRIGHTNESS\0"` 11 → `"BRIGHT\0"` 7) | 0 | **Negative** | Eases the 51 px tight cell. Optional if felucca is measured and already fits. |
| 4 | Defer: extend `adsr_row` + a **Juno** time path in `adsr_values`. Do not call Chicago VCA laws. | 80–200 B | 0 stack | Needs a flash decision; Chicago overlap risk | Tails already tell attack/decay/release. Not in the first package. |
| 5 | New pictogram | +18 B each + draw code | 0 | Avoid | Reuse is enough. |
| 6 | Enable `original_wave_project` for Juno | more code in `ui_original_wave.h` | 0 (cache exists) | Avoid | `draw_wave_glyph` is enough. |

**Zero / negative:** reuse of every picture (0 glyph bytes); shortening BRIGHT (−4 B); no new framebuffer or cache. Removing the 1075 early-return without adding units would only save a few bytes and leave the same percent display.

**Do not remove** a control, a page, or the SOUND row to free flash.

## Writer ownership

Second Builder is **not** coding a chord display in this work. SEQ chord and note drawing (`draw_303` 5572–5918, `held_chord_*` 3348–3493, `bass_write_held_chord` 3348, piano-roll comments at 5557 and 5918) must not be edited here. That design lives under `/workspace/smk-ux/seq-chords/`.

Main Builder owns presets. Do not edit `R_BPRESET` / `R_BENGINE` / `R_BSTATUS` cases in `original_value_text` (1056–1068), `draw_preset_name` (7158), the SOUND page emit (1524), `engine_bass_preset_*`, or `original_sound.c` factory data. The SOUND row stays as shipped.

Proposed engine-page work is disjoint from both, if it stays in the functions below.

| Change | Functions and lines | Overlap flag |
|---|---|---|
| Units | `original_value_text` `ui.c` 1056–1110, **only** the JUNO/DRONE early-return at 1075. Optionally `engine_param_si` `engine.c` 512–521 (today’s `X0X_U_NONE` default) and `unit_value` 1121–1142 (1-decimal Hz). | **Adjacent** to Main Builder’s 1056–1068 preset/engine/status cases. Do not touch those arms. `engine.c` is shared; a SI hook for T_303 Juno/Drone only, no preset path. |
| Glyph dispatch | `orig_strip_slot` 7734–7740; `orig_strip_glyph` 7846–7960 (**add** JUNO/DRONE arms, do not rewrite Chicago/Miami); `wave_shape` 6913–6944; `original_wave_slot` 6937–6951; `draw_param_widget` 7204–7305 (already calls these; may need no edit if the helpers return a shape or a slot). `dg_tail` 8012–8040 is called, not copied. | Chicago/Miami/909 draw is **adjacent**. Add-only. A rewrite of the existing `if (chi && …)` chain is out of scope. |
| BRIGHT | `ui_original_name` in `ui_original_controls.h` 46–47. `matrix_name` 7001 already uses that table. | Do not change `original_sound_drone_params` display names used for storage (`"Brightness"` in `original_sound.c` 46) unless Main Builder agrees; the on-screen string is `ui_original_name`. |
| Deferred ADSR | `adsr_row` 7369–7374; `adsr_values` 7312–7360; `draw_adsr_inline` 7402; caller at 5757. | **Chicago overlap.** A Juno path must not change Chicago laws or the slot-7..10 test’s result for Chicago. |
| Not in this work | SOUND page 1524; `draw_preset_name` 7158; `R_BPRESET` early-out in `draw_param_widget` 7248–7254; `original_sound.c` factories 38–56. | Main Builder presets. |
| Not in this work | `draw_303` 5572; held-chord 3348–3493. | `/workspace/smk-ux/seq-chords/`. |

## Frames

Generator: `/workspace/smk-ux/render_engines_juno_drone.py` (imports the existing A/B/C primitives). RGB565-quantised, 240×240, nearest-neighbour 2×.

| Path | Contents |
|---|---|
| `frames/1x/shipped-juno.png`, `shipped-drone.png` and `frames/2x/` | SHIPPED reconstructions, JUNO POLY and Cathedral. |
| `frames/1x/A-juno.png`, `A-drone.png`, `B-…`, `C-…` and `frames/2x/` | PROPOSED, in that direction’s marks. |
| `A/engines-juno-drone.png` (and `-2x`), same under `B/`, `C/` | Per-direction SHIPPED vs PROPOSED sheet. Plain captions. |
| `engines-juno-drone.png` and `-2x` | Combined four-column sheet (SHIPPED, A, B, C). |

Captions on the images are plain language, no code and no line numbers.

## Recommendation

Take the **same glyph-and-unit package** into whichever of A, B, or C the owner already picks for the journey (B was recommended there). The engine-page work does not pick a new direction.

Do units first (rank 1), then a compact add-only mapping that reuses existing pictures (rank 2), then optional `BRIGHT` (rank 3, saves 4 B). Measure flash at link before taking the ADSR graph. Do not invent discrete WAVE names. Do not flip `orig_strip_slot` without a kind map. Do not edit SEQ chord drawing or the SOUND / preset row. Combined ESTIMATE for 1+2 can exceed the 248 B Elite headroom; if the link goes over, keep WAVE, SUB, filter, tails, LFO, chorus, LEVEL, and the millisecond / hertz text, and drop ENV knee, WIDTH pulse, or REGISTER heights. 0 additional RAM if no new cache is declared.

No implementation until the owner approves a plan. These frames were not seen on a device.
