# Engines/UI latest-base scoped proof, 2026-10-30

Candidate at time of this proof: `24a36371fe7779bdca6b72b1bc2aee9d58502ec4` on `wip/engines-ui-one-build`, direct parent `wip/one-build@094bb1a55c159aefb0e2bfa39790a1b4b74cee40`. The new test in the next evidence commit uses the **same exact helper blobs** and is independently reproducible without the firmware SDK.

## Source identity

Source bytes were fetched through the connected GitHub API, written to an isolated Linux x86-64 scope, and checked with `git hash-object` against the exact blob SHA for the candidate. These are not transcribed models:

| Exact production source/test | Blob SHA |
| --- | --- |
| `binary32_math.h` | `37ffc4c5ae4a38d273808d36ff127d6d74c70f8c` |
| `chicago_clock_span.h` | `7786c93147ff5669c18845fa2432818f49fdac7e` |
| `chicago_clock_span_test.c` | `265452219f6837ed59add497a17dc9eb33cac4c4` |
| `chicago_chorus_span.h` | `fa122964123593df78c074d4d12aca05fe47df3a` |
| `chicago_chorus_span_test.c` | `328d6b91fda7de60095d267a4d03f5d54d367dd5` |
| `miami_idle_clock.h` | `efe52f8599bbd9b7c698f809a294732de484db93` |
| `miami_zero.h` | `62eaf66539ad3a36c99c3805a129dab74ee2e1fb` |
| `miami_idle_helper_smoke_test.c` | `df9a22bb8bea6b701c94d94fbfa35fb159371995` |

All eight hashes matched the checked files. The Chicago tests and both Miami helpers compiled and executed in the cloud with Debian GCC 14.2.0, 64-bit Linux, `-std=c99 -Wall -Wextra -Werror -fno-fast-math -ffp-contract=off`, normal `-Os` and separately `-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all`. Exact results:

| Test | Normal | ASan/UBSan | Cases |
| --- | --- | --- | ---: |
| Chicago float32 clock span vs literal repeated addition | PASS | PASS | 414,240 |
| Chicago chorus recurrence vs literal per-sample update | PASS | PASS | 324,902 |
| Miami float32 idle clock span vs literal repeated addition | PASS | PASS | 300,308 |
| Miami signed/float zero buffer full-range and guard preservation | PASS | PASS | 8,193 |

Run from a current checkout:

```sh
sh tests/host/run_chicago_clock_span.sh build/engines-ui-clock
sh tests/host/run_chicago_chorus_span.sh build/engines-ui-chorus
sh tests/host/run_miami_idle_helper_test.sh build/engines-ui-miami-helpers
```

The new Miami test is source-scoped: randomized 20–400 BPM/clock and boundary probes, 0–65535 frame spans, and stores of 0–8192 elements for word, halfword and float scratch arrays. The signed-zero output is compared at the raw binary32 level and guard regions are checked. This is **not** proof of voice/state/FX or factory audio.

## Target compiler/input observations

The uploaded JieLi archive SHA-256 was verified as `ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19`. It safely contains 44 relative symlinks; `clang --version` reports 4.0.1. An isolated wrapper calling exactly the tested Chicago and Miami helper source compiled successfully with JieLi clang `-target pi32v2 -Os -mcpu=r3 -mfprev1 -ffp-contract=off -fno-fast-math`, with the toolchain's `pi32v2/include` added via `-idirafter`. The resulting local object is a 32-bit Pi32v2 relocatable ELF. This is **TARGET HELPER COMPILE PASS**, not an Elite/Pro firmware link or memory-budget PASS.

The separate private-input archive SHA-256 `2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805` was checked. `kit.py verify` passed **all 11 pinned V1.2.1 SDK inputs**. The attached large V1.2.0 SDK is not substituted for V1.2.1.

## Remaining candidate qualification: NOT RUN

A complete *current* GitHub source checkout, exact factory/preset/private DSP input closure and full V1.2.1 SDK build environment are not established in this container. The available offline source TAR is an older `a7082231` checkpoint and cannot serve as a `wip/one-build` source tree. GitHub DNS is unavailable to the sandbox; all branch/file writes and readback therefore used the authenticated GitHub connector, not Git network commands. No tests from the old TAR are reported as tests of this commit.

Still **NOT RUN** on the integrated candidate: full `host/build_host.sh`, production six-theme framebuffer UI/selector captures, full Miami/Chicago factory PCM/state and 15/15 golden renders, long idle-then-note, BLE/SMC/BREAK regression, Callgrind active/idle instruction counts, full Pro/Elite firmware/controller builds, `DEV CHECKED` records, install preview and device PERF. The exact Mac commands and guarded gates remain in `docs/lanes/engines-ui.md` and `docs/lanes/engines-ui-dev-plan.json`.

No private headers or binaries are committed; no device flash, protected-region change, merge, release or publication is performed. Receiving roles: independent exact-candidate QA and the sole firmware integrator.
