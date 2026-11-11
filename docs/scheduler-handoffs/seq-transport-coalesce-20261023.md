# Sequencer Clock — coalesce pending Start / Continue requests

**Owner:** Sequencer Clock specialist (source-only). **2026-10-23, native Linux x86_64.**
**Git base:** `gobbajogger/X0X37-for-Loopy-Pro nightly@58b5b45a2dd90581de7a674c331ca5cbe8a67c7e` (tree `71b759dd206d844cceed9e40ae68966142720e9c`).
**Original sequencer blob:** `83b03f73139d8e679c94526c66b390b6e5bc2f39`.
**Patched sequencer blob:** `6f0484488d64661b24ec64dbe36fed96fd08bfdf`.

## Observed defect

Calls to `seq_start()` and `seq_continue()` before the next audio advance set separate sticky flags; `seq_advance()` then calls `do_start()` **twice**, sending Start **and** Continue while following internal clock. When the last user input was Continue, the earlier Start resets the 909 playhead instead of preserving phase. With input arriving within one block this can double-fire transport and disturb synchronization.

## Scoped change

- `seq_start` clears a pending Continue, and `seq_continue` clears a pending Start: **last Start/Continue wins** within one block.
- Leave `seq_stop` intact. A later Stop still cancels an earlier Start/Continue; Stop then Start still executes in order.
- Do not alter MIDI clock ownership, BLE/SMC routing, note delivery, storage or persistent data.
- Register one new `transport-coalesce` host regression covering both gesture orders, four-message bursts, phase preservation, Start↔Stop ordering, 303 MIDI Note Off, real MIDI Start/Continue via a clock owner, and no external-clock echo.

## Baseline synchronization

While this run was in progress, sole Safe Merge merged docs-only PR #60 into firmware `nightly`. Updated HEAD is `58b5b45a2dd90581de7a674c331ca5cbe8a67c7e`, tree `71b759dd206d844cceed9e40ae68966142720e9c`; its only changed path is `docs/rules/firmware-nightly-promotion.md`. The affected sequencer, helper and host-test Git blobs remain **byte-identical** to the earlier `c9718ae833eb64a5191c350922593df8a36017c9` reviewed checkpoint, so targeted native test evidence is unchanged. The new isolated lane will branch from the updated HEAD and will not incorporate another worker's unmerged source.

## Actual local checks and provenance

The owner-provided historical source-bootstrap archive `LoopyX0X37-Project-Sources-Bootstrap-20261023.zip` (SHA-256 `ea173ec3afd6adea9219622652ed0dcafe48f8a6cba43395c4dab62895928319`) was unpacked **only as a local host-test workspace**. Its historical repository tree is `152ca944e9cd3d0913906a1d3fa9d8d2826ea639`, not current NIGHTLY; no complete current checkout is claimed.

The compiled `firmware/src/seq/sequencer.c` source was independently verified byte-for-byte by Git blob against the **actual nightly** `83b03f73139d8e679c94526c66b390b6e5bc2f39`; `sequencer.h`, `pattern.c`, `tb3po.c`, `seq_test.c` and `clk_lock_test.c` also match current nightly Git blobs. Patched source and committed regression have exact locally tested Git blob identities. The historical `tests/run_tests.sh` is NOT byte-for-byte the current runner; its local registered selector was executed, and the same one-line selector was independently applied to the current remote runner for this PR. **Full exact-PR-tree execution is pending independent QA.**

1. Before patch: compiled source + new standalone test with GCC  `-O2 -Wall -Wextra -Werror`; exit **1**, **10 of 27** checks fail. Raw output `docs/scheduler-handoffs/seq-transport-coalesce-20261023-before.log` (SHA-256 `95dd0246237c897ac935054c0965c4559e1cfcd7c2752f2373e7fda19b60cd8f`).
2. Patched GCC same flags: exit **0**, 27/27 pass; patched Clang ASan+UBSan: exit **0**, 27/27 pass; patched GCC UBSan: exit **0**, 27/27 pass. Identical raw success line in `docs/scheduler-handoffs/seq-transport-coalesce-20261023-after.log` (SHA-256 `a56ff5790972d19146bd06a163c968ad75f8656ee1c13eb0c619b330cef3f158`).
3. Existing standalone sequencer host regression: exit **0**, `seq: ok`; standalone `clk_lock_test`: exit **0**, `PASS clock lock (MID-15): 0 fail`. Raw local log SHA-256 `632a3a6b86e8b54c739cbcef86ee9fcf9cf37230e638e5f108a3def776155ff1` and `0f5b892adc95fc5a3e3a957f0142dd684e4c8b1624378ebd0bb3130ae68b4a34`, respectively.
4. `X0X_TEST_JOBS=1 sh tests/run_tests.sh transport-coalesce sequencer clk-lock`: exit **0**, three named tests passed; raw output `docs/scheduler-handoffs/seq-transport-coalesce-20261023-suite.log` (SHA-256 `f93de843b238e8a2d5c912a9ea5306ac5f6c575139ab1fa408268f1c271f9faa`). The archived local runner differed from nightly as above.
5. Elite/Pro native firmware builds, controller/package hashes, RAM/flash budgets, actual device and full PR-tree tests **NOT RUN** in this source specialist cycle. No binaries, JieLi/SDK/DSP, user sample bytes or site content produced.

## Git handoff / gates

**Independent QA:** re-fetch exact PR HEAD, run `X0X_TEST_JOBS=1 sh tests/run_tests.sh transport-coalesce sequencer clk-lock` against complete exact PR checkout, scrutinize host-gesture and MIDI Start/Continue ordering, and compare collisions with open clock/seq PRs #35, #41, #48, #55, #65, #72, #80 and sole Safe Merge #70. **Sole Safe Merge** then owns conflict reconciliation/integration into firmware nightly (do not assume any unmerged PR is present in nightly). **Hourly Build** separately qualifies actual pinned Elite/Pro + matching controller and model/CRC/backup/negative/legal gates before owner-testable NIGHTLY publication. No physical owner PASS required for source review. This PR remains **DRAFT / scoped native host-checked / full-tree QA pending / DEVICE NOT TESTED / NOT INSTALLABLE**.
