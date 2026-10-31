# SMC and mixer test results

Final-source aggregate: **34 PASS / 10 FAIL / 0 selected NOT_RUN**, 44 SMC selectors.
This is not an all-pass or merge-ready claim. The 40-selector runner failed;
final individual reruns supersede fixtures changed while it was running, and
four Pillow-guarded selectors ran separately. Their commands all exited 0.
The runner's unselected package-test footer is outside these 44 selectors.
Full host suite NOT_RUN. No physical device testing.

Nine remaining failing selectors also fail on the unmodified pinned base;
their current failures need not have the same cause. `smc-ui-negative` additionally
has a stale mutation anchor after replacing keyboard context with SMC context.
Its first two mutant runs also encounter the unrelated existing UI pixel failure,
so they are not claimed as proof of mutation sensitivity.

| Selector | Final | Base replay when selected |
|---|---|---|
| smc-action-source | FAIL | FAIL |
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
| smc-ui-negative | FAIL | PASS |
| smc-ui-popup | PASS | not replayed |

Focused additional verification:

- Popup/queue/dispatcher: 3723 checks PASS; same test under ASan/UBSan with
  two-peer configuration PASS. LeakSanitizer disabled because of sandbox ptrace.
- Deferred engine switch and stale keyboard page: 14 checks PASS.
- Mixer edges: 1281 checks; mixer page: 477; mode page: 1308, all PASS.
- Pickup: 847069 assertions plus 354 footer hint checks PASS.
- Drum R/S: 125096 checks, 88 real pad writes and 88 audio cases PASS.
- Recording: four real pattern writes, nine LED projections PASS.
- Peer context: 18 exact-source assertions PASS.

Remaining failures: select-mode (LED assertion), action-source and solo-overflow
(obsolete extracted FX fields/helpers), boot-saved-roles (default source anchor),
fx-preset-preserve (strict fixture warning), lock (legacy keyboard-part assertion),
multi-peer (fixture expects two peers in single-peer default), ui (existing pixel
assertion), clip-visibility (extraction dependencies), ui-negative (stale mutation
anchor). #788 behavior was not changed or imported.

Reproduce the selected run with `X0X_TESTS` set to the selector column and
`X0X_TEST_JOBS=3 X0X_HOST_LOAD=0 sh tests/run_tests.sh`; run the new engine
regression with `sh tests/host/smc_popup_engine_test.sh`.
