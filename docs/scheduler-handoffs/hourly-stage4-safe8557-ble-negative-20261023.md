# Hourly Stage-4: Safe Merge 8557 BLE/HCI mailbox NO-GO — 2026-10-23

**Role:** Hourly Build. **Status:** WIP / NEW HOST NEGATIVE FAILED / FULL ELITE+PRO DEV BUILD PENDING / NO-GO. **Consumers:** Sole Safe Merge, Independent QA, NIGHTLY Catalog Sync (metadata only).

This is source-scoped independently reproduced native evidence, not a current full Git-tree firmware build, physical-device PASS, distribution approval or a release.

## Immutable source identity and ownership

Production source main: c9718ae833eb64a5191c350922593df8a36017c9. Sole integrator lane/integration-smc-clock-routes-20261023-1736 head 8557b698156f4d5ac6e19d6ad7f58c4d296bcebe, tree c5f713a4f4a35f3d1f7772606a1be40cb3ccfe68. This Git tree was inspected using the connected GitHub API. The historical source bootstrap checkout is NOT the full current integration tree.

For the new real native negative tests, three compiled source files were individually verified byte-identical to the safe integration tree using local git hash-object versus GitHub blob SHA:
- firmware/src/ble/ble_midi_host.c — ece7f0cb3c2ba878dc4a351fcb864fcb2da5b8f7
- firmware/src/ble/ble_midi_host.h — 488977fab5e6300e8e2f7ad96cb834fe62c490f1
- firmware/hal/x0x_ble_abi.h — 8716949b32a1bb11fb3bf5f338f0265dffae9de9.

Safe Merge includes #28 fastmath blob a5c26c95cad0d91a548a8db073f59777e36d4e86 and #37 generator cleanup blob 776d9bd2b82693e8cfb4a180ccb1a28c23cf1e39. Its modified tests/run_tests.sh 5fb5b4b2227ab2d7989883d3afa82686edc33a7a has source-level fail-closed missing worker/unknown selector guards, but the full aggregate script was NOT executed against the current complete tree. Distinct draft #51 output-write repair is NOT in Safe Merge. Draft PR #49 controller atomic-publish follow-up is NOT in Safe Merge. Most critically draft PR #50 BLE source repair is NOT present; Safe Merge retains BLE blob ece7f0c versus PR #50 proposed f4415750533c0e43f1a6c45dcd3a54dac1af6605. These proposed owner fixes must not be treated as fully reviewed or integrated.

## New independent native semantic failures (real candidate safety gate)

Actual Linux x86_64 GCC and Clang each compiled an isolated original scratch harness with -std=c99 -g -O1 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all against those three exact remote-matching C/header blobs. ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 and UBSAN_OPTIONS=halt_on_error=1.

The harness fills the real xb_ring_t HCI mailbox with 341 empty 3-byte frames, confirms ring usage 1023/1023, and makes the BLE host callback return the real xb_frame_put rejection:
1. bleh_start() Reset HCI packet is refused; rejection=1 yet cmd_wait=1 (expected 0 to allow retry). A command never enqueued is incorrectly marked awaiting completion.
2. With connected/notifying state and queued MIDI Note Off 0x80 0x3C 0x00, bleh_poll() has outgoing ACL mailbox rejection but advances q_r from 0 to 1 and lowers credits from 4 to 3. Expected q_r=0, credits=4. The Note Off is permanently discarded by the app path.

GCC compile exit 0; GCC semantic negative test exit 1, exactly two failed assertions. Clang compile exit 0; Clang semantic negative test exit 1 with the same two failures. Neither report is a sanitizer-detected memory error. **THE SAFETY NEGATIVE GATE FAILS.** No device interaction.

Scratch harness SHA256 ea10886bac5682692c00a4357b6d47d1b4e92fd9870446b3030844181a7c7d34 (kept outside Git). GCC negative log SHA256 f411df0d2f1edf84dd54516d2e2ce1324b4622d332ec154d3297b00c48ac1785. Clang negative log SHA256 dd9e98bf657689bf9563f5b2eff4c865a9985dc7a75c950e498b7317a7852299.

See the full preexisting producer-owned PR #50 regression tests/ble_mailbox_backpressure_test.c for permanent executable regression coverage; avoid duplicating that owner's BLE code/test lane. Its PR body also explicitly says the immediate ATT response path is outside the focused repair's guarantee. Safe Merge and Independent QA must review the full rejection/response semantics before allowing candidate promotion.

## Newly executed source-scoped positives (NOT full combined-tree PASS)

