# Mixer FX page port from PR #688

Status: IN PROGRESS, scoped implementation owned by this Classic thread.

## Immutable inputs and base
- Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
- Source: PR #688, `wip/mixer-ux` at `7b2d1726f3515b319d5edcab34529007d33ab3a8` (closed, unmerged).
- Handoff base: `integration/all-in-one@ea859a8639f7448fb53196f6feb8c8c761b114d9`.
- Live integration HEAD at entry: `64ed879fc5358229e5febfafa95a22b4580ad2a1`, 13 commits later. Use the *live newer head* to avoid dropping concurrent UI changes. No edits to integration branch.
- Target: draft PR from `chat/mixer-fx-page-port` into `integration/all-in-one`.

## Scope
1. Inspect live `AGENTS.md`, feature registry, affected SMC and UI sources, #688 mapping, and live test contracts.
2. Reuse the **minimal** #688 FX-page controls/captions and related host regression only; resolve `smc_mixer.c` and `ui.c` conflicts, preserving the live integration code and changing hold checks to `smc_held()`.
3. Deliver clearly labeled, visually reviewed **before and after** screen mockups alongside the PR (not represented as screenshots of actual hardware).
4. Preserve existing Chicago/Miami engine behavior, project revision 13, other UI features and existing private header files. No `.github/` changes, third-party SDK files, firmware binaries, or platform/device writes.
5. Run full `tests/run_tests.sh` and `BOARD=smk37 MODEL=elite/pro sh build.sh` with exact local SDK V1.2.1 when possible. Preserve exact logs, exit statuses, pass/fail/skip totals, and build artifact hashes. Test evidence must never claim an unexecuted check passed.
6. Push incremental source/tests/evidence commits via the GitHub connector, read back the remote SHA, open one draft PR, and post verified outcomes. No main/nightly merge or release.

## Exit
Draft PR exists with scoped diff, source identifiers, mockups, test/build reports and an explicit PASS/FAIL/NOT_RUN for each required check. Test/build failures keep the stage incomplete and actionable.
