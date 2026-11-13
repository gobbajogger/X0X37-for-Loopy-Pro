# Orchestrator handoff notes (2026-10-30)

**Start here:** [Operating guide for the new orchestrator](OPERATING-GUIDE.md).

This folder holds the old orchestrator's notes, so the new owner can continue the DnBOX work.

## Current state

- `wip/one-build` is at `cb5423c4`. Nothing has been merged since #702.
- Merge order starts with #699. #699 is on the old base `1f27d2fb` and has no full test pass.
- #708 and #712 are on hold.
- #688 needs the fixes from #714.
- Independent QA says #717 and #718 need changes.
- The SMC page-lock spec is in `BEHAVIOR.md`, section SMC-11, on #717.
- #721 adds the private headers (owner approved 2026-10-30). All 12 manifest headers plus `state_codec_private.h` are on the branch, and the input check passes. Note: `EXTERNAL-INPUTS.md` still says the headers are kept out of Git; update that text.
- Recovered commits are on `backup/recovered-0ed772a1` and `backup/recovered-df15c595`.
- Stale lanes (no push for more than 10 hours): `chat/pr-conflicts`, `smc-a-port-20261029`, `wip/web-site`. Push or close each one.

## Box host-test results (base cb5423c4)

The base fails 69 tests on the box (see `hosttest-results/base.fails`). The `.summary` files count 154 failures, because they count failing lines, not unique tests.

- About 25 failures come from the box setup: a stricter compiler, one clash with a system library, and a missing image library.
- About 40 failures look like real code problems in the base:
  - undeclared or mismatched functions: `x0x_pan_gains`, `x0x_pot_exp`, `ED_MAINTENANC*`, `ui_boot_flash_busy`
  - `smk-clip-isolation` keys
  - `rec-held-duration` unlink
  - about 22 scenario tests that fail their behavior checks
  - `bass-tagged-storage`
- #705 fixes 2 tests and adds no failures. #710 changes nothing.

Unless the cloud suite with the real SDK compiler is clean, `cb5423c4` needs a fix lane for these ~40 failures before any PR can pass the full suite.

## Files in this folder

- `dnbox-routines-handoff.md`: the five routines to set up on the new account.
- `hosttest-results/`: summaries and failure lists for base, #705 and #710.
- `workspace/`: text notes from the old orchestrator's box: ledger, index, feature coverage, missing-feature list, host-test script, Mac cleanup plan, owner requests, recovered patches, clip/arp report, bench notes, a build-runner log, and the docs keeper's notes.

## Not included

- Full host-test logs (`*.log`, about 50 KB each): large; the failure lists cover them. `pr714.log` has no summary yet.
- `*.fwsc` firmware images and `*.bundle` git bundles: binaries.
- `DnBOX-LoopyX0X37-full-feature-list-20261029.docx`: binary document.
- `features.txt` (220 KB): large raw dump.
- `dnbox-private-inputs/`: the headers are already in #721.
- Builder working copies (`x0x37-*` folders, `gh-x0x37*`, `x0x37.git`): clones of this repo.

## Backup branches from the box (2026-10-30)

Work that existed only on the orchestrator's box was pushed as `backup/*` branches. Original commits are kept. Nothing here is merged.

- `backup/gh-x0x37-fmb-pr684-ui`, `backup/gh-x0x37-synth684-harness`, `backup/gh-x0x37-trial-638-650-651`: local branches that were never pushed.
- `backup/fmb-pr684-wip`, `backup/gh-x0x37-merge-wip` (staged bass303 removal on merge-trial-20261029), `backup/smcfix-wip`: uncommitted changes.
- 98 `backup/bundle-*` branches: commits found only in bundles (Mac codex-tasks backup, x0x37-inner, codex-lanes). Full list: `git ls-remote origin 'refs/heads/backup/bundle-*'`.
- Not pushed: bundle commits that contain binaries, PNGs, WAVs or capture logs (45 tips), two wobble.c experiment branches, and two incomplete SMC bundles.
