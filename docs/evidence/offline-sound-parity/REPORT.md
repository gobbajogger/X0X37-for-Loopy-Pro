# Offline editor sound contract at 94197fc8

The offline editor has an internally consistent draft catalog but only partial alignment with the current firmware sound contract at the source pin below. Its sound values remain an offline sketch: sound-to-project conversion and device sound writes are unavailable. The seven findings describe that source contract; they do not establish acoustic fidelity or a device corruption defect.

## Source and coverage

Source commit: [94197fc839b00aff47b98ca02f3c6396e4a6eabf](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/94197fc839b00aff47b98ca02f3c6396e4a6eabf). Root tree: `72d28fbc741244d18fa3d18495da16ab0fcb1d24`. “Current” below means this immutable source, not every later branch.

[matrix.json](matrix.json) preserves 329 control rows: **303 catalog entries plus 26 missing Juno/Drone controls**. Four auxiliary controls are separate. The 364 expanded global draft keys and 12 pattern-local BREAK settings are different counting units.

| Classification | Rows |
|---|---:|
| Index/name/range descriptor match | 262 |
| Legacy Chicago position or unsupported control | 32 |
| Miami domain mismatch | 4 |
| Miami unsupported extension | 5 |
| Missing Juno/Drone control | 26 |
| Total | 329 |

The **262 matches are descriptor metadata, not DSP fidelity**. They include 248 non-synth rows and 14 Miami rows. Five mixer descriptor rows expand across five parts; they are not 25 extra catalog entries. No percentage or product-acceptance score follows from these counts.

The matrix provides public source paths, exact Git blob identities, line ranges, intended descriptors and finding references. No source file bytes are embedded. All 28 referenced source paths were checked against the complete, non-truncated public tree and their exact Git blob bytes during this preservation stage.

## Seven source-contract findings

### S103-1 — Chicago positions and envelopes are stale

