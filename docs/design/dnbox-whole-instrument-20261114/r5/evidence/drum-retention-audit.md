# Drum workflow source audit after R4

The drum PARA, SEQ and MIX controls, parameter ownership, defaults and RGB-pad behaviour are unchanged across the four verified source points below. The 197-parameter packet remains valid as semantic metadata. Two qualifications must travel with it: the public EXP-table packing changed at `d158`, and the later UI question/LED line locators moved at `e581`. This is source evidence for the sole design owner, not acceptance of a revised proposal or runnable firmware.

## Exact intake

Repository: `gobbajogger/X0X37-for-Loopy-Pro`. Connected GitHub GET tools supplied full commit metadata, complete non-truncated Git trees and byte-verified public source. Requested model/effort: Sol 6.1 / ultra; effective settings are not exposed.

| Role | Verified commit | Verified tree |
|---|---|---|
| Current comparison baseline | `3c41ef4bedf9701f549ce2752b73306e2fab1eb8` | `fa0c3fe75483534f12839eeb8f5e29ec9cb52c6d` |
| Accepted source reported by parent | `d158444101d569f9ee64ba05d7cb54f24de1be30` | `0b966725cb55713dcf8817156130cc3f57f61e06` |
| Public #931 base | `e581333f1ef2586c6c37f72e80d0b51fe5a00723` | `9a570ed0b69cbd8e07e53a9b3e7048e58d52e109` |
| Public #931 head | `ea10fb1492527932476b0ecfa668348f5c1a8a03` | `152873b89c7f68c3a8369d629bf111d09964b2da` |

The parent chain is exactly `3c41 → d158 → e581 → ea10`. The public #931 metadata matched the requested base/head and reported an open draft. Its Drone archive/runtime profile is a component, with dispatcher/UI integration still required in its own description. Public source identity is not a qualified emulator/package identity. Parent-reported acceptance of `d158` is not a new acceptance assessment here.

## What changed in source

The complete change inventory contains 23 file entries: 13 at `d158`, four at `e581` and six at `ea10`. The JSON packet retains every path, blob, change count and scope disposition; the individual commit receipts retain public patches.

- `d158` keeps `ui.c` byte-identical to `3c41`. Public Chicago/Miami table/linkage savings and IMA correction-table storage change; these do not add or remove drum UI controls. The public drum-table generator changes its EXP representation from variable-width base/offset/width packing to `origin + pot*step + 21-bit residual`, with an 84-word row stride. Static AST comparison confirms unchanged `EXP_RANGES` and `pot_exp`; this helper did not run their assertions or audio comparisons.
- `e581` changes `ui.c` only at question YES drawing, question footer and SCALE button lamp. They now use the existing `action_key_cue()` availability gate. Physical SCALE is masked in clip mode, so the unavailable YES cue/lamp disappear there; PATCH/NO remains. Drum PARA/SEQ/MIX input and RGB branches are untouched. This is the only newer UI delta, not a new drum interaction system.
- `ea10` changes Drone identity validation, unavailable DSP profile/preset glue and focused test sources. It leaves the `e581` UI and all drum descriptor/default/control/light files unchanged. No missing Drum feature is inferred from the new synth-profile work.

New UI blob: `0694f20d1e7106d2479aa3846873426af7d114c9`, 390,507 bytes. Old UI blob: `0d1fb3962b3d2773fde935bf032c2167551b321e`, 390,311 bytes. New generator blob: `a347501f0787291b99714d01fef577bb9a09d706`, 16,023 bytes. Engine, project, input mapping, pad projection, colour routing, glyph atlas and drum source blobs remain identical in the crosswalk.

## Keep what already works

