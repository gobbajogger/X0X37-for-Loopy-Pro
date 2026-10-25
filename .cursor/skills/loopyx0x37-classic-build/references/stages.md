# Stages, ownership, and publication

Read this when the task touches integration, the nightly catalog, Pages, hardware status, or production. Command-level build steps are in [builds.md](builds.md).

## Two repositories

Verify both refs at the start of any publication or integration task. Re-check the names below before writing.

| Repository | Role |
| --- | --- |
| `gobbajogger/X0X37-for-Loopy-Pro` | Firmware and installer source. This checkout is that repository. `origin/nightly` was present when this skill was edited; read its tip before targeting it. Production promotion is a merge to `main` plus the production picker. A commit already on `main` is not a device pass for a new package. |
| `gobbajogger/x0x37-for-Loopy-Pro_nightly` | Separate browser and status repository from the 2026-10-23 owner note. Confirm it still exists before writing it. It may record firmware SHAs and links. It receives no firmware source, `.fwsc`, `ctl.img`, SDK, toolchain, DSP blobs, samples, or credentials. It is not the publisher documented in this checkout. |

The live installer documented in `README.md` and `BUILDING.md` is this repository's `gh-pages` branch: `https://gobbajogger.github.io/X0X37-for-Loopy-Pro/`.

Producer firmware work in Cursor opens a pull request. Target `nightly` when the user assigned firmware integration. Otherwise use the run's base branch. Do not retarget or flatten stacked pull requests.

This run is not the standing Safe Merge owner or the sole Pages publisher. Those writes happen only when the user assigns that stage in this run and the evidence below is present. An assigned merge to `nightly` or `main` updates the expected head without force. An assigned `gh-pages` publication uses the force-push inside `tools/publish_pages.sh` and no other force-push.

## Owner decision, 2026-10-25

Automatically validated firmware merges without another owner prompt.

Automatic validation means the applicable host checks exited 0 and `tools/dev_validate.py` recorded `DEV CHECKED` for the affected SMK-37 Elite and Pro development packages on that exact commit. A missing distribution-license receipt, corresponding-source receipt, or native-build receipt does not block this, because owner policy waives those receipts as loop stops (follow LICENSE and LICENSING.md; do not invent an MIT claim for the tree). A hash mismatch, a failed package check, or a compiler failure still blocks.

The merge is a normal fast-forward of firmware `nightly` in `gobbajogger/X0X37-for-Loopy-Pro`. If the remote refuses, record the refusal and do not force-push. Production `main`, production GitHub Pages, and a device flash are not this merge. Independent human QA stays `QA_PENDING` when the author cannot supply it. Same-account approval is not that review.

This dated decision is the assignment for that `nightly` fast-forward only. It does not assign a release gate, a merge to production `main`, production GitHub Pages, or a device flash.

## Dependency graph

Source pushed, then source ready for review. Independent QA and `BUILD_ONLY` may proceed together on that immutable commit, in separate worktrees. Reviewed combined source can integrate to firmware `nightly` when the user assigns that merge. A release publication of this repository follows the section below and waits for the release gate. Live HTTP and hash verification comes after the `gh-pages` update. Owner hardware feedback starts once a candidate is actually downloadable. Production promotion stays a separate approval.

Browser and history source do not wait for a firmware package. A public selectable package waits for the applicable prehardware, independent, integrity, data-safety, and distribution checks.

## NIGHTLY test candidate

A qualified nightly candidate is labeled `DEV / HARDWARE TEST — DEVICE NOT TESTED`. Prior physical pass is not one of its gates. The user's publication assignment in this run is. A ChatGPT scheduler prompt does not authorize a Cursor run.

`tools/make_pages.py` accepts release identities only: `SMK-37 Pro_9` plus six digits, `SMK-37 Elite_9` plus six digits, `MKE-P37_9` plus six digits, and the FM-1 release form. Development identity `SMK-37 Pro_900` / `SMK-37 Elite_900` is rejected. Do not rename a dev package or edit its bytes so the generator will take it. Until a tested publisher path accepts a dev identity, a hardware-test candidate stays `PACKAGE_NOT_PUBLISHED`. `tools/publish_pages.sh --shell` builds an installer shell whose catalog can be empty. That shell is not a served candidate.

