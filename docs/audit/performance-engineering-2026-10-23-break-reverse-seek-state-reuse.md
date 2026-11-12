# Performance Engineering — memory-bounded BREAK reverse decoder state reuse
2026-10-23 UTC | **HOST MODEL CHECKED / DRAFT / NO CURRENT TARGET BUILD OR HARDWARE TEST**

## Scope and source identity
This supersedes the earlier PR #93 480→960 checkpoint patch at `74389d4bb4abc2f8a36b91bc16d1822b63264986`, which failed actual pinned Elite and Pro native builds: pool 339580/344064 B, **4484 B free** versus mandatory 8192 B (both actual exits 1). Source firmware NIGHTLY parent `58b5b45a2dd90581de7a674c331ca5cbe8a67c7e`, original `breaks.c` blob `1d3d692b6788d9d1f13901cd7ab13df506cdb72f`, `ima_adpcm.h` blob `9ba16e4fe9f0cb643c95a68d540042ab2581d4de`.

Restore `BRK_NCK=480` and reuse one saved IMA predictor/index before the previous adjacent 512-sample reverse-cache refill per voice. The saved state is consumed only at its exact position, replaced for a new checkpoint search and explicitly invalidated in `voice_start` on a new bank, voice or direction. Baseline/checkpoint decode, PCM, slices, sample storage and sequencer phase paths otherwise remain as before. **The source change has not been compiled or tested in the full firmware renderer.**

## Measured native host model (Linux x86_64, GCC 14.2, Clang 17)
The bench mirrors the authoritative seek/refill algorithm, compiles the **unaltered production IMA decoder**, and was executed independently using `-DBRK_PROFILE_NCK=480 -DBRK_SEEK_CACHE=0` (baseline) and `=1` (proposed). Exact commands and exits:
- GCC/Clang `-std=c11 -O2 -Wall -Wextra -Werror -pedantic -ffp-contract=off`, both variants compiled **exit 0**; each `--check` and `--bench` **exit 0** (four compiles, four checks, four benchmarks).
- GCC/Clang candidate `-O1 -g -Wall -Wextra -Werror -pedantic -fno-omit-frame-pointer -fsanitize=address,undefined` compile and `--check`: **exit 0** each; no triggered sanitizer.
- Both `--bogus` selectors: **exit 2 expected**, not a false PASS.
- Full reverse output compared exactly to independent PCM decoding at eleven sizes **64, 65, 511, 512, 513, 122880, 122881, 176400, 705600, 1411200, 2116800**; **2048 random seeks per size**. No sample mismatch or unexpected refill-bound violation.

Deterministic decoder calls for 65,536 descending output samples:

| Loop samples | Baseline | Proposed | Reduction |
| ---: | ---: | ---: | ---: |
| 705600 | 164928 | 131648 | 20.18% |
| 1411200 | 297088 | 197760 | 33.43% |

Nine-trial host median ± MAD, ns/sample (noisy CPU timing; neither a target CPU speedup nor measured XRUN):
- 705600: GCC **12.008±0.986 → 10.472±0.652**; Clang **24.509±1.429 → 21.442±1.904**.
- 1411200: GCC **19.843±0.984 → 13.207±0.103**; Clang **50.891±4.773 → 40.645±9.323**.

Worst *single* reverse refill decode count remains the existing limit (2048/4096/8192 for representative lengths); only repeated descending refills avoid replay. No gain is claimed for random-access or normal-device workloads. Test implementation is a **source-algorithm host model with an exact decoder and 480/512 constant shim**, not a current-repository full BREAK renderer test.

Native command ledger SHA256 `169ed5c727ce2c568d7c43d138cfb88aace8c07c0e874a01d9d2e2cc51735bf4`; host model source SHA256 `5f068b5b1225febdd0bf18840b1b99a399062656e7dcdaccbe174fab58603515`.

## Memory budget and consumer gates
One 4-byte checkpoint state and one 4-byte position per voice = **+8 bytes/voice, +16 B projected pool** (instead of +3840 B). Earlier actual **baseline** pool use 335740/344064 and 8324 B spare would project 335756 used/8308 spare if compiled layout is otherwise unchanged, only **116 B over the 8192 B reserve**. This calculation is NOT a new Elite/Pro target compile or RAM/flash/pool PASS. Firmware code size can also change; do not merge on this projection.

**Hourly Build (next)**: immediately BUILD_ONLY this NEW exact source SHA for Elite and Pro DEV with pinned JieLi V1.2.1 + matching controller. Record real compiler exit, updated pool/RAM/flash/code results and negative guards; previous failed 960 source does not certify this revision. **Independent QA**: use full current tree to run `tests/host/run_breaks.sh`, positive/negative click control, 22050/44100 source, random seeks, loop/bank/voice changes, reverse toggles, phase and NoteOff regressions; independent review before sole Safe Merge. No firmware binaries, user samples, vendor files, Pages, production ref or device were modified here.
