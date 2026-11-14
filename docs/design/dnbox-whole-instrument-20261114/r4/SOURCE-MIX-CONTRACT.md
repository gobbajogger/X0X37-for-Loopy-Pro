# R4 current-source drum-MIX contract

Read-only contract for the incumbent design writer. Current source is `3c41ef4bedf9701f549ce2752b73306e2fab1eb8`, tree `fa0c3fe75483534f12839eeb8f5e29ec9cb52c6d`. Existing exact copies and `led-source-intake.json` were reused. No candidate, renderer, runtime, shared plan, Settings or CP1 file was changed. No emulator, host, firmware, audio, device or render execution occurred.

Requested settings remain Sol 6.1 / ultra; effective model/effort are not exposed.

## Drum-MIX draw contract

**Eleven vertical channel strips are arranged horizontally.** Neither kit uses the five-part PARTS display on its drum-MIX page. `draw_mix()` delegates immediately to `draw_drum_mix(band, visible_kit)` and returns (`ui.c:6218–6223`). The same geometry applies to 909 and 808.

Coordinates below are main-area coordinates. Add **20** for absolute LCD y. The header occupies y0–19, the main area y20–157, and the knob/footer strip y158–239 (`4694–4698`). For track `v=0…10`, let `x=4+21*v`; strip starts are **4, 25, 46, 67, 88, 109, 130, 151, 172, 193, 214**.

| Element | Exact source geometry / meaning |
|---|---|
| Outer track strip | `(x,0,19,134)`, with a selected-track edge in `dim(kit_color,8)`; other strips use the ordinary edge |
| Track caption face | `(x+1,1,17,12)`, centered label at `x+10`; selected face uses kit color and dark label ink |
| Source labels | 909: **BD SD LT MT HT RS CP CH OH CR RD**. 808: **BD SD LT MT HT RS CP CB CY OH CH**. These are current abbreviations, not existing instrument pictograms. |
| Level shadow / well | Shadow `(x+6,15,7,98)`; inner well `(x+7,16,5,96)`; **96 px** vertical level travel |
| Static level height | `f=floor(raw_level*96/127)` from `proj.sound` for this kit and voice; not a five-part master meter |
| Live hit segments | Twelve 5×6 segments, spaced every 8 px from the bottom of the 96 px travel. `engine_drum_view(kit,voice)` supplies actual envelope state; lit extent is approximately `f*envelope`. Selected live ink is white; other live ink is kit color; muted segments do not claim an audible hit. This helper did not execute that live behavior. |
| Pan track / thumb | Track `(x+3,123,13,3)`; thumb `(x+3+floor(pan*10/127),121,3,7)`; selected thumb white, otherwise gray |
| Numeric values | No per-strip numeric percentage is drawn in this main display. The selected track’s four numeric values are in the bottom knob cards. |

`ui.c:6129–6165` is the full renderer. Selection is `v==ui.sel[visible_kit]`; mute appearance checks `(seq.mute|seq.vmute)` for that kit/voice. The selected caption takes priority over its muted label color, while the level body still reflects mute. Header identity is **MIX: 909** or **MIX: 808** (`5446–5450`).

The owner’s new instrument pictures may replace the abbreviation ink, but the exact kit order, eleven destinations, selection, level travel, live/value distinction and pan functions are existing useful content to account for explicitly.

## Selected-track knob and footer contract

MIX page order is **PARTS, 909 MIX, 808 MIX, MASTER, WAVE** (`1555–1574`). Drum pages resolve parameter references by the names `Level`, `Pan`, `Rev`, `Dly` in the selected voice of the visible kit; they do not reuse a single selected part’s sound table.

| Physical control | Actual selected drum-MIX target / readout |
|---|---|
| K5 | **LEVEL** — engine gain formatted as **OFF** or signed **dB**, not a universal percentage |
| K6 | **PAN** — **L64 … C … R63** |
| K7 | **REV** — engine send amount in percent |
| K8 | **DLY** — engine send amount in percent |

The context tag is **`909 MIX: <voice>`** or **`808 MIX: <voice>`**, for example `909 MIX: BD` (`8281–8294`). The ordinary four cards remain: a context rail `(3,0,234,11)`, four `(i*60+3,13,54,55)` cards, and a footer `(0,70,240,12)` within the y158 strip. The five page dots preserve active-page context (`8354–8371`). Parameter labels use the existing caption/name path, their hue is the visible kit, and values use `pref_value_of`; an active recorded-motion readout can replace the base value (`8394–8439`). Do not invent current numeric values or audio meter activity for a static fixture.

Readout sources: `ui.c:1123–1129,1177–1182,1222–1224`; `drum909.c:253–265`; `drum808.c:1924–1938`. The ordinary cell keeps number and unit together, centered using their actual font advances (`ui.c:6903–6908`).

After a parameter turn, the existing 900 ms touch readout can cover the lower main band: a **224×50** name/value box at absolute LCD **(8,103)**, with parameter name left and formatted value/unit right (`6457–6508,6816–6822,6894–6896`; touch duration `4674–4676`). This is a transient overlay, not eleven per-track numeric values. In MIX its name is the parameter caption such as LEVEL; the kit/voice remains in the knob tag. Its source border uses `part_col()` rather than a separate visible-kit override, so do not silently describe that border as an already-correct kit-identity cue when `ui.part` differs.

After initial hold-help discovery, the drum-MIX footer is **BLACK: TRACK / PATCH+BLK: MUTE**. PARTS instead says **BLACK 1-5: PART / PATCH+BLK: MUTE**. MASTER/WAVE say **AGAIN: NEXT PAGE / SCALE: ALL AS A LIST** (`6765–6773`). A list/question has its own higher-priority footer; it is not simultaneously the ordinary drum-MIX footer.

