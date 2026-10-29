# smc-port status

Owner request: local intent port on `codex/smc-port`, base `e888d046bd16d7f9f64e9c824ca4027570bb38fe`.
Only this worktree is writable. No push, device access, install or flash is authorized.
Chicago/Miami only; project revision 13 stays unchanged. Private headers and build artifacts must not be committed.

## Progress

- Read feature evidence in `../feature-audit/FEATURE-MAP.md` and fetched PR heads #112, #117, #122, #168 and #70. #112/#122 need a deliberate union over the two-peer code already in this base.
- Ordinary local workflow. The existing #168 strip/name/lock presentation will be reused. Rec/Select/navigation must stay live while locked.
- #679 command-credit startup behavior is present and will be preserved and regression-tested.

## Feature acceptance

Statuses below describe this source-port scope; hardware acceptance is NOT_RUN for every item. The local source commit and the common final Docker build B1 will be recorded after validation.

- 19: partial. Repeated saturated MMC STOP boundaries are fixed and both-source/cursor-rollover regressions PASS. Existing MIDI clock/Start/Stop/Continue successors are preserved. General USB/BLE MIDI-ring FIFO/IRQ work identified as #677 in the feature map is outside these requested PR ports. Build: B1 pending. Commit: pending.
- 32: done in source/host scope. Pause keeps capture, 900 ms deliberate unlock, beat/steady LED and separate MMC STOP; locked Rec/Select/navigation stay live. Lock regression PASS (724); new mid-poll Pause deadline regression PASS for both peers. Build: B1 pending. Commit: pending.
- 33: done in source/host scope. Existing two-peer controller/mixer topology is preserved; ingress, loss epochs, locks, pickup, Solo/FX and feedback retain separate owners. Multi-peer, five runtime ownership mutants, command-credit and Linux heap regressions PASS. Physical dual-SMC acceptance remains with the device lane. Build: B1 pending. Commit: pending.
- 34: done in source/host scope. Loss/link seqlock, monotonic watermarks, stale-host cutoff, mid-poll guards and feedback retries are ported per peer. Production port regression PASS (2677); reconnect/Solo-loss/Pause-loss regressions PASS. Build: B1 pending. Commit: pending.
- 35: done in source/host scope. Direct User BLE accepts maintained channels 1/16 and rejects foreign channels including overflow; preset-at-ingress and host channel-16/source filtering prevent old/foreign controls. Host routing still needs an explicit external port/routing agreement when physical senders were already mixed; documented rather than guessed. Routing/data7/production ingress tests PASS. Build: B1 pending. Commit: pending.
- 36: partial. Connected peer names are now listed in slot order with bounded storage and independent measured-width shortening. All six actual renders and independent review PASS; 29 native name/buffer assertions PASS. Existing Bluetooth state and About behavior is preserved. Late private About requirements and physical LCD acceptance are not established here. Build: B1 pending. Commit: pending.
- 107: partial. Scoped host result is 58 PASS / one inherited FAIL; original-engine host build PASS. Final target build is pending. Full engine/drum/loop/FX/motion/MIDI/UI deadline, stack and hardware combined-load qualification remains with perf-test; this lane does not claim it. Build: B1 pending. Commit: pending.
- 108: done for the requested PR scope. #112/#117/#122 are deliberately reconciled, useful #168 presentation/controls are retained with the later lock requirement, and #70 successors are preserved with runner coverage. No wholesale merge, project migration or legacy engine restoration. Build: B1 pending. Commit: pending.

## Port decisions

