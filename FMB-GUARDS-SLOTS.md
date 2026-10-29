# FMB guards and slots

**Docker Elite `=1` build: PASS. Host suite: FAIL.** Free: **118,512 bytes flash**, **3,952 bytes static RAM**, **10,328 bytes pool**. Migration is reverted; only revision/stream 13 loads. All changes are local.

Current owner instruction: 2026-10-30. Lane: `codex/fmb-guards-slots`; source starting point: `e888d046`. Older projects were never supported. Only `X0X_ORIGINAL_SYNTH=1` builds and revision/stream 13 project loads remain supported. The previous migration plan and its proof are superseded; Git history retains that record.

## Local steps

1. `b88e204e` reverts migration commit `d87e5b7b` as a separate commit. Its converters, default-patch replacement helper and migration test are removed.
2. `c9a5e946` removes `firmware/src/dsp/bass303.c` and the legacy target/host build entries. Target and host builders reject `=0`; app headers reject direct `=0` compilation. Chicago occupies the former bass303 slot (303B), and Miami occupies the former wobble slot (303A). Presentation order stays **909, 808, BREAK, CHICAGO, MIAMI**; saved part IDs do not move. The whole `experiments/wobble/` directory is preserved, including unbuilt reference `wobble.c`; its Miami conversion/decimator dependencies remain in the original-engine build.
3. `45dbcab3` deletes older stored-stream, sound-cell, clock-padding and portable-file conversion/adoption paths. The editor no longer accepts short legacy imports or offers opcode 75 adoption. Remove migration tests and legacy cases from retained storage/staging checks. Unsupported data is rejected without storage writes.
4. `264cc704` removes retired backend tests and compiler entries. Retained whole-app tests share the shipping original source list through `tests/host/original_build.sh`. The host runner now includes original integration, UI and build-composition checks. The clock scenario keeps current revision 13 persistence/normalization and drops older-layout load cases.
5. `025b5327` removes the two remaining older project migration scenarios and the old-file tail from `clocksrc`. The retained clock-source scenario passes on the same current `=1` simulator.
6. This report records the completed Docker build, host results and public-file audit. This step changes this report only.

The earlier compiler guards (`674b3930`, `457e7094`), original slot/control hooks (`1811a341`) and independent MIDI/validation repairs (`16f660de`) remain. Their old migration and `=0` claims do not apply to this candidate.

## Current scoped proof

Original integration **PASS**: 210,599 assertions in both normal and ASan/UBSan runs, matching output/captures and audio hash `79d686b167ffdfef`. Added checks reject every older sound revision 0–12 and stream 1–12 without storage writes, followed by a successful revision 13 reload. Evidence: `build/fmb-current-integration/`.

Scoped host run **PASS**, five selectors:

- Project staging: 24,334 checks, zero failures; real storage, current roundtrip, invalid input and capacity rejection.
- Recording storage: current endpoint roundtrip and unsupported stream/continuation rejection without rewriting storage.
- Editor project protocol: exact chunks, CRC/capacity, confirmation, timeout, and sample boundaries.
- Part order: 261 checks covering real UI/panel/SMC mappings and stable IDs.
- Original UI: 22,949 assertions and 36 matching normal/sanitized captures.

Command: `PATH=/tmp/fmb-native:$PATH X0X_ORIGINAL_SYNTH=1 sh tests/run_tests.sh project-stage rec-endpoint-storage project-protocol part-order original-ui`. Log: `build/fmb-current-evidence/scoped-final.log`; per-selector logs are retained beside it. Build composition: 6 PASS. Host asset setup/dispatch: 9 PASS. All twelve pinned private inputs MATCH.

## Full shipping host suite

**FAIL.** The completed full invocation at `264cc704` reports **222 PASS, 59 FAIL, 2 NOT_RUN** and exits 1. It ran in disposable source copy `/tmp/fmb-revision13-host` with `PATH=/tmp/fmb-native:$PATH X0X_ORIGINAL_SYNTH=1 X0X_TEST_JOBS=4 sh tests/run_tests.sh`. Its public tracked source/test bytes were verified against that commit, excluding only this report; all twelve local private inputs match their pins. The wrapper invokes Apple Clang with `_DARWIN_C_SOURCE` for the macOS SDK.

After removing the two obsolete migration scenarios in `025b5327`, the retained result ledger is **222 PASS, 57 FAIL, 2 NOT_RUN**. This ledger reuses the full run for unchanged retained inputs and the successful shortened `clocksrc` rerun; it is not a second full invocation. No clean-host or zero-regression result is claimed. Several retained fixtures still assume legacy synth controls or lack mocks required by the original graph; the actual failures remain FAIL.