The actual firmware already starts on the visible 909 SEQ grid. `ui_init()` zeroes selection and opens `V_SEQ`, not HOME (`ui.c:8592–8616` at #931). A fresh/default project is empty with 16-step `1/16` drum patterns (`project.c:106–116`, `seq/pattern.c:13–21`). Keep this immediate entry. Do not treat a loaded existing project as an empty fixture.

| Current surface | Useful controls/context to preserve | Fast access |
|---|---|---|
| HOME bottom | Tempo, swing, master filter, pump pictures and values; WHITE pattern / TWO chain / BLACK mute; conditional SMC fader map | Each performance value: one turn, no page press |
| Drum SEQ bottom | K5 length, K6 rate, typed readouts/schematics; BLACK track / WHITE step / PATCH+black mute | Direct turns; one black-key selection or pad step entry |
| PARA | Complete multi-row sound overview; active physical K5–K8 slots; sparse rows; KIT accent row; bottom row/page pictures and range | One K2 event or PARA tap advances row; no confirmation |
| Held PARA | Captured STEP n, K2 chance, lock value/mark and release ownership | Hold pad, PARA, turn; release completes original step gesture |
| Drum MIX | Eleven vertical strips across screen; current voice, real envelope segments, pan marks; K5 level, K6 pan, K7 reverb, K8 delay | Four direct controls, black-key selection/mute and pad step entry remain available |
| MIX PARTS / MASTER / WAVE | Selected mix part and four controls; master and wave four controls; SCALE list / next-page hints | Retain remembered MIX page, repeat NOTE REPEAT, K2 wrap and K3 part-follow route |
| Lists and help | K2 move / K3 change / PATCH back; physical captions; first-use hold-help hint | Existing list edit and button hold routes remain |

The PARA bottom area is continuation of the same matrix, not a spare duplicate knob strip (`ui.c:8344–8350`). Removing it removes useful rows/context. The generic `BLACK: SELECT / WHITE: SOUND / SEQ: STEPS` strings exist, but the ordinary sound matrix renders its own row/page hints; do not claim both footers appear simultaneously.

Current actual drum identities are abbreviations such as BD/SD, in both grid and MIX. The existing semantic atlas and animated drum parameter schematics are not kick/snare identity icons. Recognizable new instrument pictograms are a proposal requirement; retain all eleven voice identities, 808 sound variants, selection geometry and useful context while developing them.

## End-to-end access baseline

Counts are source-derived physical presses, with release included, and deliberate turn gestures. They are not elapsed time, mechanical detents, musician testing or audible completion. The full JSON contains 36 explicit starting-state tasks; these are the representative linked paths.

| Task and starting condition | Actual route | Baseline |
|---|---|---|
| Four kicks from fresh/default empty 909 SEQ, known kick selection, stopped | Pads 0,4,8,12; SEQ PLAY | 5 presses; clock/audio outcome separate |
| Four kicks from HOME, selected909, empty working P01, explicit kick selection | SEQ; first black F#3; four pads; SEQ PLAY | 7 presses, or 6 when existing kick selection is trusted |
| Choose snare while playing, REC off, KEY SOUND STOPPED | Second black G#3 | 1 press; selection only, no extra audition/write |
| Audition snare while stopped | Second black G#3 | 1 press; select+audition |
| Add / correct a step | Empty pad creates on press; existing pad short-tap removes on release | 1 press each; existing long hold preserves |
| Reach steps49–64 from page1, loop already64 | OCTAVE+ three times | 3 presses; K5 length is a separate direct turn |
| Change rate1/16→1/8 | K6 supplies four positive value events in current enum order | 4 value events; physical turn sessions unknown |
| Set existing step5 chance100→95 | Hold pad4; one negative K2 event; release | 1 press +1 turn; no PARA entry |
| Lock kick Tune, remembered sound row1 | Hold pad4; PARA; K5; release | 2 presses +1 turn; add actual row movement if needed |
| Enter 909 MIX from SEQ | NOTE REPEAT with remembered909 page; otherwise rememberedPARTS then repeat | 1 press or 2 presses under those explicit page conditions |
| Correct step while in drum MIX | Same pad short-tap/long-hold route | 1 press; MIX does not become a lock/chance editor |
| Mute one track in SEQ or drum MIX | PATCH+chosen black key | 2 physical presses in one chord; no menu |

The prior inventory rate row put `4` in a turn-gesture field despite its note. This packet corrects that field to four value events with unknown turn-gesture count. Several earlier line locators pointed beside their function; this packet supplies exact function locators without changing their action-count result.

## Control and light constraints

SEQ black keys use source velocity and audition when stopped, KEY SOUND on or REC; local write requires SEQ + REC + playing. Drum MIX black keys instead audition at fixed0.75 only when stopped or KEY SOUND on; REC is not a MIX audition exception, and it never locally records those black-key hits. PARA white keys audition selected sound without a hidden step write (`ui.c:3195–3215,3771–3805`).

Pad release matters. Empty step creation is immediate; an existing step is removed only on release under300ms with the same kit/voice/pattern/step and no creation/edit-use flag. A long hold preserves it. Voice change cancels ownership before the later release. SEQ→PARA preserves the owner, while PARA→SEQ while held says RELEASE STEP FIRST. The ordinary MIX→PARA transition cancels rather than granting a new held sound lock. Keep these distinctions (`ui.c:2754–2780,2840–2860,3024–3079,3252–3295,3645–3656`).

The typed drum owner is `(target, voice, local parameter ID)`. MIX uses the same underlying parameter through name-resolved Level/Pan/Rev/Dly refs. A knob slot or a Chicago parameter ID is not a drum owner. Held PARA uses the step lock; normal MIX/base edits use normal motion policy. Do not substitute percentages for unqualified units/defaults from the unchanged typed packet.

Only sixteen logical white-key positions project to physical RGB pads. The 37-key keybed has no lamps on these SMK models. In drum MIX, visible kit owns selected track, step content, mute target, knob targets and hue, even if global `ui.part` differs. PARTS has no drum step content; MASTER/WAVE/FX have no drum pad owner. CHORD selects shared accent content; playing inverts the current step; held input later forces brightness. Normal mute and REC are not separate invented pad colours (`ui.c:8450–8588`; unchanged `main_fm1.c:868–879`, `plat_fm1.c:157–178`).

HOME pattern current/content rules must not leak into empty SONG. A usedP1 in empty/END SONG is dim, with no HOME current-slot bright fallback. In the dark909 palette its byte arithmetic is RGB(31,18,5), not measured luminance. Source HOME cue blink remains eligible while stopped/WAIT; any static stopped/wait treatment is a proposed intent change owned by the parent, not current-source behaviour (`ui.c:8505–8525`).

Input beyond loop length is not disabled by current source. `drum_step()` rejects only step≥64, while the grid and step-light content mask at current length. An off-loop hit can therefore be stored without extending the loop. A safer new treatment requires a named disposition and changed validation; do not silently present it as retained current behaviour.

## Improvement opportunities without access loss

1. Develop recognizable instrument pictograms in the existing identity slots; retain all voices, selected context and fast black-key mapping. Test recognition at native scale before replacing useful names.
2. Reduce repeated prose while retaining HOME/SEQ cards, full PARA row overview, units, physical knob slots and row hints. A prettier glyph is not a reason for another page or confirmation.
3. Make existing selection/audition/write scope legible through coordinated LCD, transport and REC context; preserve one-key selection and one-pad write/correction.
4. Keep total length and current16-step range distinguishable when cue/status competes for header space. Preserve OCTAVE page access and direct length/rate controls.
5. Reveal existing held-step chance and lock routes when useful, with typed drum metadata; preserve the immediate route and original release/cancel behaviour.
6. Reveal the already-working pad correction route in drum MIX while retaining eleven strips, pan marks, four controls and visible-kit light ownership.

These are bounded design inputs, not newly implemented or independently accepted improvements. The source packet gives B01–B16 preservation requirements and the rejected a898 displacement ledger. It does not claim a current revised proposal has retained them; the parent must bind each to its actual candidate location/route and action delta.

## Compatibility and limits

Unchanged packet: `R4-TYPED-DRUM-PARAMETERS.json`, SHA256 `302e49cec9744cb5496de97d25485d4a065771924c0f8a343bc63d3e7e7243a0`, 197 records: 97 for 909 and 100 for 808; 22 playable voices plus two KIT records. Eleven of its 13 source files are byte-identical at all newer points. The UI change leaves those drum domains intact. The generator changes encoding, with unchanged ranges/formula. No generated drum header is tracked in the newer tree; none was generated or fetched here. Do not reuse older decoder-layout bytes/provenance as new-head output.

No candidate, firmware or public source was edited. No firmware/host/emulator/audio/render test, reviewer, device operation, release, merge or publication ran. The only executions were source retrieval, metadata parsing, byte/hash checks and evidence assembly. Historical79280e76 captures, actual3c41 firmware, newer public source points and proposed walkthroughs remain separate. No runnable package or WASM identity is qualified by this audit. Battery, reservedSettings, CP1/CP2/CP3 and fresh reviewer admission remain with their existing owners.

The parent remains the receiving designer. Its next concrete action is to place each retained function/control in the revised whole-instrument proposal, record actual candidate before/after action counts, and send affected native comparisons/interaction traces through the already admitted independent route. Source inspection alone does not establish ease of use or a complete audible musical journey.
