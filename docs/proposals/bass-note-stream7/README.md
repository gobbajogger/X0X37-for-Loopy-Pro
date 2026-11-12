# Sparse note-sound representation checkpoint

This is the representation/storage stage after75ea4845, not a completed selector rollout. X0X_BASS_NOTE_SOUND, X0X_BASS_TAGGED_SOUND and X0X_BASS_PRESET_PREVIEW all remain OFF by default. No native/device/publication claim.

## Contract

When explicitly built with note sounds, stream7 assigns existing motion-pool target0xFD/version1 to one bass onset's complete product cell32, canonical identity12 and five per-part mixer bytes. The remaining15 payload bytes are zero. pat/part/step identify the onset; no pointers or global indices require remapping. Duplicate records, orphan records at rests/ties, invalid values/identity/version/reserved bytes fail validation. Global processors/master remain project state; Chicago Comp's shared ownership is still an open runtime requirement, so this representation alone is not whole audible-sound completion.

All160 slots remain shared with ordinary motion. Capture prepares/validates a complete local record, then publishes used last. Pool exhaustion returns-7 before any source/project byte changes. Existing part copy now preflights total pool capacity before copying notes or lanes, preventing partial sound loss. Records copy with their onset and clear with their part. Motion step/ramp/release skips0xFD records.

Project size stays36420 and sound2712. Representation adds zero static/project bytes beyond rev11's existing48 live/stage delta. Capture uses one72-byte automatic record plus validator scratch; code size needs real native qualification. Host clang -Os engine object only: note-enabled __TEXT+816 bytes and __DATA+0 versus the same tagged source with note sounds off. This excludes project/UI changes and is not a native-link budget. No compiler estimate substitutes for Pro's prior5024B code/284B static margin.

Known stream6 files retain their exact layout and validate before stream7 export; a0xFD record smuggled in stream6 is refused. Frozen streams4/5 and target0xFE remain unsupported and preserved. Rev10 preparation/adoption rules remain explicit; no legacy engine is inferred from lane number or values. Browser parser recognizes exact (7,11) size36452 alongside prior known pairs.

## Executed focused evidence

`BASS_NOTE_FLAGS='-DX0X_BASS_NOTE_SOUND=1' sh tests/host/bass_tagged_storage_test.sh`

Pass1653 WBL0/1692 WBL1 plus19 browser checks. The same command with `SAN_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer'` passes. Coverage adds real stream7 storage reload, payload/tag/mixer bytes, reserved/version rejection, duplicate/orphan refusal, stream6 snapshot-smuggling refusal, copy/clear/source independence, and atomic full-pool capture/copy refusal. Existing legacy codec/adoption/editor assertions also run in each profile.

## Next bounded stage

Wire accepted live MIDI/keyboard REC onsets and stopped step writing to capture before pattern mutation; define overflow feedback. Add audio-owned snapshot playback and restoration of the selected base after snapshot ownership, preserving existing accepted notes/releases/clock and other parts. Resolve Chicago Comp shared ownership explicitly. Integrate edit/regenerate/delete lifecycle, effective motion capture, engine identity reservations and identity-aware undo/redo. Add onset/tie/repeat/copy/clear/undo/import/export/REC/pending-choice tests, then independent review and real Elite/Pro builds. Do not enable flags based on this checkpoint.

## Installed-Pro diagnosis scope

Read-only checkout at3e7fed606ec3b2858811c080a39a976c87d1b117 was verified clean. It binds A to WBL1 and B to Chicago and dispatches actual note/set/render by the selected backend. Its descriptor count is32 for both voices and WBL1 descriptor lookup falls back to Chicago for indices23..31, explaining a possible mixed parameter display. That does not establish the owner's silence cause. The dedicated read-only audio investigation owns further diagnosis; no retained project bytes were available here and Output0 must not be rewritten by inference.

## Accepted capture checkpoint (successor)

Conditional note-sound builds now capture keyboard/MIDI REC onsets before the recorder changes any pattern byte, including the currently applied motion/ramp cell and target mixer values. An edited motion capture keeps canonical engine/preset identity and marks its cell edited, without changing the selected base. Pool/identity refusal sets `NOTE SOUND NOT SAVED` while the accepted live edge still auditions and releases normally. Recorder continuation replaces prior onset snapshots with ties and removes their records. Stopped note writing, empty-step creation and rest/tie-to-note editing preflight capture; rest/tie/deletion frees it. New-line generation clears the prior line's note records; mutation keeps records only at surviving onsets. Existing copy/clear behavior remains.

Executed note-enabled ordinary and ASan+UBSan storage suites pass1682 WBL0/1721 WBL1 and19 browser assertions. New tests use actual CQ keyboard REC and MIDI input, applied motion, held continuation cleanup and full-pool atomic project refusal. Flags remain OFF. This is capture lifecycle only: audio-owned snapshot playback/restoration, shared Comp ownership, full edit/undo/import lifecycle, independent review and native Elite/Pro fit are still NOT_PROVEN. Base selector admission is the next owner-priority stage and does not require enabling note snapshots.
