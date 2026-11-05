# M1 panic completion report

Lane: `lane/dev-m1-panic-complete-20261105`

Built on `wip/m1-panic-core-20261103` at `69351a26a756bec914b81633636793f3df3ea827`. That branch was not pushed, force-pushed, or otherwise modified. `main` was not modified. No pull request was opened.

Code tip before this report: `184d737d40749742980c07c30838f5b222f9551d`

Tree of that tip: `bfeefddce643d3591ffe7b2cea0c4e034b2b0719`

This report was first committed as `ddbeb3c8205f8d4978ace0a97087ddcea7326fea` (tree `e3293c4387ad519bd2beca2a88c57fe403d8381c`). The commit that records that SHA is the branch tip.

## Commits

```
184d737d gobbajogger | Keep panic hooks out of copied host checks
b4469956 gobbajogger | Record the panic choices the roadmap left open
c39e32e9 gobbajogger | Register the panic host tests and reject six behavior mutants
bd1e3e9e gobbajogger | Fire panic from the FX+SEQ and DAW bank chords
cbb901f1 gobbajogger | Clear sounding voices while transport continues
e2d6de7f gobbajogger | Release sequencer notes without stopping playback
```

Author and committer for every commit: `gobbajogger <255244738+gobbajogger@users.noreply.github.com>`. A search of the commit messages for co-authored, signed-off, and made-with trailers returned 0.

## Item status

| Item | Status | Tests |
| --- | --- | --- |
| Keyboard FX+SEQ panic path | DONE | `panic-route` (`keyboard_route`), `panic-chord` |
| SMC BANK L+R, DAW only | DONE | `panic-route` (`smc_route`): DAW fires, User and MMC do not, channel arrows `0x30`/`0x31` do not |
| Audio and MIDI note clear while transport continues | DONE | `panic-notes`, `panic-clear` (Chicago damper build and Miami voice build) |
| Integration and device-relevant validation | PARTIAL | Host routes and the engine clear are done. Device, native Elite, native Pro, controller and package runs are NOT_RUN |

## Host results

PASS:

- `panic-chord` (81 checks)
- `panic-notes` (34 checks, then the sanitizer build)
- `panic-clear` (wobble on and wobble off)
- `panic-route`
- `panic-mutants` (6 killed)

Sanitizer runs for the new panic tests: NOT_RUN. The address and undefined sanitizer runtime does not link in this environment (`libclang_rt.asan` is missing). The suite already records the same gap as NOT_RUN for `ble-midi-receive`, `ble-capture-replay`, `ble-role-switch` and `ui-303-entry`.

Full `sh tests/run_tests.sh` before the lane and after the harness fix: the FAIL set is the same 32 names. No new FAIL. No FAIL disappeared. The NOT_RUN set is the same.

Baseline and final FAIL names:

- fastmath-ubsan
- project-save-preflight
- editor-ota-dispatch
- project-stage
- smc-buttons
- ble-central-first-notification
- smc-reachability
- smc-ui-bounds
- smc-ui-negative
- smc-mapping
- smc-drum-mutes
- smc-drum-rs
- smc-filter
- smc-feedback
- smk-clip-chord-display
- padcal-flag
- smk-clip-knob
- rec-held-duration
- rec-live
- rec-live-smk
- scenario-getting_started
- scenario-patterns
- scenario-perf
- scenario-plock
- scenario-song
- scenario-sound_matrix_chicago
- scenario-sound_matrix_chicago_idle
- scenario-sound_matrix_lock
- scenario-sound_matrix_miami
- scenario-steps64
- scenario-undo
- perf-scratch

NOT_RUN, before and after:

- ble-midi-receive (ASan runtime)
- ble-capture-replay (ASan runtime)
- drum909 (reference tree missing)
- drum808 (reference tree missing)
- bass303 (reference tree missing)
- chicago-au-compare (CHICAGO_AU_REFS)
- miami-au-compare (MIAMI_AU_REFS)
- ble-role-switch (ASan runtime)
- ui-303-entry (ASan)

Device, native Elite, native Pro and controller: NOT_RUN. They were not claimed as passed.

## Mutants killed

- `notes-off-empty` — `panic-notes` fails if `seq_notes_off` does nothing
- `transport-stops` — `panic-notes` fails if panic clears `playing`
- `queued-note-kept` — `panic-clear` fails if a queued bass note-on is still applied
- `silence-skipped` — `panic-clear` fails if the audio clear is not applied
- `chord-not-consumed` — `panic-route` fails if FX and SEQ releases still change the screen
- `user-mode-fires` — `panic-route` fails if User-mode bank keys are treated as DAW

## ASSUMPTION

The list lives in `docs/engineering-delivery/panic/README.md`. Short form:

- Keyboard releases of a finished chord are consumed. A key released early still does its old action. No undo.
- The first SMC bank press keeps its old action, including drum select on drum SEQ. The second press does not repeat it. No undo.
- Channel arrows are not the chord. MMC bank commands are not holds and never panic.
- Leaving DAW while a bank is held blocks panic until both bank keys are up.
- SMC connect, disconnect and a project lock clear the SMC chord.
- Panic sends owned bass Note Offs only. No CC 123. No MIDI Stop.
- Drum voices stop. Reverb and delay tails stay. Pots, mixer, pattern and project stay.
- A killed tie does not resume. The next NOTE step still plays. Clock and playheads stay.
- Queued note-ons in that audio block are dropped. Parameters and note-offs in the block still apply.
- Chicago damper pot stays; held notes are released anyway. The arp on/off pot stays; the running arp stops.
- SMK builds also request the existing clip-key all-notes-off. The keyboard chord works on every screen, including clip mode. HOME-hold and STOP-hold do not panic.

## DECISION-NEEDED

None. The `.x0x` format and `settings_t` (40 bytes) were not changed. No persisted field was added. `rsv_bright` and `midi_ch` bit 7 were not used.

## Files

Touched:

- `firmware/src/app/ui.c` (keyboard chord only; this lane was the writer for `ui.c`)
- `firmware/src/app/smc_mixer.c`
- `firmware/src/app/engine.c`
- `firmware/src/app/engine.h`
- `firmware/src/dsp/drum909.c`
- `firmware/src/dsp/drum909.h`
- `firmware/src/dsp/drum808.c`
- `firmware/src/dsp/drum808.h`
- `firmware/src/seq/sequencer.c`
- `firmware/src/seq/sequencer.h`
- `tests/host/panic_notes_test.c`
- `tests/host/panic_notes_test.sh`
- `tests/host/panic_clear_test.c`
- `tests/host/panic_clear_test.sh`
- `tests/host/panic_route_test.c`
- `tests/host/panic_route_test.sh`
- `tests/host/panic_mutants.sh`
- `tests/run_tests.sh`
- `docs/engineering-delivery/panic/README.md`
- `docs/engineering-delivery/panic-completion-REPORT.md`

Not touched: project format, `settings_t`, `smc_buttons_test.py`, `smc_action_source_test.py`, `miami_*`, `smk_clip_knob_test.*`.

## Remote refs

`git ls-remote origin 'refs/heads/lane/dev-m1*'` after the code tip, and again after the report commits. The lane ref moves forward with those commits. The code tip above is `184d737d40749742980c07c30838f5b222f9551d`. The first report commit is `ddbeb3c8205f8d4978ace0a97087ddcea7326fea`.

`refs/heads/wip/m1-panic-core-20261103` is still `69351a26a756bec914b81633636793f3df3ea827`.

`refs/heads/main` is still `1a6bc3fe0245277b5ae31ab554c73d74b9ab4d83`.

No pull request exists for this lane.
