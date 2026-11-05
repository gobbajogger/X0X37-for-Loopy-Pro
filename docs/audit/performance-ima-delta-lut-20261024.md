# Performance Engineering — IMA exact delta LUT candidate (2026-10-24)

## Scope and policy
Original decoder Git blob `9ba16e4fe9f0cb643c95a68d540042ab2581d4de`, candidate LUT blob `d1b85b49effdc7997043c837a92c3f6d523a80b5`.
This change computes the mathematically exact IMA increment using multiply/shift and an 8×8 correction table indexed by `(step & 7, code & 7)`, retaining identical predictor saturation and index bounds. It affects shared BREAK and 909 decoding; it does **not** change SMC/MIDI/clock/OTA/backup code.

## Actual native host measurements (x86-64 proxy; NOT target CPU/ISR)
Workload order alternated across five runs, each containing nine timed trials and four deterministic ADPCM patterns; median of within-run medians shown (ns/nibble).
Compiler | prng baseline → LUT | all-00 | all-ff | alternating |
---|---:|---:|---:|---:|
gcc | 3.564 → 3.405 | 3.565 → 3.351 | 3.580 → 3.355 | 3.532 → 3.345 |
clang | 3.665 → 3.200 | 3.720 → 3.157 | 3.636 → 3.257 | 3.681 → 3.170 |

- GCC/Clang candidate stand-alone decoder optimized compiles and 91,168 exactness comparisons per configuration exit 0.
- GCC/Clang ASan+UBSan decoder reference/regression compiles + executions exit 0.
- 3,193,789 exact decoder/reference and correction-algebra comparisons per configuration across nine sample lengths (64 through 2,116,800); 4 configurations exit 0.
- Whole actual `breaks.c` renderer source from Git-matched R18 compiler capsule: 13 lengths, 4,705,792 rendered frames, GCC/Clang optimized+sanitizers, outputs byte-for-byte identical with PCM SHA `fe7272407f98fb90c0c892fdeaeb6b7a4e788454483fc8c0ce47e88da06075a6`; read-only phase/slice/key release proxy (not real MIDI transport).
- Malformed command negatives expected exit 2. Incorrect first `mulcorrect` experiment failed its isolated check and was discarded; packed8/maskbit pass exactness but exhibit weaker/negative host throughput.

## Actual native target resource A/B (scoped R18 projection, NOT new PR HEAD)
Both model compiles used verified JieLi Clang 4.0.1 archive SHA `ed379c48...`, the same three pinned AC79 V1.2.1 SDK bytes, `BOARD=smk37 MODEL=elite|pro X0X_DEV_PADCAL=1 bash build.sh --board smk37`; **all four** table64/packed8 model commands exit 0. Baseline R18 is genuine prior build and was reused, not rebuilt.
Metric | Prior exact R18 baseline | table64 A/B projection | packed8 A/B projection |
---|---:|---:|---:|
App image bytes | 547052 | 547100 | 547036 |
App code-area free | 5620 | 5572 | 5636 |
RAM .data+.bss used | 97844 | 97844 | 97844 |
Pool free (minimum 8192) | 8324 | 8324 | 8324 |
Controller flash headroom | unchanged historical 44 | not rebuilt | not rebuilt |
Built PRIVATE packages (table64) have model IDs SMK-37 Elite_900 / SMK-37 Pro_900, 651396 / 651392 bytes. Read-only nested package/model/CRC integrity positives and deliberate invalidity rejections: 4 positive model packages and 28 invalid negative rejections across the two variants.

## Acceptance, nonclaims, and next owner
Source can be considered independently reviewable after a Git owner-lane commit. Exact new PR compiler inputs have **not** been compiled here: compiler evidence is for independently tree-verified R18 compiled inputs with only the target header replaced, which is NOT the same as a complete future new branch root. Hourly Build must compile actual new PR SHA for both models. Independent QA must inspect current PR source. No code was integrated into the first NIGHTLY candidate, no public firmware/controller was hosted, no hardware test was performed and no production claim exists.

The 64-entry correction table has an x86 host throughput benefit but a small +48-byte embedded app image cost on the R18 projection; no target CPU or XRUN improvement is proven. Safe Merge must keep optional performance separate until the first safe owner-installable NIGHTLY is qualified.

Detailed hashes, commands, model resource numbers and negative results are in the paired JSON evidence and private log ZIP.
