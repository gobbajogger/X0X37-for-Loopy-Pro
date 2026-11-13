# Source mapping — DnBOX SMK-37 directions

Every on-screen element in every frame, mapped to a source `file:line` at
commit `94197fc839b00aff47b98ca02f3c6396e4a6eabf`, or marked **PROPOSAL
(planned only)** with the function that would have to change. Nothing in this
document is implemented. Nothing was verified on hardware.

Line numbers are from the read-only worktree used for `/workspace/smk-ux/source-map-94197fc8.md`.
Fonts in the frames are repo Barlow TTFs at the source roles (T 10, XS 12, S 15,
B 15, M 23, `tools/gen_font.py` and source map §3.1), not the packed felucca
bitmaps. Advances may differ by a few pixels. Colours are RGB565-quantised with
the same `RGB()` expansion as `firmware/src/gfx.c:65`. Strip layout is the real
20+69+69+82 (`ui.c` 4692–4703, `gfx.c` 17–21).

Illustrative fixture (not a claim about a factory project): 909 BD, hits on
steps 1/5/9/13 when a pattern is shown, LENGTH 16, RATE 1/16, tempo 125, PARA
values in the family of `drum909.c` 79–82 (Tune / Attack / Decay / P.Dpth on
K5–K8; exact numbers are examples). Part display order is 909, 808, BREAK,
CHICAGO, MIAMI (`PART_ORDER` `ui.c` 99; Chicago is the 303 engine, Miami is
not). Keyboard-alone footers (not the SMC fixture).

Frames live in `/workspace/smk-ux/directions/frames/1x/` (and `2x/`).

---

## SHIPPED 94197fc8

Reconstructions. Reused from `/workspace/smk-ux/mockups/1x/` where a CURRENT
frame already existed; `SHIPPED-07-live` and the pattern-cue on that frame are
new reconstructions of the same source.

### `SHIPPED-01-first-sound`

| Element | Source |
|---|---|
| Boot view is 909 SEQ, empty | `ui.c` `ui_init` 8604–8606; empty patterns `project.c` 110–112 |
| 909 part-colour chip + mode `SEQ` | Chip and mode path `ui.c` 5418–5443; `VIEW_N[V_SEQ]` `"SEQ"` 88; 909 orange `RGB(255,150,40)` 89–93 |
| Pattern badge `#` + `1` at x≈132 | `ui_position_badge` 5203–5218; `draw_header` 5470–5472 |
| Tempo `125`, dim play icon (stopped) | 5495–5498; icon primitive 5153–5162; dim when `seq.metro_view` is 0 |
| 12-row kit BD…AC, BD selected, no hits | `draw_drum` 5511–5549 |
| Knob tag `P1 SEQUENCE`; K5 LENGTH 16, K6 RATE 1/16 | SEQ page 1473–1477 |
| Footer `HOLD ANY BUTTON: WHAT IT DOES` | `footer_text` 6738–6740, `any_button` 249, until the first press |
| Black key 1 would select BD and, while stopped, play it (not drawn as audio) | `drum_key` 3193–3213; `keysound` default 0, `project.c` 87; `key_sounds` 3409 |

### `SHIPPED-02-pattern`

| Element | Source |
|---|---|
| Same 909+`SEQ` header, kit grid, LENGTH/RATE | As 01 |
| Hits on 1/5/9/13 (fixture) | `night_mark` / playhead path 5543–5545 |
| `UNSAVED` XS badge, 2 px amber dot | Badge 5450–5460; dot 5501–5502 |
| Footer `BLACK: TRACK / WHITE: STEP / PATCH+BLK: MUTE` | 6783–6786 (after `any_button`) |
| White key / pad step rule (behaviour, not a caption) | empty on immediately; lit off on a tap shorter than `STEP_TAP_MS` 300 ms; hold is for p-locks, 3250–3253 |

### `SHIPPED-03-sound`

| Element | Source |
|---|---|
| Compact PARA header: name `909` in accent green, no `SOUND`, no pattern, no tempo | Early return 5373–5408; `VIEW_N[V_PART]` is `""` 88; word `SOUND` at 5437 is skipped |
| Page-bank dots | 5373–5408 |
| Dim play icon; unsaved 2 px dot | 5495–5498, 5501–5502 (still drawn on the compact path) |
| 4-row night matrix: `1 BD` Tune / Attack / Decay / P.Dpth (K5–K8), `2 BD` Pitch, `3 KIT` | `draw_part_sound` 5740–5834; `add_track_pages` 1377–1422; `drum909.c` 80–82; `UI_FIRST_KNOB` 5 |
| Bottom hint `< >` and `1-3` | 5678–5832 |
| Strings `BLACK: SELECT / WHITE: SOUND / SEQ: STEPS` are **not drawn** | Written at 6776–6779; `draw_knobs` returns before `draw_footer` 8337–8343 |

