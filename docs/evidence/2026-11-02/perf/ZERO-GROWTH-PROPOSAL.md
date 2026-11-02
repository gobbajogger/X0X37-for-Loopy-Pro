# PERF sound preservation: existing-shadow lease proposal

**Status: read-only proposal; implementation HOLD.** Completed current design
stage only. Next implementation requires fresh scoped admission. No implementation, native qualification or device operation is established by this proposal.

## Decision

Prefer a lease of the already allocated `project_stage.sound` (the UI undo
shadow), rather than rewriting engine base resolution. Preserve the original
sound there while the existing benchmark continues using `proj.sound` defaults.
Thus `apply_base`, `mot_base`, motion release and queued C_BASE keep their existing
reference behavior; no transient/dangling base pointer or duplicate defaults.

This existing-buffer lease is a **hypothesis with explicit verification gates**,
not a claimed ownership release. It replaces the broader prospective engine
base-resolver request with one smaller success-event notification hunk.

## Evidence and pins

Base remains c007de94f6afd4e46b22ff3e88dd0ec581cc2674. Preserved17-case packet:
`perf-sound-restore-characterization-c007.zip`, SHA256
`e5fb342c82074f0da4a600990b21c86cabe400e33e50788eafa62d096d7200fc`.
No execution was repeated for this proposal.

Pinned source:
- `firmware/src/app/ui.c`:66f71cc688291f6059a815cb4d60edebabeed7bc.
- `firmware/src/app/project.c`:82cc9eba89884e4310b3273941fb9b6335611415.
- `firmware/src/app/undo.c`:89b66232bb899953c5581f1f2007536f78cff10a.
- `firmware/src/app/editor.c`:fc9fc24ccbd8d5dd77f40a5d29d01b38b01ffa8a.
- `firmware/src/app/x0x.h`:bf8c0f92eb167d2db3e54e18e736e0edbb3ff8ae.
- `firmware/src/app/engine.c`:8ddeb9d588ece0612b8aedaafe1900d573c7215f.
- `firmware/src/app/engine.h`:b63872df6fc471ec024fad80965cf28d27c3092a.
- `firmware/src/app/original_presets.inc`:aed274ea038c7b39b61bfb8928b45c80e3db7c13.

`project.c:22` already allocates project_stage; x0x.h calls it exclusive staging,
otherwise the undo shadow. ui.c1932–1970 binds undo.shadow to its sound-and-later
region and prevents ordinary undo_tick during PERF. project_acquire8291 rejects
PERF, and editor.c EXPORT/IMPORT BEGIN calls ui_project_begin before touching
stage bytes. Ordinary project_save writes proj through io_buf, not project_stage.

Two important holes must not be concealed: deferred undo_wait at ui.c8485–8490
runs before undo_tick without a PERF guard; already-open editor/recovery ownership
must be excluded at lease acquisition. The materialized source closure is not a
substitute for a complete native caller audit of all direct stage writers.

## Concrete lifetime

1. **Start/phase0 waiting:** retain c007's real-execution ui_live_cancel_all,
   reentry rejection, Stop request and ordinary PERF-page continuity. A new
   one-byte `sound_saved` flag starts false. No snapshot is treated as valid
   before phase0 actually reaches the stopped ownership boundary. Existing
   project/staging ownership must exclude starting the lease.
2. **Acquire after the last permitted queued save:** while phase0 is stopped,
   retain the existing record_finish ordering. Use the existing nested
   engine_bass_edit_hold only around snapshot/transition, not the entire PERF.
   Commit the current undo baseline with undo_commit_stable, then establish that
   project_stage.sound equals the exact current proj.sound. This retains ordinary
   undo history semantics rather than overwriting shadow behind the history.
   Initialize the small scratch commit mask, mark the lease valid, then retain
   existing perf_build/defaults/application ordering. No new sound_t buffer.
3. **Run unchanged benchmark:** project sound temporarily contains the same
   defaults as the reference, so C_BASE and motion restoration still see21, not
   the custom original3. Do not rebind mot_base_snd, alter scenarios, suppress
   input, defer all presets, reset histories or change DSP quality/meters.
   Block only shadow users during the lease: ordinary undo_tick is already
   blocked; deferred undo retry must be paused, not discarded, until restoration.
4. **Accepted preset commits:** one notification on the *successful* publication
   path in bass_preset_boundary records the affected part in the scratch mask.
   It changes no request/adoption timing and copies no sound in the audio ISR.
   Do not infer a commit from bass_applied alone: that variable also acknowledges
   cancellation/failure. Same-default factory requests still need a commit bit.
