# Integrated one-build host-validation plan

Owner request: build and test in cloud without waiting for owner or Mac.
Source: `gobbajogger/X0X37-for-Loopy-Pro`, `wip/one-build` at `094bb1a55c159aefb0e2bfa39790a1b4b74cee40`.
Execution branch: `wip/one-build-host-validation`. This is not a firmware integration or release lane.

## Scope and stopping condition

1. Keep the exact integrated source unchanged until real host failures are known. Use an isolated GitHub-hosted Linux runner to test the complete current branch because this chat's shell has DNS disabled for GitHub and its mounted historical bootstrap is not this source.
2. Run the integrated host simulator build, the original-engine composition test, the Free Play rate-pad compiled regression, and the full host suite. Record exact source commit, every exit code, logs and skips, without weakening assertions or hiding errors.
3. Diagnose only failures in test harness/build integration that are not already owned by active Chicago, Miami, BLE/SMC, BREAK, UI or features lanes; add regressions and rerun affected tests. Leave the sole integrator in charge of shared firmware changes.
4. If full current source and the required pinned private inputs become available on a verified Linux runner, run Elite/Pro development builds and record real package/model/budget/identity evidence. Until then, report firmware target builds NOT_RUN, not PASS.
5. Use the GitHub API connector with default author for isolated commits. Verify branch HEAD and open a draft PR against `wip/one-build`. No merge, release, flash, force-push, binaries, private #684 headers, credentials, or publication.

## Input evidence and limits

- Source root tree at starting commit: `ce0b0b9555e31ddd8d8ac54f6da385052e3565af` (4,582 entries, untruncated GitHub tree).
- The mounted `source-at-7537a46.tar` is historical. Its successfully compiled host simulator is NOT evidence for the current integrated source.
- Mounted JieLi toolchain ZIP passes CRC; mounted SDK archive is V1.2.0. Firmware target requires pinned V1.2.1, so do not substitute it.
- Existing PR #698 owns integration, PR #700 owns SMC, PR #691 owns Miami, PR #701 owns Chicago/UI, and PR #702 owns spare-rate pads. Keep changes disjoint.
- Cloud CI can prove host behavior, but does not by itself establish bit-exact reference output, target build, installer validity or device safety.

**Exit gate:** all affected host failures reproduced and repaired with validated source, and any required target builds actually passed. If an essential check cannot run, report the precise pending gate and keep the stage incomplete.

## Measured cloud and local results (2026-10-30 UTC)

The first GitHub Actions execution was [run 41003193610](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/actions/runs/41003193610) for commit `32907755c013744ccb4991ba56fe29a3f0219067`. The workflow concluded **failure**, but its job contained **zero steps**. Job log download returned GitHub blob-not-found. Therefore **no host tests executed on that workflow**, and neither test failure nor PASS was established.

The chat's Linux x86-64 container actually compiled and executed these two isolated host regressions. Their full consumed source paths were compared by Git blob SHA against the pinned integration commit and were identical, even though they came from a historical local bootstrap. These scoped checks do not substitute for the complete current-tree build:

- **PASS** `gcc -O2 -w -Ifirmware/hal -o /mnt/data/work/proof/encoder_test tests/host/encoder_test.c && /mnt/data/work/proof/encoder_test` (exit 0, `encoder: ok`), using exact-matching `tests/host/encoder_test.c`, `firmware/hal/fm1_input.h`, `firmware/hal/fm1_time.h`, and `firmware/hal/fm1_gpio.h`.
- **PASS** `gcc -std=c99 -O2 -Wall -Wextra -Werror -o /mnt/data/work/proof/smc_protocol_test tests/host/smc_protocol_test.c && /mnt/data/work/proof/smc_protocol_test` (exit 0, `SMC protocol: 43 buttons, pickup, encoders and host isolation ok`), using exact-matching `tests/host/smc_protocol_test.c` and `firmware/src/app/smc_protocol.h`.

All required checked source closure for these two leaf tests matched the remote commit, but other files from the historical checkout were not assumed current. Full integrated host simulator: **NOT_RUN**. Full suite and compiled Free Play regression: **NOT_RUN**. AC79 SDK V1.2.1 target Elite/Pro builds: **NOT_RUN**. The uploaded SDK archive is V1.2.0 and cannot replace V1.2.1. No firmware, controller or installation package was produced.
