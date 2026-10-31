# MIDI queue ordering on original Chicago/Miami integration

## Starting point and protection

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
- Immutable base: `wip/one-build@094bb1a55c159aefb0e2bfa39790a1b4b74cee40`.
- This is an isolated transport fix. **No new Chicago/Miami DSP, UI, project-migration, controller, bootloader or protected flash changes.**
- Source provenance: portable queue/IRQ correction from PR #677 and focused regression from PR #695. The existing two-peer SMC BLE host union, original Chicago/Miami runtime and rev-13-only format remain unchanged.
- The original old #695 branch had 208 PASS/101 FAIL on its own Mac run. Those failures have **not** been repaired or converted into passes by this transport-only fix.

## Confirmed defect and exercised repair

On the exact source function text fetched through GitHub's connected API, the current producer replaced an earlier disposable CC in place while the FIFO was full. An arriving Note Off could therefore precede an earlier queued Note On, and transport commands could pass older messages. The BLE output ring had 56 slots so uint32 cursor rollover violated the physical modulo slot progression. TIMER5 could also preempt queue compaction or the input queue read/consume transaction.

Repair: append critical events after all retained packets by compacting the ring; mask/restore TIMER5 only during actual producer/consumer queue transactions; change BLE ring capacity to 64 (power-of-two) while preserving two central peers. This adds **32 bytes** to the BLE output ring; final target memory/pool qualification is required.

## Executed host proof (Linux x86-64, source-extracted fixtures)

- Source captures: `usb.c` blob `7996232882a1d4b36f4a58c46ef2cc9d68dbf94c`, `ble.c` blob `b3bad4062164dc0ed8f5e55e270f5674dea3cde4`, `plat_fm1.c` blob `49dd6edfa00152694a4b4fb2348b46436e6f76a4` at base.
- Baseline: command `python3 test_fifo.py --out results/base-expanded-red.txt`, **1,280 checks, 675 failures, exit 1**.
- Patched: `python3 test_fifo.py --out results/repaired-expanded.txt`, **1,320 checks, 0 failures, exit 0**.
- Patched UBSan: `python3 test_fifo.py --sanitize --out results/repaired-expanded-sanitize.txt`, **1,320 checks, 0 failures, exit 0**.
- Patched Clang: `CC=clang python3 test_fifo.py --out results/repaired-clang.txt`, **1,320 checks, 0 failures, exit 0**.
- Patched Clang UBSan: `CC=clang python3 test_fifo.py --sanitize --out results/repaired-clang-ubsan.txt`, **1,320 checks, 0 failures, exit 0**.
- Checks cover 3 paths, cursor near UINT32_MAX, protected packets and a simulated TIMER5 producer/consumer interrupt crossing.
- This is a **scoped source-extracted C harness**, not the full authenticated `wip/one-build` tree. The new committed `midi-overflow-order` regression compiles the production fragments directly and still needs to execute on a verified full checkout.

## Cloud build inputs and actual build limit

- Compiler: GCC 14.2.0, Clang 17.0.0; vendor JieLi `pi32v2` Clang **4.0.1** was extracted and ran `--version`.
- Supplied toolchain ZIP SHA-256 `ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19`.
- The separate private V1.2.1 kit SHA-256 `2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805` passed `kit.py verify` (11/11 pinned input hashes).
- Historical `f550a6b8` source closure was present; a native Elite build attempted with the verified compiler/kit, but **FAILED before compiling the application**, because that archive lacks `BarlowSemiCondensed-SemiBold.ttf` and `assets/909/hh.wav`. Do not treat this as a test/build of the current source tree.
- The full latest `wip/one-build` source archive cannot be reconstructed from the existing local archives; native HTTP DNS to `api.github.com` failed (`[Errno -3] Temporary failure in name resolution`). Connected GitHub API reads and writes work, but do not mount a complete checkout into the native runtime. **Current-head full host suite, DEV package, RAM/pool budgets and bit-exact audio: NOT_RUN/NOT_VERIFIED**.

## Required exact-HEAD tests when full checkout is available

```sh
git rev-parse HEAD
sh host/build_host.sh
X0X_TEST_JOBS=1 X0X_TESTS='midi-overflow-order midi-mq-noteoff-overflow midi-mq-rt-overflow midi-out-mq-noteoff-overflow ble-out-noteoff-overflow ble-out-rt-overflow smk-clip-knob smk-clip-usb-clear trs uac' sh tests/run_tests.sh
X0X_TEST_JOBS=1 sh tests/run_tests.sh
```

For an actual DEV qualification use `tools/dev_validate.py` with a plan at the exact HEAD, the full pinned source and correct model/toolchain/SDK/reference paths. Do not treat a historical build, a source-extracted host test or a GitHub API write as a completed current-HEAD native build. Independent QA and SMC/Chicago/Miami integrated runtime validation remain pending. Keep the PR draft; never merge into nightly or main without separate authority.

Cloud verification update: Clang ASan+UBSan on the repaired source-extracted transport functions passed 1320/1320 assertions (exit 0). JieLi Clang 4.0.1 compiled the same freestanding transport code to a pi32v2 ELF32 object (exit 0). These scoped results do not qualify a complete firmware build; the latter remains NOT_RUN.


## 2026-10-30 cloud scoped retest

Source branch at the time of the retest: `da16ff754b655642cee7124edadb6fc50c80d14e`, based on `wip/one-build@094bb1a55c159aefb0e2bfa39790a1b4b74cee40`. Native environment: Linux x86-64, GCC 14.2, Clang 17 and JieLi pi32v2 Clang 4.0.1.

