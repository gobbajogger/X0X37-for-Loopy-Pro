# BREAK host-regression restoration, PR #689

Repository: gobbajogger/X0X37-for-Loopy-Pro  
Base: `wip/one-build` at `cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21`  
Recovered source references: `163191c3b679b5204db7a1808b6b2b34a99cb972`, `953aa683b2024fddf92143391c037d43c73f3d32`  
Runner platform: Linux x86_64, GCC, Clang and Python 3.13

## Recovery scope

The upstream `wip/one-build` already contains identical Git blobs to the previous BREAK tip for `firmware/src/dsp/breaks.c`, `breaks.h`, the portable BREAK performance test and script, the adaptive DSP and MIDI tests, and the built-in break generator. Replacing those bytes would not restore any missing behaviour and would risk discarding newer Chicago/Miami integration.

The actual missing delta was six `tests/run_tests.sh` entries. The restoration reinstates **adaptive-break-dsp**, **adaptive-break-controls**, **adaptive-break-storage**, **adaptive-break-midi-pair**, **break-performance**, and **builtin-break-output** while keeping the newer combined-suite entries.

## Cloud checks executed

These scoped runs used a reconstructed historical source checkout with the named BREAK source/test and relevant dependency files rewritten to match their GitHub blob hashes at PR #689. This is scoped evidence only. It is not a verified complete checkout of the current PR.

| Check | Result | Observation |
| --- | --- | --- |
| `sh tests/host/run_adaptive_break_dsp.sh` | PASS | 51,481 checks, zero failures |
| Same DSP test with GCC ASan/UBSan | PASS | 51,481 checks, zero failures |
| `CC=gcc sh tests/host/run_break_performance.sh` | PASS | 2,413,370 checks, zero failures |
| `CC=clang sh tests/host/run_break_performance.sh` | PASS | 2,413,370 checks, zero failures |
| Same performance test with GCC ASan/UBSan | PASS | 2,413,370 checks, zero failures |
| `CC=clang sh tests/host/run_breaks.sh` | PASS | 89/89 at 22.05 kHz, 89/89 at 44.1 kHz; negative no-crossfade click control fired as expected |
| `python3 tests/host/run_adaptive_break_midi_pair.py` | PASS | 27,348 checks, zero failures; source files `plat_fm1.c` and `fm1_irq.h` matched the remote Git blobs |
| `python3 tests/host/test_builtin_break_output.py -v` | PASS | Seven test methods, zero failures |
| `sh -n tests/run_tests.sh` | PASS | Runner's reconstructed Git blob was exactly `2c0eb955384cd0e6e0596d3694d3828477798d1a` |

The six-name selector on the reconstructed source ran all six entries: four PASS; the integrated controls and storage fixtures failed to compile because their surrounding Chicago/Miami source tree was historical and not an exact current candidate. **Those two tests have no valid verdict for PR #689's complete tree.**

A full historical/mixed-source `X0X_TEST_JOBS=8 sh tests/run_tests.sh` was attempted in the cloud, but exceeded a 180-second run limit with incomplete results and some historical-base failures. **A full-suite PASS or FAIL on the exact PR source is NOT_RUN**, not inferred from that attempt. The current source comparison showed 178 compiler/test paths changed or added beyond the historical checkout, so exact-source full-suite verification is still missing.

The JieLi Linux toolchain archive matched SHA-256 `ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19`; extracted Clang reports version 4.0.1. The pinned V1.2.1 kit matched SHA-256 `2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805`; `python3 kit.py verify` passed 11/11 pinned SDK inputs.

## Open verification

- Full host suite on the complete, exact current PR source: **NOT_RUN**.
- Exact-current Elite development firmware build: **NOT_RUN**.
- Exact-current Pro development firmware build: **NOT_RUN**.
- Hardware, package installation and audio bit-exact golden comparisons: **NOT_RUN**.

This receipt contains only sanitized source identifiers and test results, not private headers, toolchain/SDK payloads, firmware binaries, credentials or raw captures. Keep the PR draft until its exact-source integrated checks and required target builds have credible results.
