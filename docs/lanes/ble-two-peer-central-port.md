# Enable the already integrated two-peer SMC BLE central

## Owner task and exact starting point

Task D, 2026-10-30. Target: `integration/all-in-one`, initial requested base `ea859a8639f7448fb53196f6feb8c8c761b114d9`. Reuse FMB's `lane/mb-aio-port-650-687@ae0a62e3774545d4dc7544d481b0d9da10b26055` and PR #687; do not redo #650/#651/#638 or Chicago/Miami audio. Preserve #700's handle-zero, saved Bluetooth-role and command-credit guards.

The live integration head at intake is `c54d9f9b4262709bb4ea5852ae4ed008fa100996`. GitHub confirms FMB's #687 BLE source was already ported at `180710099d42774dce7f7d59003370f4cc727b8a` and integrated at `7e5be72e91b1cf469b05df05716e498c108b1a1b`; `ble_midi_central.c` has identical blob `791cf4070b60bc0591ec9b6ed48c6966faa8d7c0` in FMB and integration. Start the owned branch from the **live integrated successor**, rather than reopening all 91 diverged FMB commits. This preserves the FMB implementation already integrated while keeping the PR reviewable.

## Remaining material defect

The integrated pair scheduler is **disabled in normal firmware**: `BLEC_TWO_PEER` defaults to 0, locked controller configuration has one LE link, and `plat_ble_smc_send_slot` rejects the second peer. The host runner compiles the pair only using a test-only `-DBLEC_TWO_PEER=1`, and skips the controller heap test. The generated device remains single-peer despite the merged source.

## Scoped implementation and acceptance

1. Enable the established two-peer path by default, without replacing the production #700 command-retry, handle-zero, STOP or saved-role logic. Adapt controller configuration to the proven #687 two-link footprint (2 LE links, 3 ACL TX credits, multilink enabled) **together** with the maintained locked-settings contract. Confirm heap allocation and no unbounded RAM increase.
2. Remove the inappropriate single-peer runtime gate on SMC output slot 1. Keep the single-peer fallback explicitly testable where applicable; ensure native two-peer tests run under the exact default configuration and no longer silently report NOT_RUN.
3. Add/adjust focused default-build regression coverage for two link paths, controller configuration and active slot 1. Preserve existing source isolation and old/new session epoch tests.
4. Display proposed before/after UI mockup of connected-peer status (proposed, not hardware screenshot), if the second connected device changes visible UI state; validate source-level UI naming semantics.
5. Attempt full `X0X_TEST_JOBS=8 sh tests/run_tests.sh`, `BOARD=smk37 MODEL=elite sh build.sh`, `BOARD=smk37 MODEL=pro sh build.sh` using exact checked candidate and SDK V1.2.1; retain true exit codes, PASS/FAIL/NOT_RUN counts, logs and artifact hashes. No SDK V1.2.0 substitution. Tests that cannot run are NOT_RUN, not PASS.
6. Push changes/test evidence on `chat/enable-two-peer-ble-central` using connected gobbajogger default GitHub identity. Draft PR into `integration/all-in-one`; no Actions, `.github/`, zipballs, force-push, release, bootloader, device operations, binaries or unrelated engines.

## Verification and handoff

Only the exact pushed branch SHA with remote readback is delivered. A mockup is a design reference, not a device capture. Hardware two-device acceptance, latency and RF robustness remain separate from host pass. Record real tested results in PR; if target builds are unavailable, mark the qualification stage incomplete instead of claiming a pass.
