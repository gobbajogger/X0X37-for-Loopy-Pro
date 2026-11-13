# BLE L2CAP zero-credit admission — SMC Transport producer evidence (2026-10-24)

**Repository/branch:** `gobbajogger/X0X37-for-Loopy-Pro` / `lane/smc-ble-mailbox-backpressure-20261023-run3`. Consumed source commit `faeceb982f891a1f7b557839ad2b5382e0bcb6af`, full root tree `25d0883d08488f793fb915c77f0ed942a6d546c3`, **distinct** compiled firmware subtree `e72b1fea89c755113880172c519d584c2ae9b234`. Source BLE C blob `7e8ad4a268856edd66cf90ebbaa8e38a9ad996df`. The new commit and compiled subtree must be read from live Git after remote update, not inferred from this consumed tree.

## Reproduced defect and bounded correction

`l2cap_send()` called the core-to-controller HCI ACL send callback when `h->credits==0`. An accepted connection-parameter signalling request then set `cpu_state=1` even though there was no controller buffer credit. This is a code-observed HCI flow-control hazard, **not** evidence of a measured device failure. Corrected admission to refuse L2CAP sends when `n>BLEH_MTU` **or** `h->credits==0`; rejected requests do not advance connection-update state. The guard covers ATT/SIG/SMP common path but leaves explicit ATT response retry behavior as previously implemented.

Added `tests/host/ble_acl_zero_credit_test.c` (18 checks: zero credits, one-credit commit/no duplication, mailbox rejection/retry, disconnected state) and registered `ble-acl-zero-credit` in `tests/run_tests.sh`. This is source producer scope; PR #89 SysEx and #94 ACL receive fixes remain separate unmerged dependencies.

## Actual fresh native tests, not device tests

Linux x86_64: GCC and Clang with plain, UBSan, ASan+UBSan (six configurations per variant). Each command compiled the **source-matched `l2cap_send` and `bleh_conn_update` function fragments** from verified live PR #50, injected with `-include`, together with the **delivered regression test bytes**, and used a local stub header for that narrow replay. Command template: `${CC} -std=c99 -O1 -g -Wall -Wextra -Werror ${SANITIZER_FLAGS} -include credit_source_${VARIANT}.c -I<local_native_replay_dir> ble_acl_zero_credit_test.c -o <local_executable>` then run the executable synchronously.

All six **baseline** compilations exit `0`; executions exit `1` with **4/18 expected failures each**. All six **patched** compilations exit `0`; executions exit `0` with **18/18 PASS each**. Test SHA256 `970d602bd920894d3bc536948d13f112b3c7ddc097f4c08ad2c6106ebfe27cbe`. Before/after fragment SHA256 `bd6b92bf3867a1d07def5e1cb8b873ef696a82fedef0784c07bde8afe499ef62` / `779918aaa9ca9d81e9a7204af9309b8ad8d2272fb2cf95e891be7fe132b24f7a`. Local logs and per-command exits recorded in `final_scoped_results.json` (SHA256 `009341e9f52dba3f69e7009886a7ace1c76bf77da4f83196cd3ae6f19c85f377`).

**Not executed:** the new registered host test against the complete linked BLE source, a full exact-remote checkout, combined #50/#89/#94 regressions, Elite/Pro/ctl builds, Independent QA, hardware, preservation/OTA, licensing/hosting/HTTP. A sanitizer source-fragment PASS is NOT full-source QA, firmware BUILD PASS, or installability.

## Next owners and gates

**Hourly Build:** compile the new pushed PR #50 exact full source/compiled subtree for Elite+Pro+controller **BUILD_ONLY immediately**; do not wait for QA, merge, Pages, controller hosting, owner hardware PASS, or separate per-build consent.

**Independent QA:** consume the exact branch HEAD/root, run the linked `ble-acl-zero-credit` regression and affected mailbox/ATT/SysEx/ACL/Note Off/reconnect suites with malformed, zero-credit and boundary negatives. Record factual REVIEWED/NEEDS CHANGES disposition with exact SHA.

**Sole firmware Safe Merge:** later reconcile reviewed #50 with #89 → #94 without flattening other owners' work. No protected `nightly`/production `main` mutation here.

**Disposition:** SOURCE_SCOPED_PRODUCER_TEST_PASS; INDEPENDENT_QA_AND_TARGET_BUILD_PENDING; NO_PUBLIC_INSTALLATION; DEVICE_NOT_TESTED.