- Source-extracted queue regression: **1,320/1,320 PASS** with GCC, GCC UBSan and Clang ASan+UBSan. Each invocation returned exit code 0.
- A nine-selector host run returned **9 PASS, 0 FAIL**, exit code 0: `midi-mq-noteoff-overflow`, `midi-mq-rt-overflow`, `midi-out-mq-noteoff-overflow`, `ble-out-noteoff-overflow`, `ble-out-rt-overflow`, `trs`, `uac`, `smk-clip-knob`, and `smk-clip-usb-clear`.
- The tested `firmware/src/usb.c` has the **exact Git blob** `3df6f132ce984a0397116801efaa6d1fd93fd9c8`; the nine host fixture inputs' blob SHAs also matched the GitHub PR #705 paths. The current `ble_out`, `plat_midi_in` and `smk_ck_poll` function bodies were compared against the API-fetched source using deterministic FNV64 fingerprints.
- Verified pinned SDK V1.2.1 input subset: **11/11 PASS**. JieLi compiled a standalone transport USB translation-unit probe successfully. This is **not** a linked firmware application.
- A locally combined historical `f550` source closure plus bootstrap assets failed both `host/build_host.sh` and the Elite native build because that historical archive lacks `experiments/wobble` sources. Its first clip-knob fixture also failed due an outdated UI stub; the same fixture and polling function were restored to the current GitHub versions and passed. These are **historical source-closure problems**, not evidence that the current original Chicago/Miami runtime fails.
- The cloud shell cannot resolve the GitHub API hostname for a complete checkout; the connected authenticated GitHub API can inspect and update the repo but cannot mount the complete tree in this native shell. A separate GitHub Actions integration lane had two runs on 2026-10-30 with **zero executed job steps**, hence no usable host results.

**Qualification limit:** full current-HEAD host suite, original Chicago/Miami audio golden renders, target Elite DEV build, RAM/pool budgets and device behavior remain **NOT_RUN / NOT_VERIFIED**. The historical PR #695 Mac result (208 pass / 101 fail) remains a separate unresolved result on a different source profile.

**Evidence:** local read-only ZIP `current-transport-evidence-20261030.zip`, 50,572 bytes, SHA-256 `63be403e6eeb0f8952c228a8fb25dde3fe70d3de074e98dd4f558b22da28cc0e`. It contains only sanitized test logs, source fixtures and scripts, with no firmware binaries, SDK, private headers or credentials. The ZIP is available in the project's ChatGPT conversation, not committed to Git.

## Latest integration-base reconciliation (2026-10-30)

This branch now incorporates the newer original-runtime `wip/one-build` integration SHA `cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21` as a second merge parent. The former tested queue-fix HEAD is `5a75197afa47a5ee47f0fbe9cc5d84e75055ec65` (first merge parent). This is an **isolated branch synchronization only**, not an integration into protected `nightly` or `main`. New integrated source changed only `docs/lanes/one-build.md`, `firmware/hal/smk_input.h`, `host/build_host.sh`, `tests/host/test_freeplay_rate_pad_port.py`, `tests/host/test_original_build_composition.py`, and `tools/build.py`; **none overlaps the 15 repair paths**.

### This session's local Linux evidence

- Pinned queue source-extracted C test: `python3 test_fifo.py --root <exact archived source>`, **1,320/1,320 PASS**, exit 0, GCC.
- Same test with `CC=clang --sanitize --asan` and `ASAN_OPTIONS` controlled inside the runner, **1,320/1,320 PASS**, exit 0.
- Five individual Python host regressions run against the exact `usb.c` blob `3df6f132ce984a0397116801efaa6d1fd93fd9c8` and matching exact test blobs: `midi_mq_noteoff_overflow_test.py`, `midi_mq_rt_overflow_test.py`, `midi_out_mq_noteoff_overflow_test.py`, `ble_out_noteoff_overflow_test.py`, `ble_out_rt_overflow_test.py`. Each had exit 0. The BLE source is a captured function window, **not** a full authenticated checkout.
- JieLi vendor `pi32v2` Clang 4.0.1 compiled the exact transport-function object fixture to ELF32, **exit 0**. This does not test the full firmware, pool budget, or sound.
- Current latest full `wip/one-build` source could not be mounted in this native sandbox: available historical ZIP archives omit the original Chicago/Miami source integration, and direct GitHub API DNS resolution from the shell fails. The authenticated GitHub connector was used to verify the complete remote tree and create this merge. No native git network command was used.
- Historical Mac report from PR #695 (`208 PASS / 101 FAIL`) relates to a different obsolete engine source profile, not this current merged branch.

**Still NOT RUN at this merged HEAD:** full host suite, genuine current-source Elite/Pro SDK V1.2.1 app build, linker/RAM/pool validation, original Chicago/Miami bit-exact audio proof, independent review and device tests. The development PR remains draft.

**Re-run on the full, matching source:**
```sh
git rev-parse HEAD
sh host/build_host.sh
X0X_TEST_JOBS=1 X0X_TESTS='midi-overflow-order midi-mq-noteoff-overflow midi-mq-rt-overflow midi-out-mq-noteoff-overflow ble-out-noteoff-overflow ble-out-rt-overflow' sh tests/run_tests.sh
X0X_TEST_JOBS=1 sh tests/run_tests.sh
```