### `SHIPPED-04-step`

| Element | Source |
|---|---|
| Same compact PARA header as 03 | 5373–5408 |
| Same matrix; Decay example 1.50 (illustrative p-lock) | 5740–5834 |
| Amber hint `STEP 5  < >: CHANCE  PARA` | 5803–5806; `UI_CAP_EDIT` is `"PARA"` 41 |
| Held-step can be carried SEQ→PARA; PARA→SEQ blocked with `RELEASE STEP FIRST` (not lettered on this frame) | 2755–2759 |
| SEQ-side tag `BD P-LOCK STEP n` is source (8284–8286) and is not a row on these sheets; it is unchanged in every direction | 8284–8288 |

### `SHIPPED-05-save-playing` (persistent queued state)

| Element | Source |
|---|---|
| 909+`SEQ`, playing, playhead on step 5 | Play icon bright 5495–5498; playhead 5543–5545 |
| Badge squeezed to `Q` | After the toast, `SAVE AT STOP` (294, 5450–5452) shrinks to `Q` when it would cross x=130 (5458–5460) |
| 2 px amber dot | 5501–5502 |
| Kit, LENGTH/RATE, SEQ footer | As 02; playing |

### `SHIPPED-05-save-toast` (1300 ms, the moment of the BT tap)

| Element | Source |
|---|---|
| Full-header toast `SAVES WHEN STOPPED` in the part colour, black text | `say(queued_save_message())` 300, 2293–2294; duration 1300 ms 407; paints the whole header 5345–5369 and hides pattern, tempo and the play icon |
| Deferred write, flash must not run during play | 2222–2294; continuation 8783–8789 |
| Printed BT is Save, not Bluetooth | `B_SAVE`, `UI_CAP_SAVE` `"BT"` 46; `button_tap` 4105; device Bluetooth is a different path, `BLUETOOTH SAVED` 8725–8744 |

### `SHIPPED-06-saved`

| Element | Source |
|---|---|
| Full-header `SAVED` in the part colour | `say("SAVED", 0)` only if `project_save` `rc==0` 2260–2265; 1300 ms 407; 5345–5367 |
| Pattern and tempo covered for the duration, then the normal header, no persistent panel | `SAVE_SAVED` draws nothing after the toast |
| Failed writes would say `SAVE FAILED` / `NEWER DATA: NOT SAVED` / `MEMORY FULL: NOT SAVED` (not this frame) | 2262 |

### `SHIPPED-07-live`

| Element | Source |
|---|---|
| 909+`SEQ`, playing, playhead, BD selected, hits 1/5/9/13 | As 05, with transport running |
| `UNSAVED` badge + 2 px dot | 5450–5460, 5501–5502 |
| Queued next pattern: NEXT glyph + `2`, cue ink | 5470–5475; blink is `(ui.frame & 16)` — this frame is the “on” phase |
| `CHN` is not shown (no chain is set) | `CHN` only when `chain_a != chain_b` 5476–5477; chain is live-only and is not saved |
| SEQ footer | 6783–6786 |

---

## Direction A — Plain speech (PROPOSED)

Gestures unchanged. Planned display work only, in `ui.c` `draw_header`,
`footer_text` / `any_button`, and the PARA path of `draw_knobs` /
`draw_part_sound`.

### Shared A header / footer (all A frames)