- #112 `d33debf135dc77cfc7767a7669e2aaeafc9a1dc6` plus included #117 `b69fb25ac2dcb510895745eff291f3166753f9c3`: ported link/loss seqlock and old-release watermarks per peer; offline/disabled central input cannot release hosted Solo. Kept current Pause-loss epochs, 900 ms unlock and FX/Solo ownership.
- #122 `6b2296f98ace4488dab213e2a9922a805117fb9a`: ported repeated STOP cutoff FIFO, preset-at-ingress, exact lengths, channel filtering, host Solo cutoffs, mid-poll epoch guards and retry/epoch-safe feedback. Used current per-peer ingress records rather than replacing them with its historical one-peer primitive. Added no new topology.
- #168 `0c5fc921b617d7edb2a831e40cbf20a0034707dc`: reused existing clearer eight-strip UI, selected lane/actions, real short drum names, Pause capture/hold/LED and pickup successors. Removed its obsolete locked-button restriction; Rec/Select/navigation stay live. No unrelated DSP/sample generator changes were imported.
- #70 `9564d183eba77c62fbd1af8555499c9c7e73c931`: clock-owner handover, pending-tick reset, malformed-host-data rejection, host sender isolation, help/confirmation routing and BLE mailbox backpressure already have current successors. Kept them unchanged and wired the existing clock-handover/owner-reset/mailbox regressions into the runner. The old page peek is superseded by the full SMC overlay; no old engine or migration code was imported.
- #679 / `5ed2693d`: ancestor of this base. Controller config, command-credit central code and peripheral host code remain byte-identical to the base. Command-credit test PASS; exact-config heap lifetime replay PASS on Linux.

## Validation and commits

- Commits: none yet.
- Host results: 59 distinct scoped checks, 58 PASS / one inherited FAIL. New production port regression PASS (2677), native peer-name/render checks PASS (29), lock regression PASS (724). Full original-engine host build PASS. Initial environment/fixture failures and their fixes remain recorded in `build/smc-port/host-tests.log`, `host-attempt1/` and `host-retry.log`. The final affected nine-check rerun is `build/smc-port/pause-epoch-final.log` (all PASS).
- Visual proposal: `build/smc-port/peer-name-proposal.svg`; independent proposal review completed with per-peer truncation and native-width requirements. Implemented actual renders: `build/host/smc-peer-names/names-*.png`. Both identities remain visible at maximum glyph width. Independent actual-render review PASS for all six renders; the missing BLE-disabled platform stub found in review was added and its closure verified.
- First Docker Elite build: FAIL, old pi32 clang parser crashed in `DiagnoseEmptyLoopBody` while source files were still changing. This is not a size PASS. The owned container was stopped; final frozen-source retry is pending. Flash/RAM free are not measured yet.
- All 12 private headers were copied from `../chicago-opt` into this lane only; manifest verification PASS (`9c7ac9b0a71e775fb7009e2d4035fce827b2c56c4e8e692a7a8910c9c6c881bc`). They remain ignored and untracked.
- Broad host checks plus environment/fixture reruns leave one genuine inherited failure: `smc-drum-rs`, two assertions at lines 452/458 for a held-pad p-lock across a pattern cue. Exact `e888d046` firmware/host source reproduced both failures with identical audio fingerprints (logs `build/smc-port/drum-rs-exact-base.log` and `build/host/smc-drum-rs.log`). It remains FAIL, not PASS. No drum/engine source was changed to hide it.
- Device validation: NOT_RUN by explicit owner restriction.

## Final verification in progress

- Host results across the scoped runner and retries: 59 distinct checks so far; 58 PASS, one inherited FAIL (`smc-drum-rs`). Existing #70 mailbox backpressure PASS (1834 assertions), clock handover PASS and INTERNAL-owner reset PASS are now wired into `tests/run_tests.sh`.
- The exact-base drum failure was reproduced in `build/smc-port/base-e888d046` without editing another lane. The two failing held-pad p-lock assertions and audio fingerprints match the candidate.
- A new two-peer Pause deadline test exposed a mid-poll reconnect unlock race. The pre-fix test failed at the intended assertion; the epoch guard was added and independently reviewed. All nine affected regression selectors PASS; independent follow-up review found no blocker. The frozen target snapshot will be updated for this source change after its current compile finishes.
- No push, connected-device access or device write has occurred.
