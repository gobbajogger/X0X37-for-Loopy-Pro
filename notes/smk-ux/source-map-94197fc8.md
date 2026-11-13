# DnBOX SMK-37 source interaction map

**Commit:** `94197fc839b00aff47b98ca02f3c6396e4a6eabf` (`origin/lane/026-combined-20261112`, PR #869), “Merge reviewed SMK host fixture identity corrections”, parents `d4088e606` and `d5f2e3e92`.
**Published build compared below:** 0.0.25.1, tag `v0.0.25.1`, base `79280e763341d03ba43266d3e25dad66f12de860`.
**Spell:** DnBOX exactly.
**Evidence grades:** **SOURCE** = read in this tree at this SHA; **DOC** = repository manual or behaviour text; **INFERRED** = reasoning, not claimed as device behaviour. This map does not claim hardware behaviour.

**Method:** Existing clone `/workspace/X0X37-for-Loopy-Pro` was left untouched. A detached read-only worktree was added at `/workspace/smk-ux-src`. All line numbers are from that worktree at the SHA above. No builds, flashes, tests, commits, or device access.

**ui.c identity:** SHA-256 of `firmware/src/app/ui.c` is `b21c83aace675322c56ee84731cef430078f50307ccb86e12ffae5ebdf7a9f9b` at both `94197fc8` and parent `d4088e606` (git blob `eb911f58db83e417c67dd71576331803fa6c829e`). At 0.0.25.1 it is a different file (`61c2e821…`, blob `9c435dc3…`). Tracked blob count at `94197fc8` is 5345. **SOURCE.**

---

## 0. Elite vs Pro

**SOURCE.** Elite and Pro are the same SMK-37 firmware under `BOARD_SMK37=1`. Interaction code is not split by model. The build system (`tools/build.py` 169–180, 724–729) uses “one hardware, three names”: `pro` (default, no extra `SMK_MODEL` flag), `elite` (`-DSMK_MODEL="SMK-37 Elite"`), and `mke`. USB display strings only (`firmware/src/usb_product_name.h` 15–18): `DnBOX <version> SMK-37 Elite` vs `… SMK-37 Pro`. Package identity prefixes are `SMK-37 Elite_9` / `SMK-37 Pro_9` (`tools/build.py` 167, 726–727). Panel comments still say “SMK-37 Pro” (`firmware/src/app/panel.c` 11, 29; `firmware/hal/smk_input.h` 4) even when the Elite package is built. There is no Elite-only or Pro-only control, LED, or screen path in `ui.c`.

The FM-1 is a different board (`BOARD_SMK37` off). This map is the SMK path. Printed SMK legends are wired through `#if X0X_SMK_CAPTIONS` / `BOARD_SMK37` in `ui.c` 36–71. Logical roles stay Felucca’s `B_*` / `EN_*` (`firmware/src/app/plat.h` 36–40).

---

## 1. Physical controls the firmware handles

### 1.1 What the hardware scan exposes

**SOURCE** `firmware/hal/smk_input.h` 4–26, 50–105, and the scan in the same file:

| Hardware | Count | IDs / notes | Lines |
|---|---|---|---|
| Keybed | 37 keys | `FM1_NKEY`; two-contact velocity. Middle 27 (F3–G5, keys 5–31) are the FM-1 note bits (`SMK_KEY_F3` 5). Outer keys C3–E3 and G#5–C6 are queued separately. | 52, 56, 161–166 |
| Buttons | 18 | Stock matrix ids, printed names (unit-confirmed 2026-10-09): 0 KNOB BANK, 1 FADER BANK, 2 `<`, 3 `>`, 4 OCTAVE-, 5 OCTAVE+, 6 ARP, 7 NOTE REPEAT, 8 SCALE, 9 CHORD, 10 GLOBE, 11 BT, 12 PATCH, 13 PARA, 14 FX, 15 SEQ, 16 SEQ PLAY, 17 SEQ REC | 51, 78–82 |
| Button LEDs | 19 | LED i = button i; SEQ PLAY is two-colour (16 green, 18 red unused); SEQ REC has its own red at `0x1b` | 55, 86–89 |
| Encoders | 8 (K1–K8) | K1 is MASTER (virtual pot, never a taught role) | 53; `panel.c` 32–34, 54, 113 |
| Pads | 16 | Analog, velocity-curved; mapped to the 16 white keys; RGB WS2812 on SPI1 (`firmware/src/lcd.c` 183–189) | 54, 96–105 |
| Faders | 4 | mux C addresses; FADER BANK selects bank A/B | 93–94 |
| Wheels | 2 | pitch and mod | 93; `main_fm1.c` 982–987 |
| Pedal, battery | 1 + 1 | mux C 6, 7 | 93 |
| Display | 240×240 ST7789, RGB565 | `lcd.c` 4, 92–97, 122–131 | |

Debounce: press ~3 ms (`SMK_DEB_PRESS` 2 frames), release ~6 ms (`smk_input.h` 57–58). Encoder: 4 counts per detent (60–61). No timed double-press detector exists. “AGAIN” in help text means a second tap of the same view button.

### 1.2 Printed SMK legend → logical role

**SOURCE** `firmware/src/app/panel.c` 28–39 (Dot’s citation is exact) and `ui.c` 39–54, 1916–1917:

| Printed (SMK) | Logical `B_*` / `EN_*` | Caption macro |
|---|---|---|
| FX | `B_FX` | FX |
| SCALE | `B_SEL` | `UI_CAP_SEL` “SCALE” |
| CHORD | `B_ENV` | “CHORD” |
| NOTE REPEAT | `B_LFO` | “NOTE REPEAT” |
| PARA | `B_EDIT` | “PARA” |
| GLOBE | `B_GLO` | “GLOBE” |
| PATCH | `B_HOME` | “PATCH” |
| BT | `B_SAVE` | “BT” |
| ARP | `B_ARP` | ARP |
| SEQ | `B_SEQ` | SEQ |
| SEQ PLAY | `B_PLAY` | “SEQ PLAY” |
| SEQ REC | `B_REC` | “SEQ REC” |
| OCTAVE- / + | `B_OCTDN` / `B_OCTUP` | “OCTAVE” |
| `<` `>` | not in `NB`; `SMK_BTN_LEFT`/`RIGHT` → SELECT | `UI_CAP_ARROWS` “< >” |
| KNOB BANK | `SMK_BTN_KNOB_BANK` (outside FM-1 map) | toggles clip mode, §2 |
| FADER BANK | `SMK_BTN_FADER_BANK` | fader bank A/B |
| K1 | MASTER pot | — |
| K2 | `EN_SELECT` | “K2” |
| K3 | `EN_ALGO` (part) | “K3” |
| K4 | `EN_PRESET` (pattern) | “K4” |
| K5–K8 | `EN_K1`–`EN_K4` | `UI_FIRST_KNOB` 5 |

Default matrix map: `panel.c` 37 `{14, 8, 9, 7, 13, 10, 12, 11, 6, 15, 16, 17, 4, 5}` for the 14 `B_*` roles; encoders `{1,2,3,4,5,6,7}` (K2–K8). OCTAVE- + OCTAVE+ at power-on starts HARDWARE CALIBRATION, which re-teaches every button and K2–K8, never K1 (`panel.c` 33–34, 81–147).

### 1.3 Holds, combos, and press vs release

**SOURCE** `ui.c` `input_body` 4516–4523, `button_tap` 3985–4169, and hold timers:

- **SEQ PLAY and SEQ REC act on press** (timing). Every other `B_*` button acts on **release**, and only if `ui.btn_used` is clear (4520–4523). A hold used as a modifier sets `btn_used` so the release does nothing.
- **Hold-help:** one button alone, unused, no keys, no question, for `HELP_HOLD` 900 ms (BT/SAVE: `HELP_HOLD_SAVE` 400 ms). The card appears and the release is consumed (`ui.c` 1905–1938, 2020–2037).
- **BT hold 250 ms** (`COPY_HOLD`): `say("CHOOSE PASTE TARGET")` without claiming a snapshot yet (2002–2017). Then BT + white key pastes (3701–3714). BT + OCTAVE sets copy bank (4109–4114). BT + SEQ REC asks to clear the part (4027–4032). BT + knob forgets that knob’s motion (4665–4670). BT + knob on a p-lock removes the lock (`ui.c` 2951–2953).
- **PATCH held:** + SEQ REC = undo; + SEQ PLAY = redo (4007–4024); + K2 = tempo (4552–4560); + white key = pattern from any screen (3733–3752); + black key = mute track or mixer channel (3754–3795). In clip mode, PATCH + SEQ REC / SEQ PLAY are Loopy undo/redo (`main_fm1.c` 773–775, `ui.c` 4497–4500: “PATCH release does not change the X0X view”).
- **CHORD held + key:** accent (help 1934–1937; write path 3203, 3325–3331, 4330). **NOTE REPEAT held + key:** slide on a 303 (1942, 3329–3331).
- **NOTE REPEAT + OCTAVE** on SEQ keyboard: tie; **CHORD + OCTAVE**: rest (4142–4147).
- **FX + SEQ** together: keyboard panic chord, all notes off (`keyboard_panic` 4175–4193). Consumed so those buttons do not also change view (4484–4494).
- **`<` `>`:** one SELECT step per press when no menu; in a list, they change the row value (hold-repeat 400 ms then 80 ms). They do nothing while PATCH is held. **DOC** `BEHAVIOR.md` GEN-29, GEN-56; **SOURCE** `smk_arrows.h`, `main_fm1.c` `smk_arrows`.
- **No double-press** primitive. View buttons retapped while already on that view advance the page (`button_tap` 4064–4103).

### 1.4 Pads, faders, wheels

**SOURCE.** Pads alias the 16 white keys (`smk_input.h` 96–101). Layout flips to a 2×8 step grid (`smk_pad_grid`) on HOME, SONG, drum-mix pages, while PATCH or BT is held, or whenever it is not “303 + REC keyboard” (`main_fm1.c` 1994–1998). Pad RGB follows the on-screen part colour at two brightnesses (`main_fm1.c` 868–880).

Faders: bank A = levels of the first four **displayed** parts (`PART_ORDER`); bank B = fifth part level, reverb return, delay return, master FILTER (`main_fm1.c` 883–935). Soft pickup: bit 7 of `last[]` means waiting; takeover when the raw value is within 2 of the target or crosses it (953–965). Until then the header shows `PICK Fn UP` / `PICK Fn DOWN` and leaves pattern and tempo visible (960–963; `draw_header` 5319–5320, 5410–5411). Bank change rearms pickup and says `FADERS: …` (902–913). FADER BANK LED is on for bank B (**DOC** GEN-30).

Wheels: on a 303 they bend / open cutoff (or Miami wobble depth) **without writing saved values**; REC can record bend motion (`main_fm1.c` 982–987). A gesture stays with the 303 it started on (**DOC** GEN-36A).

---

## 2. Modes, pages, states, and transitions

### 2.1 Views and overlays

**SOURCE** `ui.c` 87–88, 169–170:

Views: `V_HOME`, `V_PART`, `V_GEN` (TB-3PO), `V_FX`, `V_MIX`, `V_GLO`, `V_SONG`, `V_PERF`, `V_SEQ`. Header names: `VIEW_N` = HOME, (empty on V_PART), TB-3PO, FX, MIX, GLOBAL, SONG, PERF, SEQ. Overlays: `O_NONE`, `O_LIST`, `O_ASK`.

**Boot default (SOURCE `ui_init` 8604–8606):** `ui.view = V_SEQ`, `ui.prev_view = V_PART`, `ui.part = PART_909`. Comment: “start on the visible step grid; EDIT opens sound.” This is already 909 step editing, not HOME.

Part presentation order is **not** engine ID order (**SOURCE** 94–99): `PART_ORDER = {909, 808, BREAK, 303B, 303A}`. Default engines: Chicago on 303B, Miami on 303A (comment 96–97). “Bass slots are not named 303: Chicago is the 303 engine, Miami is not” (94). Display names follow the actual engine (`part_name` 138–149): CHICAGO / MIAMI / JUNO / DRONE, with A/B when both slots share a kind. K3 (`EN_ALGO`) walks `PART_ORDER` via `part_neighbor` (108–111, 4573–4606).

Page groups for settings lists (**SOURCE** 1291–1292, 1677–1691): `GR_TRACK` “THIS TRACK”, `GR_TMIX` “TRACK MIX”, `GR_KIT` “ALL TRACKS”, `GR_PAT` “THE PATTERN”, `GR_SOUND`, `GR_MORE`, and GLO sections `GR_MIDI`, `GR_AUDIO`, `GR_KEYS` “KEYS AND SCREEN”, `GR_PROJECT`, `GR_SYSTEM`, `GR_BT` “BLUETOOTH MIDI”. GLO section titles (419): “MIDI + CLOCK”, “BLUETOOTH”, “AUDIO”, “KEYS + SCREEN”, “PROJECT”, “SYSTEM”.

### 2.2 What each input does to the view

**SOURCE** `button_tap` 4042–4105 and `set_view` 2752–2778. All of these fire on a **single release** (transport on press):

| Input | Effect | Accidental? |
|---|---|---|
| PATCH | If GLO subsection: `global_back`. If overlay or GLO: `close_overlay` (GLO returns to `prev_view`, else just closes list). Else **toggle: HOME → SONG, anything else → HOME** (4050–4051). | **Yes.** One TAP from SEQ/PARA/MIX jumps to HOME. One TAP already on HOME opens SONG. There is no “always HOME” that is safe from HOME itself. |
| PARA | Enter `V_PART`, or if already there, next sound page (4059–4067). | Single press leaves SEQ. |
| SEQ | Enter `V_SEQ`, or next sequence page (4099–4103). Blocked from PARA if a step is still held: `say("RELEASE STEP FIRST")` (2757–2759). | |
| NOTE REPEAT | Enter `V_MIX` (the mixer), or next MIX page (4085–4089). Help: “THE MIXER AND THE MASTER” (1939–1940). | **Yes.** Printed “NOTE REPEAT” is the mixer. In clip mode the same button is panic (§5). |
| FX / ARP / GLOBE | Enter that view; GLOBE opens the global list; GLOBE again closes (4079–4097). ARP on a non-303 says `TB-3PO: PICK CHICAGO OR MIAMI` (4070–4072). | |
| SCALE | Open this screen as a list, or run the list row / confirm a question (4053–4057, 3994–3997). | |
| KNOB BANK **press** | XOR `clip_grid_mode`; `say("LOOPY CLIPS")` or `"LP OFF"` (`main_fm1.c` 2098–2121). Header chip becomes “LP” (5413–5415). | **Yes. A single press leaves the whole X0X instrument UI.** |
| SEQ PLAY | Start/stop, or PATCH+PLAY redo. Playing keeps `seq.song_on` as it was when started (4539–4540). | |
| SEQ REC | Toggle record; `RECORD ON: KEYS + KNOBS` / `RECORD OFF` (4034–4040). | |
| BT | `save_project()` (4105). | Printed Bluetooth is Save. |
| K2 | Page / list row / held-step chance / PATCH+K2 tempo (4552–4570). MIX/FX/PART pages wrap; others clamp. | |
| K3 | Part, or held 303 step velocity, or list value (4573–4606). Leaving a 303 on TB-3PO jumps to PARA (4594–4595). | |
| K4 | Cue pattern (part-local on a part screen, all five on HOME); ends the chain (4608–4623). | Single turn ends a live chain. |

`set_view` closes overlays, rebuilds pages, cancels SMC UI, and (unless SEQ→PARA with a held step) cancels held drum/bass steps (2752–2778).

**Clip-mode context (SOURCE):** SCALE, CHORD, ARP, NOTE REPEAT belong to the clip keybed (`main_fm1.c` 1641, `plat_fm1.c` 39). NOTE REPEAT + ARP toggles LATCH (1655). NOTE REPEAT alone on release is panic: all notes off + CC 123 (1669). PATCH+SEQ REC / SEQ PLAY are Loopy history, not X0X undo (`ui.c` 4497–4500).

### 2.3 How to get back to a known state

**SOURCE, no single “reset UI” control.**

- From most views: PATCH → HOME (4051). From HOME: PATCH → SONG (not back to SEQ). From SONG: PATCH → HOME.
- From a list or question: PATCH closes / answers NO (3998–3999, 4043–4045, 3926–3933). GLOBE close returns `prev_view` (3928–3930).
- Boot-like state (909 SEQ, no overlay) is **not** a dedicated button; it is `ui_init` only (8604).
- Confusing held function: release the held step / button; help release is a no-op (1908, 2036).
- `RESET PROJECT?` in GLOBE (asks; samples kept) (`list_enter` 3884–3885, `ui.c` 8721 `PROJECT RESET: SAMPLES KEPT`).
- FX+SEQ panic stops notes, does not change view.
- Power-cycle reloads the saved project (autosave may already have written). **INFERRED** from save rules, not observed on a unit.

**Accidental single-press transitions:** PATCH HOME↔SONG; PATCH from SEQ to HOME; NOTE REPEAT → MIX; KNOB BANK → clip mode; K4 ending a chain; GLOBE opening settings over live play.

---

## 3. Feedback: display and LEDs

### 3.1 240×240 layout primitives (for native mockups)

**SOURCE.**

- Panel: 240×240, RGB565, MADCTL origin top-left, RGB order (`lcd.c` 4, 122–131; `lcd_tiles.h` 7–9).
- Three strips that tile the panel (`ui.c` 4692–4703, `gfx.c` 17–21): header `HDR_H` 20; main two bands of `BAND_H` 69 (rows 20–157); knob strip `KNOB_Y` 158, `KNOB_H` 82. `20+69+69+82 = 240`.
- Canvas one strip at a time, `CV_ROWS` 138 (`gfx.c` 22). Dirty tiles 16×16, grouped up to 48 px wide (`lcd_tiles.h` 7–12; `ui.c` 4707–4709). SMK SPI 12 MHz (`lcd.c` 12).
- Default fontset **Barlow Semi Condensed** (`tools/gen_font.py` 25–38, `FONTSET` default `"barlow"` 59): role sizes **T 10, XS 12, S 15, B 15, M 23, L 31**. L (“the 31 px face for numbers”) was dropped from the readout to free flash (`ui.c` 6487; **DOC** `BEHAVIOR.md` GEN-54). XS/S/B have a reduced lower-case set (**DOC** GEN-55).
- Theme materials, night and light, 16 slots (`firmware/src/ui_theme.h` 5–25): `UI_N_BASE/FACE/EDGE/LEFT/SIDE/SHADOW/SELECT/SELECT_EDGE/SELECT_SIDE/INK/WHITE/GRAY/DIM/LINE/ACCENT/AMBER`. Palettes GREEN/AMBER/CYAN/RED/MONO/LIGHT (`gfx.c` 42–49). `RGB()` macro `gfx.c` 30. Helpers: `rbox`/`rframe`/`box`/`frame`/`disc`/`dot`/`arc`, `text_c`/`text_r`, `night_panel`, `surface_panel`, `play_icon`, `ui_pictogram`, `ui_position_badge`, `ui_state_toggle` (`ui.c` 5021–5218).
- Part colours, dark theme (`ui.c` 89–93): 909 orange `RGB(255,150,40)`, 808 red `(255,72,64)`, 303A (Miami default) green `(130,240,90)`, 303B (Chicago default) cyan `(70,205,255)`, BREAK violet `(214,120,255)`.

### 3.2 Header: selected, playing, queued, save

**SOURCE** `draw_header` 5311–5503, `ui_position_badge` 5203–5218, `save_state` 279–300.

- Left: view name (`HOME`, `SONG`, `FX`, `MIX`, `GLOBAL`, `PERF`) or a part-coloured chip + mode (`SEQ`, `SEQ KEYS`, `FREE PLAY`, `SOUND`, `TB-3PO`) (5418–5448). Clip mode: “LP” (5414–5415). Pickup: the `PICK …` message in the left chip, pattern and tempo stay (5410–5411).
- Mid: pattern/song **position badge** (glyph + number) at x≈132. If a cue is pending and not song mode: a NEXT glyph and the queued pattern number, cue ink blinking (`ui.frame & 16`) (5470–5475). If a **chain** is on and nothing is queued: small amber **“CHN”** (5476–5477) — not “CHAIN 01”. If a part is longer than 16 steps: the visible range e.g. `17-32` (5478–5480).
- Right: tempo (amber if external clock), then `play_icon` (217, 4): **dim when `seq.metro_view` is 0 (STOP/WAIT), white on the downbeat, accent otherwise** (5495–5498). Rec: red dot (5499–5500). Unsaved: amber (or red if failed) **dot at (237,3)** always when not saved (5501–5502), plus a text badge if space: `UNSAVED`, `SAVE AT STOP` / `SAVE WHEN QUIET` / `REC OFF TO SAVE`, `SAVE FAILED`, or compact `*` / `Q` / `!` (5450–5460).
- Toasts (`say`): fill the whole header for **1300 ms** (`ui.c` 402–407). Examples: `SAVED`, `SAVES WHEN STOPPED`, `AUTOSAVED`, `CHAIN P`, `LOOPY CLIPS`, `BLUETOOTH SAVED`. Pickup toasts are special-cased so they do not cover the right side.

On PARA for a drum or 303, a compact sound header is used instead (5373–5408): engine name, optional `PADS` division label, page-bank dots, pattern cue, play icon, rec and save dots — **no “HOME” / “SEQ” word, no “UNSAVED” text**.

### 3.3 Main views (current, not proposed)

- **HOME** (`draw_home` 5996–6046): 16 pattern slots for the current bank, per-part strength glyphs, current-pattern number, amber underline on chained slots, five lanes in `PART_ORDER` with playhead. Footer if SMC is connected: `SMC F1-5: PART LEVEL` / `F6: REV  F7: DLY  F8: FILT` (6750–6754) — this is what Dot’s current framebuffer shows. Without SMC: `WHITE: PATTERN` / `TWO: CHAIN` / `BLACK: MUTE` (6757–6759). Until any button: `HOLD ANY BUTTON: WHAT IT DOES` (6738–6740).
- **SEQ drums** (`draw_drum` 5511–5549): **all 11 tracks + AC**, 16 columns, selected track highlighted, playhead, p-lock notches. Not a single-track 2×8 pad diagram.
- **SEQ 303** (`draw_303` 5572–5666): piano roll, AC/SL rows, held-step line `STEP n  <note> … V<vel>` or scale/seed.
- **PARA / sound:** matrix of sound pages (`draw_part_sound`); a turned knob replaces the bottom band for 900 ms with `draw_readout` (6454–6506, 6816–6817) — name left, FONT_M value right, part-coloured frame. Held step + knobs: p-lock (`knob_tag` “BD P-LOCK STEP n”, 8284–8286).
- **Help card** (`draw_help` 6670–6683): printed legend as title, up to 6 XS lines.
- **Ask / list:** yes/no and grouped rows (`draw_ask`, `draw_list`).

### 3.4 Button and pad LEDs

**SOURCE** `leds()` 8457–8581. Button LEDs:

- SEQ PLAY lit on the 909 downbeat while playing, or slow blink if waiting for a clock (8465–8466). In clip mode, Loopy owns PLAY/REC lamps (8469–8476).
- SEQ REC lit while `ui.rec`; PLAY’s red LED also (`plat_play_red`, 8580).
- View lamps: PATCH lit on HOME or SONG (and blinks if song is running from another view, 8478–8479); SEQ, PARA, ARP, FX, NOTE REPEAT (means MIX!), GLOBE, SCALE (overlay) similarly (8480–8493).
- OCTAVE+ lit when pattern bank 17–32 or step page > 1 (8494–8495).
- BT/SAVE: solid when `SAVE_QUEUED`; blink when unsaved; fast blink when failed (8496–8497).

Key / pad lights if `proj.set.keyled` (default on, `project.c` 86): bright = on or chosen; dim = present. HOME: used patterns dim, playing pattern bright, cued pattern blinks, playing parts bright, muted parts dim (8498–8518). Drums: selected track’s **hits** on white pads (playhead inverts), selected track bright on black (blink if muted) (8531–8540). 303 step mode: notes bright, ties dim (8541–8552). 303 keyboard: Cs dim as landmarks (8553–8556). The SMK **keybed has no LEDs** (**DOC** GEN-41); black-key meaning is on the screen and, for pads, only the white-key pad RGBs light.

**Ambiguous / shared feedback (INFERRED from SOURCE mappings):**

- NOTE REPEAT’s lamp means “you are on MIX”, not note-repeat.
- PATCH’s lamp means HOME **or** SONG (or song-playing blink).
- BT’s lamp means unsaved / queued / failed save, not Bluetooth.
- The play icon is visible while stopped (dim). The current HOME framebuffer shows a triangle next to 125 that can be read as “playing”.
- “CHN” is easy to miss; chain is not saved.
- Save badge “SAVE AT STOP” vs compact “Q” depending on header width; PARA compact header has only a 2 px dot.
- Pickup `PICK F1 UP` vs SMC mixer’s own “PICKUP: MOVE UP TO TARGET” (`smc_mixer.c` 1433) — two pickup dialects.

---

## 4. Flows mapped to the six tasks

### 4.1 First sound

**SOURCE.** Boot is **909 SEQ**, empty patterns (`project.c` 110–112; `ui_init` 8604–8606). **DOC** MANUAL §2 agrees: “When X0X starts, the screen shows the 909 and every part is empty.”

On 909 SEQ, **black key 1** selects BD and, while **stopped**, plays it (`drum_key` 3193–3213: default `keysound` is 0, so “while the pattern plays a black key only selects”). **White keys** toggle steps (`drum_step` 3250–3253), they do not “play a note” as a keyboard. On HOME, white keys are patterns and black keys are part mutes (3784–3787) — **no sound**. 303 keys play only when `kbd_on()`: part is a 303 **and** (`V_PART` or REC or FREE PLAY) (345). Live 37-key path requires `ui_live_key_owner()` (4279–4297): SEQ/PARA, 303, FREE PLAY or PARA, not REC, not PATCH/BT held, not clip, not SMC UI.

**INFERRED problem:** a novice on HOME (after one accidental PATCH) can press keys and hear nothing.

### 4.2 Programming a drum pattern

**SOURCE.** Already on 909 SEQ at boot. Black key = track (plays if stopped). White key / pad = step: empty step turns **on immediately**; a lit step turns **off on a tap** shorter than `STEP_TAP_MS` 300 ms; a hold keeps it for p-locks (3250–3253). CHORD+white = accent; CHORD+black = loud hit (help 1936). SEQ PLAY starts/stops a loop. Length/rate are K5–K6 on SEQ’s SEQUENCE page (1473–1477). Mute: PATCH+black. MANUAL §2 steps 1–4 match this **if** the user is still on SEQ.

### 4.3 Programming a bassline (Chicago is the 303; Miami is not)

**SOURCE** `ui.c` 94, 1509–1524, `bass_kbd` / `bass_write`. K3 walks to 303B (Chicago, third click from 909, **not** two) or 303A (Miami, fourth). SEQ: white keys are steps; knobs on a held step edit note / gate / accent / slide (4649–4662). REC + keys write notes. FREE PLAY (`R_FREEPLAY` on SEQ KEYS page, 1484–1485) plays without writing. TB-3PO (ARP) generates Chicago **or** Miami lines (1972–1973); OCTAVE+ new line, OCTAVE- mutate (4116–4138). Accent and slide flags are Chicago-style; **DOC** `BEHAVIOR.md` BAS-13/14: Miami ignores accent and slide. JUNO and DRONE are additional engines at this SHA (`BENGINE_N` 426).

### 4.4 Editing a step / note vs editing the sound

**SOURCE. This must stay.**

Journey **HOME → 909 step edit:** if the user is on HOME, PATCH from HOME goes to **SONG**, not SEQ. Correct path: **SEQ** (or, at boot, already there). K3 if the part is not 909.

| | Step edit (SEQ) | Sound edit (PARA) |
|---|---|---|
| Enter | SEQ button; lamp on SEQ | PARA button; lamp on PARA |
| Display | Full kit grid (`draw_drum`) or 303 roll; header mode `SEQ` | Sound matrix / engine name header; mode `SOUND` on 303 |
| White keys, drums | Toggle that track’s steps | **Audition** the selected sound only, “never write a hidden step” (3801–3803). Footer: `WHITE: SOUND` (6776–6779) |
| Hold step + K5–K8 | P-locks (SEQ may hand the gesture to PARA, 2755–2756, 4629–4643) | Same p-lock if the step is still held; tag `P-LOCK STEP n` |
| Leave PARA→SEQ with step held | Blocked: `RELEASE STEP FIRST` (2757–2758) | |
| Knob turn, no hold | Pattern length/rate on SEQ | Sound pots. First 909 BD page after rank-and-filter: **Tune, Attack, Decay, P.Dpth** on K5–K8 (`add_track_pages` 1377–1422; Level/Pan/Rev/Dly go to MIX; Drive/Dist to MORE). Pitch is the next page. Defaults: Tune 34, Attack 13, Decay 90, P.Dpth 0 (`drum909.c` 79–82) |
| 900 ms readout | “P*n*  LENGTH” etc. | “BD  TUNE” (`draw_readout` 6468–6470) |

There is **no** current banner “EDITING: SOUND” or “EDITING: THIS STEP ONLY”. Held-303-step feedback is the `STEP n` line on the roll (5645–5659) and the knob tag (8287–8288).

### 4.5 Save / recall (chain is not saved, on purpose)

**SOURCE.** BT tap → `save_project` (2269–2294, 4105). If flash would run during play, rec, notes, or clip activity: `save_wait = 1`, toast **`SAVES WHEN STOPPED`** (or `SAVES WHEN QUIET` / `REC OFF TO SAVE`) (296–300, 2293–2294). Header badge **`SAVE AT STOP`** (or the quiet/rec variants) (290–294, 5452). Actual write only when stopped and quiet: `ui_project_save` → `project_save()`; toast **`SAVED`**, **`SAVE FAILED`**, **`NEWER DATA: NOT SAVED`**, or **`MEMORY FULL: NOT SAVED`** (2260–2265). Duration 1300 ms. BT LED stays until complete (8496–8497). Autosave (default on): after 4 s quiet (`AUTOSAVE_QUIET`), at most every 20 s (`AUTOSAVE_GAP`), stopped only; toast **`AUTOSAVED`** (8771–8804). Continuation of a queued explicit save is `autosave` 8783–8789 (Dot’s “8783 continuation conditions”).

Recall: GLOBE → PROJECT → LOAD (asks if dirty: `LOAD SAVED PROJECT?` / `DISCARD UNSAVED EDITS`, 3886–3887).

**Chain is live only and is not saved.** **SOURCE** `seq_chain` + UI 3737–3746; **DOC** MANUAL 458–462 and `BEHAVIOR.md` PAT-06. Two white keys on HOME make a chain; one pattern selection ends it (`seq_chain(&seq,0,0)` 3746, also K4 4618).

### 4.6 Recover to a known state

See §2.3. Scripted “confusing state” (wrong view, held function, clip mode, help card): PATCH often returns to HOME **unless already on HOME** (then SONG) **or in clip mode** (KNOB BANK again, toast `LP OFF`). Held help: release. Held step: release (PARA will not return to SEQ until then). There is no “just play my song” that also restores clip-off, bank 1–16, and 909 SEQ in one gesture.

### 4.7 Start / stop transport

**SOURCE.** SEQ PLAY on **press** toggles `seq_start` / `seq_stop` (4007–4019). Waiting for external clock: slow PLAY blink, toast `WAITING FOR CLOCK` (8465–8466, 8847–8849). Clip mode: Loopy owns transport LEDs; SEQ PLAY also sends the APC play note unless PATCH is held (`ui.c` 4513–4514). Song vs pattern: if you start while the SONG screen is up, `seq.song_on` is set and kept even after leaving SONG (4539–4540).

---

## 5. Inconsistencies in interaction vocabulary

**SOURCE**, then **INFERRED** as UX impact.

1. **Printed BT vs function SAVE** (`panel.c` 30–31, `button_tap` 4105). Bluetooth lives in GLOBE (`R_BT` 840–845).
2. **Printed NOTE REPEAT vs function MIX** (4085–4089, help 1940). In clip mode the same button is panic / (with ARP) LATCH (`main_fm1.c` 1655, 1669).
3. **Printed PATCH vs header “HOME”** (`VIEW_N[0]` is `"HOME"`, 88; captions say `"PATCH"`, 40).
4. **White keys** mean: steps (SEQ), patterns (HOME), song-bar patterns (SONG), sound audition (PARA drums), 303 notes (keyboard), paste targets (BT held), chain ends (HOME two keys).
5. **Black keys** mean: tracks, slices, part mutes (HOME), mixer channel select, mute when PATCH is held.
6. **OCTAVE±** mean: step pages, pattern banks 1–16/17–32, 303 keyboard octave, held-step octave, TB-3PO new/mutate, copy-bank when BT held (4106–4165).
7. **SCALE** is SEL (list/yes), not a scale menu, except clip-mode SCALE really is scale.
8. **KNOB BANK** does not bank knobs; it enters Loopy clip mode (one press). `panel.c` 32 still says “KNOB BANK stays free.”
9. **`<` `>`** are page (no hold-repeat) vs list-value (hold-repeat 400/80) vs clip-key paging.
10. **PATCH** is back, no, HOME, SONG, and a modifier. One release without a modifier changes mode.
11. **Part names:** MANUAL still says 303A/303B and order 909, 808, 303A, 303B, BREAK. Source displays Chicago/Miami (and Juno/Drone) in order 909, 808, BREAK, Chicago, Miami.
12. **Fader bank A** follows `PART_ORDER` (909, 808, BREAK, Chicago), **not** “909, 808, 303A, 303B” as `BEHAVIOR.md` GEN-31 states. Bank B’s first fader is Miami (the fifth displayed part), then returns and FILTER (`main_fm1.c` 883–935).
13. Two pickup message grammars (keyboard faders vs SMC mixer, §3.4).
14. **Transport on press, almost everything else on release.**

---

## 6. User docs vs source

Repo manuals: `docs/MANUAL.md` (1000 lines, titled “X0X”), `docs/manual.html`, `docs/handoff/OPERATING-GUIDE.md`, `SMK37.md`, `PAD_MAP.md`, `BEHAVIOR.md`, `README.md` (still titled “X0X for the M-VAVE FM-1”).

**Agree (DOC matches SOURCE):**

- Hold a button ~1 s for a help card; release does nothing (MANUAL 163–164; `HELP_HOLD` 900).
- SAVE while playing: `SAVES WHEN STOPPED` / header `SAVE AT STOP`; never writes during play; autosave 4 s after stop; `AUTOSAVED` (MANUAL 686–701; `ui.c` 2222–2294, 8771–8804).
- Chain live-only, not saved (MANUAL 458–462; PAT-06).
- Boot shows the 909, empty (MANUAL 83–84; `ui_init`).
- KEY SOUND default: picking a drum while playing is silent (MANUAL 89–90; `drum_key` 3205–3208; `keysound` default 0 in `project.c` 87).
- SMK legend table in `SMK37.md` 43–44 and `BEHAVIOR.md` GEN-27 matches `panel.c` 28–31.
- Soft fader pickup (GEN-33) matches `smk_faders`.
- Undo PATCH+SEQ REC / redo PATCH+SEQ PLAY.

**Disagree or stale:**

| Claim | Where | Source at this SHA |
|---|---|---|
| ALGORITHM order “909, 808, 303A, 303B or BREAK”; tour “two clicks clockwise to 303A”, then “two more to BREAK” | MANUAL 92–97, 123 | `PART_ORDER` is 909, 808, **BREAK, 303B, 303A**. Two clicks = BREAK; 303A is four. |
| “EDIT: The keys still write its steps” | MANUAL 130 | Drum PARA white keys **audition only** (3803). |
| Header “303A green, 303B blue”; names 303A/303B | MANUAL 201, §6 title | Display is CHICAGO/MIAMI (and JUNO/DRONE); 303B cyan, 303A green; order Chicago then Miami on HOME. |
| Faders bank A = 909, 808, 303A, 303B | BEHAVIOR GEN-31 | Follow `PART_ORDER`: 909, 808, BREAK, Chicago (`main_fm1.c` 883–927). |
| “10 outer keys do nothing” | BEHAVIOR GEN-21 (cites `main_fm1.c:766–767`, which at this SHA is APC layout); stale comment `main_fm1.c` 1991–1992 | Outer keys play in the 37-key live path (`ui_live_key_owner` 4279–4297, `live_edge` 4318–4344). C3/D3 and C#3/D#3 page BREAK slices (`ui_brk_page_keys` 3596–3625; `smk_frame` 2128–2130). E3 unassigned. |
| “Keys and pads play without velocity” | BEHAVIOR GEN-38 | `drum_key` uses `plat_key_velocity` (3204–3209); HAL has velocity curves (`smk_input.h` 26, 234–263). |
| “Bluetooth MIDI is not available yet” / “BT button … only in clip mode” | BEHAVIOR GEN-08, GEN-09 (GEN-10 correctly says BT is SAVE) | Full BLE + `device_bt_save` path exists. |
| “303 parts use only Chicago and Miami” | BEHAVIOR BAS-07 | This SHA adds **JUNO and DRONE** (`ui.c` 426, 1512–1518). That is a 0.0.25.1 → 94197fc8 change. |
| MANUAL and README name the product “X0X”, FM-1 drawings (`img/fm1-keys.svg`) | MANUAL 1, 145; README 1 | USB product is “DnBOX … SMK-37 Elite/Pro”. |
| “KNOB BANK stays free” | `panel.c` 32 | `smk_clip_mode_poll` 2108–2121: it toggles clip mode. |

---

## 7. Feature verification on this `ui.c` (and 0.0.25.1)

All four features exist in `ui.c` at **both** `d4088e606` and `94197fc8` (identical file). They already existed at **0.0.25.1** (`79280e76`); the `ui.c` delta to 94197fc8 is +101/−24 and is Juno/Drone labels, held-chord recording, and related engine pages — **not** these four.

| Feature | Verdict | File:line at 94197fc8 |
|---|---|---|
| **Hold-help** | **Present.** 900 ms (BT 400 ms); printed SMK titles; release consumed. | `ui.c` 1905–2037, `draw_help` 6670–6683, footer 6738–6740 |
| **Grouped settings** | **Present.** `GR_*` page groups and GLO’s six sections; SEL list headers. | `ui.c` 1291–1292, 1677–1714, 419, `open_global` 1787+ |
| **Deferred saving** | **Present.** SAVE during play queues `save_wait`; flash only when stopped/quiet; autosave 4 s / 20 s gap. | `ui.c` 279–300, 2222–2294, 8771–8789 |
| **Soft fader pickup** | **Present** (in `main_fm1.c`, identical idea at 0.0.25.1). 2-count window or crossing; `PICK Fn UP/DOWN`. | `main_fm1.c` 886–965; header 5319–5320, 5410–5411 |

### 7.1 Cheap diff 79280e76 → 94197fc8 (UX-relevant)

`git diff --stat` on UI/docs: `ui.c` +101/−24, `main_fm1.c` +9/−1, `project.c` +29/−2, `gfx.c` +38/−4, `smk_input.h` +6, `plat.h` +4. **Unchanged:** `panel.c`, `smc_mixer.c`, `lcd.c`, `ui_theme.h`, `drum909.c`, `docs/MANUAL.md`, `BEHAVIOR.md`, `SMK37.md`, `PAD_MAP.md`.

UX-visible hunks in `ui.c`: Juno/Drone display names and engine selector (`BENGINE_N` gains `"DRONE","JUNO"`); Chicago/Miami/Juno/Drone PARA page tables; **held-key chords** recorded on polyphonic slots (`held_chord_write`, toast `CHORD NOT SAVED`); live-key / chord clear on generate/mutate. **Hold-help, grouped settings, deferred save, pickup, PATCH navigation, BT-save path, NOTE REPEAT→MIX, HOME footer, live-note owner rules, SAVE strings, and 909 BD knob order do not change between these commits.**

---

## 8. Extra depth (requested flows)

### 8.1 PATCH: back / home / song

**SOURCE** `button_tap` 4042–4051, help 1947–1956, `close_overlay` 3926–3933.

1. GLOBE inside a subsection: `global_back`.
2. Any overlay, or GLOBE’s root list: close (GLOBE returns `prev_view`).
3. Pending 303 sound request on PARA: PATCH can cancel it (`cancel_sound_request` 3939–3957).
4. Else: `set_view(ui.view == V_HOME ? V_SONG : V_HOME)`.

Help: “THE PATTERN: ALL FIVE PARTS”; “AGAIN: SONG”; “+ WHITE KEY: PATTERN”; “+ SEQ REC: UNDO  + SEQ PLAY: REDO”; “LIST: BACK   QUESTION: NO”. **There is no PATCH-to-SEQ.** From 909 SEQ, PATCH is HOME, then SONG.

In clip mode, PATCH is a modifier for Loopy history; its release is marked used (`ui.c` 4500).

### 8.2 BT save (Bluetooth-triggered), distinct from the BT **button**

Two different “BT save” ideas:

**A. The printed BT button** is project SAVE (§4.5). Feedback: toasts `SAVES WHEN STOPPED` (1300 ms), header `SAVE AT STOP` or `Q`, BT LED, then `SAVED` or failure strings. Not Bluetooth.

**B. Device Bluetooth intent** (`project_device_bt_set` / `project_device_bt_save`, `project.c` 64, 214–250; `ui.c` `device_bt_save` 8725–8745). Toggling BLUETOOTH in GLOBE (`R_BT` 840–845) sets the live bit immediately and `device_bt_pending`, independent of project dirty/autosave and of reset/load. Flush runs from `ui_frame` (8868) only when not busy, not during notes/transport, after an engine hold. Feedback: `RESTART TO TURN ON` if the controller only starts at power-on; `BT SAVE PENDING`; on flush `BLUETOOTH SAVED` or `BT SAVE FAILED: RETRYING` (retry 1 s, 8743–8744). Writes **only bit 0** of the stored settings object, “preserving unsaved project edits” (`project.c` 214–216).

### 8.3 The NOTE REPEAT mixer

**SOURCE.** Outside clip mode, NOTE REPEAT **is** the mixer: `set_view(V_MIX)` (4085–4089). Pages: PARTS (black keys 1–5 pick the part), 909 MIX, 808 MIX, MASTER, WAVE (`describe_pages` 1551–1565). Footer on PARTS: `BLACK 1-5: PART` / `PATCH+BLK: MUTE` (6763–6765). Help 1939–1942. Lamp on NOTE REPEAT means MIX (8488–8489). SMC, if connected, takes the same mix targets with its own pickup UI (`smc_mixer.c`). In clip mode this button is **not** the mixer (§2.2).

### 8.4 Context shifts (same input, new meaning)

See §5. High-risk shifts: KNOB BANK → clip (SCALE/CHORD/ARP/NOTE REPEAT leave X0X); PATCH already on HOME → SONG; NOTE REPEAT vs clip panic; white keys HOME vs SEQ; OCTAVE on HOME (banks) vs SEQ (step pages) vs 303 keys (octave) vs TB-3PO (generate). `live_boundary` **cancels sounding live notes** when owner, arp, or context changes (4358–4376). That rule is load-bearing and must be preserved.

### 8.5 Selected, playing, queued — screen and LEDs

**Display (SOURCE):** position badge = playing (or selected song bar) number; queued = NEXT glyph + blinking number (5470–5475). HOME slot: white frame blinks on a cued pattern that is not yet playing (6020). Lane playhead: white tick (`draw_home` / `lane`). Drum grid: playhead inverts the current step (5544–5545). Chain: amber bar under slots + “CHN”. Selected drum track: filled coloured side panel (5527–5529). Selected 303 step: white note body (5616).

**LEDs (SOURCE `leds`):** cued HOME pattern blinks on the pad (8509–8510); playing pattern solid; playhead inverts drum/303/break pad lights (8451, 8546–8547, 8561–8562). Muted selected track blinks.

### 8.6 Live-note and shortcut behaviour (preserve exactly)

**SOURCE. Do not “simplify” these.**

- `ui_live_key_owner` (4279–4297): owner is the current 303 voice only on `V_SEQ` or `V_PART`, 303 part, FREE PLAY **or** PARA (PARA plays, `kbd_on`), not project-busy, not perf-test, not REC, not O_ASK, not PATCH/BT held, not clip, not SMC UI, not pad-cal. A settings list or HELP on SEQ/PARA **does not** steal the keys (comment 4282–4283).
- `live_boundary` (4360–4402): entry before `input_body`, exit after (4683–4688). Context or generation change cancels the old voice and suppresses still-held physical keys. Overflow cancels and republishes. Held-note continuity across page changes is intentional and fragile.
- `live_edge` / `live_send` (4308–4344): 37 physical keys, velocity, CHORD as accent, token map `live_token` (4277). Max `UI_BASS_HELD_MAX` 37.
- `live_central_event` (4405–4423): middle 27 also enter this route when the owner is live; legacy pad-note releases stay on the old path.
- Shortcuts to preserve: PATCH+REC/PLAY undo/redo; PATCH+K2 tempo; PATCH+white pattern; PATCH+black mute; BT+white paste; BT+knob motion forget; hold step + K5–K8 p-lock; hold step + K2 chance; hold 303 step + K3 velocity; CHORD/NOTE REPEAT as accent/slide; NOTE REPEAT+OCTAVE tie, CHORD+OCTAVE rest; FX+SEQ panic; two white keys chain; KNOB BANK clip toggle and its toasts; clip PATCH+transport history; fader pickup; wheel bend that does not write pots; `RELEASE STEP FIRST`; live cancel on context change.

---

## 9. Dot handoff check

Handoff path: `/workspace/handoff-dot-20261113/`. **All seven SHA-256 sums in `SHA256SUMS` verified OK** (firmware README, `before-after-home.png` 1040×600, `journey-6-screens-proposal.png` 1040×1740, `website/AUDIT.md`, and three website reference JPEGs). The README lists other files (`comparison-01..04.png`, `before/`, `proposed/`, `render-before.c`) that **are not in this drop** — UNVERIFIABLE here.

The journey PNG is labelled **PROPOSAL, NOT IMPLEMENTED**. The before-after left frame is labelled **CURRENT · production framebuffer**, 240×240 at 2×, “94197fc8 host UI, synthetic state.” Website `AUDIT.md` is a site-design audit (Baud Girl / Felucca pages), not a firmware control map; it is not used as firmware evidence.

### 9.1 README claims

| Claim | Verdict | Evidence |
|---|---|---|
| ui.c SHA-256 `b21c83aa…f7a9f9b` equals d408 and 94197fc8 | **CONFIRMED** | `sha256sum` of `git show` at both SHAs; §0 |
| “all 5345 tracked blobs match 94197fc8” | **CONFIRMED** that this commit has 5345 blobs; the handoff’s private checkpoint was not on this box | `git ls-tree -r --name-only 94197fc8 \| wc -l` → 5345 |
| `panel.c:28–39` actual physical controls | **CONFIRMED** | `panel.c` 28–39, quoted in §1.2 |
| `ui.c:1395–1425` track parameter page creation | **CONFIRMED** | `add_track_pages` 1395–1423; rank Tune/Tone/Attack/…/P.Dpth/Pitch; Level out |
| `ui.c:1460–1503` “HOME/SEQ mapping” | **CONFIRMED** for HOME PERFORM (1468–1472: Tempo, Swing, Filter, Pump) and SEQ SEQUENCE (1473–1497). The range also includes `V_GLO` sharing HOME’s PERFORM page and the start of `V_PART`. Citation is slightly loose, not wrong. | 1460–1508 |
| `ui.c:1905–2035` help, physical labels, combinations, safe release | **CONFIRMED** | 1905–2037 |
| `ui.c:279–300` save states | **CONFIRMED** | `SAVE_SAVED/UNSAVED/QUEUED/FAILED`; labels `SAVE AT STOP`, `SAVE WHEN QUIET`, `REC OFF TO SAVE`; messages `SAVES WHEN STOPPED`, `SAVES WHEN QUIET` |
| `ui.c:2222–2294` deferred saving | **CONFIRMED** | Flash would stop audio; queue + toast |
| `ui.c:8783` continuation: stopped, not recording, not busy | **CONFIRMED** | 8783–8789; also clip quiet 4 s |
| `ui.c:6455–6505` enlarged readout | **CONFIRMED** | `draw_readout`; FONT_M (23 px), 31 px dropped |
| `ui.c:6728–6800` contextual footer | **CONFIRMED** (footer function is 6733–6804; 6728 is the end of a host-only `help_fits` test) | SMC vs WHITE/TWO/BLACK etc. |
| `drum909.c:80–82` BD Tune/Attack/Decay/P.Dpth | **CONFIRMED** as the **table order of those four sound knobs after Level is removed**. Raw table is Tune, Attack, Decay, **Level**, P.Dpth, Pitch (79–82). First PARA page is Tune, Attack, Decay, P.Dpth on K5–K8 (`add_track_pages`). | |
| Existing Barlow, role sizes 10/12/15/23 | **CONFIRMED** (T/XS/S-or-B/M). FONT_L 31 exists but is not used in the readout. | `tools/gen_font.py` 34–38, 59 |
| “SAVE AT STOP already exists” | **CONFIRMED** as a **current** header/help label when a save is queued during play | `queued_save_label` 294 |
| “Saved is only valid following successful storage callback” | **CONFIRMED** | `ui_project_save` 2260–2265: `SAVED` only if `rc==0`; `ui.dirty` cleared only then |
| Fixture SMC connection ⇒ current HOME footer shows SMC hints | **CONFIRMED** that source does this when `smc_connected()` | `ui.c` 6750–6754. Matches the left before-after frame exactly: `SMC F1-5: PART LEVEL` / `F6: REV  F7: DLY  F8: FILT` |
| Host fixture uses production `ui.c` | **UNVERIFIABLE** in this drop (no `render-before.c` here). The left HOME image is consistent with `draw_home` + `draw_header` + SMC footer at this SHA. | |

### 9.2 Proposed screens vs source (behaviour the source does not have)

All six journey frames are proposals. The left before-after HOME **does** match current source (header `HOME`, `# 1` badge, 125 + dim play icon, five lanes 909 / 808 / BREAK / CHICAGO / MIAMI, PERFORM Tempo/Swing/Filter/Pump with glyphs, SMC footer). The **right** proposed HOME and the six-screen sheet add behaviour or chrome **not** in source:

| Proposed element | Source at 94197fc8 |
|---|---|
| Title **“DnBOX”** on HOME | Header is **“HOME”** (`VIEW_N`, 88, 5445). Product name “DnBOX” appears in USB strings and “ABOUT DnBOX” (6525), not as the HOME title. |
| “125 BPM / **STOP**” (or PLAY) as text | Tempo number + `play_icon` (dim when stopped). No “STOP”/“PLAY” word in the header (5495–5498). |
| Large **“PATTERN 01”** and always-on **“CHAIN 01”** | Badge `#` + number; **“CHN”** only when `chain_a != chain_b` (5476–5477). |
| **“UNSAVED”** as a large right-side word on HOME | XS badge or `*` if it fits before x=130; plus a 2 px amber dot (5450–5460, 5501). |
| Footer **“K3: part  SEQ: steps  PATCH: home/song  HOLD BUTTON: help”** | Actual non-SMC HOME footer: `WHITE: PATTERN` / `TWO: CHAIN` / `BLACK: MUTE`. First-button: `HOLD ANY BUTTON: WHAT IT DOES`. K3/SEQ/PATCH hints are not on HOME (6757–6759, 6738–6740). |
| K5–K8 captions on the HOME PERFORM row | PERFORM knobs are titled TEMPO/SWING/FILTER/PUMP without K5–K8 in `describe_pages` 1470–1471; current framebuffer matches that. |
| Screen 02: **2×8 numbered pad grid for one track**, “909 / STEPS”, “TAP PAD: ADD/REMOVE HIT” | `draw_drum` is a **12-row all-track** grid, labels BD/SD/…/AC, no 1–16 numerals in cells (5511–5549). Dot’s README itself says this trades away kit overview. |
| “BD BASS DRUM” as a large subtitle | Voice name is the short engine name (e.g. “BD”) on the row and in tags (`engine_voice_name`). |
| Screen 03 banner **“EDITING: SOUND”**, “Applies to this drum sound”, huge DECAY 1.0 s | No such banner. Sound edit is the PARA matrix plus a 900 ms `draw_readout` (“BD  DECAY”). |
| Screen 04 banner **“EDITING: THIS STEP ONLY”**, “Step 05 held. Release to return.” | No such banner. Held-step UI is the grid highlight, p-lock notches, and tag `P-LOCK STEP 5` / 303 `STEP 5` line (8284–8288, 5645–5659). |
| “BT + knob: remove its step lock” as on-screen copy | That shortcut **exists** (2951–2953) but is help-card text, not a persistent step-edit caption. |
| Screen 05 **“SAVE QUEUED”** as a title, body “SAVE AT STOP / Playback continues. / SEQ PLAY: stop and allow saving” | State enum is `SAVE_QUEUED`; **visible** strings are `SAVE AT STOP` / `SAVES WHEN STOPPED` / compact `Q`. No “Playback continues” or instructional SEQ PLAY line. |
| Screen 06 **“PROJECT SAVED”** / “Your changes are stored.” / green treatment | Toast is **`SAVED`** (or `AUTOSAVED`) for 1300 ms, then the normal HOME returns. No “PROJECT SAVED” or “Your changes are stored.” Success does not keep a persistent green saved panel. |

**INFERRED:** none of those proposed strings should be treated as current firmware copy. The proposal’s *intent* (explain SAVE AT STOP, distinguish step vs sound, keep live shortcuts) aligns with source rules, but the screens are design studies.

---

## 10. Top 10 interaction problems (file:line)

These are **INFERRED** from SOURCE (and DOC mismatches). Not claimed as measured device trials.

1. **PATCH on a single release toggles HOME↔SONG and, from SEQ/PARA/MIX, jumps to HOME.** Easy accidental mode change; no safe “always go to the boot 909 SEQ” button. `ui.c` 4042–4051, 8604.
2. **Printed BT is Save, not Bluetooth.** A novice looking for wireless uses the button that writes the project (or, on 250–400 ms hold, starts copy/help). Bluetooth is inside GLOBE. `panel.c` 29–31; `ui.c` 2004–2016, 4105, 840–845.
3. **Printed NOTE REPEAT opens the mixer;** in clip mode the same button is panic. Lamp “NOTE REPEAT” means “MIX”. `ui.c` 4085–4089, 1939–1940, 8488–8489; `main_fm1.c` 1655, 1669.
4. **KNOB BANK, one press, enters Loopy clip mode** and remaps SCALE/CHORD/ARP/NOTE REPEAT. Toast `LOOPY CLIPS`. `main_fm1.c` 224, 2108–2121. `panel.c` 32 still says the button “stays free.”
5. **First-sound dead ends:** HOME keys do not play (`ui.c` 3733–3787). After an accidental PATCH from boot SEQ, a novice is on HOME or SONG. 303 keys are silent unless PARA / REC / FREE PLAY (345, 4279–4286).
6. **MANUAL tour and part order are wrong for this firmware:** two K3 clicks are BREAK, not 303A; displayed order is 909, 808, BREAK, Chicago, Miami. MANUAL 92–97, 123 vs `ui.c` 94–99, 108–111.
7. **Step vs sound is easy to confuse, and MANUAL is wrong about it.** PARA drum whites audition, they do not write steps (`ui.c` 3801–3803 vs MANUAL 130). SEQ→PARA can carry a held-step p-lock; PARA→SEQ is blocked with `RELEASE STEP FIRST` (2755–2759) — correct, but easy to feel “stuck.”
8. **Save / Bluetooth / copy share one control with tight timings:** tap = save (or queue), 250 ms = copy invite, 400 ms = help. Queued-save feedback shrinks to a “Q” or a 2 px dot on busy headers (`ui.c` 5450–5460, 5373–5405). Proposed “PROJECT SAVED” panel does not exist; only a 1.3 s `SAVED` toast (402–407, 2262).
9. **Same gesture, many meanings** (white keys, OCTAVE, PATCH as back/no/home/song/modifier) — §5. A single PATCH release that was meant as a mute modifier, if no key was hit, changes view (`btn_used` only set when the combo is used, 3734–3735, 4522–4523).
10. **Docs and comments that still say “outer keys do nothing” and “faders are 909/808/303A/303B” are stale**, so design work that trusts them will mis-assign outer-key live play, BREAK paging, and fader banks. GEN-21, GEN-31 vs `ui.c` 3596–3625, 4279–4344; `main_fm1.c` 883–935, 1991–1992 (stale comment).

---

## 11. Unresolved

- **No device or host-UI run in this pass.** All behaviour is source- and doc-read. Pickup feel, LED brightness, and LCD latency are not measured.
- **Dot’s full render harness** (`render-before.c`, `before/*.png` besides the one composite) is not in the handoff drop; only the composite current HOME was checked visually against source.
- **Default first-boot SMC:** the fixture forced an SMC connection. Whether a real unit shows the SMC footer on first boot depends on a live mixer link (`smc_connected()`), not verified here.
- **GEN-24 pad brightness** (“half / one eighth”) is DOC; pad path is `main_fm1.c` 868–880 (`>> sh` with sh 1 or 3) — consistent in kind, not visually measured.
- **`BEHAVIOR.md` line citations** (e.g. GEN-21’s `main_fm1.c:766-767`) are often stale at this SHA.
- **Elite vs Pro** interaction is identical in source; any hardware difference (silkscreen, plastics) is outside this tree.
- **Flash-size / “little flash reserve”** (Dot README): not measured (no build).
- **Clip-mode pad colours from Loopy** and SMC mixer pages are large subsystems; mapped at the seams only.
- **MKE-P37** shares the SMK build with a different `SMK_MODEL` string only; not otherwise mapped.

---

*End of map. Worktree: `/workspace/smk-ux-src` at `94197fc839b00aff47b98ca02f3c6396e4a6eabf`. Existing clone working tree not modified.*