| Element | SHIPPED or PROPOSAL |
|---|---|
| 909 chip + mode word (`SEQ` or `SOUND`) on every part view | **SHIPPED path** 5418–5443, used also for PARA instead of the compact early return 5373–5408. Word `SOUND` is already at 5437. **PROPOSAL (planned only):** skip the compact PARA header so this path runs. |
| Pattern badge and tempo always visible | **SHIPPED** on SEQ 5470–5498. **PROPOSAL:** apply the pickup exception 5410 (“song/pattern and tempo stay”) on PARA and during save toasts, so they are not dropped or covered. |
| Play icon as shipped (filled, dim when stopped, bright when playing) | **SHIPPED** 5153–5162, 5495–5498. A does not change it. |
| 2 px unsaved / queued-save dot | **SHIPPED** 5501–5502 |
| Full-word `UNSAVED` only when it fits left of x=132; never a squeezed `*` or `Q` | **PROPOSAL (planned only):** same 5450–5460 function, but omit the compact `*` / `Q` fallback and keep the 2 px dot. |
| 12-row kit, LENGTH/RATE, night matrix, part colours | **SHIPPED**, kept |
| SEQ footer from boot | **PROPOSAL (planned only):** draw the already-written 6783–6786 strings when `any_button` is still 0, instead of only 6738–6740. Hold-help 1905–2037 is unchanged. |
| PARA footer `BLACK: SELECT / WHITE: SOUND / SEQ: STEPS`, plus `1-3` | **PROPOSAL (planned only):** draw the already-written 6776–6779 in the existing 13 px hint (5832), which means not returning from `draw_knobs` before `draw_footer` (8337–8343), or drawing those strings in the hint slot. |

### Per-frame A notes

| Frame | Elements that differ from SHIPPED | Mapping |
|---|---|---|
| `A-01-first-sound` | SEQ footer from boot; otherwise the SHIPPED 01 grid and header | Footer: PROPOSAL on 6738 / 6783, as above. Grid, chip, dim play icon: SHIPPED. |
| `A-02-pattern` | Same kit and `UNSAVED` badge as SHIPPED 02 (the word fits here) | SHIPPED 02. The “proposal” is what was not taken from Dot (no 2×8 grid). |
| `A-03-sound` | `909` + `SOUND` + pattern + tempo; PARA footer in the hint; no compact green-only name | PROPOSAL: compact header 5373 skipped; word 5437; pickup 5410; footer 6776–6779. Matrix: SHIPPED 5740–5834. |
| `A-04-step` | Same SOUND+clock header as A-03; amber `STEP 5  < >: CHANCE  PARA` as SHIPPED 04 | Header: same PROPOSAL as A-03. Hint: **SHIPPED** 5803–5806 (the tell). No banner. |
| `A-05-save-playing` | Left amber words `SAVE AT STOP`; 909+`SEQ` yielded for the words; pattern, tempo, play stay; no `Q` | **PROPOSAL (planned only):** `draw_header` 5345–5369 and 5450–5460, reuse pickup 5410. Strings **SHIPPED** 294. Write path untouched. |
| `A-05-save-toast` | Same treatment for `SAVES WHEN STOPPED` (1300 ms) | String **SHIPPED** 300, 2294, 407. Layout: same PROPOSAL as the persistent chip. |
| `A-06-saved` | Left amber words `SAVED`; clock stays; no “PROJECT SAVED” panel | String **SHIPPED** 2262, 407. Layout: PROPOSAL, pickup 5410. After 1300 ms, normal SEQ header (SHIPPED behaviour). |
| `A-07-live` | Same sentence as the other A screens; cue and playhead as SHIPPED 07 | Cue **SHIPPED** 5470–5475. `UNSAVED` **SHIPPED** badge (it fits). Play icon **SHIPPED**. |

---

## Direction B — Live signals (PROPOSED)

Everything in A (footer from boot, `SOUND`+clock, full save words, kit kept,
gestures unchanged) plus the mark language. Planned display work in the same
functions, plus `play_icon` (5153–5162) and small header primitives.

### B mark language

| Mark | Meaning | SHIPPED or PROPOSAL |
|---|---|---|
| Part-colour fill of a row | Selected (track, and the same idea on a matrix row) | **SHIPPED** `draw_drum` 5511–5549 (selected track highlight) |
| Filled play triangle, bright | Playing | **SHIPPED** 5495–5498 |
| Hollow play triangle, same footprint | Stopped | **PROPOSAL (planned only):** `play_icon` 5153–5162, empty vs full. No new icon, no `STOP` word. |
| White playhead bar | The step that is playing | **SHIPPED** 5543–5545 |
| Amber outline pill or frame, with a name | Waiting: `SAVE AT STOP`, `SAVES WHEN STOPPED`, `UNSAVED`, `STEP n`, next-pattern number | **PROPOSAL (planned only):** new drawing in `draw_header` and the PARA hint, using **SHIPPED** strings 294–300, 5450, 5803–5806, 5470–5475. Replaces the `Q` squeeze 5458–5460 and the filled `UNSAVED` badge when this style is on. |
| Filled amber / part-colour pill `SAVED` | Confirmed write, 1300 ms | **PROPOSAL** layout; string **SHIPPED** 2262. The waiting rail is **off** for this state. |
| 2 px amber bar under the header | Something is waiting | **PROPOSAL (planned only):** inside the existing 20 px strip, y=18–19. Not a new strip, not an LED. |
| 1 px quiet rail at y=19 | At rest (including after `SAVED`) | **PROPOSAL (planned only):** same slot as the waiting bar. |
| 3 px part-colour tick | Part identity when a long `SAVES WHEN STOPPED` cannot sit next to the 909 chip | **PROPOSAL (planned only).** The 909 chip **stays** next to `SAVE AT STOP` and `SAVED`, which fit (`ui.c` header x=132 rule, 5450). |

