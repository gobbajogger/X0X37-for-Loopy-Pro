# Offline editor integration intake

The offline editor has usable source for local musical preparation, but this intake does not establish a deployed editor or accepted corrected sound controls. Preserve the existing editor-controls writer, `6af84a83-48d8-8191-aeef-62c85b3be0db`. No source, test, firmware, MIDI, device or publishing work was taken over.

## Exact inputs consumed

Live canonical [PR #913](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/913) remained open, draft and unmerged at `f65de4148cdbbadefa896f2b59f6c156f7e5ed74`. It adds only `docs/evidence/offline-sound-parity/REPORT.md` and `matrix.json`. There were no PR comments, submitted reviews or inline review threads at intake. Its body reports an independent technical review; an underlying review receipt was not delivered in this PR.

The application pin is separately `94197fc839b00aff47b98ca02f3c6396e4a6eabf`, root tree `72d28fbc741244d18fa3d18495da16ab0fcb1d24`. Live canonical firmware `nightly` was `83fc6d60289869116f93352cf845a9a7e271cdba`; these versions must not be presented as interchangeable.

- [PR #913 report](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/f65de4148cdbbadefa896f2b59f6c156f7e5ed74/docs/evidence/offline-sound-parity/REPORT.md): Git blob `4cb1bf89f8a0d07305c180bf3f3f81912974ab51`, SHA-256 `e2380f001e99b19f5b663aafde3c60ca96415e9f24ecd67e96bd7888783f5cc6`.
- [PR #913 matrix](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/f65de4148cdbbadefa896f2b59f6c156f7e5ed74/docs/evidence/offline-sound-parity/matrix.json): Git blob `fd290041bf0c665576a98a8254b013ba1fab5fef`, SHA-256 `0b523ac2d448f52805dd843903ce45f3bbdc0610beaa4df3aa26bf74f4fa63bb`.
- Nineteen editor files and both PR evidence files were actually fetched and independently matched to their Git blob identities. [Byte receipt](receipts/byte-identity-check.json) records all 21 files, byte counts, Git SHA-1 and SHA-256.
- All 80 matrix references match the 28 unique source paths in the complete, non-truncated candidate tree. This tree check does not mean all 28 files were byte-consumed here. [Identity receipt](receipts/parity-source-identity-check.json).
- A disjoint read-only helper additionally consumed the three exact firmware contract files and checked the adapter/model contract. [Helper receipt](receipts/disjoint-semantics-check.json). No application tests were rerun.

Project AGENTS.md, task/routing/ownership and plain-writing instructions were read. Only relevant Working/Pending/Complete excerpts were consumed; scored ROADMAP history was not read. The complete tracker snapshots themselves are coordinator-owned. [Selected excerpts](receipts/tracker-excerpts.json).

## Entry and user workflow contract

The application entry is [`web/offline-sequencer/index.html`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/index.html). Serve the repository's `web` directory from a trusted HTTPS or localhost origin with ES modules and workers enabled; its route is `/offline-sequencer/`. Serving repository root instead places the same entry at `/web/offline-sequencer/`. These are source routes, not verified deployed URLs.

The parent intake owner observed normal-TLS HTTP 404 for canonical and Nightly `/offline-sequencer/` at 13:29–13:31 UTC. That is a deployment gap, not absent implementation. This child did not repeat those requests or use a certificate workaround. No public editor launch should be advertised until the publisher supplies a working entry for the exact accepted bytes.

| Existing rows | Source-backed preparation workflow | Integration limit |
|---|---|---|
| #90–#92 | Two synth piano rolls, separate 909/808 hit grids and BREAK gate/slice grid; 32 local patterns. Synth/drum storage has 64 steps; BREAK has 16. | The draft omits probability/condition, motion/p-lock, per-note sound, arrangement/song/chain and device routing/clock/transport. |
| #93–#94 | SMF 0/1 PPQN import in a worker; explicit track/channel/window mapping, replacement and loss review, then one local Undo transaction. Bounded to 1 MiB, 256 tracks and 100,000 events. | BREAK import is disabled. No automatic polyphony reduction, wrapping or pattern splitting. Drum per-hit velocity and unsupported MIDI data are reported losses. |
| #95/#100 | Local JSON open/export; validated primary and last-good local storage; exclusive Web Lock, stale-tab comparison, queued writes; 32-frame Undo/Redo. | Requires safe origin/locks for saving. Failed saves halt further writes; export/reload or explicit recovery remains necessary. The recovery copy is not a device backup. |
| #96 | Synth New/Mutate creates a revisioned preview with an explicit replacement/loss report. | No local audio, live auto-mutate, qualified BREAK output or dedicated drum generator. New replaces all 64 stored cells, including inactive cells. |
| #98/#99 | Complete-note group copy/cut/paste/delete/move/transpose/velocity; full-part copy/cut/paste/clear includes hidden tails and pattern-local settings. | Collisions, bounds or changed revisions reject. Part actions exclude global sound/engine assignment; clipboards are tab-local. |
| #97/#101/#102 | Root/scale/new-note snap, labelled read-only other-synth ghost, local pitch keyboard, mobile Editor/Sound views and gesture cancellation paths. | Keyboard has no audio. Grid/keyboard view state is transient. Actual iPhone Safari/VoiceOver acceptance remains unverified. |

Source contracts actually read include [model](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/model.js), [SMF mapping](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/smf.js), [note groups](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/note-groups.js), [part operations](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/part-operations.js), [storage guards](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/draft-guards.js) and their editor UI integration. Source presence is not new runtime acceptance.

## Saved controls versus performance

At candidate94197fc8 the firmware has:

| Engine | Controls | Saved patch controls | Performance-only PBend/Damper IDs |
|---|---:|---:|---|
| Chicago | 22 | 20 | 20 / 21 |
| Miami | 18 | 16 | 16 / 17 |
| Juno | 18 | 16 | 16 / 17 |
| Drone | 8 | 6 | 6 / 7 |

The final two controls reject patch writes and restore neutral bend/damper. Chicago arp IDs 13–16 are saved patch macros with live-keyboard semantics. Older schema-2 cells can retain historical performance bytes/edit bits without making those values current saved patch edits. [Firmware setter/restore](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/original_sound.c#L237-L300).

The consumed browser application still has historical Chicago 32-position and Miami 23-position families. It has no Juno/Drone keys. PBend is disabled; historical damper keys remain editable as global `doc.sound` values. Thus serialized browser sound drafts must not be represented as the firmware's saved patch contract. Global local sound values span patterns; BREAK generator settings 0–11 are pattern-local; BREAK Rev/Dly use `mix/4/1` and `mix/4/2`. Shared FX/master scopes are global. Editor part order needs translation `2/3/0/1/4` before any future firmware dispatch.

PR #913's 303 catalog entries plus 26 missing Juno/Drone controls are distinct from expanded draft keys. Its 262 index/name/range matches establish metadata correspondence, not sound or DSP fidelity. None of its seven findings is fixed by this documentation PR.

## Project, sample and connected-device boundary

The [project adapter](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/project-adapter.js#L15-L123) accepts a narrower, explicit raw revision/stream-13 subset: exact 36,452-byte format-1 data with model selection and CRC/identity validation. It does not open portable `.x0xproject` bundles, sample archives, firmware files or streams 14/15.

Admitted project sound remains opaque in the retained original bytes; the editable draft begins with `sound={}`. Nonempty sound drafts, changed engine assignments, changed local generator RNG and unrepresentable BREAK/rate intent reject export. The adapter admits Chicago factory 0–54 and FxType 0–6; candidate firmware supports factory 0–80 and FxType 0–11. Juno/Drone identities are rejected. These are compatibility limits; fail-closed rejection is not evidence of device corruption.

Raw project origins follow exact document snapshots through supported edits and Undo/Redo. The UI does not borrow an earlier file origin after unrelated draft replacement. Preserve the original project and its separate sample backup.

[DeviceSync](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer/device-sync.js#L4-L57) rejects read/write before commands with `unsupported-atomic-sync`. Browser storage locks are not firmware owner-bound transaction/CAS. There is no editor audition, MIDI-port request, live triggering, sample transfer or firmware operation. Community draft files, DNPR preset files, TouchOSC and the separate performance companion remain distinct interfaces under their existing owners.

## Findings and receiving actions

1. **Current controls correction is not yet consumable here.** The tracker names S103 archive SHA-256 `5664a4cce007bc4ca8f777dcacd7a775ca5de7805997f9fb22d1ec5701e5b9f8`, patch `13196607186759a026b673ad3860d219a6f6600906937f812b19479302477a71` and virtual tree `da2b1f159dadf24005caaf75755765d116595331`. These are pointer-level identities from the coordinator snapshot; their bytes were not received by this child. Do not label the changed controls reviewed or ready from these pointers.
2. **Unset ambiguity remains in consumed source.** `editor.js:254–258` displays “Unset” for missing sound keys but positions the range at its minimum. It has no explicit numeric-entry alternative, Unset `aria-valuetext` or one-step Unset action. A set value already has a numeric text output. The existing writer is recorded as correcting the ambiguity after independent mockup review. Receive its exact changed runnable bytes plus focused persistent/performance, zero/Unset, Undo/Redo and saved-draft evidence before website integration.
3. **Six-rate MIDI import defaults still use four divisors.** `editor.js:313` divides length by `[4,6,8,3][rate]`; rates 4/5 therefore produce an invalid default Beats value. This source cause matches existing R1. This intake did not reproduce it in a browser or run the suite. The existing editor source owner should correct the default and requalify the affected import workflow; the manually selected valid window is only a workaround.
4. **Mobile/group acceptance remains partial.** The tracker reports unnamed modals/preview focus loss and group rejection text hidden behind a modal. Consumed `index.html` has no dialog accessible-name binding; `editor.js:75` changes the heading but does not bind it, and `editor.js:405` writes group errors to the page status outside the modal. The underlying runtime receipts were not supplied here. Reuse existing reviewer evidence and route only unresolved fixes to the current writer; no complete phone-accessibility claim.
5. **Deployed discovery remains unavailable.** Integrate an exact accepted source entry under the sole publisher only when permitted. A Git source link can identify implementation; it cannot replace a usable public editing route.

## Proof, export and ownership state

Existing 37 scoped and 16 independent model tests, 34 group assertions, Chromium tasks and earlier generator oracle claims are reported historical evidence, not new passes from this intake. The raw execution packets were not supplied here. PR #913 explicitly reports earlier C/Node/extractor successes as history and includes no investigator scripts; it cannot reproduce them alone. F6b source handoff's NOT_RUN statements belong to that older stage and do not erase later reported evidence. No fresh full-candidate acceptance of the changed controls was received.

The inherited editor public-export denial remains binding. Its exact denied tool/target/reason is not exposed in the delivered tracker excerpt; this child attempted no export, retry, bypass, Git mutation, public comment or deployment. PR #913's separately successful documentation preservation does not remove that denial. PR #130 and all publication denials remain with their existing owners.

Next intake belongs to the parent capability owner when the active editor writer supplies a legitimate exact candidate and review receipt. Preserve fail-closed project/device boundaries and version labels. Keep website integration, W3/librarian, companion, importer and sole-publisher authority separate.

Sol 6.1/ultra was requested for this work and explicitly requested for the helper. Effective model routing was not exposed.

