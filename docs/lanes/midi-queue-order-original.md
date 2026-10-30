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
