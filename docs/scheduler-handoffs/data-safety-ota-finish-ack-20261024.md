# Data Safety — OTA FINISH ACK send-failure false completion (2026-10-24)

Status: **SOURCE REPAIR VERIFIED IN PRODUCER-SCOPED SOURCE-WINDOW HOST REGRESSION / FULL-ROOT INDEPENDENT QA PENDING / NO INSTALLABLE NIGHTLY**.

Source owner: LoopyX0X37 Data Safety. This consumes Bugfinder BF-018, exact candidate #103 `94c093e01680d69e22b64a176b3dc27b7fffdd7f` with full root tree `c95bf58439d4b5fc3a438dc91cbebc97f7ecae2b`, firmware subtree `d3b67bf127121acdbb2d056890b8c6987ffb363e`, OTA CLI `tools/fm1_install.py` Git blob `5b60164f056352f763b53053fa5635fb036821fc`. Bugfinder evidence: PR #104 comment `6374997866`, cross-reference PR #103 comment `6375005584`, revision `bugfinder-pr103-ota-finish-ack-send-20261024-r1`.

## Reproduced original defect

In `Updater.serve()`, the FINISH_CHECK and FINISH_WRITE response path called `link.send(response(...))` but discarded its Boolean result, unlike ordinary data read responses. On a failed MIDI send, the method returned `finished=True` or continued to the next event despite never successfully transmitting a finish ACK. There was no attributable physical device observation or confirmed corrupted device state.

The original exact 1,229-character `serve()` source window (FNV1a64 `be3363ac1dc37ad1`) was reverified against the current Git file. The one-condition correction fails closed on an unsuccessful finish send: `if not link.send(...): return served, False, True`; corrected source window FNV1a64 `fad11c7cbe116cd5`. Data-response and successful-finish semantics remain unchanged. Existing `Updater.check`/`write` now receive a non-finished result and retain their established error/recovery path. No hardware flash or OTA command was executed.

## Actual executed host checks

Commands from `/mnt/data/LoopyX0X37-DataSafety-OTA-FinishAck-20261024`:

- `python3 -B fm1_install_finish_ack_test.py fm1_install_serve_before.py`: **exit 1 EXPECTED FAIL BEFORE**, 9 tests with 3 failures (rejected FINISH_CHECK, rejected FINISH_WRITE, rejected nonmatching finish). Log SHA256 `a1cc45eb9e1c27998db13ec0c915630157682c20fa63f384ede59f323384b2fe`.
- `python3 -B fm1_install_finish_ack_test.py fm1_install_serve_after.py`: **exit 0**, 9/9 PASS. Log SHA256 `34638e03dcd2dc2ad3ea48f5ad2ded8cded05202ff44a247cc9bc7c1cbf8754b`.
- `python3 -O -B fm1_install_finish_ack_test.py fm1_install_serve_after.py`: **exit 0**, 9/9 PASS, identical log SHA256.
- `python3 -m py_compile fm1_install_serve_after.py fm1_install_finish_ack_test.py`: exit 0.

New regression `tests/fm1_install_finish_ack_test.py` Git blob `df0b4cd1cc8400fa26e5d3d36f5f66116145f799`, locally verified by `git hash-object`. The test mocks only wire request parsing, response encoding, MIDI send and simulated loss; runs actual full `Updater.serve` from the complete project when invoked with no fixture path. `tests/run_tests.sh` registers this plus the previous slot-migration matching regression. **Tests in this run used the byte-identical serve source-window fixture, not a reconstructed entire current remote root and not a physical device.**

## Exact change and next ownership

On OWN isolated PR #105 stacked upon Safe Merge's first NIGHTLY candidate #103, this adds `tools/fm1_install.py` patched Git blob `5498084bed95985f091f9525a936e4c4bef154af`, effective test and dispatcher registration. It also preserves preceding maximum matching repair and evidence (source `tools/x0x_slots.py` blob `78b041ab3e0c736517dbf2f509c9776c7c632f10`). No `firmware/` compiled inputs are changed.

**Independent QA:** rerun the real registered tests on exact full source; exercise loader and running-firmware simulated install flow, failed finish send/OTA interruption and backup/readback/restore, wrong-model/CRC/controller negatives. **Hourly Build:** BUILD_ONLY from the exact source immediately and check full compiler input equivalence or rebuild real Elite/Pro/controller private images; prior package evidence is not automatic installation approval. **Sole Safe Merge:** review PR #105 and reconcile into source PR #103 through authorized integration only. **Catalog and sole Publisher:** maintain known-safe live versions, hold public candidate until full independent prehardware safety/model/controller/source/license/immutable-HTTP gates; no per-build physical PASS required for NIGHTLY, but none is claimed.

Owner hardware evidence UNKNOWN; no private data, firmware, controller, SDK, DSP, protected refs or Pages changed.