### Per-frame B notes

| Frame | What to look at | Mapping |
|---|---|---|
| `B-01-first-sound` | SEQ footer from boot (as A). Hollow triangle. Quiet rail. | Footer: same PROPOSAL as A. Icon: PROPOSAL on 5153–5162. Grid: SHIPPED. |
| `B-02-pattern` | Filled BD row (SHIPPED selected). `UNSAVED` as an outline pill if shown; 2 px dot. Hollow triangle. | Selected: SHIPPED. `UNSAVED` outline: PROPOSAL on 5450–5460. |
| `B-03-sound` | `SOUND`+clock and PARA footer, as A. No waiting mark at rest (reserved). Hollow triangle, quiet rail. | Same PROPOSAL as A-03, plus the rail and icon. |
| `B-04-step` | `SOUND` stays. `STEP 5` outline pill (fits at XS left of x=132). Outlined hint with the source line. Amber rail. Hollow triangle. | Pill: PROPOSAL in `draw_header`. Hint outline: PROPOSAL around the **SHIPPED** 5803–5806 string. `RELEASE STEP FIRST` and `P-LOCK` unchanged. |
| `B-05-save-playing` | 909 chip stays. `SAVE AT STOP` outline pill. Clock and play stay (filled triangle, playing). Amber rail. Never a `Q`. | PROPOSAL on 5450–5460 and 5410. String SHIPPED 294. Write path untouched (2222–2294). |
| `B-05-save-toast` | `SAVES WHEN STOPPED` outline pill, 3 px part tick, clock stays, 1300 ms | String SHIPPED 300, 2294, 407. Layout PROPOSAL. |
| `B-06-saved` | 909 chip stays. `SAVED` **filled** pill (waiting became written). Rail quiet. Hollow triangle (stopped). Clock stays. | String SHIPPED 2262. Layout PROPOSAL. Rail off is part of the mark language. |
| `B-07-live` | Filled triangle (playing). `UNSAVED` outline. Next pattern `2` in an amber frame (SHIPPED cue glyph and number, 5470–5475, plus PROPOSAL frame). Amber rail. Playhead SHIPPED. | Combined waiting marks, different names, one family. |

---

## Direction C — Two rooms (PROPOSED)

Everything in A (footer from boot, `SOUND`+clock, full save words, kit kept,
gestures unchanged, play icon as shipped) plus the room edge and tickets.
Planned display work in the same functions, plus a 3 px spine in the left of
the main and knob strips (y=20–239) and a 2 px hairline at y=18–19.

### C room language

| Element | Meaning | SHIPPED or PROPOSAL |
|---|---|---|
| 3 px spine, part colour, y=20–239 | You are in this part’s room (SEQ or SOUND) | **PROPOSAL (planned only).** Uses **SHIPPED** part colours 89–93. Starts below the header so it does not cut a chip or a ticket. Not a control. |
| 2 px hairline at y=18, part colour | The same room, in the header strip | **PROPOSAL (planned only),** inside the 20 px header. |
| Spine + hairline + bottom plate in amber | The room’s scope is this held step | **PROPOSAL (planned only).** Plate uses the **SHIPPED** string 5803–5806, black on amber, in the existing 13 px hint (227–239). 909 orange and amber are close; the plate and the three-sided frame carry the tell. |
| Filled ticket, dark text, same family as the 909 chip | A named object: `SAVE AT STOP`, `SAVES WHEN STOPPED`, `SAVED`, `UNSAVED`, a queued pattern number | **PROPOSAL (planned only)** in `draw_header`. `SAVED` uses the **part colour** (same face as the shipped toast 5345), not a new success green and not a persistent panel. Strings SHIPPED. |
| Footer key name dim, action word in ink; `SEQ: STEPS` in accent on PARA | What the keys do; the way back | **PROPOSAL (planned only)** colour on the **SHIPPED** strings 6783–6786, 6776–6779. Not a new caption. |
| Play icon | As shipped | **SHIPPED** 5153–5162, 5495–5498. C does not hollow it. |
| 12-row kit, matrix, LENGTH/RATE | As shipped | **SHIPPED**, kept |

