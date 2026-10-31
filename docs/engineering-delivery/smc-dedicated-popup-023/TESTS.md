# SMC and mixer results

This is the audit at `02c5224d`, before the transport correction.
[Fresh transport checks](TRANSPORT.md) are reported separately.

Aggregate: **38 PASS / 9 FAIL / 0 selected NOT_RUN**, 47 selectors.
The full selected runner exited 1 (36 PASS/11 FAIL before fixture follow-ups).
Fresh final checks supersede the peer fixture and Help-card drum expectations;
core/popup/mixer regressions were rerun after the locked-kit correction.
Unchanged checks retain their recorded coverage. The unselected package footer
is outside these 47 selectors. Full host suite and hardware tests NOT_RUN.

All nine remaining selectors also fail on pinned base `bb190749`; first failures
can differ. They remain FAIL, not waived or claimed pre-existing in every cause.

| Selector | Result | Base replay when selected |
|---|---|---|
| smc-action-source | FAIL | FAIL |
| smc-audit-contract | PASS | not replayed |
| smc-audit-core | PASS | not replayed |
| smc-ble-payload | PASS | not replayed |
| smc-boot-saved-roles | FAIL | FAIL |
| smc-buttons | PASS | not replayed |
| smc-clip-visibility | FAIL | FAIL |
| smc-drum-mutes | PASS | PASS |
| smc-drum-rs | PASS | PASS |
| smc-fader | PASS | not replayed |
| smc-feedback | PASS | not replayed |
| smc-filter | PASS | not replayed |
| smc-fx | PASS | not replayed |
| smc-fx-caption | PASS | not replayed |
| smc-fx-page-port | PASS | not replayed |
| smc-fx-peer-view | PASS | PASS |
| smc-fx-preset-preserve | FAIL | FAIL |
| smc-fx-two-page-strip | PASS | not replayed |
| smc-host-data7 | PASS | not replayed |
| smc-host-overflow-data7 | PASS | not replayed |
| smc-interaction-negative | PASS | not replayed |
| smc-link | PASS | not replayed |
| smc-lock | FAIL | FAIL |
| smc-lock-beat | PASS | not replayed |
| smc-mapping | PASS | not replayed |
| smc-mixer-edges | PASS | not replayed |
| smc-mixer-page | PASS | not replayed |
| smc-mode-page | PASS | not replayed |
| smc-multi-peer | FAIL | FAIL |
| smc-multi-peer-negative | PASS | not replayed |
| smc-part-select | PASS | PASS |
| smc-pause-loss | PASS | not replayed |
| smc-performance-abort | PASS | PASS |
| smc-pickup-acceptance | PASS | PASS |
| smc-popup-engine | PASS | not replayed |
| smc-protocol | PASS | not replayed |
| smc-queue | PASS | not replayed |
| smc-reachability | PASS | PASS |
| smc-reconnect-epoch | PASS | not replayed |
| smc-record-feedback | PASS | PASS |
| smc-select-mode | FAIL | FAIL |
| smc-sender-isolation | PASS | not replayed |
| smc-solo-overflow | FAIL | FAIL |
| smc-ui | FAIL | FAIL |
| smc-ui-bounds | PASS | not replayed |
| smc-ui-negative | PASS | PASS |
| smc-ui-popup | PASS | not replayed |

Final focused evidence:

- Popup 3718 checks PASS, including two-peer ASan/UBSan (leak checking disabled).
- Fourteen clean controls PASS and fourteen production mutations rejected at
  their intended assertions. Compile failures do not count as negative proof.
- Core 25 checks PASS: refusal/toggle messages, unlock page retention, cross-kit
  locked bank 2, bank-popup refresh and actual panel-HOME/remote-HOME combination.
- Engine 21 checks PASS: unlocked cache refresh on a real Chicago/Miami switch,
  deferred keyboard layout and stale keyboard page isolation.
- Held 303/drum steps and song lanes: 12 checks PASS. Mixer page 485 checks PASS.
- Peer 30 checks PASS; pickup 847069 plus 354 hint checks PASS.
- Drum R/S 125096 checks, 88 pad writes and 88 audio cases PASS; fingerprints
  remain 909=2b1a2725 and 808=a0999a81. Recording four real writes/nine projections PASS.
- SELECT lifecycle supplementary 3744 checks PASS. The standard SELECT test still
  fails its FX-preset assertion (current line 99, base line 92); it is not called PASS.
- Standard UI now reaches unchanged project-ownership assertion 278; lock reaches
  the same guard at 352. Their prior stale modal/keyboard/clip expectations were
  converted to explicit SMC context plus keyboard-invariance checks.

Other failures: action-source/solo-overflow obsolete extraction fields/helpers;
boot-saved-roles source-default anchor; fx-preset-preserve strict fixture warning;
multi-peer expects two peers under single-peer default; clip-visibility extraction
dependencies. No #788 implementation imported.

Descriptor/timing workload: one cold direct popup, 1000 warm draws and 10000 idle
SMC-poll/UI loops. Same host compiler/options/emitter; three alternating CPU-time
samples. This measures host CPU, not hardware response time. Ranges overlap.

| Work | Before | After |
|---|---:|---:|
| Cold descriptor passes | 230 | 3 |
| Warm passes/1000 draws | 230000 | 0 |
| Popup CPU median ms/draw | 0.2427 | 0.2379 |
| Idle CPU median ms/loop | 0.1425 | 0.1261 |

Run selectors with `X0X_TESTS="<selector list>" X0X_TEST_JOBS=3 X0X_HOST_LOAD=0 sh tests/run_tests.sh`.
