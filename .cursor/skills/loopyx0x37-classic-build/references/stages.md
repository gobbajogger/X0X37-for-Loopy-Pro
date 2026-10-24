# Stages, ownership, and publication

Read this when the task touches integration, the nightly catalog, Pages, hardware status, or production. Command-level build steps are in [builds.md](builds.md).

## Two repositories

Verify both refs at the start of any publication or integration task. Names below are the owner's repositories as of 2026-10-23 and must be re-checked.

| Repository | Role |
| --- | --- |
| `gobbajogger/X0X37-for-Loopy-Pro` | Firmware and installer source. `main` is production and hardware-proven. `nightly`, when it exists, is the firmware integration branch. This checkout is that firmware repository. |
| `gobbajogger/x0x37-for-Loopy-Pro_nightly` | Separate browser and status site. It may record firmware SHAs and links to qualified immutable packages. It must not contain firmware source, `.fwsc`, `ctl.img`, SDK, toolchain, DSP blobs, samples, or credentials. |

Producer firmware work in Cursor opens a pull request. Target `nightly` only when that branch exists and the user assigned firmware integration. Otherwise use the run's base branch. Do not retarget or flatten stacked pull requests.

This run is not the standing Safe Merge owner or the sole Pages publisher. Those writes happen only when the user assigns that stage in this run, the evidence below is present, and the write is a non-forced update of the expected head.

## Dependency graph

Source pushed, then source ready for review. Independent QA and `BUILD_ONLY` may proceed together on that immutable head. Reviewed combined source can then integrate to firmware `nightly` when assigned. Exact combined-tree model builds and full prehardware QA come next. Lawful immutable hosting, catalog review, website `main`, and sole Pages publication follow. Live HTTP and hash verification comes after the Pages update. Owner hardware feedback starts only once a candidate is actually downloadable. Production stays a separate approval.

Browser and history source do not wait for a firmware package. Public selectable firmware does wait for the applicable prehardware, independent, integrity, data-safety, and distribution checks.

## NIGHTLY test candidate

A qualified nightly candidate is labeled `DEV / HARDWARE TEST — DEVICE NOT TESTED`. Prior physical pass is not one of its gates. The user's explicit publication assignment in this run is. Standing consent stored in a ChatGPT scheduler prompt does not authorize a Cursor run.

Eligible publication evidence:

1. Affected behavior reviewed, including negative and boundary paths. For SMC changes, cover the path that actually changed: BLE or HCI, decode, UI, feedback, forwarded origin, user mode, and mixer selection when the diff touches them.
2. Applicable static, host, and regression checks with exits. Mocks do not prove hardware.
3. Real Elite and Pro development builds where firmware inputs changed, with model identity, controller identity, SHA-256, CRC, and budgets from the build. Nonnegative static RAM and the pool headroom enforced by the current gate and `tools/locked_settings.json`. The 2026-10-23 checkpoint used at least 8192 bytes of pool free. Do not lower that figure to obtain a pass. Re-read the current checker.
4. Package integrity: length, hash, source and config identity, license notices, sample preservation, bootloader exclusions.
5. Installer negatives: wrong-model refusal, backup before write, interrupted update, rollback, readback, and per-slot preservation. A changed installer page is exercised in a browser.
6. Every known failure disposed. Remaining items are physical observations only.

Keep the previous live per-model candidate when a new build fails or is unchanged. Report one of `NEW VERIFIED LIVE`, `PRIOR VERIFIED LIVE RETAINED`, `NO INSTALLABLE YET`, or `REVOKED/UNSAFE`, with URL and hash or the exact missing gate. `latest-built` and `latest-installable` are different. A malformed catalog fails closed.

## Pages update

`tools/publish_pages.sh` runs `git push -f` from an unrelated root. Do not execute it unchanged. A Pages update that the user has assigned:

1. Fetches the current `gh-pages` head and records its SHA.
2. Generates the candidate in a scratch tree and tests that output.
3. Overlays only the candidate's site and catalog files onto a worktree of that fetched head. Preserves unrelated assets. Does not merge firmware source branches into the site.
4. Commits with that fetched head as parent, fast-forwards, and reads the remote SHA back. If the branch moved, repeats the overlay. No `--force` and no `--force-with-lease`.
5. Fetches the live installer URL, catalog, and download bytes and matches them to the qualified package. Records `DEVICE NOT TESTED`.

Firmware bytes stay out of the website Git tree. They are referenced from lawful immutable hosting or a verified owner-local acquisition path. The controller image is built by the owner and is not shipped in this tree. Do not drop it silently when the package contract requires it, and do not publish it without a distribution right.

The production picker accepts a package only with attributable same-byte, matching-model physical proof and a separate promotion approval. `hardware_tested: true`, `dev-checked`, or a well-formed SHA string is not that proof. Historical catalog bugs of this kind are remediation targets, not evidence they are already fixed. Re-read the current site source.

## Hardware and production

No Cursor agent flashes a keyboard. The owner tests a live candidate on the matching model, with slot backup, readback, and per-slot CRC. Record the model, build identity, connection, steps, expected and observed behavior, and logs that contain no private backups.

A reported failure becomes one regression on an owned branch, with a failing check before the repair and a passing check after, then returns through QA and build. The same hardware session does not certify a different model or different bytes. An unsafe candidate is removed from selection by the assigned publisher. Evidence and the previous safe version stay.

Production `main` and the production picker move only with that physical proof and a separate user promotion approval in this run. Do not promote automatically.

## Single-agent review limit

Independent QA requires a reviewer who did not write the change. When this run is the author, the review state is `QA_PENDING` even if every local check passed. Same-account approval does not satisfy a required human review. Say so in the pull request. Do not mark the pull request merged.