### Per-frame C notes

| Frame | What to look at | Mapping |
|---|---|---|
| `C-01-first-sound` | 909 edge (spine + hairline). High-contrast SEQ footer from boot. Play icon dim, as shipped. | Spine/hairline PROPOSAL. Footer strings SHIPPED 6783, contrast PROPOSAL. |
| `C-02-pattern` | Same kit, inside the 909 room. `UNSAVED` as a ticket if shown. | Kit SHIPPED. Ticket PROPOSAL on 5450. |
| `C-03-sound` | SOUND+clock (as A). 909 edge. `SEQ: STEPS` in accent. | Header: same PROPOSAL as A-03. Accent on 6779: PROPOSAL. Matrix SHIPPED. |
| `C-04-step` | Edge turns amber. Source STEP line as a full-width amber plate. `SOUND` and the clock stay. | Plate: PROPOSAL around SHIPPED 5803–5806. `P-LOCK` and `RELEASE STEP FIRST` unchanged. |
| `C-05-save-playing` | 909 chip + `SAVE AT STOP` ticket. Edge of the room still there. Clock stays. Never a `Q`. | Ticket PROPOSAL; string SHIPPED 294; write path untouched. |
| `C-05-save-toast` | `SAVES WHEN STOPPED` ticket, 3 px part tick, clock stays | String SHIPPED 300, 2294. Layout PROPOSAL. |
| `C-06-saved` | Brief `SAVED` ticket in the part colour; spine and clock remain; no persistent panel | String SHIPPED 2262. Face colour matches the shipped toast 5345, as a compact ticket (PROPOSAL). |
| `C-07-live` | Playing in the 909 room. Next pattern is a small ticket (SHIPPED cue 5470–5475, PROPOSAL ticket). `UNSAVED` ticket. Play icon as shipped. | Named waiting, not a save. |

---

## What is deliberately not in any frame

| Item | Why | Source |
|---|---|---|
| Gesture remaps (PATCH, NOTE REPEAT, KNOB BANK, BT times, `SEQ` rename) | Decision record 1, 5–10 | Plan decision record; `ui.c` 4042–4051, 4085–4089, `main_fm1.c` 2098–2121, `ui.c` 1909–1910 |
| Tutorial layer, “TAP PAD: ADD HIT”, “HOLD ANY BUTTON: help” as a permanent replacement for the live footer | Out of scope; hold-help already exists | 1905–2037, 6738 |
| New LED / “lit shortcut group” layer | Out of scope (Deluge praise is not taken as a lamp redesign) | `leds()` 8457–8581, unchanged |
| New save semantics, flash during play, merged `BLUETOOTH SAVED`, “PROJECT SAVED” | Display only; write path and distinct Bluetooth path stay | 2222–2294, 8725–8744, 2262 |
| Digitakt-style reload, HOME keys playing sound, 2×8 one-track grid, title “DnBOX”, always-on “CHAIN 01”, “125 BPM / STOP” as words | Forbidden without the owner, or dropped from Dot as not source | See plan “Not on the table”; `draw_drum` 5511; `CHN` 5476–5477; 5495–5498 |
| `presets-considerations.md` work | Owned by another worker; not touched | — |

## Implementation pointers (planned only, after approval)

If the owner picks a direction, the first files to touch are `firmware/src/app/ui.c` (`draw_header` 5311–5503, `footer_text` 6728–6800, `draw_part_sound` 5740–5834, `draw_drum` 5511–5549, `play_icon` 5153–5162, `draw_knobs` 8337–8343) and only those display paths. Do not start with `save_project`, `ui_live_key_owner`, `live_boundary`, `button_tap` gesture cases, or project stream numbers. Elite and Pro share this `ui.c`. A host snapshot of journeys 01–07 at 240×240, and the expert muscle-memory checks already in `/workspace/smk-ux/plan-94197fc8.md`, belong in that later plan. CPU, flash and LCD blit cost are not measured here.


---

