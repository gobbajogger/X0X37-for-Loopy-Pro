# Revision-13 project editing and live sync boundary

`project-adapter.js` opens a raw inner project exported by the existing project
page. The caller supplies the verified package model, `SMK-37 Elite` or
`SMK-37 Pro`. A USB MIDI display name is never device identity. This adapter
does not open ports, install firmware, save flash, migrate projects or read samples.

`openProject(bytes, {model})` returns a `ProjectDraft` with a copied `.draft`
and `.exportDraft(editedDraft)`. It admits exactly the 36,452-byte revision-13
layout and checks the file header, CRC, settings layout and original-engine
identities. It reuses the existing `parseProject` and CRC implementations.
It does not claim complete firmware semantic validation of opaque arrangement
or sound data; the firmware's own stage validation remains necessary before
any separately qualified import.

The adapter retains the original file privately. It changes only represented
pattern fields and the payload CRC. Song, motion, probability, p-locks, swing,
sound patches, device settings, spare bytes and sample references outside the
edited BREAK slot fields retain their original bytes. Source array mutation
and mutation of a returned draft cannot change that retained baseline.

The current offline draft has a narrower BREAK representation than firmware:
eight A slices and default A/B slice counts. Other count modes, slice intents
above eight, and high BREAK gate bits are refused without clamping. Engine
assignment, local sound edits and local generator RNG changes are refused at
project export because those controls have no established lossless mapping.
No omitted edit is silently reported as exported. BREAK slot choices must fit
their stored byte; this does not establish that the corresponding sample exists.

## Source pins and layout

All layout sources were read at
`bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`:

- `firmware/src/app/x0x.h`: settings are 40 bytes; the project orders settings,
  sound, 32 patterns, then the arrangement.
- `firmware/src/app/engine.h` and `bass_identity.h`: sound has 7 × 12 × 32
  control bytes plus two 12-byte original-engine tags, totaling 2,712 bytes.
- `firmware/src/seq/pattern.h`: drum parts are 100 bytes each; melodic parts
  are 208 bytes each; BREAK is 36 bytes; swing and padding use four bytes.
  Each pattern is therefore 656 bytes. The first pattern starts at file byte
  2,784, after the 32-byte file header, settings and sound.
- `firmware/src/app/project.c`: only stream/sound revision 13 is admitted;
  CRC covers the raw project payload. Runtime generator RNG is not in the
  stored `tb3po_cfg_t`.
- `firmware/src/app/original_sound.c`: Chicago has 22 controls and 55
  factories; Miami has 18 controls and 30 factories. Original cells use
  schema 2 and explicit OCH1/OMI1 tags.

## Why live read/write stays unavailable

`DeviceSync` accepts the connection owner's already identified `ProjectDevice`.
Attachment and reconnection never replace a local draft or erase its history.
`preview(bytes, {model})` prepares an explicit file replacement;
`applyRead(preview)` rejects a changed local revision or connection generation.
`exportProject()` preserves the accepted file's unrepresented data.
Raw origins are attached to exact `DraftStore` document snapshots, so Undo and
Redo restore both music and its original file. `DeviceSync.edit(mutator)` carries
that origin through supported local edits. An unrelated replacement or external
store edit is unassociated and cannot export by borrowing the last file's bytes.

The active `firmware/src/app/editor.c` PROJECT protocol supplies a global
export/import hold, sequential chunks, validation and commit. It has no owner
token, revision compare-and-write, or atomic transition from a read snapshot
to its write transaction. A fresh read followed by a separate import can lose
another client's intervening edit. Blind ABORT cleanup after a failed BEGIN
can also affect another client because the hold has no owner identity.
The historical commands in `web/EDITOR_PROTOCOL.md` are not implemented by
the current firmware and cannot fill this gap.

Consequently `DeviceSync.read()` and `.write()` reject with
`unsupported-atomic-sync` **before sending any command**, even when attached.
This is an explicit remaining row-104 dependency on the firmware transaction
owner, not a claim of delivered bidirectional sync. Do not replace this guard
with a browser-only read/compare/write sequence. Qualifying future live sync
requires an owner-bound snapshot, a revision-checked atomic commit, explicit
reconnect recovery, and matching firmware-side regression tests.

## Box validation

The new `project-adapter.test.mjs` and `device-sync.test.mjs` cover exact-byte
no-op round trips, raw tie/velocity/hidden-tail preservation, correct drum and
generator offsets, independent caller mutation, unsupported representations,
identity and CRC rejection, read-only previews, stale local/connection guards,
Undo, reconnect preservation and zero-command rejection of unsafe live sync.

**NOT_RUN in Codex**, as directed by the owner. The Orchestrator must execute
the editor suites, host suite and Elite/Pro builds on the box. No firmware,
device, sample backup or RAM result is claimed by this source-only change.