## Drum-MIX input and hold contract

- **Black key:** selects `ui.sel[visible_kit]`, rebuilds the four knob references and stays on the current drum-MIX page. If the track changes, the old drum step hold is cancelled. The source auditions only when `keysound || !playing`, at fixed 0.75 gain (`3771–3780`). Unlike `drum_key()` in SEQ, this branch does **not** add a REC-on audition exception or write a recorded local onset. Do not normalize those two routes into one handler.
- **White key / GRID pad:** both edges go to `drum_step(visible_kit, white_index, down)`, even under the page’s list or HELP; the earlier question guard blocks ordinary editing in O_ASK (`3782–3784`; question guard at `3699–3702`). It uses that kit’s current `pat_of(kit)`, selected voice and the global 16-step page.
- **Step press/release:** an empty step turns on immediately. An existing step turns off only on a same-owner release under 300 ms when neither created nor used; a longer hold stays on. The owner includes kit, voice, pattern and absolute step (`3252–3295`). Input permits steps below64 even beyond loop length; the loop is not extended, and the LED content mask remains bounded by length.
- **CHORD + pad:** toggles the shared accent bit for the visible kit, consuming CHORD. The pad light display also switches to that shared accent row (`3279–3283,8445–8457`). CHORD + black key in drum MIX still follows the MIX selection branch; it is not the SEQ loud-hit/record route.
- **PATCH + black:** mutes the visible kit’s chosen track (`3756–3762`). With QUEUED MUTES enabled while playing, it toggles a pending track change and a repeat cancels it; otherwise it toggles the actual track mute. Do not clear or replace the stored hit pattern (`3560–3573`).
- **Held pad + K2/K5–K8:** MIX is **not** `steps_view()` and `lock_step()` accepts only V_PART. Therefore K2 does not become chance, and K5–K8 do not become step locks merely because a MIX pad is held (`341–342,2846–2854,3034–3040`). K2 keeps its MIX page/list role; K5–K8 keep ordinary mix targets, with normal motion behavior (`4554–4572,4630–4674,3100–3136`). A mix-knob turn does not pass through the p-lock path that marks a held step edited, so do not promise “release keeps the edited step” for a short MIX hold/knob gesture.
- **PARA while held:** only **SEQ→PARA** preserves a step for the established p-lock route. Leaving MIX for PARA calls the normal hold cancellation (`2754–2780`). Do not extend that preservation rule to MIX.
- **Navigation:** NOTE REPEAT advances MIX pages and re-entry retains the existing MIX page. K2/arrows browse the existing pages. K3 selects the matching drum page for a drum part, or PARTS for another part; changing visible MIX kit must not silently change `ui.part` just to choose a color (`1427–1431,4087–4092,4554–4572,4589–4610`). OCTAVE controls the global 16-step page in this non-keyboard context (`4158–4166`).

## Full normal light branch order

This is the **ordered** source branch at `ui.c:8500–8570`, entered only while LIGHTS is enabled. The order matters: PARTS/MASTER/FX must not fall through merely because `ui.part` is a drum. PART must not lose its retained drum step picture merely because its pads audition sound rather than edit steps.

| First matching context | Content masks before held-input override |
|---|---|
| **HOME or SONG** | Occupied visible-bank patterns dim. Current **PART_909** pattern bright, or the selected valid Song bar’s PART_909 pattern bright. Only HOME adds its in-bank PART_909 cue on frame bit8. Part mutes are logical black-key masks. Do not show drum-step content here. |
| **MIX PARTS or drum MIX** | PARTS: logical black part-selection/mute masks only. Drum MIX: selected visible kit’s step/accent mask, plus logical black track-selection/mute masks. |
| **Other MIX or FX** | No stored-content key picture. This includes MASTER and WAVE even if `ui.part` is 909/808. |
| **Remaining drum part** | Selected `ui.part` kit’s step/accent picture. This includes ordinary drum **SEQ and PART**; other views/overlays do not gain a new exclusion automatically. |
| **Remaining melodic part, keyboard inactive** | Notes bright, ties dim, rests off; playhead inverts the note bit. |
| **Remaining melodic keyboard context** | C reference positions dim. These are logical positions projected to pads, not piano-key lamps. |
| **Remaining BREAK** | Stored sixteen steps with playhead inversion; slice availability is logical black-key data. |

Then, **regardless of LIGHTS content gating**, source ORs `ui.keys` into bright: a held mapped white input can light a pad even on FX/MASTER or LIGHTS OFF. An eligible build-gated Miami division owner replaces both masks afterward. Dim is masked by bright. On SMK only sixteen **white-note** positions reach RGB pads; logical black masks have no physical keybed LEDs (`8572–8583`; `plat_fm1.c:157–178`).

Normal pad hue is the visible drum MIX kit when one exists; otherwise `ui.part`, **not `ui.mixsel`** (`main_fm1.c:868–879`). Clip mode and development pad calibration own RGB through separate paths; the normal branch is not their color contract. Existing exact build gates, clip feedback and division/calibration limits remain in `led-feedback-map.json`.

Source light semantics remain: hit/accent below length → invert at this kit’s absolute playhead → held white input wins bright. No normal mute/REC-specific pad hue is created. UI mode/REC/PLAY/WAIT button flags are separate from content ownership. The prior LED ledger retains those exact phases and button mappings.

This contract identifies current behavior and preservation boundaries. It does not approve the corrected proposal, demonstrate audible success or substitute for the admitted reviewer’s affected assessment.
