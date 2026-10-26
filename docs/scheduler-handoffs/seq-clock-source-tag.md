# Sequencer clock: reserved MIDI source tag can capture the clock

Source: `gobbajogger/X0X37-for-Loopy-Pro` firmware `nightly` at `c9718ae833eb64a5191c350922593df8a36017c9` (tree `715fbd98d427c8ddd6b64d1b923e9d1fb8ae64ea`).
Scope owner: Sequencer Clock. Producer branch: `lane/seq-clock-reserved-source-tag-20261023-1226`.

## Reproduced boundary defect

`firmware/src/app/plat.h` defines a 2-bit `PLAT_MIDI_SRC`: 0 USB, 1 Bluetooth, 2 TRS; 3 is unassigned. The current `seq_clk_take` accepts origin 3 for MIDI Clock/Start/Continue. On an idle clock it sets `clk_lock=4`, so legitimate USB/BLE/TRS clocks are discarded until timeout. The uint32 API also narrows `src+1` to uint8: origin 256 aliases USB owner 0, allowing a bogus tick through an existing ownership lock. These are injected-invalid-origin host reproductions; not reports of a confirmed physical device sending tag 3.

Guard origin `src>2` for clock/transport/Song Position only, before narrowing. Valid origins and nontransport MIDI (notably Note-Off) retain their existing behavior. No BLE input parser, storage, UI, installer or mixer changed.

## Executed native reproduction and checks

The checked, committed `tests/host/clk_source_tag_test.c` byte identity equals local Git blob `5adda4a77e07614038e64ce8a96987271dc6d0a3` (SHA-256 `488519c3298400f98d5a7ad95528acc0dfba69f525176eeb08bb5c1e25ef07e8`). It was compiled against an extracted `seq_clk_take` and `seq_midi_rt` harness derived from base source blob `83b03f73139d8e679c94526c66b390b6e5bc2f39`, not a complete checkout; `PATCH_GUARD=0` reproduces the original, `PATCH_GUARD=1` adds the checked guard. Native Linux x86_64.

- Original GCC (`-std=c11 -O2 -Wall -Wextra -Werror -DPATCH_GUARD=0`): **82/137 failures**, exit **1** (expected red).
- Patched GCC (`-DPATCH_GUARD=1`): **137/137 PASS**, exit **0**.
- Patched Clang `-fsanitize=address,undefined`: **137/137 PASS**, exit **0**.
- Patched GCC `-fsanitize=undefined -fno-sanitize-recover=all`: **137/137 PASS**, exit **0**.

See `docs/scheduler-handoffs/seq-clock-source-tag-test.log` and JSON for exact invocations and immutable local raw-log hashes. Covered AUTO and EXTERNAL, 0/1/2 valid, reserved 3, overflow 255/256/MAX, F8/FA/FB/FC/F2, persisted owner, Note-Off passthrough, INTERNAL mode.

**Not run:** complete Git checkout `tests/run_tests.sh clk-src-tag clk-lock sequencer`; local shell `git ls-remote` failed DNS, while connected GitHub exact source/blob/PR reads work. Native target Elite/Pro builds and physical devices not tested. This is **EXTRACTED HOST CHECKED — EXACT-TREE QA PENDING — DEVICE NOT TESTED — NOT INSTALLABLE**.

## Handoff

**Independent QA:** fetch this branch's exact remote HEAD; compile and run `clk-src-tag`, `clk-lock`, `sequencer` on the complete source, check compiled test and guard together, and assess stacked clock patches #35, #41, #48, #55, #65, #72 and Safe Merge #70/#71 for collisions. **Sole Safe Merge:** integrate only after independent exact-tree QA. **Hourly Build:** owns subsequent real pinned Elite/Pro firmware/controller qualification; never treat extracted host test as a DEV package.