5. **Restore after actual Stop acknowledgment:** acquire the existing preset
   edit-hold briefly. For marked parts, preserve their currently committed cell
   and identity tag in the unused scratch tail. Copy the complete saved original
   sound from project_stage.sound back to proj.sound, then reinstate only those
   genuinely committed bass cells/tags. Keep project_stage.sound unchanged, so
   the later normal undo commit can record a real accepted preset change.
   Reapply the resulting sound through the existing restore path; retain its
   metadata, motion/guard and loop restoration. Clear lease validity before
   releasing the hold and before phase4 admits saves.
6. **Abort before acquisition:** no snapshot read or restoration from scratch;
   the current live project remains authoritative. All post-acquisition manual,
   transport and normal exits converge on the same restoration. A still-blocked
   preset stays pending and may later commit normally; no cancel/drop is added.

This policy restores the exact original bytes **except** legitimate preset
commits accepted independently during the test. Queued/dirty saves then persist
that restored-plus-accepted result, never the benchmark defaults. No write is
introduced merely by PERF. The earlier pre-phase0 queued save remains allowed
and writes the original before the lease.

## Zero-growth rationale and limits

- Full2712B saved sound reuses existing project_stage storage; it is not added to
  the1968B benchmark pattern allocation.
- Native PERF scratch remains4096B, with all new metadata below0xD00. Three
  patterns plus a commit mask and two32B cells/two12B tags need about2060B,
  comfortably below3328B. Use sizeof/alignment assertions, not these host numbers
  as native proof. No editor/OTA upper-region borrowing or flash scratch.
- Proposed `sound_saved` fits the existing alignment padding in perf: one byte
  after res_peak[3] before its pointer (or an equivalent proven gap). Require
  before/after native sizeof/offset and map equality; if the actual ABI grows,
  STOP rather than adding RAM silently. This layout inference is not a build.
- No large automatic copy, persistent pointer or new queue. Preset notification
  is event-driven, not per-frame/per-sample. Scratch copies occur only at start/
  restore. Existing preset edit-hold does not silence audio or cancel requests.
- Full project/undo baseline commit has an existing bounded implementation and
  may evict history under its existing capacity policy. Tests must prove PERF
  does not add spurious benchmark undo entries or lose valid deferred undo/redo.

## One minimal engine ownership handoff

Reconcile and release **only the successful commit notification point in
`original_presets.inc::bass_preset_boundary`**, after runtime/cell/tag publication
succeeds and before final request acknowledgment, plus its minimal public
notification declaration in engine.h. The UI side owns the leased mask and
sound lifetime. No changes to engine_sound_defaults/apply_base/mot_base are
needed under this proposal; their existing behavior is the reference to keep.

This introduces an explicit UI lifetime notification at an application-layer
commit boundary. The fresh producer must audit standalone engine host link
closures for the required notification implementation; no weak production no-op
may hide a missing implementation. If that dependency is rejected, return an
alternative without introducing a hidden callback pointer allocation.

No ownership release has been established. The narrower successful-commit notification hunk requires explicit handoff before implementation. Existing engine cache, DSP, queue and glyph ownership remains untouched.

## Falsifiable gates for the fresh implementation task

Before source admission: complete exact native caller closure for project_stage,
undo entrypoints and editor recovery; establish no active lease at PERF start.
Then, only under fresh scoped authority:
- All ordinary restoration/storage cases become green without weakening their
  full-byte/control assertions. Accepted-preset expectations explicitly compare
  original sound plus only the successfully committed parts.
- Preserve the existing blocked/unblocked preset observations; add same-default
  successful request, superseded/cancelled request and failed-adoption negatives.
- Prove normal/deferred undo/redo and redo history survive, no benchmark state
  enters history, and post-restore accepted changes are undoable.
- Preexisting editor/export/recovery prevents acquisition; incoming staged writes
  cannot modify leased sound. Early abort, reentry, pending Stop and queued saves
  retain the characterized ownership ordering.
- Scratch canaries, native object sizes/map and stack checks show no growth and
  >=8192B pool reserve. No temporary view outlives its storage.
- Exact generated patterns, applied benchmark configuration and all three PCM
  references match for identical no-extra-input runs; boundary cases preserve
  their recorded C_BASE/motion behavior. Any mismatch is investigated, not
  normalized away or excused by restored bytes alone.
- Independent exact-input source review and native qualification remain pending.
  Owner-operated full on-device PERF, including WORST CASE, must show zero
  observed audio dropouts. Host plat_xruns=0 proves nothing about that gate.

## Handoff / stop

This stage ends with this proposal and preserved evidence. No source is admitted
by the document. Existing action-specific denials remain binding: this document
is not authorization to retry blocked PR creation or executor creation through
an alternate route. The existing reviewed c007 branch remains immutable. A fresh
implementation stage must consume the exact pins and finite characterization,
receive explicit scope and handoff, and resolve lease, link and ABI gates before
implementing. Documentation delivery does not lift implementation HOLD.
