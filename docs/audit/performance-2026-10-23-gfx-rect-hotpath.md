# Performance engineering: clipped GFX canvas rectangle fill (2026-10-23)

**Status: SOURCE CHANGE + EXACT-FUNCTION HOST CHECKED / FULL-TREE QA + TARGET BUILD + DEVICE PERFORMANCE UNVERIFIED / NOT INSTALLABLE.**

## Scope and provenance

Firmware repository `gobbajogger/X0X37-for-Loopy-Pro`; development destination is the **firmware `nightly` branch in this repository**, not the separate installer-preview/nightly-status repository. Base `main` and `nightly` both resolved to `c9718ae833eb64a5191c350922593df8a36017c9` when the run started. Source file `firmware/src/gfx.c` original Git blob `7bd2139116c95d2505d58a27f6c562e715ab9687`, updated blob `77e71c3bdb5d42220b3bef11e9ad4a960c0dfea3`. Focused source changes were committed at `d9b6090ef0a84b142948c19eea14c8e166d779d4`; the reproducible regression/benchmark script at `e58d6e1128cf9947aee35e6c0ab9a150e0bc7781` (script blob `5455fb49c5f707bcc39d2968bc25c71a9f60d733`).

**Changed firmware path:** only `firmware/src/gfx.c::cv_rect`. The original hot path recalculated a two-dimensional pixel index for every pixel when filling clipped rectangles. The candidate clips once, refuses empty spans before forming pointers, advances a contiguous pointer across the row, and uses a single contiguous sweep for the full-width case. It retains byte-swapped RGB565 writes, band offsets, canvas bounds and full/partial-rectangle behavior. No global buffers, memory pools, audio sequencing, BLE/SMC, project/sample or installer operations are changed. No LCD DMA-rate or hardware-SPI changes are claimed.

## Source-matched native Linux host evidence

Executed locally on **Linux x86_64**, GCC 14.2.0 / Clang 17.0.0 and Python 3.13.5. The test script `tests/host/gfx_rect_perf_test.py` extracts the real `cv_rect` function from the `--source` file, compiles a bounded C harness, and compares the entire 240×144 canvas buffer after each operation against a simple independent per-pixel rectangle oracle. Coverage for each execution: **9,216 deterministic rectangles**, 1/64/240 pixel canvas widths; full and shifted/banded canvases; mixed, small and full-canvas fills including clipped, zero and negative dimensions.

Commands (run separately with both compilers; Python propagates a failing compile/child exit):

```sh
python3 tests/host/gfx_rect_perf_test.py --source firmware/src/gfx.c --compiler gcc --sanitize
python3 tests/host/gfx_rect_perf_test.py --source firmware/src/gfx.c --compiler clang --sanitize
python3 tests/host/gfx_rect_perf_test.py --source firmware/src/gfx.c --compiler gcc --bench
python3 tests/host/gfx_rect_perf_test.py --source firmware/src/gfx.c --compiler clang --bench
```

The GCC and Clang **AddressSanitizer + UndefinedBehaviorSanitizer** compile and run steps each exited **0**, each with `PASS: 9216 exact-source rectangles, oracle pixel-equivalent`. An intentional negative-control alteration of the rectangle color write returned **run exit 1** on the oracle mismatch, demonstrating that the test fails closed on an incorrect fill. The before/after inputs were the source-matched baseline and candidate functions from the above Git blobs. The local candidate function text matches the remotely retrieved updated function (945 characters, FNV-1a32 `aab871c6`). Benchmark binaries used `-std=c11 -O2 -Wall -Wextra -Werror`, no sanitizer. Whole-firmware compilation, code-size/flash/RAM/pool budgets and physical Elite/Pro display timings were **not** executed.

## Reproduced before/after HOST PROXY timings

Values are **median nanoseconds per `cv_rect` call**, first taking the median of five identical-workload timing loops in each execution, then the median across complete independent baseline/candidate pairs. These are x86 CPU-only function-execution proxies; **not** embedded timings or perceptible UI frame-rate measurements.

| Compiler; completed before/after pairs | Rectangle workload | Baseline ns | Candidate ns | Reduction |
| --- | --- | ---: | ---: | ---: |
| GCC 14.2; 3 pairs | Mixed rectangles / clipping | 1107.753 | 551.728 | **50.2%** |
| GCC 14.2; 3 pairs | Small clipped rectangles | 17.070 | 11.442 | **33.0%** |
| GCC 14.2; 3 pairs | Full-screen full-width fill | 11077.713 | 10307.313 | **7.0%** |
| Clang 17; 2 pairs | Mixed rectangles / clipping | 201.092 | 188.228 | **6.4%** |
| Clang 17; 2 pairs | Small clipped rectangles | 12.543 | 9.252 | **26.2%** |
| Clang 17; 2 pairs | Full-screen full-width fill | 1362.088 | 1268.934 | **6.8%** |

Five passes of deterministic timing loops were used per benchmark invocation; absolute GCC/Clang throughput differed substantially due to optimizer choices. A third Clang paired timing attempt did **not complete** and is excluded rather than reported as PASS. Scheduling noise and host compiler differences mean even these repeated numbers are not an on-target speedup guarantee. Source correctness was independently tested, not inferred from timing.

## User relevance, limits and next verification

If the target pi32v2 compiler benefits from the new pointer-index loops, repeated canvas fills on display updates may do less CPU work, potentially improving control responsiveness during dense redraws. **This is a hypothesis until independent target build and physical display/frame-time measurement.** The optimization does not by itself repair MIDI Note Off drops, fix SMC/BLE congestion, reduce SPI transfer bytes or establish audio XRUN improvement. The exact local host tests verify only the extracted function and do not establish a complete Git checkout test run. Do not claim this branch DEV CHECKED, device tested, installable, nightly catalog approved, production-approved or released.

**Next consumers:** Independent QA: rerun both host sanitizer modes and additional width/band/negative dimension cases from exact PR head, inspect ABI/embedded code size and semantics, and run whole-tree GFX/UI/host regression suite. Safe Merge/Hourly Build: evaluate any code/RAM/pool budget delta with the real pinned JieLi native toolchain and current exact PR source before choosing integration. Owner physical Elite/Pro: measure LCD update time/perceptible responsiveness separately after a safely approved build. **No other worker-owned SMC/BLE, sequencer, sample, installer, `main`, `gh-pages`, or nightly-status paths were edited.**
