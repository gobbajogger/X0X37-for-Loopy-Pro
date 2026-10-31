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

## Additional current-source checks

A second independent host compiler, LLVM Clang in the same Linux runtime, was used at `-Os` with `-std=c99 -Wall -Wextra -Werror -fno-fast-math -ffp-contract=off`. These exact source/test blobs again returned **PASS** for Chicago clock (414,240 cases), Chicago chorus (324,902 cases) and Miami clock/zero stores (300,308 / 8,193 cases). No Golden PCM or native application was linked.

The newest integrated build also contains the original previously ported Free Play rate-pad logic. To validate it without reconstructing an imaginary UI model, five **exact function bodies** were extracted from GitHub blobs at candidate `77be40162e72ea30f59610ac06b88c42e0d4ab98`:
- `firmware/src/app/ui.c`: `division_pad_owner` (FNV-1a `ba0f2918`), `division_pad_index` (`553c62a8`), `division_pad_label` (`a8508092`), `division_pad_input` (`fad57fe4`);
- `firmware/hal/smk_input.h`: `smk__division_poll` (`f877c9bf`) and the actual `smk__padq` rate-ownership preprocessor branch (`268c7cb7`).

Only platform/runtime dependencies were stubbed around the **unaltered production function bodies**. This source-scoped harness was compiled using Linux GCC with `-std=c11 -O1 -Wall -Wextra -Werror -Wno-unused-function -fsanitize=undefined -fno-sanitize-recover=all`. It ran and returned:
```text
PASS: exact current Free Play rate-pad source: 8 divisions, held release, handoff, overlays
```

Coverage includes all eight Miami rate pads, retained press/release ownership across mode changes, switching selected engine/voice without stale edits, and preventing rate selection under save/home, recording, help/overlay, SMC, clip and other guards. This confirms the preserved handler logic, **not** production host-build linkage, physical pad scanning, saved project sound or real-device LEDs. The existing canonical full-checkout regression remains `python3 tests/host/test_freeplay_rate_pad_port.py`; it is **NOT RUN** in this scope and has not been relabeled as passed.

## Remaining candidate qualification: NOT RUN

A complete *current* GitHub source checkout, exact factory/preset/private DSP input closure and full V1.2.1 SDK build environment are not established in this container. The available offline source TAR is an older `a7082231` checkpoint and cannot serve as a `wip/one-build` source tree. GitHub DNS is unavailable to the sandbox; all branch/file writes and readback therefore used the authenticated GitHub connector, not Git network commands. No tests from the old TAR are reported as tests of this commit.

Still **NOT RUN** on the integrated candidate: full `host/build_host.sh`, production six-theme framebuffer UI/selector captures, full Miami/Chicago factory PCM/state and 15/15 golden renders, long idle-then-note, BLE/SMC/BREAK regression, Callgrind active/idle instruction counts, full Pro/Elite firmware/controller builds, `DEV CHECKED` records, install preview and device PERF. The exact Mac commands and guarded gates remain in `docs/lanes/engines-ui.md` and `docs/lanes/engines-ui-dev-plan.json`.

No private headers or binaries are committed; no device flash, protected-region change, merge, release or publication is performed. Receiving roles: independent exact-candidate QA and the sole firmware integrator.

## Current-branch follow-up verification (2026-10-30)

Source scope: `wip/engines-ui-one-build` at `0becc7651629aa9860aa6d93374b48f893181612`, with verified integrated merge ancestor `wip/one-build@cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21` (zero commits behind). The earlier `094bb1a5` base above is provenance for the earlier test, not the integration base of this current candidate.

A fresh Linux cloud run re-fetched the exact eight public production helper/test blobs listed in this record, checked their `git hash-object` values against the GitHub API, and executed GCC 14.2 at `-Os` and independently with ASan/UBSan: **PASS** Chicago clock (414,240 exact cases), **PASS** Chicago chorus (324,902 exact cases), **PASS** Miami idle clock (300,308 exact cases), **PASS** zero-buffer guard checks (8,193 sizes), with exit code zero for all six test executions. A fresh JieLi Clang 4.0.1 invocation compiled those unchanged Chicago/Miami helper blobs for Pi32v2 (`-target pi32v2 -Os -mcpu=r3 -mfprev1 -ffp-contract=off -fno-fast-math`), producing a 32-bit relocatable ELF object, SHA-256 `76eb66f137eddeae7448e571d832ee9eb7a0a90d638b15bb82675f4a7466160d`. The private kit separately passed all 11 pinned AC79NN SDK V1.2.1 input checks. An API recursive-tree audit of the current head confirmed **36/36** fixed source-closure Git blob SHA pins, with no mismatches; these pinned original engine/UI files were preserved byte-for-byte from the earlier optimization integration.

This proves isolated exact-source arithmetic and compilation only, **not full current-head build linkage, 15/15 golden renders, factory PCM/state, full host tests, Pro/Elite firmware packages, instruction-count or device PERF**. The historical local complete-source TAR is not this integrated Git tree. No product source, DSP algorithm or protected component was changed by the base-pointer/QA-plan correction.
