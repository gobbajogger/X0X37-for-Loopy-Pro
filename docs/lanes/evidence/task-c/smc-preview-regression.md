# Task C SMC native-page popup regression, 2026-10-30

## Source and scope

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`
- Isolated branch: `chat/port-ui-popup-and-break`
- Exact source commit: `2a66c6ddac69f1a553283d8546aa30cf54cce8e4`
- Production code blob `firmware/src/app/smc_mixer.c`: `e05feb85bb8ef76eed76b92302336aeaf3665240`
- Test blob `tests/host/smc_page_peek_production_test.c`: `42f454d33bf5bd05c186210ae2e9e7e523dfdd11`
- The Linux test source is an exact copy of the seven SMC popup function bodies, verified after removing the extra blank line at the end of the GitHub extraction range: 3,131 UTF-8 characters, FNV-1a32 `888c84c2`. The test generator compiles these real production bodies with isolated model/UI stubs. **It does not compile the full firmware or the entire SMC source file.**

## Red, fix, green

The new regression starts with the native page peek open, then starts a non-eligible locked-control feedback overlay. Before the repair, the old peek remained active. Its timeout could restore the wrong screen while the newer control overlay was displayed. This was observed as `FAIL line 137` in `locked-overlay-red.log`, exit code 1. The repair calls `smc_peek_finish()` before creating the new full-control overlay, so the original UI context is restored and the earlier peek is retired. The regression now passes.

A separate negative mutation changed the expected delay page for fader 7 to the wrong FX page. After removing an accidental local shadow include and compiling only the generated production snippet, the test failed the correct assertion at line 97, exit code 1 (`wrong-delay-page-red.log`). No mutation or shadow include was committed. The original source was restored and rerun to PASS.

## Host checks: real commands and exit statuses

Host: Linux x86_64. GCC 14.2.0, Clang 17.0.0. `sh tests/host/smc_page_peek_production_test.sh` first generates the actual functions from `firmware/src/app/smc_mixer.c`, then builds with `-std=c99 -Wall -Wextra -Werror` at O0 and O2. The same script was rerun with `CC=clang`. A GCC build with `-fsanitize=address,undefined -fno-omit-frame-pointer` ran at O1. Results from `focused-host-checks.log`:

| Check | Result |
| --- | --- |
| GCC -O0: extracted production helper | PASS, 27 assertions, 0 failures |
| GCC -O2: extracted production helper | PASS, 27 assertions, 0 failures |
| Clang -O0: extracted production helper | PASS, 27 assertions, 0 failures |
| Clang -O2: extracted production helper | PASS, 27 assertions, 0 failures |
| GCC ASan+UBSan: extracted production helper | PASS, 27 assertions, 0 failures |
| GCC -O0: standalone timeout state | PASS, 31 checks, 0 failures |
| GCC -O2: standalone timeout state | PASS, 31 checks, 0 failures |

The five production function executions equal **135 focused assertions**. The two state runs add **62 separate assertions**. These are repeated checks under different host compiler settings, not a full-suite result. The exact complete firmware and DSP source was not reconstructed in this runner.

## Unrun checks and freeze

- Full host `sh tests/run_tests.sh`: **NOT_RUN**, including full application SMC UI regressions. The older full-screen UI fixture still assumes no `ui.view` mutation, which conflicts with the newly proposed mapped-page behavior. The full fixture must be reconciled and actually rerun before a green claim.
- Elite SDK V1.2.1 firmware build: **NOT_RUN**. A full verified exact-head source tree and matching complete SDK V1.2.1 closure were not available in this container. The separate available V1.2.0 archive is not an acceptable substitute; the private V1.2.1 kit only pins selected files.
- Pro SDK V1.2.1 firmware build: **NOT_RUN**, same source and SDK dependencies.
- Chicago/Miami bit-exact audio, device/SMC radio, LED, installation or publication: **NOT_VERIFIED / NOT_RUN**.
- #734 at its own immutable HEAD requires a separately posted **full host suite + Elite + Pro, 0 fail and 0 skip** before *any* new feature integration. This Task C test result is not that gate. Do not merge or push to #734. PR #738 stays DRAFT on its own branch.

## Files

- `focused-host-checks.log`: complete successful focused host-run stdout.
- `locked-overlay-red.log`: original observed incorrect handoff.
- `wrong-delay-page-red.log`: negative mutation proving the generated production test detects incorrect page mapping.
- `docs/mockups/smc-mapped-page-peek-before-after.svg`: earlier proposed UI before/after illustration, not a hardware screenshot.