The catalog exposes 32 historical positions while the current Chicago contract has 22 controls. For example, local Cutoff at index 0 does not describe current macro 0, Wave. Independent amp/filter envelope edits also do not describe the current Env A/D/S/R macro family. The prior investigation reported coupled envelope semantics; its standalone check specifically reported Env Attack changing both filter and amp, without independently exercising every A/D/S/R macro. Directly reusing the old indexes would reinterpret intent; no such device adapter is currently active. See the [editor](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/editor.js#L250-L275), [current descriptors](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/original_sound.c#L9-L16) and [macro labels/states](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/ui_original_controls.h#L10-L69).

Affected-path acceptance: a versioned descriptor family must give every current ID one canonical key, project selectors through the current q127 mapping, and preserve coupled envelopes and FX 17–19 identities. Legacy keys need explicit review or rejection with the original draft retained. One sound transaction must preserve Undo/Redo.

### S103-2 — Miami selectors and extension IDs are stale

The editor labels wave value 1 Square. The prior standalone semantic check reported that current q1 still selects Triangle; the descriptor declaration alone does not establish that result. Current carrier, XMod and wobble selectors have 3, 4 and 4 states. The 18-ID current contract has no IDs 18–22 for the local AAtt/ADec/ASus/ARel/BassSpl extensions. See [editor choices](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/editor.js#L252-L258), [current descriptors](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/original_sound.c#L60-L78), [control-value projection](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/original_sound.c#L161-L172), [selector state counts](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/ui_original_controls.h#L55-L74), [native shape preview](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/ui_original_controls.h#L125-L145) and [shape names](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/ui.c#L1092-L1101).

Affected-path acceptance: all current shapes must be reachable at their actual q positions; q1 must not be labelled Square. Unsupported extension keys need explicit legacy handling and must never silently become current macros. Preserve original program mapping and untouched factory precision.

### S103-3 — Performance inputs are not saved patch edits

PBend is disabled in the editor, but Miami Damper/17 and historical Chicago Damp/31 remain editable as global sound values. The current contract reserves each engine's final two controls for performance; patch writes reject them and restore supplies neutral bend/damper. Chicago arp IDs 13–16 remain saved patch macros with live-keyboard behavior. See [editor](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/editor.js#L254-L268), [patch setter](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/original_sound.c#L238-L245) and [restore](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/original_sound.c#L275-L280).

Affected-path acceptance: exclude performance IDs from patch edits. Any local performance sketch must be explicitly non-exportable. Retain stored arp semantics.

### S103-4 — Juno and Drone are unsupported by the editor

Firmware exposes 18 Juno and 8 Drone controls, including performance inputs. The editor only admits Chicago/Miami and the adapter rejects Juno/Drone project identities. This is an editor coverage limit. See [engine selection](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/editor.js#L264-L265), [validation](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/model.js#L15-L29) and [firmware descriptors](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/original_sound.c#L31-L65).

Affected-path acceptance for any later extension: engine selection, key namespaces, validation and stated project support must agree. Keep final two controls performance-only and preserve the one-Miami-context guard without applying it to Juno/Drone. Preserve approved original engines and gain.

### S103-5 — Sound export fails closed; admitted original ranges lag firmware

Import keeps sound opaque and leaves `doc.sound={}`; any nonempty `doc.sound` rejects project export. Even admitted raw revision/stream 13 projects reject Chicago factories 55–80 and FxType 7–11: the adapter uses 55 factories/max 6 while current firmware uses 81/max 11. Unsupported identities, streams 14/15 and extended-rate data remain rejected. See [adapter admission](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/project-adapter.js#L18-L78), [export guard](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/project-adapter.js#L81-L122) and [current original contract](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/original_sound.h#L19-L54).

Affected-path acceptance: keep rejection until an independently reviewed original-cell/edit-mask adapter exists. State an explicit conversion matrix, align admitted IDs/ranges, retain opaque bytes on rejection and preserve unedited factory floats. **Fail-closed sound export is not a device corruption claim.**

### S103-6 — Numeric metadata omits choices and dependent meanings

The 248 matching non-synth rows do not encode all control meaning. Drum Dist 0 means Diode, shared FX Dist 0 means Off; BREAK Pitch position 12 means zero semitones. 808 Sound selects the voice that later pots edit, while Clap Attack always targets maracas. Master COMP/FILTER update dependent controls; independent draft integers do not simulate that state. See [909 choices](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/dsp/drum909.c#L56-L66), [808 routing](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/dsp/drum808.c#L1909-L1987), [Clap Attack target](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/dsp/drum808.c#L1741-L1746), [BREAK choices](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/dsp/breaks.c#L63-L76) and [master setters](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/dsp/master.c#L118-L175).

Affected-path acceptance: show exact choices/units and selected voice/part dependencies. Distinguish pot positions from physical values. Preserve BREAK Rev/Dly aliases at `mix/4/1` and `mix/4/2`; translate editor part order 2/3/0/1/4 before any future firmware dispatch. Any later dependent-write implementation must make one Undo transaction.

### S103-7 — BREAK Slice mode has a historical meaning

The local 0/1 toggle means GM36–43/note%8. Firmware instead packs independent A/B count codes: 0=8, 1=4, 2=16, 3=32 slices. The adapter rejects nonzero packed counts on import and local mode 1 on export. See [toggle](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/editor.js#L274), [packed count contract](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/seq/pattern.h#L68-L87) and [adapter guard](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/project-adapter.js#L69-L76).

Affected-path acceptance: label the historical sketch accurately or provide a separately versioned exact A/B count representation. Represent or visibly reject 4/8/16/32 intent and retain unchanged unsupported bytes. The old toggle is not a count-code adapter.

## Reproduction and evidence limits

Open the commit-pinned links or inspect a local copy with `git show 94197fc839b00aff47b98ca02f3c6396e4a6eabf:<path>`. Compare each matrix source object's blob with `git ls-tree` at the same commit, then inspect its line range. Compare catalog rows with `controls.json`, validation with `control-schema.js`/`model.js`, and source meaning with the cited firmware descriptor/setter. This reproduces the source comparison without a device or SDK.

The completed investigation recorded standalone C synth-semantic assertions (compile/run exit 0), metadata extraction (compile/run exit 0; 243 non-synth descriptor rows), and targeted Node schema/JSON/Undo/Redo assertions (exit 0). The 248 non-synth catalog matches include five mixer descriptors. These are **prior reported results**, consumed from the verification receipt and not rerun for this documentation stage. Prior exploratory setup failures remain failures of those attempts; corrected runs do not turn them into product regression results. Investigator assertion/extractor scripts are not included here, so this pair alone does not independently reproduce those execution receipts.

This stage verified intake bytes, source identities, inventory consistency, public citations and sanitized documentation. It did not repeat application assertions, browser/storage suites, codec tests, firmware/native builds or audio renders. Browser, device, MIDI, performance and DSP-fidelity acceptance are unclaimed. Source preservation and technical review do not close partial sound parity or grant product acceptance.

Any later correction must preserve original engine behavior, approved gain, legacy drafts and untouched factory precision. This evidence changes no application, DSP, project format or device behavior.
