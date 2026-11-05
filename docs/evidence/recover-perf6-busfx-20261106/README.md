# PERF #6 and bus-FX recovery

Base: `a06e20189141ff5fb8766e2c6152be460f4356c3`.
Lane: `lane/recover-perf6-busfx-20261106`.

Restored two lost optimizations so project saves do less assembly work and bus FX
avoid repeated block-invariant calculations. Project bytes, object and flush
boundaries, capacity and error behavior, and audio/filter/gate/ring state retain
their previous behavior.

Recovery provenance is pinned in `input-receipt.json`. The recovered stream test
needed JSONL escape repair, and the bus-FX test needed the current baseline pin.
`recovered-project.patch` contains unrelated font/UI transcript data and no
project.c hunk; none of that transcript was applied.

## Native development builds

This task ran both base and candidate Elite/Pro builds here on Linux x86-64 with
JieLi clang 4.0.1 and SDK V1.2.1. All 11 required SDK hashes match. Both candidate
builds pass the image, RAM, pool, ROM-call, single-precision and RAM-code guards.
Configuration: `BOARD=smk37`, `MODEL=elite|pro`, `X0X_DEV_PADCAL=1`; no release flag.

The two models have the same measured sizes:

| Resource | Base | Candidate | Delta |
| --- | ---: | ---: | ---: |
| App image | 474,840 B | 474,872 B | +32 B |
| App `.text` | 471,088 B | 471,120 B | +32 B |
| App RAM, including alignment | 95,780 B | 95,780 B | 0 B |
| `.data` | 428 B | 428 B | 0 B |
| `.bss` | 95,332 B | 95,332 B | 0 B |
| Pool used | 335,464 B | 335,464 B | 0 B |
| Pool free | 8,600 B | 8,600 B | 0 B |
| `.ble_ram` | 3,880 B | 3,880 B | 0 B |
| `.ram_text` | 2,712 B | 2,712 B | 0 B |

App RAM headroom is 2,524 B against 98,304 B. Pool headroom remains 408 B above
the 8,192 B minimum. `.ram_text` is byte-identical in each model: 844 instructions
and no calls. Package length remains 651,457 B for each model. Hashes are in
`native-metrics.json`; binaries are excluded from this delivery.

Symbol sizes explain the 32 B increase: `st_put` grows from 88 to 100 B and
`busfx_apply` from 1,710 to 1,730 B. The bus-FX prologue's explicit stack allocation
falls from 124 to 116 B, with the same saved-register set. This excludes callees
and does not establish whole-program stack high-water.

At 44.1 kHz and 320 MHz the budget is about 7,256 cycles/sample. Target cycles and
total save latency are unmeasured. Runtime counters prove one cutoff calculation
per nonempty call below the cutoff threshold, and one tempo-period calculation
per nonempty rhythmic call. A fully unheld rhythmic call can now calculate one
unused period. No measured CPU saving or device qualification is claimed.

## Host proof

Before production edits, repaired characterization tests passed 5,760 stream
cases and 125,440 bus-FX calls at each of O0/O2. The added call-count contract
then failed on the base as intended: a two-sample call calculated each cutoff
exponential twice instead of once. The restored implementation passes it.

The strengthened bus-FX matrix checks 188,160 calls at each of O0/O2, including
channel clamping, cutoff and BPM boundaries, empty/negative blocks, press/release
coincidence, null/out-of-range ring channels, tails and sample-counter wrap. It
also compares the standalone rhythm helper. The stream harness compares 5,760
cases, including all object indices, boundaries, callback sizes, capacity
overflow and injected flush failures. It checks writer state and complete object
and buffer bytes. Existing project/storage integration tests cover the real save
path beyond the mocked flush boundary.

Seven focused regressions passed. The complete host suite passed: **334 PASS,
0 FAIL, 0 SKIP, 0 NOT_RUN**, exit 0. See `full-host.log` and `host-summary.json`.
This includes all fourteen UI negative controls and the final scratch-buffer
audio comparison. No test failures required a production fix beyond the recovery.

The full-suite command uses the verified external references and local host
clang/clang++ 19.1.7, with `X0X_STRICT_PREFLIGHT=1 X0X_TEST_JOBS=8 sh
tests/run_tests.sh`. The preflight passed all 16 checks. Native builds use
`MODEL=elite|pro ./build.sh --board smk37` with the verified toolchain/SDK paths.
The generated-header cache is explicitly located under the task workspace;
the initial default home-cache write failed and was corrected by configuration.

Independent code review completed with no findings; its reviewed file hashes
match `checked-source-sha256.json`. Reuse, quality and efficiency review found no
worthwhile simplification. These reviews grant no merge or device authority.

## Delivery boundary

Deliver source, tests and sanitized evidence only to the named lane. No existing
PR was found at intake. No main/nightly integration, device action, protected
flash change, firmware publication, SDK/toolchain payload, binary, token or raw
capture is part of this task. The assigned integrator can review the branch
against the pinned one-build base after remote readback.
