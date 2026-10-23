# Integration lane: SMC popup + fastmath + Stage4 selector + MIDI clock BF-005

X0X37-Role: Sole Safe Merge source integrator
X0X37-Stage: WIP / HOST-PARTIALLY-CHECKED / EXACT-SOURCE-DEV-BUILD-PENDING / NO-GO
X0X37-Consumer: Independent QA; Hourly Build
X0X37-Next: Verify full integrated source SHA; run full host/negative checks and exact both-model native DEV+ctl.img builds; no publication before remaining software and owner gates.

## Identity and intended meaning

This is **one fresh isolated integration lane** branched from `main` c9718ae833eb64a5191c350922593df8a36017c9, selectively importing source/test blobs from:
- SMC popup #13 + follow-up #19, fastmath UB #28 and Stage 4 validation fixes #37: verified producer head `a67023a19c158fc24ae855ae6b848c870f705a91`; #19 `2b310479c73c1c7dad8feb64c94109fd757c6110`, #28 `832f8dbb0655cba8b9fd512be34c1e89ace185aa`.
- BF-005 USB-to-BLE MIDI clock handover #35: verified producer head `32364a9524dd1ac6857d46b409af66029805d4ce`.

**Do not merge parent PR history**: source-only pick from exact existing blob objects, not PR #37's `evidence/` binaries. No .fwsc, ctl.img, vendor SDK, samples, secrets, history rewrite, Pages or public firmware included. Existing archived firmware and Nightly left unchanged.

Imported source/test paths: `firmware/src/app/smc_mixer.c`, `firmware/src/dsp/fastmath.h`, `firmware/src/seq/sequencer.c`, `tests/host/{run_fastmath_ubsan.sh,smc_drum_rs_test.c,smc_feedback_test.py,smc_reachability_test.c,smc_ui_popup_test.c,smc_ui_popup_test.sh,bugfinder_clock_handover_test.c}`, `tests/run_tests.sh`, `tools/gen_builtin_break.c`. `tests/run_tests.sh` additionally wires BF-005 into fail-closed `clk-handover` selector without changing success thresholds.

## Actual native checks and confidence

Linux x86-64, GCC and Clang, Python 3.13, Node v22; repository shell GitHub DNS unavailable, but GitHub connected immutable blob read-back and isolated commit delivery work.
Locally checked **offline projection** commit `c368fe14d020306aaf08d5ecc35216b554eb21aa` (not full current GitHub tree), whose affected sequencer/pattern/TB3PO, SMC mixer/fastmath, popup/SMC host files were verified byte-for-byte against producer blobs. The BF-005 patch matches exact blob `7345cab17c29ba3c26f58d1b2c042b05c1b55ef4` and imported C regression `cf877263dc3ba8e987bdfa59a09ac7c1cf603b33`.

- `gcc -O1 -g -fsanitize=undefined -fno-sanitize-recover=all -Wall -Wextra -Werror ...` BF-005 regression **exit 0**, two positive checks.
- Equivalent Clang UBSan BF-005 compile/run **exit 0**, two positive checks.
- `X0X_TEST_JOBS=1 sh tests/run_tests.sh clk-handover` with inserted equivalent selector: **exit 0**; `sh -n` PASS.
- Seven individually selected tests: `clk-lock`, `fastmath-ubsan`, `ble-midi-receive`, `smc-reachability`, `smc-ui-popup`, `smc-filter`: explicitly observed native command **exit 0** each. `smc-drum-mutes` logged `all tests passed`, but its parent batch **timed out before capturing shell exit**; it is **UNVERIFIED** on this run, not PASS.
- A combined multi-selector run **timed out after four explicit ok lines**; **combined test PASS not claimed**.
- `smk-package` skipped for lack of prepared build environment in host selection; not included in tested selector and not claimed PASS.
- This native checkout does not reproduce the **exact full newly integrated Git tree** (test runner and other source/docs differ), so every local PASS is a module-scoped diagnostic. Exact combined-tree regression and independent QA are STILL REQUIRED.

## Known open defects and exit gates

Known unresolved BF-004 MIDI NoteOff source ownership; BF-006 808 negative signed shift; BF-007 older 808/909 bass-drum PAN motion migration; BLE/SMC queue ordering and installer backup/OTA/error negative tests. Source changes for these are owned by the relevant specialist lanes; no duplicate edits made here.
Formal exact-SHA Elite/Pro DEV+matching CTL build, SDK/DSP pin verification, sample-slot backup/readback, model+package CRC/SHA, RAM/pool/flash budgets and independent second QA are **NOT RUN / NO-GO**. Historical PR28 offline firmware hashes are **not attributed to this integration SHA**.
Owner hardware PASS is **not** required for branch push, but production eligibility requires it with per-model same-byte evidence and explicit approval. The nightly preview catalog remains unapproved and non-installable until sole nightly publisher handles a specifically authorized checked candidate. Never update either `gh-pages` or `main` from this integration lane.

**Git role handoff**: this source-only WIP branch is the single reviewable integration commit, not a deployment or verified firmware.
