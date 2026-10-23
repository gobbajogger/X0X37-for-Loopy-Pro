# SMC Transport — BLE-MIDI peripheral SysEx length preflight (2026-10-23)

**Outcome:** SOURCE-MATCHED NATIVE HOST CHECKED / FULL EXACT-TREE QA PENDING / DEVICE NOT TESTED / NOT INSTALLABLE.

Firmware `nightly` base commit `58b5b45a2dd90581de7a674c331ca5cbe8a67c7e`, tree `71b759dd206d844cceed9e40ae68966142720e9c`. Original peripheral source Git blob `ece7f0cb3c2ba878dc4a351fcb864fcb2da5b8f7` and header `488977fab5e6300e8e2f7ad96cb834fe62c490f1` matched the native Linux fixture **byte-for-byte**. The historical local repository runner is not the full new Git tree.

## Reproduced defect

`bleh_sysex_send()` examined `m[len - 1]` BEFORE ensuring `len - 2 <= BLEH_SYSEX_MAX` (64). An overreported SysEx length, including `UINT32_MAX`, therefore dereferenced outside a caller's message rather than returning -1. With an actual 2-byte `F0 F7` buffer and Clang ASan+UBSan, the unchanged firmware source crashed in `ble_midi_host.c:506` **exit 134**, ASan SEGV/invalid READ. Baseline stderr SHA256: `c36fe5c6ab15b4d456cc0548cca1c838a84f78b07de045bc948c37c0e59543ad`.

## Minimal repair

Reject null buffer, length <2 and length >66 **before** either buffer read. Retain all valid data-byte checks, bounded copy, rejection return values and busy/transport behavior. No BLE central transport, HCI mailbox, SMC mixer, clock, flash, controller, installer, data or public website changes.

## Actual local native checks

- Clang ASan+UBSan negative on original exact-source blob: exit **134**, expected reproduced source failure.
- Patched `clang` and `gcc`, each with plain / UBSan / ASan+UBSan: six independent compile and run pairs, all **exit 0**. Each run reports `PASS: oversized/null/busy/invalid/max BLE SysEx and preserved Note Off` (run-log SHA256: `a1e62256b4d1c47e15549d13686b94042c2e928a0ef1cf9579f2b5f604f0b5f2`).
- Native `X0X_TEST_JOBS=1 sh tests/run_tests.sh ble-sysex-bounds ble` on isolated historical runner with the new registration: **exit 0**, both named tests OK (runner-log SHA256 `44eb17ee5d41a75e8911913a4eb167e0236faff9bc907d74e157bad128736382`). Original registered `ble` alone also exit 0.
- No firmware-target/compiler/Elite/Pro/controller build and **no hardware PASS** claimed. Full exact-current Git-tree runner not obtained.

## Consumers

**Independent QA:** Independently read new full remote commit and file blobs and run `X0X_TEST_JOBS=1 sh tests/run_tests.sh ble-sysex-bounds ble` from complete PR checkout. Recheck malformed/oversize SysEx, boundary 66 bytes, Note Off ordering and the different BLE mailbox backpressure PR #50 (which changes the same C file); resolve overlaps without assuming this draft is merged. **Sole Safe Merge** owns integration to firmware `nightly`. Hourly Build owns later pinned Elite/Pro + controller qualification. No public binaries; GPL/MPL provenance retained.