Full log: `build/fmb-current-evidence/host-full.log`. Raw/retained ledgers: `host-results.json`, `host-retained-results.json`. First diagnostics: `host-failure-summary.txt`. The two NOT_RUN selectors are `drum909` and `drum808`, because their optional reference source trees are absent.

Retained FAIL selectors:

```text
project-save-preflight
editor-ota-dispatch
storage-scratch
project-runtime
save-note-quiet
font-pack
ble-controller-heap
smc-reachability
smk-wheel-mode
smc-ui
smc-ui-negative
smc-drum-rs
smc-clip-visibility
breaks
gate-trap
ctl-compile
update-names
validation-inputs
dev-validate-composed
quickcheck-devbug
status-lanes
status-isolation
smk-clip-chord-display
padcal-flag
smk-clip-isolation
smk-clip-knob
smp-begin-cancel
rec-endpoint-lifecycle
rec-live
rec-live-smk
ui-feedback
home-overview
sound-matrix
sound-layout
chicago-presentation
chicago-readout
matrix-knob-caption
preset-navigation-noop
ui-303-entry
scenario-blenotes
scenario-brkslice
scenario-brkslice_mode_toggle
scenario-brkslice_trs_overlap
scenario-getting_started
scenario-midiowner
scenario-patterns
scenario-perf
scenario-plock
scenario-song
scenario-sound_matrix_chicago
scenario-sound_matrix_chicago_idle
scenario-sound_matrix_lock
scenario-sound_matrix_miami
scenario-steps64
scenario-undo
scenario-usbnotes
perf-scratch
```

An initial full-suite launch used the working checkout rather than the prepared copy. Only that invocation's process tree was stopped, then restarted in the disposable copy. Its `host-full-incomplete-wrong-cwd.log` contributes no final counts. Earlier fixture-adaptation failures remain in `scoped.log`, `project-tests.log` and `project-tests-final.log`; the completed scoped run above supersedes them for those five selectors. The optional local-proof planner is NOT_RUN because `tools/local_proof_plan.py` is absent from this checkout; checks were selected from the changed consumers.

## Docker Elite target build

**PASS** in a fresh isolated tree. Build only; no device mount, installer or hardware validation.

```sh
docker run --rm --platform linux/amd64 \
  -v "$PWD/build/fmb-current-target-source:/work" \
  -v /Users/mac/.jieli/toolchain:/opt/jieli:ro \
  -v /Users/mac/fw-AC79_AIoT_SDK:/opt/sdk:ro \
  -w /work -e JIELI_TOOLCHAIN=/opt/jieli -e AC79_SDK=/opt/sdk \
  -e X0X_DEV_PADCAL=1 -e X0X_ORIGINAL_SYNTH=1 \
  x0x37-validation:local sh build.sh --board smk37 --model elite
```

Image: `sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1`. All tracked source/test bytes in the isolated tree match `025b5327`, excluding only this report; all twelve private inputs also match the pinned local bytes. Log: `build/fmb-current-evidence/target-isolated.log`. Measured app size: **434,160 of 552,672 bytes**, **118,512 bytes flash free**. Static `.data + .bss`: **94,352 of 98,304 bytes**, **3,952 bytes RAM free**. Pool: **333,736 of 344,064 bytes**, **10,328 bytes free**, above the 8,192-byte reserve. RAM-code call, ROM-reference, single-precision and HAL-register checks pass. The loader is 7,303 bytes; the local package is 651,419 bytes, identity `SMK-37 Elite_900`. Exact local artifact paths, sizes, SHA-256 values and generated-header digests are in `build/fmb-current-evidence/target-artifacts.json`. Historical margins are not reused.

The first target invocation **FAILed** linking, with XIP overflow of 101,228 bytes. A concurrent host asset-generation check changed its shared `build/gen/x0x_break_bank.h` from the target's empty bank to two built-in loops. The failed engine object contains `X0X_BREAK_0` and `X0X_BREAK_1`, 110,250 bytes each. This attempt mixed generated configurations and establishes no valid shipping flash/RAM margin. Its raw log remains `target.log`. The fresh invocation uses its own source, generated headers and outputs at `build/fmb-current-target-source/`; no host check runs there.

## Public repository and device boundaries

No private header, firmware package, controller image, plugin binary or output directory was staged or committed. Every commit was preceded by `git status`. The changed-path audit from `e888d046` passes; public changed-source manifest SHA-256 is `8f87d5e71a962fdb92a52bb55fc71a1319368b7842c540ec268ce28b22f1d8b4` (`build/fmb-current-evidence/public-source.json`); the index contains only explicit public source, tests or this report. `experiments/wobble/` has no changes. No push, install, flash or connected SMK-37 Pro access occurred. The perf-test lane retains installation ownership; firmware integration stays with the assigned integrator. This report makes no DEV CHECKED, release or hardware claim.
