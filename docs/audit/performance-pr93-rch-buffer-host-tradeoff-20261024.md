# BREAK reverse-cache size versus measured decode cost — Performance Engineering R7

**Decision: keep `BRK_RCH=512`.** No firmware, controller, installer, or production branch changes from this experiment. Reducing `BRK_RCH` to 256 or 128 shrinks `sizeof(breaks_t)` by 1,024 or 1,536 bytes on this **host**, but substantially increases measured long-loop reverse-render compute cost.

## Git-exact source and isolation

- Repo `gobbajogger/X0X37-for-Loopy-Pro`, owned non-draft performance PR #93; source HEAD `1074d667c106472bb3f05db571abb4b9fea13ceb`; root `4f6c4734f20a33d79e248860622ef77689008fa7`; **different** firmware subtree `a87a83e15116422eed84528d95f52ddef5029927`.
- Git blob identities: `firmware/src/dsp/breaks.c=0fcdbe65b674dea3becb26fc265cb2514908f1a5`, `breaks.h=6cb71d336c2c90fdc60a9594c2356f3a4379c264`.
- Inputs are exact source-projected C/H plus Git-exact dependent firmware files; experiment changes **only the buffer-size macro** in temporary copies, never real branch firmware. `BRK_NCK=480` and mandatory 8KiB target free-pool reserve remain untouched.

## Positive and negative host validation

- `bash tests/host/run_breaks_renderer_adversarial.sh` in each RCH=512/256/128 temporary source copy: 3 scripts EXIT **0**. Each runs GCC+Clang optimized and ASan/UBSan, 20 deterministic renderer scenarios and 2,334,720 frames/configuration. All 12 output the same PCM SHA256 `6a29c3d7296ed6c6fdc8ac573e9199d8c009befb2e1b5ced20d32de5dcb41de6`; invalid-selector expected exit 2, corrupted PCM rejected. This covers renderer phase, macro key-up, direction, loop replacement, stop/restart, **not MIDI transport NoteOff**.
- `python3 tests/host/replay_rch_sweep.py --trials 2 --skip-adversarial` on exact source projection: EXIT **0**, six GCC14.2/Clang17 `-O2 -std=c11 -Wall -Wextra -Werror -pedantic -ffp-contract=off` compiles and 48 trial commands EXIT 0, 1,048,576 rendered frames/trial; all PCM FNV64 hashes equivalent within each workload.
- Same runner with one trial at actual prospective repository test-file paths: EXIT **0**, 24 more samples.
- The earlier five-trial unbounded orchestration exceeded the container timeout after 86 recorded successes. It is **NOT PASS**. An independently checked 54-observation, balanced first-three-trial reverse subset is reported below; samples outside that balanced subset were excluded.
- Forward control: GCC+Clang 3 trials per buffer (18 total) EXIT 0, same synthetic audio hash.

## Measurements (x86_64 synthetic host proxy, median first three balanced trials)

| Loop length (ADPCM samples) | Host compiler | RCH 512 | RCH 256 | RCH 128 |
| --- | --- | ---: | ---: | ---: |
| 4,096 | gcc | 5.611 ms | 5.727 ms (+2.07%) | 5.777 ms (+2.96%) |
| 4,096 | clang | 6.558 ms | 6.206 ms (-5.37%) | 6.015 ms (-8.28%) |
| 176,400 | gcc | 18.885 ms | 23.455 ms (+24.20%) | 26.893 ms (+42.40%) |
| 176,400 | clang | 20.863 ms | 26.722 ms (+28.08%) | 32.920 ms (+57.79%) |
| 2,116,800 | gcc | 556.414 ms | 873.358 ms (+56.96%) | 1616.094 ms (+190.45%) |
| 2,116,800 | clang | 740.490 ms | 1121.981 ms (+51.52%) | 1879.245 ms (+153.78%) |

Host `sizeof(breaks_t)`: RCH512 **6,296 B**, RCH256 **5,272 B**, RCH128 **4,760 B**. No claim these are target static RAM deltas; target struct layout and build budgets can differ. The longest reverse loop (2,116,800 samples) is approximately **52–190% slower** at reduced cache sizes. Short 4,096-sample Clang cases are sometimes faster; the workload-dependent regression rules out a universally favorable reduction. Timings include PCM hashing and host overhead, not target ISR timing; no audio XRUN or hardware result claimed.

## Exit gates and next owner

New files only: native host microbenchmark and replay script, factual audit report and JSON handoff. No changes to `firmware/`, `ble/`, `assets/`, `build.sh`, model build inputs or toolchain pins; pre-existing exact-compiler-input Elite and Pro DEV BUILD_ONLY proof for PR #93 (8,308 bytes pool free against 8,192 minimum) is **historical**, not newly executed by this experiment. No new target compilation is appropriate for a **rejected** throwaway macro variant. The full PR root still needs sole Independent QA; source-complete is not independently accepted. Data Safety's current OTA NO-GO and the sole first candidate #103 are completely separate; never insert optional PR #93 into #103 before the smaller candidate qualifies.

**Next consumer:** existing Independent QA, independently inspect this exact source/test delta and continue full-root PR #93 checks when capacity permits. Safe Merge is the sole firmware integrator. The first NIGHTLY candidate is not qualified/published by this research. Physical owner observations UNKNOWN.
