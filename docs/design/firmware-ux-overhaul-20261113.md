# DnBOX firmware UX overhaul — U1, design checkpoint 1 (2026-11-13)

Status: DESIGN PROPOSAL, NOT IMPLEMENTED OR HARDWARE-VERIFIED. Design-only branch. Owner review required before invasive firmware changes.

## Evidence and provenance
- Base: `lane/026-combined-20261112`; inspected `firmware/src/app/ui.c` blob `eb911f58db83e417c67dd71576331803fa6c829e` and `firmware/src/app/main_fm1.c` blob `85fe641796f734744e5745f8f26017f851fb9e49`.
- `ui.c` describes four bands (header, two content bands, knob strip), a dirty-pixel canvas/blit, physical SMK captions, 16 white step keys and 11 black track/slice keys. View enum: HOME/PART/GEN/FX/MIX/GLOBAL/SONG/PERF/SEQ. `PART_N`: 909, 808, BASS A, BASS B, BREAK. `PART_ORDER` is presentation-only and does not remap IDs.
- `main_fm1.c` describes 512-frame audio DMA halves at 44.1 kHz and UI work outside the audio ISR. This is NOT a measured Chicago page-entry diagnosis.
- PR #887 identifies U1 as the canonical firmware UX work item, distinct from installer W1/W2 and from modwheel, pattern, dropout, Juno and flash owners.
- Engine inventory remains **PARTIAL**: above are verified part labels, not an exhaustive list of selectable engines. Chicago/Miami and Juno/Drone are known project work, but their exact selectable current-source enum, availability and persistence require further pinned source reads before declaring a complete engine matrix. Do not infer an engine from a part label.
- Public Reddit/YouTube/search-interest research: NOT YET COLLECTED. No ratings, popularity, sentiment or analytics claimed.

## Product rules proposed for review
1. Every page answers: which part, which engine, what is sounding, what action the controls will take, what changed, and how to return. Maintain stable part-colour identity without relying on colour alone.
2. Direct manipulation first: select part -> sound; turn a parameter -> show exact target and current value immediately; edit step -> show note and position. No implicit page jump for momentary feedback.
3. Physical control labels are the truth. On-screen help maps K2/K3/K4 and printed PATCH/PARA/SEQ REC/SEQ PLAY to their active actions. Never present a fictional dedicated SAVE button.
4. Preserve note ownership across page changes and release events. A display transition must never send a note-off, reset a held chord, change a selected part, or interrupt audio.
5. SEQ always shows actual pitches even when chord analysis is possible. Display `Cmin7` for unambiguous C–Eb–G–Bb; `C/E` for C–E–G with E in bass. Interpret plausible voicings musically: choose a defensible conventional chord name, including inversions, extensions, omissions and slash-bass notation (e.g. `C/E`, `Cmin7`, `Dm9/A`) when supported by the actual pitches and harmonic context. If multiple names are plausible, prefer the simplest useful reading, show the full actual notes, and provide an alternate only when it adds value. Use note names alone only if no reliable chord interpretation exists. The chord label is a derived annotation, never the stored truth.
6. Juno/Drone: proposed compact held-note strip plus a zoomed piano-roll view for recorded notes. Keep physical 37-key range navigable, show out-of-window notes, and avoid implying that a held key is a recorded step. Evaluate usefulness on other melodic engines only after verified engine inventory.
7. Engine glyphs: one consistent 1-bit/limited-palette visual grammar with readable silhouette at native 240x240, paired with a text name. Never encode a critical distinction in glyph alone; no gratuitous animation or full-screen repaint.
8. Save/load/import: distinguish volatile edit, saved local preset, imported file and pattern-linked engine choice. Show confirmation and rollback semantics. Do not claim host/plugin compatibility until actual Chicago/Miami schema and version checks pass.
9. All destructive actions need a clear confirmation and safe cancellation; navigation and held-note gestures remain independently owned.
10. Experienced workflow: no forced tutorials or extra confirmation on reversible edits; retain shortcuts and parameter depth.

## Task journey inventory and acceptance (proposed, not test results)

