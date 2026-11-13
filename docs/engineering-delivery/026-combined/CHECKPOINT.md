# 0.0.26 combined source checkpoint

This is source integration, not a qualified firmware package. No device operation or nightly publication was performed.

## Exact inputs

All lanes started from `lane/025-zero-dropout` at `78f81bad07b8e53161326c056a2111c663349772`.

| Lane | PR | Input SHA |
|---|---|---|
| String Machine drone | #865 | `4aa5aaeb20f9c79caa4f0423a3ab956c2af7fa5b` |
| Held-key sequencer chords | #866 | `be48fcaed4e75f4ca9d6baf46cbfac1343e949bd` |
| Juno plus shared chorus | #867 | `021bfa9371fcbc96864cdd91035d997d72a0f024` |
| Carry-over tests | #868 | `68ef8df33ac2f4c988741ed320a4bf789cde3158` |

Juno already contains the drone adapter, header correction and slot tests. Its unchanged carry-over files differ only by an extra trailing newline. Retain its extended engine IDs and codec; do not apply an older drone file over it. Chord and test changes are then applied. Conflict resolutions keep both project includes and all runner entries. Generated-input chord tests run in the serialized G class.

## Purpose and behavior

The combined tree enables stored held-key chords to reach the real Drone/Juno runtime, while Chicago remains monophonic and Miami remains unison. New integration tests check exact sequence-owned pitches, Stop, mute and PANIC for both polyphonic engines in both slots. These are runtime ownership checks, not proof of perceived sound or device timing.

Chord projects retain the same payload layout but use stream marker 15 to prevent old firmware silently discarding metadata. The browser project-transfer path accepts this marker as an opaque payload: export, archives and restore preserve all bytes; firmware VALIDATE still runs before sample mutations. Older firmware rejection is tested. The offline editing adapter still rejects stream 15 rather than reinterpreting its metadata. Existing project-format-2 rate compatibility is separate and is not changed here.

## Checks at checkpoint

- Actual combined Elite SDK V1.2.14 / PADCAL=0 build: **FAIL**, XIP exceeds the unchanged limit by **6,848 B**. This includes chord code, so the standalone Juno overflow is not the combined figure.
- Focused sequencer/held-chord store tests: PASS.
- Production storage/UI/admission: 21 checks PASS.
- Drone slot integration: 102 checks PASS.
- Juno slot integration: 102 checks PASS.
- Juno runtime/PANIC: PASS.
- Combined polyphonic sequence/runtime test: 55 checks PASS with synthetic C project export enabled (51 without optional file-write checks).
- Actual C stream-15 export archived and read back byte-identically for portable and project-only containers.
- Browser project module: 42/42 tests PASS; project page: 7/7 PASS. Independent reviewer also checked offline rejection.
- Full combined host suite, final Elite/Pro builds, resource/performance qualification: pending. Local emu360 intentionally not run per latest owner instruction; no emulator or hardware result is claimed.

An initial local check attempt hit a full temporary filesystem; its incomplete results are not qualification. The integration workspace was moved to sufficient storage and the focused checks above were rerun. No source assertions or limits were weakened.
