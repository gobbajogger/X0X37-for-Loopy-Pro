# Mixer FX page port from PR #688

Status: IN PROGRESS, scoped implementation owned by this Classic thread.

## Immutable inputs and base
- Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
- Source: PR #688, `wip/mixer-ux` at `7b2d1726f3515b319d5edcab34529007d33ab3a8` (closed, unmerged).
- Handoff base: `integration/all-in-one@ea859a8639f7448fb53196f6feb8c8c761b114d9`.
- Entry integration HEAD: `64ed879fc5358229e5febfafa95a22b4580ad2a1`.
- Owner-corrected port base: `c54d9f9b4262709bb4ea5852ae4ed008fa100996` (#650/#651/#687 work already integrated). This commit includes the 873479c7 HOME caption test fix. The correction is handled by a no-force, two-parent merge-forward tree using this base plus the previously pushed scoped port; no source or history is discarded.
- Current integration branch has since advanced beyond this pin; the draft PR still targets the moving `integration/all-in-one` ref and must be checked against its live head before completion. No edits to integration branch.
- Target: draft PR from `chat/mixer-fx-page-port` into `integration/all-in-one`.

## Scope
1. Inspect live `AGENTS.md`, feature registry, affected SMC and UI sources, #688 mapping, and live test contracts.
2. Reuse the **minimal** #688 FX-page controls/captions and related host regression only; resolve `smc_mixer.c` and `ui.c` conflicts, preserving the live integration code and changing hold checks to `smc_held()`.
3. Deliver clearly labeled, visually reviewed **before and after** screen mockups alongside the PR (not represented as screenshots of actual hardware).
4. Preserve existing Chicago/Miami engine behavior, project revision 13, other UI features and existing private header files. No `.github/` changes, third-party SDK files, firmware binaries, or platform/device writes.
5. Run full `tests/run_tests.sh` and `BOARD=smk37 MODEL=elite/pro sh build.sh` with exact local SDK V1.2.1 when possible. Preserve exact logs, exit statuses, pass/fail/skip totals, and build artifact hashes. Test evidence must never claim an unexecuted check passed.
6. Push incremental source/tests/evidence commits via the GitHub connector, read back the remote SHA, open one draft PR, and post verified outcomes. No main/nightly merge or release.

## Verified source inventory
- GitHub tree API read at commit `82acc31e21b81b0d66399c02134aac2a6eebdab2`: **5,035 entries / 4,612 blobs**, `truncated=false`. This verifies the reachable remote tree index, **not** a fully reconstructed native source checkout.
- Relevant C files were read directly from their exact GitHub blob paths on the same branch. Existing caption, FX table and multi-peer edge helpers were found already integrated.
- Reuse the newer integrated `ui.c` unchanged rather than copying historical PR #688's 7,385-line UI code over the 7,756-line live version.

## Exit
Draft PR exists with scoped diff, source identifiers, mockups, test/build reports and an explicit PASS/FAIL/NOT_RUN for each required check. Test/build failures keep the stage incomplete and actionable.
