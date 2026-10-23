# Nightly-source Safe Merge WIP — 1/32 recording and slot-archive identity (2026-10-23)

**Role:** sole firmware SOURCE integrator; **stage:** ISOLATED SOURCE WIP / TARGET BUILD AND INDEPENDENT QA PENDING / DEVICE NOT TESTED.
**Base integration parent:** `93af3b53f19330086e38b99d08ef3d834a75061b`, 3 commits ahead of original `nightly` branch `c9718ae833eb64a5191c350922593df8a36017c9`.
**Source owner inputs (unmodified):** sequencer PR #55 `181dac3296d247f6217f26c4e0cb2b7d1a6e5cbd` and Data Safety PR #56 `0d2705253882bff6b8554861abfe35504b775bb7`, each base `nightly`. Independent PR reviews were not submitted as of this integration cycle.

## Changed files and exact content

- `firmware/src/seq/sequencer.c`: contains parent’s existing clock handover, INTERNAL-owner reset, and stopped-owner pending-tick reset; applies PR #55 ceiling midpoint to `seq_rec_step` only. Parent source Git blob `2d3a68eddd0ffb335fc37bc7a023b01263faa92e`, combined content blob `51f1bea69f8116e4501f5b08784db9497c050aef`.
- `tests/host/seq_test.c` producer #55 blob `bfdadbe0c6058e7b498fa2f142dc67b095452492`: true three-clock 1/32 midpoint and wrap-around regression for 909, 808 and 303.
- `tools/x0x_slots.py` producer #56 blob `eddeb430724c5d4f04e4bf677e2efee5b05e2baf`: count/ordered 1-based slot-ID validation before restore planner loads archive bytes.
- `tests/x0x_slots_test.py` producer #56 blob `b4518c1bc7f81c21be4dd9b0f02ffcb29eaf66c9`: swapped, duplicate, skipped and inconsistent-count valid-CRC negative archives.
- This source-only integration handoff; no firmware binary/controller/SDK/DSP/sample attached.

## Native focused tests — executed locally

Runner: Linux x86-64, GCC and Clang `-std=c99 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all`; all ran in ChatGPT native container, **not GitHub Actions**.
1. Reconstructed sequencer source byte-for-byte from main-matching historical checkout and the exact three previous clock-owner fixes; `git hash-object` matched immutable remote parent source blob `2d3a68ed...` before applying PR #55. Merged sequencer SHA verified `51f1bea...`.
2. `gcc ... tests/host/seq_test.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c -o seq-gcc && seq-gcc`: **compile 0, execution 0**, contains all original sequencer checks plus PR #55 midpoint.
3. Equivalent **Clang UBSan** compilation and run: **compile 0, execution 0**. Both print `external 1/32 recording midpoint: 909, 808, 303 and 16-step wrap checked`, then `seq: ok`.
4. `python3 tests/x0x_slots_test.py`: exit **0**, including mock-data-preserving slot restore and four crafted archive outer-CRC-valid negative cases; `python3 -m py_compile tools/x0x_slots.py tests/x0x_slots_test.py`: exit **0**.
5. Negative baseline using exact pre-#55 parent C hash and producer #55 regression: GCC execution **1 EXPECTED**, six 1/32 early-advance failures. Negative baseline using exact pre-#56 main Python hash and producer #56 regression: Python execution **1 EXPECTED**, four invalid-archive acceptance failures. These are reproduced defects, not post-fix failures.
6. Owner-owned private toolchain ZIP SHA256 `ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19`, relative symlinks in archive root 44/44, running JieLi clang 4.0.1 binary SHA256 `c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f`; private pinned AC79 V1.2.1 input kit SHA256 `2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805`, `python3 kit.py verify` **11/11 PASS**. Both retained OUTSIDE Git.

## Precisely pending / NO-GO

- The native checked source is an **offline historical tree** with individually remote-Git-SHA-verified changed and compiled dependencies; its **complete tree is not** the official merged Git commit tree. `git ls-remote https://github.com/gobbajogger/X0X37-for-Loopy-Pro.git refs/heads/nightly` failed with DNS resolution in the native shell. GitHub connector permitted immutable blob reads and this isolated source write. Full exact-commit checkout/build/test remains pending.
- Prior integration parent `93af3b53` remains SOURCE WIP with separate unresolved producer/QA/SMC/BLE negative findings (including Hourly Build draft PR #54); no independent QA reviews for #55/#56. Review clock sequence with new midpoint, archive format provenance and all negative/error states against exact combined tree.
- **Hourly Build consumer:** native exact combined SHA (record actual new commit below) Elite/Pro DEV package plus matching `ctl.img`, verified JieLi SDK V1.2.1, DSP pins, SHA-256/CRC/model and code/RAM/flash/pool budgets, validated backup/readback/installer. This document is a Git-ledger handoff, not a direct message or a claim of a successful build.
- **Independent QA consumer:** actual remote branch diff, PR #55/#56 own evidence, full affected-source behavior and error cases; software checks only, not physical hardware.
- **Catalog Sync consumer:** source-only / DEVICE NOT TESTED / build pending status, NO install URLs for this combined SHA.
- Do not merge firmware `nightly` automatically; no production `main` promotion, no `gh-pages`, no binaries, no flash, no stable. Production requires attributable per-model owner physical PASS plus explicit production approval.
