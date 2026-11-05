# Pinned M1 recut validation — 2026-11-05

State: BUILD_ONLY. The results below were executed by the task's primary agent on Linux x86-64. No device, install, publication or measured target-cycle qualification is claimed.

- Required base: `a06e20189141ff5fb8766e2c6152be460f4356c3`.
- Original lane: `lane/build-25-m1-port-v2-20261106`, tip `a18561f17977be8592cef5a3d3cbebf70e4dbf1b`.
- Own tip boundary: `a0311156fab64e889516203d5676e77039b6a06b`; apply only its 12 following panic commits. The earlier integration stack is excluded.
- Validated source: `e4c7fc671437ecdcb06c35582df54ca9feb08763`, tree `b6d666abc6d575a4ede2211ab48d01773b7c9122`. This report is a later evidence-only update.
- Full host suite: **337 PASS, 0 FAIL, 0 SKIP, 0 NOT_RUN; exit 0**. All 11 distinct panic mutants were killed by executed assertions. GCC/Clang sanitizer coverage ran; compiler/setup errors are rejected by the mutation runner.
- Full-suite log SHA-256: `3391ed96cd1d4ec49b1a42d3dcf8cef3b705ac4f9490be7c2fa27974d013ae69`.
- Independent source review completed; all actionable findings and the navigation-fixture follow-up were closed.

## Native DEV builds

Both Elite and Pro builds completed with SDK `AC79NN_SDK_V1.2.1_2023-12-13` at `d179b4484759423312073f5fbb232501aa491047` and JieLi Clang 4.0.1. All three app SDK inputs and eight controller-source inputs matched their pins. The ten required private synth inputs matched the committed manifest; their bytes remain excluded from Git.

Native builds used source `690aef5b754d27ba8626290b8e808da510b36e65`. The only later source edits before the full-suite run were two host-test corrections; all native build inputs remained identical, and all eight recorded native artifact digests still match.

| Metric, both models | Base | Recut | Delta |
| --- | ---: | ---: | ---: |
| App image | 474,840 B | 475,992 B | +1,152 B |
| App RAM region | 95,780 B | 95,780 B | 0 B |
| `.ram_text` | 2,712 B | 2,712 B | 0 B |
| Pool | 335,464 B | 335,464 B | 0 B |

App RAM headroom is 2,524 B against 98,304 B. Pool headroom is 8,600 B, above the 8,192 B minimum. `.ram_text` has 844 disassembled instructions and no calls. Neither model links soft-double routines; all 20 locked settings pass. Target cycles/sample remain unmeasured against the approximately 7,256-cycle budget.

Package digests (bytes are retained locally, not committed):

- `build/x0x-smk37.fwsc`: `079a6855a89b56b15373a96eae7e9eece714be1d32eebf7ebafb786172a773e1`
- `build/x0x-smk37-elite.fwsc`: `51c08a8ac4be5f0a6d84f8e0383b0a4180ead8b184cbecbcc9004696ed72b4b2`

## Repairs verified in this recut

Panic consumes pending physical-key presses before silence, keeps SMC chord state per peer, resets a peer's stale chord after a lost BANK release, and preserves BREAK's bank/phrase progression. Tests cover bank B/slice 2 through panic followed by bank B/slice 3, audible 808 silence, fresh physical-key input, independent peers and queue overflow. Chicago fixtures explicitly select the native engine and controls.

The existing drum-mute navigation fixture now releases BANK R before pressing BANK L, retaining every original assertion. Simultaneous BANK L/R remains covered by the panic-route regression.

Executed commands, with pinned compiler/SDK/reference environment configured:

```sh
X0X_DEV_PADCAL=1 MODEL=pro ./build.sh --board smk37
X0X_DEV_PADCAL=1 MODEL=elite ./build.sh --board smk37
X0X_STRICT_PREFLIGHT=1 X0X_TEST_JOBS=8 sh tests/run_tests.sh
```

Reference commits: 9W9 `10cbe5c1e1c9e65b0818bb34644ab1a13092082c`, 8W8 `94aa2716d4c2e5851fc7c5dbb4c17adfd133c997`, TB3PO `f49d81d05d1e29ae37728a08e30f193892be3bc9`. SDK/toolchain/private inputs, firmware binaries, credentials and raw captures are excluded from source delivery. Source delivery is verified separately by live lane-ref readback.

---

## Historical original-lane report

The following report retains its original source pins and earlier results.

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
