# BF-017 — stale motion requests survive transport stop

**2026-10-24 | Firmware source defect | Producer: LoopyX0X37 Hourly Bugfinder | Evidence revision: BF-017/v1.**

**Owner-visible impact:** An automation-record knob movement queued immediately before STOP (or a project-motion replacement) can be applied to the next run, silently overwriting an existing per-step p-lock. A queued nonrecording knob-override request can likewise suppress the next playthrough's automation for up to one pass. This is a host-reproduced *reachable state*, not an owner-observed device failure. No matching live NIGHTLY firmware candidate or attributable physical feedback was identified in the empty NIGHTLY preview catalog; owner-device outcome remains **UNKNOWN**.

## Exact Git sources

Repository `gobbajogger/X0X37-for-Loopy-Pro`, firmware `nightly` base commit `58b5b45a2dd90581de7a674c331ca5cbe8a67c7e`, **full root tree** `71b759dd206d844cceed9e40ae68966142720e9c`, distinct `firmware/` subtree `19840d87388fb1f5734d9765bf8a2c0b7d3ebaaf`.

Before blobs: `firmware/src/seq/motion.c` `f94c93428153eaf6f42f25aee4c21cfb1e7cdd96`; `motion.h` `aca33934b0006fe2db2abe0a3048312ac79991f1`; `pattern.h` `9a0703418aae95b14f1b70905cfac916da38d7dd`; `tests/host/motion_test.c` `e147123792ef8da15c964412010258ffbbf549a7`. All four base blobs matched `git hash-object` on the native Linux runner before the test. No historical full-root checkout is claimed.

After blobs: `motion.c` **`305132e4a5a47b441e370e3ade15dced51279e6b`**; `motion_test.c` **`6c28ecc4e55671f27bcd2c0822b991aed2dcfeea`**. Remote branch and produced commit SHA must be checked independently before consumers use them.

## Root cause and bounded correction

`motion_req_rec` and `motion_req_hold` set `rt[k].rec_req` and `rt[k].hold_req`. `motion_release` restores applied values and clears previous gesture/hold counters, but failed to clear the two request flags. The next `motion_step` processes those old flags after restarting, as if the knob had just moved. Two assignments in `motion_release` now discard queued record/hold requests at the stop/load boundary. No persistent lane schema, knob mapping, clock, MIDI, SMC, sample flash, model identification, installer or controller code changes.

Regression extends the existing `smk`-independent `motion` host suite. It checks: stale recording does not alter saved lock 37; saved lock applies immediately on restart; stale nonrecording hold does not suppress lock 46; *new* post-stop recording and hold still work; releasing a genuine hold restores lock 25. Existing sweep, interpolation, pattern change, stop, reused-slot, capacity and single-touch checks remain in the same test file.

## Executed native tests (not CI)

Runner: Linux x86_64; GCC 14.2.0 and Clang 17.0.0, each at `-O0` and `-O2`. Every source unit compiled from exact Git-source blob headers on a local filesystem; this is **the complete real motion.c and motion_test.c host unit**, not a synthetic reimplementation. Source-blob integrity was checked with `git hash-object`. The shell command in each of four modes was:

```sh
cc -std=c99 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -g -o motion_test tests/host/motion_test.c firmware/src/seq/motion.c
ASAN_OPTIONS=detect_leaks=0 ./motion_test
```

`cc` here means explicitly `gcc` or `clang`; `-O2` was also separately replaced by `-O0`. On the **original motion.c** with the **new regression test**, all four compiler invocations exited **0**, all four test runs exited **1** as expected. Each reproduced *three functional assertion failures*: recorded lock 37 became 88, first playback remained 88, and held lock 46 remained suppressed at knob 99. Baseline run-output SHA-256 for each configuration: `6e69a590462957c131f2d4607ed76f5582ab4ef7b0d5a9894ad7a4aae48091e9`.

On the **corrected motion.c** with the same test, each of four compiler invocations exited **0**, all four test runs exited **0**, each ended `motion: ok`, and no ASan/UBSan diagnostic was reported. Fixed run-output SHA-256: `4d91e54dfc9a91a2628759cd5edde76cc8e11e1d9e9ce2c811cac7fd4827a7bb`. Each compile log was empty (SHA-256 `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`). Local output files are not public artifacts or target firmware.

**Scope limitation:** There was no complete exact-remote root checkout or `tests/run_tests.sh` invocation, full repo quickcheck, pinned JieLi Elite/Pro/ctl target build, RAM/flash/pool verification, independent QA, package-model/hash checks, browser negative test, HTTP-delivered firmware, or owner-operated physical-device test. Native GitHub access from shell failed DNS; connected GitHub API could read source blobs, but Library raw materialization was denied and not bypassed.

**Stage:** SOURCE COMPLETE / REAL HOST CHECKED / INDEPENDENT QA PENDING / TARGET BUILT_ONLY PENDING / NOT INSTALLABLE / DEVICE UNKNOWN.

**Next independent consumers:** Hourly Build may compile this exact producer PR commit for Elite/Pro and controller **BUILT_ONLY immediately** without a merge or QA wait. Independent QA should reproduce the new host regression, check stop/load/rapid restart ordering and whether project reset preserves expected live knob behavior. Sole Safe Merge owns firmware `nightly` integration after independent review. Catalog Sync and sole NIGHTLY publisher must withhold a selectable device-test package until all separate model, backup, integrity, licensing, immutable HTTP and prehardware gates pass. Production stays on `main` unchanged pending matching real-device PASS and explicit owner approval.