| Journey | Existing verified foundation | Proposed target and measurable acceptance |
|---|---|---|
| First sound | PART/HOME views and 37-key input; actual default state unverified | Cold-start -> audible selected part in <=3 deliberate actions, without a manual; verify on Elite and Pro |
| Select engine/preset | BASS A/B part labels, GEN/PART views; engine picker schema pending | Show part, engine, preset, dirty status together; no accidental change to other part |
| Record with all 37 keys | White-key step mapping and black-key auxiliary mapping confirmed | Record every physical key as pitch in note-record mode; no silent step-key stealing; test 37 distinct notes and note-off ownership |
| Edit chords/steps | SEQ view and step keys confirmed | Display pitch set, bass note, step index and chord annotation; test Cmin7, C/E, Dm9/A, incomplete voicings, inversions, competing plausible readings and truly uninterpretable clusters; do not suppress a useful name solely because another interpretation exists |
| Performance and parameters | PERF/SEQ/FX and four-band canvas | One consistent immediate knob/modwheel readout with target, part and numeric value; keep original page visible |
| Save/load/import presets | Not yet source-verified | Distinct local vs imported states, explicit overwrite/cancel, round-trip and version checks |
| Engine per pattern | Existing roadmap #7; exact behavior pending | Pattern A/B may recall different engines without cross-part state leakage; focused persistence tests |
| Held-note visibility | Held-chord include in ui.c; behavior pending | Live held notes distinguish from recorded notes; transitions preserve sound until release |
| Chicago parameter entry | Owner-reported lag relative to Miami; not reproduced here | Compare entry latency, repaint area and audio continuity in identical owner scene; no regression against pinned parent |

## Native-frame journey specification (240 x 240, prototype only)
All coordinates are DESIGN targets, not claimed current render output. The renderer's real glyph metrics and colour conversion must be read before producing pixel-accurate snapshots.

- Frame A, HOME: header y0–31: `DnBOX  |  BASS B`; content y32–159: `CHICAGO` text plus meaningful glyph, preset name and playback/dirty state; lower content y160–199: current keybed/part help; knob strip y200–239: four active labels and values.
- Frame B, SEQ: header identifies part/engine/pattern; content shows step cells with pitch names, current position, note velocities and the best defensible concise chord name (including slash/extension notation) when reliable; knob strip preserves current four mappings. C–Eb–G–Bb shows `Cmin7` alongside all four notes; E–G–C shows `C/E` alongside E G C; plausible extended voicings get a proper short name rather than anonymous notes. Do not print `C minor7`.
- Frame C, HELD: header and engine retained; center shows 37-key-range held-note visibility and recorded piano-roll on separate visual layers; bottom readout shows last edited target/value. No audio gesture ownership changes.
- Frame D, PRESET: header shows engine and scope; main region lists factory/user/imported with explicit provenance; lower region previews compatibility/version and overwrite impact; cancel is always visible.
- Frame E, PARAMETER FEEDBACK: current page stays intact, existing readout area is reused where supported; e.g. `JUNO A  CUTOFF` and actual numeric value. Knob and modwheel use same grammar.

**Before/after render proof is PENDING.** Do not label these specifications as screenshots or real framebuffer captures. Next artifact: source-derived native-resolution frame harness that loads actual fonts and captures baseline/after for the same state, with image diffs and readability review.

## Path ownership and implementation slices
- This branch: `docs/design/firmware-ux-overhaul-20261113.md`, isolated host-side design/frame harness under `tools/ux_prototypes/` after inspecting existing renderer; no firmware runtime changes.
- Modwheel owner: modwheel target/value UI and its runtime integration. Design supplies only common feedback grammar.
- Pattern owner: note recording, chord/step semantics, engine-per-pattern and persistence. Design supplies frame contracts and acceptance cases.
- Juno owner: actual engine state and held-note semantics. Design supplies piano-roll/held visibility contract.
- Dropout and Chicago latency owners: runtime measurement and fixes. Design supplies measurable no-regression acceptance.
- Flash owner: savings/resource accounting. Design does not consume target flash or pool.
- Firmware integrator after owner review: `firmware/src/app/ui.c`, renderer/glyph assets, page navigation and other shared paths only with explicit ownership handoff.

## Release gates (future, NOT_RUN)
Both Elite and Pro: linked flash free >=8192 B and pool free >=8192 B; exact-head focused + full integration checks as required; pixel/readability review at 240x240; no UI entry latency regression, missed input, XRUN or dropout in the Chicago arp + Juno triad + audible BREAK + 909 four-on-floor owner scene. No claimed device proof from emulator or host tests.

## Next checkpoint
Read exact current engine selection/preset/pattern source and previous SMK custom-firmware research; collect traceable Reddit/YouTube evidence and distinguish observation from inference. Then build source-backed baseline and proposed 240x240 frame journeys with actual font assets, run pixel/layout checks, and request owner review before any invasive UI implementation.
