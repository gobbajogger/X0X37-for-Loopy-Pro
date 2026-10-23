# Safe Merge integration: SMC host isolation + clock ownership — 2026-10-23

**Owner:** Sole source integrator. **Stage:** WIP / HOST-SCOPED / FULL EXACT-TREE BUILD PENDING / NO-GO. **Next consumers:** Independent QA, Hourly Build and Bugfinder. DEVICE NOT TESTED. This is not a firmware package, a production/NIGHTLY picker record, a site preview or a release.

## Immutable inputs and scope

- Production main baseline: `c9718ae833eb64a5191c350922593df8a36017c9`; prior Safe Merge integration parent: `233d310b2281ec4ed1acc8e98c3d573ae44fff57` (original source tree `f4912f97e4052eaeb344f8f24014a4b9dd69a47e`).
- **SMC User-mode malformed payload guard**, producer PR #40: `494cef524395af2a4614e35fafcea3cafece7811`. Adapted `smc_midi_host` into already-integrated SMC popup source, preserving the remaining mixer implementation. Integration `smc_mixer.c` Git blob `56940dd72e1893cca79725c50525ed47107bc53a`; adopted existing producer's test `tests/host/smc_queue_test.py` blob `37bc78444e7d5bd6d3d73180c3dc7dec846d8bd5`.
- **Internal MIDI clock mode ownership reset**, PR #41: `e0582ccb685f1adb0dc42e8ce1282b5394493982`. Combined its single `clk_lock=0` fix with the prior BF-005 source-disconnect fix without losing either. New sequencer blob `3a2d9315f8cb3f450833b4b639d03e581e61500f`; official producer regression `tests/host/clk_source_owner_reset_test.c` blob `7e62fa075bfe77b11447132e599c5b4f1c846f8a`.
- **Host MIDI physical-source isolation**, PR #43: `b949e7af42a00084eb3b1f695073d3258f8fcd8d`. Full `engine.c` blob `fd11e81d77a0aec1afb395a2ea5b070483d87ec4` is a direct fast-forward content graft: original integration file exactly matched producer base. New `smc_sender_isolation_test.py` blob `076c3bd3c439b7d366574807264e8e5aeb0c7e98`.
- Preserve prior source fixes, `tests/run_tests.sh` Stage4 fail-closed checks, existing SMC popup and fastmath fixes, BF-005 test and all non-touched paths.
- PR #42 `71261795cfd1f71dd36d7396bd658fd09618c41e` is intentionally **not integrated** due independently reproduced post-replace directory-sync negative path: `tools/add_ctl.py` returns error after `os.replace` already overwrote existing output. Existing input cannot be claimed preserved on *every* failure; consumer should reconcile guarantee with Data Safety owner. Existing source path remains unchanged.

## Executed native source-scoped checks, exits and limits

Native Linux x86-64 with GCC, Clang, Python and Node. The historical `7537a46` Project bootstrap was extracted; its full Git checkout is **not** the actual integration HEAD, but affected main source objects were checked by immutable Git blob identities: original `sequencer.c` `83b03f7`, `engine.c` `555480d`, `add_ctl.py` `6618e9e`, `smc_queue_test.py` `e03ba06`. Local patched `sequencer.c` and `engine.c` match the final integration blobs exactly; local SMC popup surroundings are historical although `smc_midi_host` function matches the merged patch.

- New INTERNAL->EXTERNAL/AUTO owner release: GCC UBSan exit 0 and Clang UBSan exit 0; each two transitions. Existing MID-15 `clk_lock_test.c` GCC/Clang UBSan exits 0.
- BF-005 USB-disconnect queued-clock regression: GCC/Clang UBSan exits 0, both zero-old-pending and 3 pending-old-tick cases.
- TRS/unknown vs USB/peripheral-BLE SMC host source-routing: 9 locally reconstructed extracted-block assertions, GCC exit 0, Clang UBSan exit 0.
- Malformed SMC User data1/data2 and disabled-User rejection plus BLE/host/MMC queue wrap/reset: `python3 tests/host/smc_queue_test.py` local augmented test exit 0, 24 full queue interleavings; official producer test blob included in Git integration, but that exact test file was **not independently executed** as byte-identical local copy.
- Independent selected historical-source host-runner aggregate timed out (exit unavailable), after recording eight `ok` results: `clk-lock, smc-protocol, smc-fader, smc-buttons, smc-queue, smc-link, ble-midi-receive, smc-reachability`. Remaining `smc-filter` and `smc-drum-mutes` separately run exit 0. Aggregate timeout is NOT overall PASS.
- PR #42 `add_ctl.py` exact producer blob checked locally; seven atomic positive/negative assertions exit 0, but a novel second-directory-fsync injected failure reproduced `main() exit=1` while output already equals the replacement bytes. Excluded from this integration on safety grounds.
- Owner private compiler ZIP SHA256 `ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19`; safely extracted 705 paths/44 symlinks, ran pi32v2 Clang 4.0.1 (compiler SHA256 `c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f`). Private input kit SHA256 `2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805`; 11 pinned SDK V1.2.1 files verified. No private inputs are part of this tree or firmware distribution.

## Mandatory remaining gates

1. Independent QA: fetch **this actual combined Git branch**, rerun the committed producer test blobs and broader SMC BLE/USB/TRS/clock/phase/808/909/direct-forwarded routing suites, negative/no-data/reconnect/error paths, including BF-004/006/007 disposition. Review queue rollover, user-mode and feedback interactions. Full same-SHA source reconstruction remains needed; native shell `git ls-remote https://github.com/gobbajogger/X0X37-for-Loopy-Pro.git` returned DNS exit 128 while connector Git API remains functional.
2. Hourly Build: exact combined-source Elite+Pro DEV, matching `ctl.img`, pinned toolchain/SDK/all four DSP refs, model/product/CRC/SHA256, flash/RAM/pool, real model-exclusion, OTA and backup/readback/samples and browser/installer negatives. Historical/offline prior builds are NOT exact combined-source certificates.
3. Production catalogs must filter all unproven firmware and preserve old archives; NIGHTLY remains read-only until candidate-specific approval and non-hardware gates by its sole publisher. Physical test PASS and production publication need separate attributable owner evidence; no auto promotion. Do not run GitHub Actions, force push, flash hardware, or write production/nightly main/gh-pages.

**Remote handoff:** Re-read this document on the integration branch and verify the head ref and changed-file comparison against `233d310b2281ec4ed1acc8e98c3d573ae44fff57`. The new HEAD is the sole source integration ledger for this cycle; no user hardware check or full build status is inferred.