On carefully blob-matched unchanged components:
- GCC and Clang ASan+UBSan compile and run tests/host/smc_protocol_test.c (4 commands, exits 0).
- GCC and Clang ASan+UBSan compile and run tests/ble_test.c (4 commands, exits 0; BLE test has 5,153 checks, 0 failures per compiler).
- python3 -B tests/x0x_slots_test.py, tests/install_test.py, tests/install_slots_test.py, tests/install_ctl_test.py (4 commands, exits 0).

The test source and its Python helpers were Git-blob-compared against Safe Merge. Installer mock explicitly SKIPPED its real firmware-package Node comparison because no current package exists; a test suite exit 0 cannot fill that gap. Do not attribute these positives to the unreconstructed full integration tree. Host results log SHA256 f8924af2fd89792c1211c9086575cc9f0db8e19f2b8a4b836149e64a0d150da2; remote/local blob map SHA256 cb4ffdb76778b3c4949981c4b70c91f0fad2f060aa703fe9e0f4ff6bcef06c1f. Scratch tests/logs are in a private review archive, not Git firmware binaries.

## Native dependencies checked / strict target status

- JieLi native Linux pi32v2 Clang version 4.0.1 exited 0. Executable SHA256 c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f. User Library root compiler archive 49,449,905 B, SHA256 ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19; CRC and 705 entries/44 bounded symlinks verified.
- User Library private kit archive SHA256 2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805; python3 kit.py verify exited 0; all 11 pinned AC79 SDK V1.2.1 inputs match. The kit is not a complete SDK. The older V1.2.0 SDK was NOT substituted.
- Four pinned DSP reference checkouts (9W9 10cbe5c; 8W8 94aa271; 303 7680578; TB3PO f49d81d) were NOT reconstructed and independently compared in this run.
- Both exact-remote-source native Elite and Pro target builds: **NOT RUN / BUILD PENDING** due missing verified complete combined-source checkout and existing failed BLE safety negative. Matching ctl.img: NOT BUILT. Both model package/controller CRC, SHA256, sizes and code/RAM/flash/pool: NOT MEASURED. Complete model-exclusion, OTA and package/backup/restore/readback against freshly built exact-source firmware: PENDING. Independent final QA: PENDING. **No firmware .fwsc or ctl.img produced or published.** Do not relabel historical/offline builds as source 8557.
- Current Git matching tag refs and release list returned empty; source tools/build.py VERSION=None by default. **VERSION UNASSIGNED**. Preserve legacy development keys without inventing SemVer or copying NIGHTLY package.json 0.0.0-dev as product version.
- Licensing reviewed from current LICENSING.md: X0X GPL-3.0-only, BLE controller MPL-2.0, JieLi SDK aggregate Apache-2.0. No private SDK/toolchain/DSP/controller/firmware/sample bytes included.

## NIGHTLY-first status and next gates

NIGHTLY main 7757a4b78cc5bcb415c794efe7f1e89dbe521a7e; NIGHTLY gh-pages 9c5d3b6b0846f89436492851fef89ed3829b7ad7. Catalog-owner draft PR #3 contains historical records but NOT a new 8557 report; this branch handoff is not a NIGHTLY checkpoint or Pages change. Sole NIGHTLY Catalog Sync may add truthful WIP/FAILED noninstallable history in its own lane. NIGHTLY PR #2 policy/AGENTS reconciliation remains draft. No automatic flashable NIGHTLY DEV build without all software/model/controller/installer/negative/license/independent QA gates. PRODUCTION additionally requires matching per-model owner hardware proof and explicit owner publication approval.

**No firmware candidate is eligible at this source SHA.** Safe Merge must review owner PR #50 and remaining ATT response/other pending source repairs, then provide a fresh coherent integration SHA; Independent QA and Hourly Build rerun relevant negatives and exact complete native two-model validation. No new schedules/chats, direct thread messages, GitHub-hosted runs, branch merges, force pushes, either Pages edit, release, sample exposure or hardware flash.

X0X37-Role: Hourly Build
X0X37-Stage: WIP / BLE NEGATIVE FAILED / FULL EXACT TARGET BUILD PENDING / NO-GO
X0X37-Consumer: Safe Merge; Independent QA; NIGHTLY Catalog Sync
X0X37-Evidence: docs/scheduler-handoffs/hourly-stage4-safe8557-ble-negative-20261023.md
X0X37-Next: Reproduce from exact Git blobs, reconcile reviewed PR #50, rebuild/revalidate both models only on new approved combined source SHA.