Eligible release publication, after the user requests a release, uses the evidence below and the commands in `BUILDING.md`:

1. Affected behavior reviewed by someone who did not author the change, including negative and boundary paths. For SMC changes, cover the path that actually changed: BLE or HCI, decode, UI, feedback, forwarded origin, user mode, and mixer selection when the diff touches them. The authoring run records `QA_PENDING`.
2. Applicable static, host, and regression checks with exits. Mocks do not prove hardware.
3. Real Elite and Pro packages where firmware inputs changed, plus MKE when `tools/build_smk_release.sh` builds it. Record model identity, controller identity, SHA-256, CRC, and budgets. Static RAM free is nonnegative. Pool free is at least `pool_headroom_min` in `tools/locked_settings.json`. `tools/build.py` and `tools/dev_validate.py` use the same 8192-byte floor. If those three disagree, stop and change the source and the JSON together. Do not pick the lower number.
4. Package integrity: length, hash, source and config identity, license notices, sample preservation, bootloader exclusions. Release packages ship with `LICENSE`, `LICENSING.md`, and `LICENSES/` as `BUILDING.md` requires.
5. Installer negatives: wrong-model refusal, backup before write, interrupted update, rollback, readback, and per-slot preservation. A changed installer page is exercised in a browser.
6. Every known failure disposed. Remaining items are physical observations only.

Keep the previous live per-model candidate when a new build fails or is unchanged. Report one of `NEW VERIFIED LIVE`, `PRIOR VERIFIED LIVE RETAINED`, `NO INSTALLABLE YET`, or `REVOKED/UNSAFE`, with URL and hash or the exact missing gate. `latest-built` and `latest-installable` are different. A malformed catalog fails closed.

## Pages update

Assigned publication of this repository uses `tools/publish_pages.sh` as `BUILDING.md` describes it. The script archives the current `gh-pages` tree, merges kept firmware into the new site, then `git push -f` an orphan `gh-pages` branch. That force-push is the maintained publisher. It is not permission to force-push `main`, `nightly`, or the run's branch. If this run's instructions forbid every force-push, leave the stage `PACKAGE_NOT_PUBLISHED` and name the script. Do not replace the script with a fast-forward that it does not perform.

Before running it, record the fetched `gh-pages` SHA. After it exits 0, read the new remote SHA and fetch the live installer, catalog, and package bytes. Match those bytes to the qualified packages. A `gh-pages` SHA alone is `SITE BRANCH ONLY`. Matched downloads are `DOWNLOAD_HTTP_VERIFIED`. Device status stays `DEVICE NOT TESTED` until a hardware session exists.

Source branches still do not store `.fwsc` or `ctl.img`. The `gh-pages` tree produced by the assigned publisher does store the release packages it merged. The separate nightly repository, when used, stores neither.

The controller image is built for the package under test. Current SMK packages rewrite that block, so an empty controller is refused. Publish a `ctl.img` only when the destination's rules allow that file. The nightly status repository does not receive it.

The production picker accepts a package only with attributable same-byte, matching-model physical proof and a separate promotion approval. `hardware_tested: true`, `dev-checked`, or a well-formed SHA string is not that proof. Re-read the current site source before claiming a catalog bug is already fixed.

## Hardware and production

No Cursor agent flashes a keyboard. The owner tests a live candidate on the matching model, with slot backup, readback, and per-slot CRC. Record the model, build identity, connection, steps, expected and observed behavior, and logs that contain no private backups.

A reported failure becomes one regression on an owned branch, with a failing check before the repair and a passing check after, then returns through QA and build. The same hardware session does not certify a different model or different bytes. An unsafe candidate is removed from selection by the assigned publisher. Evidence and the previous safe version stay.

Production `main` and the production picker move together only with that physical proof and a separate user promotion approval in this run. A nightly or development package does not become the production selection on its own.

## Single-agent review limit

Independent QA requires a reviewer who did not write the change. When this run is the author, the review state is `QA_PENDING` even if every local check passed. Same-account approval does not satisfy a required human review. Say so in the pull request. Do not mark the pull request merged.
