# Task C isolated regression receipt, 2026-10-30

## Identity
- Repository: `gobbajogger/X0X37-for-Loopy-Pro`
- Starting source handoff: `ae0a62e3774545d4dc7544d481b0d9da10b26055`
- Branch: `chat/port-ui-popup-and-break`
- Native state-machine test corresponds byte-for-byte to these GitHub blobs:
  - `firmware/src/app/smc_page_peek.h`: `59696e088e7a6bea177a812fef0e84d3f25d0958`
  - `tests/host/smc_page_peek_state_test.c`: `75de3d66e862aad89d8ff627d57314309626484e`
  - `tests/host/smc_page_peek_state_test.sh`: `aafdba165310018c93e56a1ea59728dbb9bc45e8`
- Production integration `firmware/src/app/smc_mixer.c` updated on branch in `cfc32c183826da55cbf33fd91ce3d804d83ea425` (GitHub blob `9a2a1784fac5b947305435410dda86cbc3f2a8c4`).
- SDK and host build closure: not established for this exact current branch; the pure header tests do **not** compile `smc_mixer.c`.

## Actual Linux x86-64 commands
Host compiler: GCC 14.2.0; Clang 17.0.0. All five commands ran from `/mnt/data/task-c-work`, which contains exact blob-identical **focused test files**, not a complete Git checkout.

| Check | Command | Actual outcome |
| --- | --- | --- |
| GCC C99 `-O0` | `sh tests/host/smc_page_peek_state_test.sh` first compile/execute | PASS, 31 checks, 0 failed |
| GCC C99 `-O2` | same script's second compile/execute | PASS, 31 checks, 0 failed |
| Clang C99 `-O0` | `CC=clang sh tests/host/smc_page_peek_state_test.sh` first compile/execute | PASS, 31 checks, 0 failed |
| Clang C99 `-O2` | same script's second compile/execute | PASS, 31 checks, 0 failed |
| GCC ASan + UBSan | `gcc -std=c99 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Ifirmware/src tests/host/smc_page_peek_state_test.c -o /tmp/smc_page_peek_san && ASAN_OPTIONS=detect_leaks=1 /tmp/smc_page_peek_san` | PASS, 31 checks, 0 failed |

**Total: 155 focused host assertions passed; 0 failed.** These are five compiled state-machine executions, **not** 155 independent full-system tests. Runner registration was added to `tests/run_tests.sh` but full-suite invocation was **NOT_RUN** against this head.

## Exact current-head limits
- Full `tests/run_tests.sh`: **NOT_RUN**. The connected GitHub API successfully read the relevant remote tree and blobs, but no complete current-head checkout was materialized into the Linux runner. The mounted bootstrap's Git bundle is a historical 2026-10-23 snapshot (`f58d62f0...`), not the Task C branch tree. The shell's GitHub API DNS probe failed with `curl: (6) Could not resolve host: api.github.com`. A focused test directory is **not** a complete checkout and must not be represented as one.
- Elite target SDK V1.2.1 build: **NOT_RUN**. The mounted JieLi archive SHA-256 is `ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19`. The Library private V1.2.1 input kit SHA-256 is `2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805`, containing pinned selected SDK files, **not the full SDK tree**. The accessible full-size vendor SDK archive identifies as V1.2.0 (SHA-256 `162a38264c0d06025b3d0583f9a4cc305d91a015872b5e0919c457cb288d0c15`). It cannot be relabeled V1.2.1. Full current-head source closure is also absent.
- Pro target SDK V1.2.1 build: **NOT_RUN** for the same two exact dependencies.
- Production main SMC/UI regression: **NOT_RUN**. Existing host `smc_ui_test.c` asserts nonmutating full-screen overlays. The proposed native mapped page intentionally changes `ui.view`; those older expectations require conscious reconciliation and real rerun before any green report.
- Hardware, controller, installation and publication: **NOT_RUN / NOT AUTHORIZED**.
- No source changes were made in #734 or its branch. No CI/Actions, `.github/` edits, binaries, `wobble.c` edits or network Git commands were used.

## Integration freeze
The owner froze #734 pending a posted exact fixed-head full host suite + Elite + Pro result of **0 fail / 0 skip**. A draft Task C PR and scoped test pass do not lift that gate. Do not merge this candidate into `integration/all-in-one`, and do not claim native firmware or instrument acceptance.

## Proposed visual
`docs/mockups/smc-mapped-page-peek-before-after.svg` illustrates current full SMC control screen and the proposed temporary real MIX PARTS page. It is a *mockup*, not a firmware screenshot or verified physical LCD image.