## Appendix: Juno and Drone engine pages

Commit `94197fc8`. `ui.c` SHA-256
`b21c83aace675322c56ee84731cef430078f50307ccb86e12ffae5ebdf7a9f9b`.
Full file:line diagnosis and budget: `engines-juno-drone.md`.
SEQ chord and note drawing is not mapped here; see `/workspace/smk-ux/seq-chords/`.

### SHIPPED frames (`frames/1x/shipped-juno.png`, `shipped-drone.png`)

| Element | Mapping |
|---|---|
| Compact night header, `JUNO A` / `DRONE A` in `SOUND_GREEN`, bank dots, pattern cue at 132, play at 175, no tempo | **SHIPPED** `draw_header` 5373–5408. Always A/B from `bidx()` at 5383–5387, not the shortened `bass_display_name` 117–124. |
| Four 49 px slabs, titles `OSCILLATOR` `FILTER` `ENVELOPE` `LFO/CHORUS` / `STRING` `ENVELOPE` `SOUND` | **SHIPPED** `draw_part_sound` 5740–5848; titles and rows from `ui_original_controls.h` 18–27. Juno `pg.n=5` (hint `1-4`); Drone `pg.n=3` (hint `1-3`, third row is SOUND). |
| Name, `NN %`, 42 px bar, no 26×19 glyph | **SHIPPED** `original_value_text` 1075; `orig_strip_slot` 7734–7740 returns 0; `wave_shape` 6913 and `original_wave_view` 2911–2915 skip JUNO/DRONE; `draw_param_widget` fallback 7280–7300. |
| Factory values JUNO POLY / Cathedral as percents | **SHIPPED** `original_sound.c` 39, 52; law `(value*100+63)/127`. |
| SOUND row: ENGINE `DRONE`, PRESET `Cathedral`, STATUS `READY` | **SHIPPED** 1524, `BENGINE_N` 426, `draw_preset_name` 7158, `original_status_name` 1047–1052 (`READY`, not the non-night `ACTIVE` string). Unchanged in PROPOSED. |
| Footer `< >` and bank hint, no `BLACK: SELECT` | **SHIPPED** 5816–5832. `PARA_FOOT` is written and currently undrawn (6776–6779). |

### PROPOSED frames (`frames/1x/A-juno.png` and pairs)

| Element | Mapping |
|---|---|
| Pictures: wave, SUB squares, lowpass knee, ENV knee, HPF rise, LEVEL meter, LFO waves, chorus pair, tails, pulse, REGISTER ranks | **PROPOSAL (planned only).** Reuse of `draw_wave_glyph` 6959–6998, `orig_strip_glyph` 7846–7960, `orig_fx_glyph` type 1, `dg_tail` 8012–8040. Mapping must be kind-specific; flipping `orig_strip_slot` alone would draw Miami pictures on Juno ids (wrong). 0 new glyph bitmaps. |
| Milliseconds, seconds, hertz (one decimal under 10 Hz) | **PROPOSAL (planned only)** in `original_value_text` 1075, using existing `unit_value` 1121–1142 and DSP laws in `juno6.c` 70–86, `original_runtime.c` 101, `drone_string.h` 61–72. Stock integer Hz would print `0 Hz` at 0.2. |
| WAVE stays a percent, with a saw-to-square picture | **PROPOSAL** matches the continuous mixer at `juno6.c` 41. `s->waveform=x*2.99` is unused; discrete SAW/PUL is not proposed. |
| `BRIGHT` in place of `BRIGHTNESS` | **PROPOSAL** string in `ui_original_name` (`ui_original_controls.h` 46–47). −4 B flash. 51 px TTF HOST ESTIMATE in a 59 px cell. |
| `SOUND` + clock; A/B/C marks | Same **PROPOSAL** as the journey frames: `header_proposed` language already in this file. Part colour is the slot (`PART_303A` green, 89–93). |
| SOUND / PRESET row | **SHIPPED**, not proposed. Main Builder. |
| SEQ chord / piano roll | Out of scope. `/workspace/smk-ux/seq-chords/`. |

Implementation, if approved, starts in the functions listed in
`engines-juno-drone.md` (ownership map). Flash is ESTIMATE only; 248 B
Elite / 256 B Pro above the 8192 floor, 148 B pool, as measured by
Firmware Main Builder on this SHA. 0 additional RAM if no new cache is
declared. Combined units + glyph dispatch may exceed 248 B; measure at
link.
