# 26.3 held-key chord implementation

Source parent: `f34321c41a79910f6660e0021aa611ba80b2823c` (PR #866).
Exact source tree reconstructed through GitHub API and verified as
`1a9cabcfdecf769d8407b7edf30edf75f47537a0` before editing.

## Behavior

The polyphonic slot engines record actual held pitches, not chord intervals
inferred from one key. Up to six distinct held pitches form a snapshot. Live
recording quantizes a changed snapshot through the existing recorder; a held
snapshot uses its existing TIE/end-point lifecycle. One held key remains one
note. More than six held keys or insufficient storage refuses the new capture
with `CHORD NOT SAVED`, without replacing the prior cell.

Stopped step recording adds simultaneously held pitches to the same cell. A
key release ends that step-entry gesture through the existing UI ownership
path; the next press starts another cell. The runtime keeps its own per-source
key ownership. Voice capacity/stealing belongs to the selected engine, not a
new global allocator. Chicago remains mono and Miami keeps its unison pair.

Sparse extra pitches use target `0xfc` in the existing motion-lane pool, five
lanes maximum per synth pattern. The primary note, gate and velocity stay in
`bstep_t`. Pattern/song layout and project RAM size are unchanged. Empty chord
lanes are reclaimed. The current MIDI-output lifecycle adds 16 bytes of sequencer
state; it releases extra notes on the same original channel as the root.

A project with chord metadata uses storage stream revision 15 so older
firmware refuses the new meaning before its defaults/autosave path. Projects
without chords still select stream 13/14. The portable raw structure is unchanged.
The companion editor must preserve or reject the new metadata, not strip it.

## Verification and limits

- RED: the pre-change real sequencer overwrote 60 with the second held note 67.
- GREEN: the real sequencer and sparse store pass held snapshot, single-note,
  tie, mute, Stop, PANIC, pattern cue, orphan tie, mono behavior, capacity refusal,
  and engine changes inside the onset callback.
- Existing long-note test: 801/801 checks pass.
- ASan/UBSan focused sequencer check passes with LeakSanitizer disabled for this
  ptrace executor. This is not a hardware/emulator result.
- Production UI/project/admission fixture: 21 checks pass, including storage
  round-trip, stream downgrade rejection, duplicate lane rejection, whole-chord
  pitch movement, ordinary overwrite/erase, stopped held-key grouping, and
  combined NOTE LOCK/chord capacity admission.
- Independent source review identified and repaired combined lane admission,
  pre-adoption engine selection, stale metadata after manual editing, empty lane
  retention, and motion callbacks reaching metadata.
- Native Elite/Pro SDK V1.2.14 builds passed on an intermediate candidate:
  551336/551320 app bytes, 95572 static bytes, 8320 bytes pool reserve each.
  Later onset-callback edits require fresh final-source builds; these are not
  final qualification receipts.
- Full host suite is in progress. No final suite, performance or full release
  pass is claimed. No device was flashed or tested.

## Integration ownership

PR #866 supplies recording, sequencing, metadata validation, and UI step-entry.
The poly path activates for runtime kinds 2 and 3, supplied by drone/Juno lanes;
this PR alone on its 25.1 parent does not ship those engines. Retain their runtime
per-note dispatch and the `bass_adapt_pre` Miami-only tuning guard.

Overlaps to resolve: `engine.c`, `ui.c`, `project.c`, `tests/run_tests.sh`.
Do not reuse `0xfc` for preset identities. Retain drone/Juno pattern preset codes,
independent 25.1 chain fixes, all existing mono paths and tests. Final integrated
runtime testing must exercise actual drone/Juno audio, held-key release routing,
slot/pattern changes, all notes off, capacity/stealing, and measured resources.
