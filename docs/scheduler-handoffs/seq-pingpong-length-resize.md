# Sequencer Clock — 303 ping-pong length-shrink safety (2026-10-23)

## Reproduced issue

A live length edit can leave a playing 303 ping-pong part's existing `seq_track_t.pos` beyond the new `len`. The original `next_pos` bounce calculation returns `p - pp_dir`, which may remain outside the shortened range. Example: reduce 16-step part to 4 while at step 16 (`p=15`): next step is 15 (`p=14`), not step 4 (`p=3`). This makes removed steps sound for multiple beats; TIE lookahead can also point at an inactive step. Source: firmware NIGHTLY 58b5b45a2dd90581de7a674c331ca5cbe8a67c7e, sequencer Git blob 83b03f73139d8e679c94526c66b390b6e5bc2f39.

## Minimal source correction

On PINGPONG only, detect a runtime position beyond the new part length and immediately choose its nearest active boundary, resetting the committed ping-pong direction toward the active range. Lookahead (`commit=0`) does not mutate direction. One-step, normal bounce, forward/reverse/random modes remain unchanged. This does not edit MIDI transport, SMC, storage or sample content.

## Executed host evidence

Native Linux x86_64 real GCC and Clang. Source is the exact matching `next_pos` production function window, not the complete current firmware tree. The committed regression `tests/host/seq_pingpong_resize_test.py` extracts the function from `firmware/src/seq/sequencer.c` and compiles it with explicit host model stubs. Command: `python3 tests/host/seq_pingpong_resize_test.py --source source/firmware/src/seq/sequencer.c --cc <gcc|clang> --sanitize <none|ubsan|asan-ubsan>`.

- Original with same current regression: GCC exit 1, 79,427 failure cases, 99,498 passed expectations (current test detects out-of-range positions).
- Patched GCC plain, GCC UBSan, GCC ASan+UBSan, Clang plain, Clang ASan+UBSan: **each exit 0, 178,925 assertions pass**.
- Deliberately malformed/missing `next_pos` source window: exit 1 (fail closed).
- Python syntax compilation: exit 0.
- Detailed executed output counts and actual local log SHA256 hashes: `docs/scheduler-handoffs/seq-pingpong-length-resize-native.log`. Local raw logs are available in producer evidence ZIP.
- Complete exact-tree `sequencer`, `clk-lock`, Note Off integration, firmware Elite/Pro and controller builds: **NOT RUN**. Physical hardware: **DEVICE NOT TESTED**.

## Dependency/ownership and next consumers

Isolated change based on firmware NIGHTLY 58b5b45a2dd90581de7a674c331ca5cbe8a67c7e; no flattening of draft sequencer #35/#41/#48/#55/#65/#72/#80/#86/#90. Independent QA must execute this registered native regression and existing sequencer/clock/303 MIDI Note Off cases on an exact full PR checkout. **Hourly Build may immediately compile BUILD_ONLY Elite/Pro+controller from this draft SHA with pinned tools in parallel, without awaiting QA or merge.** Sole Safe Merge alone reconciles and integrates qualified sequencer changes to NIGHTLY. No Pages, publication, production promotion or hardware write authorized.
