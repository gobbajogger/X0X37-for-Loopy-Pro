# BREAK reverse-playback checkpoint density — native host profile (2026-10-23)

**Status: SOURCE CANDIDATE / HOST MODEL CHECKED / TARGET BUILD NOT RUN / DEVICE NOT TESTED.**

## Executive finding and exact source
Firmware repository: `gobbajogger/X0X37-for-Loopy-Pro`; firmware NIGHTLY parent `58b5b45a2dd90581de7a674c331ca5cbe8a67c7e` (tree `71b759dd206d844cceed9e40ae68966142720e9c`). Unmodified upstream `firmware/src/dsp/breaks.c` blob `1d3d692b6788d9d1f13901cd7ab13df506cdb72f`; IMA decoder blob `9ba16e4fe9f0cb643c95a68d540042ab2581d4de`; original breaks header blob `094af6650333e844311a8806145a6177a0318225`. Work is unclaimed BREAK reverse audio performance, not SMC, BLE, clock, storage, installer or build pipeline.

The change **doubles BREAK checkpoints per bank from 480 to 960**, retaining 512-sample reverse cache and unchanged decoder. During long loops, a reverse-cache refill must replay fewer IMA nibbles from the nearest prior checkpoint. Exact source-matched model code builds checkpoints and mirrors `bank_seek`/`rev_fetch`; it calls production `ima_decode` from the verified decoder blob. It is **not direct instrumentation of the full breaks.c renderer**; full BREAK engine regressions and firmware model builds are still required.

## Workload and native checks
Host: Linux x86_64, GCC 14.2.0 and Clang 17.0.0, C11 O2 `-Wall -Wextra -Werror -pedantic -ffp-contract=off`; sanitizer runs O1 with `-fsanitize=address,undefined`. Synthetic xorshift32 ADPCM, no copyrighted samples. Four loop sizes of **513, 176400, 705600 and 1411200 samples** (longest represents 8 bars at 60 BPM with 44.1 kHz mono). Each `--check` compares **every decoded sample** in reverse descent plus 1024 pseudo-random seeks per size, versus a full independent forward reference decode; validates max refill bound. Baseline compiled with `-DBRK_PROFILE_NCK=480`, candidate with `-DBRK_PROFILE_NCK=960`. Host timing is nine trials × 65,536 descending output samples for long sizes. Median/MAD/min/max are per output sample, not MCU cycles.

Executed:
```sh
gcc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -ffp-contract=off -DBRK_PROFILE_NCK=480 tests/host/breaks_reverse_seek_perf_test.c -o gcc-brk-480
./gcc-brk-480 --check
./gcc-brk-480 --bench
gcc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -ffp-contract=off -DBRK_PROFILE_NCK=960 tests/host/breaks_reverse_seek_perf_test.c -o gcc-brk-960
./gcc-brk-960 --check
./gcc-brk-960 --bench
clang -std=c11 -O2 -Wall -Wextra -Werror -pedantic -ffp-contract=off -DBRK_PROFILE_NCK=480 tests/host/breaks_reverse_seek_perf_test.c -o clang-brk-480
./clang-brk-480 --check
./clang-brk-480 --bench
clang -std=c11 -O2 -Wall -Wextra -Werror -pedantic -ffp-contract=off -DBRK_PROFILE_NCK=960 tests/host/breaks_reverse_seek_perf_test.c -o clang-brk-960
./clang-brk-960 --check
./clang-brk-960 --bench
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fno-omit-frame-pointer -fsanitize=address,undefined -DBRK_PROFILE_NCK=960 tests/host/breaks_reverse_seek_perf_test.c -o gcc-brk-asan
./gcc-brk-asan --check
clang -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fno-omit-frame-pointer -fsanitize=address,undefined -DBRK_PROFILE_NCK=960 tests/host/breaks_reverse_seek_perf_test.c -o clang-brk-asan
./clang-brk-asan --check
./gcc-brk-960 --invalid   # expected exit 2
./clang-brk-960 --invalid # expected exit 2
```
Both compilers' optimized compile, baseline/candidate checks and benchmarks **exited 0**; both sanitizer compiles/checks **exited 0**; bogus selector exited **2**, as asserted. Actual native command-exit ledger SHA-256 `f54c6f334ef2fa9fc9a9ad145eb4b027bcd63e8ffec060b3ce7b405fe074907d`. Tested standalone C source SHA-256 `f60c39203a8ae6e7d822c848129c3bede651e11ae9c7af1914ca1cf0592075b6`, Git blob `4d390087f1ffe1be9ff82569b0fe0bc0703d1e70`. Reproduce from the repository root after fetching this isolated branch; full host logs from producer remain local, with the numerical results below copied from the final executable's stdout.

## Observed host-only results
| Synthetic length | Compiler | Baseline 480 median ± MAD (ns/sample) | Candidate 960 median ± MAD | Peak refill IMA decodes before → after |
|---:|---|---:|---:|---:|
| 705600 | GCC | 11.217 ± 0.502 | 7.607 ± 0.595 | 2048 → 1024 |
| 705600 | Clang | 20.958 ± 2.204 | 14.226 ± 0.672 | 2048 → 1024 |
| 1411200 | GCC | 17.809 ± 0.677 | 11.880 ± 1.110 | 4096 → 2048 |
| 1411200 | Clang | 31.188 ± 1.046 | 19.892 ± 1.455 | 4096 → 2048 |

For 65,536 reverse-output samples, `n=705600` counts **164,928 → 98,368** total on-demand decoder calls; `n=1411200` **297,088 → 163,968**. All reference sample outputs and checksums matched baseline versus candidate. At `n=176400`, peak refill remains 512 and no speedup is established. These numbers are **a simulated source-equivalent host algorithm**, deliberately adverse reverse playback, not actual SMK hardware, XRUN incidence, full-render CPU-load improvement or everyday sample distribution. Nine-trial dispersion matters.

## Memory tradeoff and blocking gates for merge
Each checkpoint holds one 16-bit predictor plus two bytes (4 B). Changing 480 → 960 adds **1,920 bytes per bank × 2 banks = 3,840 B** to `breaks_t`'s firmware pool. The timing and correctness benefit is real in the host model, but the net model-specific code/RAM/POOL headroom is **NOT RUN / UNKNOWN**, not PASS. A larger checkpoint table also increases cold loop-loading setup bookkeeping, though decoded sample count is unchanged; this run did not measure project-load wall time.

**Next consumer — Hourly Build**: compile this exact producer PR SHA **BUILD_ONLY** in the pinned real JieLi V1.2.1 Elite and Pro DEV configurations with controllers; report both actual memory/flash/pool budgets. Neither prior QA nor Pages/owner hardware proof is required merely to compile draft code. **Independent QA**: rebuild and run this test against the exact PR branch, run native `tests/host/breaks_test.c` and applicable existing BREAK/phase/NoteOff regressions, validate worst-case short and long sample lengths and full source integration, memory tradeoff, no sample/slot changes. **Sole Safe Merge** decides reviewed NIGHTLY integration. No binary produced, installed, flashed or published by Performance Engineering.

This is an experimental performance tradeoff: do not advance to firmware NIGHTLY until QA/budget gates are actually passed. The producer does not change protected NIGHTLY, production main, gh-pages or the separate browser-only repository.
