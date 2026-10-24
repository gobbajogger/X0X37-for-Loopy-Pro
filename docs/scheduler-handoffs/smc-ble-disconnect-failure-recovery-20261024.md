# BLE HCI disconnection failure handling — smc-ble-disconnect-failure-recovery-20261024-r1

## Immutable producer input
- Repository: `gobbajogger/X0X37-for-Loopy-Pro`; owned PR #50 and branch `lane/smc-ble-mailbox-backpressure-20261023-run3`.
- Consumed full commit `b6966675df9e05b4fd6652c35bb4fd115e35c044`, **complete root tree** `ec9b12682e1b6d9216fa22cf3df18a7cc0940aef`, distinct compiled `firmware/` subtree `1305ae9823d65a8952f2537be98bf11c6876cbfc`.
- BLE source before change blob `c9144a0885cbc38c50584d8beab73da48ea9dc8b`. New BLE blob `70dc093d1613d684f5a67be01acc118eded1a6f8`; registered runner blob `09ff8e6c18a2c1464309cce44ad38c423c64d10a`; Git native BLE test blob `da7459dcc724ab823caad68a6c2ab647014bb433`. These pre-created Git blobs do **not** by themselves assert a branch was pushed; verify the actual PR HEAD and Git tree.

## Observed source defect and correction
The existing HCI `0x0F` Command Status handler clears `cmd_wait` but parks `cmd_op=OP_DISC` even when status is nonzero. A later radio-off poll refuses to retry. The `0x05` Disconnection Complete handler ignores its status byte and can falsely mark a live link disconnected, clearing queued Note Off and partial SysEx. No real physical incident is reported.

Handle a rejected *matching* Disconnect command/status without resetting the live MIDI epoch; permit a subsequent normal radio-off retry. Only successful HCI Disconnection Complete changes link state or clears the outbound queues. Preserve unrelated-handle, truncated, accepted-command and advertising semantics.

## Actually executed local native checks
- `./replay.sh`: **exit 0**. GCC and Clang each in plain, UBSan and ASan+UBSan: six before compilations exit 0, six before runs exit **1 expected** (9 of 19 assertions fail); six after compilations exit 0, six after runs exit **0** (19/19 PASS). Result log SHA256 before `e5a42e3b95232011d5392941e9a88a49f33e0ae92368822d2fc1006b29f9eb35`, after `107d7351b1e1db0e24798031b8d3acffd5fcda3422558a353c43951b1736954a`.
- `gcc -std=c99 -O2 -Wall -Wextra -Werror -I. -fsyntax-only ble_disconnect_status_test.c`: **exit 0**.
- `clang -std=c99 -O2 -Wall -Wextra -Werror -I. -fsyntax-only ble_disconnect_status_test.c`: **exit 0**.
- **Limitations**: replay uses manually lifted source-window event/poll branches, not the complete linked BLE host. Syntax checks use a local declaration-only header. The registered `ble-disconnect-status` test compiles the *real* BLE implementation when run from the full Git checkout, but was **NOT RUN against this exact full source**. Full native integration, target Elite/Pro/controller builds, independent QA and physical device validation remain PENDING/UNKNOWN.

## Explicit consumer actions
Independent QA: exact committed HEAD, `X0X_TEST_JOBS=1 sh tests/run_tests.sh ble-disconnect-status ble-link-epoch-reset ble-hci-completed ble-acl-zero-credit ble-sysex-bounds ble` plus relevant ATT/mailbox and SMC tests, inspect all failed HCI event status and queue transitions. Hourly Build: compile exact HEAD Elite, Pro and local matched controller BUILD_ONLY without waiting for QA or a merger. Sole Safe Merge: later reconcile #89→#94→#50 in an isolated integration lane after suitable QA. **Do not block the separate first #103 candidate solely for this optional SMC source extension**. Catalog and Publisher must preserve all public installation safeguards.

**SOURCE-COMPLETE WITH SCOPED PRODUCER TESTS; READY FOR INDEPENDENT REVIEW, NOT QA APPROVED; BUILD PENDING; DEVICE NOT TESTED; NOT INSTALLABLE.**
