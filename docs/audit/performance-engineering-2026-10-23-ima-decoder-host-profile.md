# IMA ADPCM decoder: exact-source native performance baseline (2026-10-23)

Role: LoopyX0X37 Performance Engineering. New benchmark and correctness regression ONLY. There is no firmware speedup, device timing measurement, build or installation.

## Checked source and ownership

Repository gobbajogger/X0X37-for-Loopy-Pro, firmware nightly commit 58b5b45a2dd90581de7a674c331ca5cbe8a67c7e, tree 71b759dd206d844cceed9e40ae68966142720e9c. The unchanged production decoder firmware/src/dsp/ima_adpcm.h matches exact Git blob 9ba16e4fe9f0cb643c95a68d540042ab2581d4de. It serves 909 cymbal and BREAK IMA sample streams. This lane adds tests/host/ima_decode_perf_test.c, not firmware, build, SMC, BLE, clock, sequencer, data-safety or installer changes.

Existing tests/host/drum909_adpcm_test.c measures a sample-specific ride stream; this new standalone profiler adds exhaustive nibble/index/predictor combinations, adversarial streams and nine-trial dispersion. Synthetic inputs contain no proprietary SDK, private DSP or user samples.

## Executed native checks

Host: Linux x86_64. GCC 14.2.0 and Clang 17.0.0. Both compilers built at -std=c11 -O2 -Wall -Wextra -Werror -pedantic (exit 0). Both optimized executables completed --check and --bench (exit 0). Both compilers also built with -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined and completed --check (all compile/check exits 0). Each compiler's --bogus invocation returned exit 2, verified by a wrapper that returned exit 0, so an unknown selector cannot falsely PASS.

All four --check runs returned: CHECK PASS: 91168 nibble comparisons, 89 step states, 16 codes, 9 predictor boundaries, both packed nibble positions, 4 streams.

The 91,168 checks comprise 89 x 16 x 9 x 2 = 25,632 isolated boundary comparisons plus 4 x 16,384 = 65,536 streaming comparisons. Every sample, predictor and decoder-step index was checked against independently expressed arithmetic. Sanitizer runs were correctness checks, NOT timing measurements.

Exact executed command record (local test-commands.log) SHA-256 ce8f96fd3c6703d366bd3f1916cf357565bf84549274327738427684d653bc88. Tested C source SHA-256 c57a1efa59b918b289c02d7604a03864090b46a62dbe5df37b1c5140dce45ec6, Git blob 68c6cddaedad409a1fd8c8d00196968668fa0588. Commands reproducible from source-repo root:

    gcc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -o /tmp/ima-perf tests/host/ima_decode_perf_test.c
    /tmp/ima-perf --check
    /tmp/ima-perf --bench
    clang -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fno-omit-frame-pointer -fsanitize=address,undefined -o /tmp/ima-perf-sanitized tests/host/ima_decode_perf_test.c
    /tmp/ima-perf-sanitized --check

## Actual HOST-only measurements

Deterministic seed 0x93d43fa5. For each workload: nine trials, 1,572,864 decoded nibbles per trial, median nanoseconds per nibble and median absolute deviation (MAD). These are x86-64 Linux host proxies, neither SMK-37 pi32v2 cycle counts nor a before/after firmware speed claim.

| Synthetic byte workload | GCC median ± MAD (ns) | Clang median ± MAD (ns) |
| --- | ---: | ---: |
| PRNG | 3.397 ± 0.078 | 3.288 ± 0.063 |
| All 00 | 3.330 ± 0.054 | 3.322 ± 0.096 |
| All FF | 3.233 ± 0.027 | 3.291 ± 0.060 |
| Alternating 0F/F0 | 3.379 ± 0.066 | 3.494 ± 0.033 |

Raw output, GCC (gcc-bench-final.log SHA-256 6ec991109a38c050f54903d5b9b4f5a58031ae6030cb2d9cc84c05191c7bfd82):

    prng             nibbles=1572864 trials=9 median=3.397 min=3.239 max=3.676 MAD=0.078 checksum=51284949879
    all-00           nibbles=1572864 trials=9 median=3.330 min=3.202 max=3.548 MAD=0.054 checksum=51539607552
    all-ff           nibbles=1572864 trials=9 median=3.233 min=3.177 max=3.413 MAD=0.027 checksum=26969272
    alternating      nibbles=1572864 trials=9 median=3.379 min=3.277 max=4.131 MAD=0.066 checksum=4741446232

Raw output, Clang (clang-bench-final.log SHA-256 ddc48761817794b9a987d710ccd4ad27c6dd30cc3561ca3a15ca85c08164159d):

    prng             nibbles=1572864 trials=9 median=3.288 min=3.203 max=3.421 MAD=0.063 checksum=51284949879
    all-00           nibbles=1572864 trials=9 median=3.322 min=3.226 max=3.899 MAD=0.096 checksum=51539607552
    all-ff           nibbles=1572864 trials=9 median=3.291 min=3.194 max=3.489 MAD=0.060 checksum=26969272
    alternating      nibbles=1572864 trials=9 median=3.494 min=3.304 max=3.633 MAD=0.033 checksum=4741446232

## Practical implications and next gate

The shared BREAK/909 decoder can now be stress-tested against independent reference values and measured across predictable workloads before any behavior-preserving optimization is proposed. Experimental replacement arithmetic explored separately in this run yielded small, compiler/workload-dependent host timings, not a defensible real target speed win; no such firmware change was committed. No changes to MIDI Note-Off, clock phase, slot/sample preservation, flash/RAM/pool or audio playback occurred. Hardware deadlines, XRUNs, real sample distributions and future code-size regressions remain unmeasured.

Next consumer: Independent QA. Reconstruct the exact branch and independently compile/run this benchmark and its negative paths. Safe Merge alone may later integrate reviewed, host-only regression source into firmware nightly; no per-model firmware build or device hardware gate is claimed for this test-only change. Do not interpret the draft scheduler evidence contract PR #78 as merged.
