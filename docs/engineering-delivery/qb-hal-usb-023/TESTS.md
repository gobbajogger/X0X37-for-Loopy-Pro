# QB + N2–N4 test evidence

Base: `bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2` (`build/0.0.23`).

## Full host-suite run

The complete runner exited **1**: **333 PASS, 23 FAIL, 2 NOT_RUN**. Package availability was fixed before this run; both Elite and Pro packages were present. The environment supplied the verified toolchain/SDK, a writable generator cache and `ASAN_OPTIONS=detect_leaks=0`. Network-enabled execution avoids this executor's Node subprocess IPC error. LeakSanitizer cannot run under tracing; address/undefined sanitizers still run.

```sh
ASAN_OPTIONS=detect_leaks=0 X0X_TEST_JOBS=4 sh tests/run_tests.sh
```

Two earlier interrupted diagnostic runs are not counted as full-suite evidence: one had changing package availability during native builds, and one preceded the owner's N2–N4 scope addition.

### Resolved after the full run

- `midi-overflow-order` initially failed to compile because its extracted BLE function lacked the new Pitch Bend helper. Added that dependency conditionally so `--source-root` still supports the older base. All original assertions remain. The corrected test passes **264,909 checks** normally, with address/undefined sanitizers, and against the pristine base. No product code changed for this correction.
- `drum909` and `drum808` were NOT_RUN because pinned reference sources were missing. Recovered the exact public reference commits through read-only API archives and ran both direct comparison scripts: **drum909 PASS, zero failures; drum808 PASS, 436 cases**.
- Repeated the sequencer comparison with the actual pinned TB3PO reference, replacing its earlier internal missing-reference skip: **31,920/31,920 steps identical**, 396 seeds across four lengths.

The final check disposition is therefore **336 PASS, 22 FAIL, 0 remaining NOT_RUN among the runner's reported checks**. This combines the complete run with the explicit follow-up checks; it is **not** a claim of a single all-green full-suite run. Hardware measurements remain NOT_RUN separately.

Exact recovered reference commits:

- 9W9: `10cbe5c1e1c9e65b0818bb34644ab1a13092082c`
- 8W8: `94aa2716d4c2e5851fc7c5dbb4c17adfd133c997`
- TB3PO: `f49d81d05d1e29ae37728a08e30f193892be3bc9`

The exact drum pins were unavailable in the historical athousanddetails upstreams and were recovered from the charlesvestal forks at the same immutable commits. TB3PO provenance is charlesvestal. Archive CRC/path/link checks passed. Reference source and SDK/toolchain bytes are not committed.

## Remaining failures: reproduced on the base

| Selector | Candidate follow-up | Untouched base |
|---|---|---|
| `original-release-ui` | FAIL | FAIL |
| `original-build` | FAIL | FAIL |
| `ui-theme` | FAIL | FAIL |
| `editor-ota-dispatch` | FAIL | FAIL |
| `smc-select-mode` | FAIL | FAIL |
| `smc-action-source` | FAIL | FAIL |
| `smc-fx-preset-preserve` | FAIL | FAIL |
| `smc-queue` | FAIL | FAIL |
| `smc-solo-overflow` | FAIL | FAIL |
| `smc-link` | FAIL | FAIL |
| `smc-reconnect-epoch` | FAIL | FAIL |
| `smc-boot-saved-roles` | FAIL | FAIL |
| `smc-lock` | FAIL | FAIL |
| `smc-multi-peer` | FAIL | FAIL |
| `ui-peer-names` | FAIL | FAIL |
| `smc-ui` | FAIL | FAIL |
| `smc-clip-visibility` | FAIL | FAIL |
| `builtin-break-output` | FAIL | FAIL |
| `sound-matrix` | FAIL | FAIL |
| `chicago-presentation` | FAIL | FAIL |
| `ui-glyph-polish` | FAIL | FAIL |
| `scenario-plock` | FAIL | FAIL |

Each remaining failure was actually run on the untouched base. `smc-lock` fails in both trees at different assertions, so identical failure signatures are not claimed for that selector. Other recorded cases match their baseline diagnostics. These failures require changes in other owners' areas or existing test fixtures, not weakened assertions in this PR. Examples include stale SMC types/fields, stale ADSR function signatures in sound-matrix/glyph tests, the missing default-Bluetooth assignment anchor, and parameter-lock lanes expected 1 versus observed 2.

The unregistered `smc_peer_names_test` was also run separately and remains FAIL on base and candidate: duplicate `plat_ble_name` and missing `bt_peer_text`. Its fixture now imports `plat.h` before extracted aggregation code so this PR does not add undefined constant errors. No UI implementation or assertion was removed to hide the pre-existing failure. The registered `ui-peer-names` failure is included above.

## Targeted regressions

All nine new selectors PASS: `irq-state`, `outer-pack`, `ble-peer-name-bounds`, `usb-sysex-parser`, `midi-ring-displace`, `midi-ingress-busy`, `midi-mq-pb-overflow`, `midi-out-mq-pb-overflow`, `ble-out-pb-overflow`.

The strengthened existing `usb-sxq-sticky-apc` also PASSes: actual EP1 retry/drain, both OTA modes, full and busy queues, UDI/introduction complete bytes and exact-once framing, twelve/six-slot boundaries and one-slot-short refusal. A runtime scratch mutation that incorrectly requires thirteen slots for the twelve-packet identity is rejected.

Observed failing → passing regressions cover nested IRQ restore, loader-independent IRQ composition, malformed SysEx cancellation, all three missing Pitch Bend overflow paths, and missing link-end preservation. Characterization checks cover already-present Note Off routing and APC retry; four scratch mutations of those paths fail at runtime assertions. No duplicate production port is claimed for already-present N4.

USB/ring, Pitch Bend, busy-ingress, BLE name bounds and APC retry tests PASS address/undefined sanitizers with leak detection disabled. The existing ordered-overflow test's sanitizer run also PASSes all 264,909 checks. Full-ring refusal remains explicit when no disposable event exists.

## Native and hardware boundaries

Elite and Pro standard/extra-warning native builds PASS with genuine JieLi clang 4.0.1, SDK V1.2.1 and `X0X_DEV_PADCAL=0`. Both retain **2352 B free static RAM** and **8536 B free pool RAM**. Warning counts remain **1 standard / 22 extra**, unchanged from the base. See `verification.json` for source hashes, input pins and measured summaries.

Independent source review has no remaining actionable findings after the loader correction and preservation of the original exact-space retry check. Simplification reviews found no useful behavior-preserving changes. The review was one independent native review, not a full multi-persona/cross-model pipeline.

BLE runtime init timing, TIMER5 frame watch, and the ten-minute real-device play/USB MIDI soak are **NOT_RUN**. No flashing or installation occurred. The brief remains pending acceptance because the broader suite is not green and hardware evidence is absent.
