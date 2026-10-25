# BF-005: queued old-source MIDI clocks accelerate a new source after disconnect

**2026-10-23 | Severity: medium | AUDIT ONLY — UNFIXED**

On current main `c9718ae833eb64a5191c350922593df8a36017c9`, `firmware/src/seq/sequencer.c:744-750` increments a shared `ext_pending` for Clock F8. `seq_clk_drop` at lines 792–795 clears only `clk_lock`, not those queued ticks. `firmware/src/app/engine.c:663-665` drains multiple MIDI messages before `engine.c:915` applies `seq_advance`. Hence buffered USB ticks are counted after a USB disconnect, Bluetooth Start and new Bluetooth Clock. The new source's 909 playback phase advances too early.

**Executable witness:** `tests/host/bugfinder_clock_handover_test.c` (Git blob `cf877263dc3ba8e987bdfa59a09ac7c1cf603b33`). Build with `cc -O1 -g -fsanitize=undefined -fno-sanitize-recover=all -Wall -Wextra -Werror -o /tmp/bf005 tests/host/bugfinder_clock_handover_test.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c -lm` and run `/tmp/bf005`. The test intentionally is not wired into the passing suite while the firmware bug is unresolved.

Exact current-main SHA-matched source produced (GCC UBSan and Clang ASan/UBSan, both exit **1**):
```
PASS old_pending=0: step 2 followed the 7th fresh Bluetooth clock
FAIL old_pending=3 fresh_ble_tick=4 expected_909_steps=1 observed=2 ext_ticks=0
```
Expected after new Bluetooth Start: second 909 step only on seventh fresh Bluetooth clock, regardless of old USB pending ticks. Clearing `s->ext_pending` in a **scratch-only** owner-matching `seq_clk_drop` makes both cases pass (both compilers, exit 0), and prior `clk-lock` host regression still passes. This supports root cause; NOT a checked firmware fix. No Elite/Pro builds, source-wide second review, flash/CRC/budget/controller or physical tests were performed.

Source blob identities were verified individually against current main: `sequencer.c` `83b03f73139d8e679c94526c66b390b6e5bc2f39`; `sequencer.h` `9bbddbf55438247469816625bd0b07a865aa0dcb`; `pattern.c` `b6b667d023a912b13de8539168c61928182ffa23`; `pattern.h` `9a0703418aae95b14f1b70905cfac916da38d7dd`; `tb3po.c` `63018311bf3bb86d6a3638f4935ae9cc9dec0cb2`; `tb3po.h` `299225dfe4cb0fa847dc0fd2aa1958d5b93f2f1c`. Current main and gh-pages `dcb41f2ead9e2166ec3dddcd658e445d22a5fac9` were unchanged. Distinct from open BF-004 Note Off ownership defect. No duplicate current open PR found from title search.

Additional current-main blob-matched host checks: sequencer, clk-lock, Projects JS (29 cases), Projects page (7 cases), and Breaks JS all PASS. The harness explicitly SKIPPED the missing Pro target package: this is not an image or device PASS. Library /LoopyX0X37 was listed, Index/Tools and Stage4 reports read. Installed skill listing was empty; equivalent Library Project skill read. Active and completed Project thread summaries retrieved but not all ten complete transcripts. No firmware, main or gh-pages edits, install or stable publication. **No new user-visible improvement this run.**
