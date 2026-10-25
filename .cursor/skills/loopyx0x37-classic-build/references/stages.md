# Stages, ownership, and publication

Read this when the task touches integration, the nightly catalog, Pages, hardware status, or production. Command-level build steps are in [builds.md](builds.md).

## Two repositories

Verify both refs at the start of any publication or integration task. Re-check the names below before writing.

| Repository | Role |
| --- | --- |
| `gobbajogger/X0X37-for-Loopy-Pro` | Firmware and installer source. This checkout is that repository. `origin/nightly` was present when this skill was edited; read its tip before targeting it. Firmware nightly fast-forwards when host checks passed and both affected development packages are `DEV CHECKED`. Firmware main fast-forwards only after a successful hardware validation of those exact bytes on the matching model. `INSTALL BLOCKED` is not success. Elite evidence does not move a Pro package. No force-push of `nightly` or `main`. Production GitHub Pages and the release picker still follow their own publisher. A commit already on `main` is not a device pass for a new package. |
| `gobbajogger/x0x37-for-Loopy-Pro_nightly` | Separate browser and status repository from the 2026-10-23 owner note. Confirm it still exists before writing it. It may record firmware SHAs and links. It receives no firmware source, `.fwsc`, `ctl.img`, SDK, toolchain, DSP blobs, samples, or credentials. It is not the publisher documented in this checkout. |

The live installer documented in `README.md` and `BUILDING.md` is this repository's `gh-pages` branch: `https://gobbajogger.github.io/X0X37-for-Loopy-Pro/`.

Producer firmware work in Cursor opens a pull request. The feature pull request uses the run's base branch. Naming `nightly` as that base is not the firmware nightly fast-forward. Firmware nightly fast-forwards when host checks passed and both affected development packages are `DEV CHECKED`. Firmware main fast-forwards only after a successful hardware validation of those exact bytes on the matching model. `INSTALL BLOCKED` is not success. Elite evidence does not move a Pro package. No force-push of `nightly` or `main`. Do not retarget or flatten stacked pull requests.

The export's Safe Merge role is not the nightly or main fast-forward. Firmware nightly fast-forwards when host checks passed and both affected development packages are `DEV CHECKED`. Firmware main fast-forwards only after a successful hardware validation of those exact bytes on the matching model. `INSTALL BLOCKED` is not success. Elite evidence does not move a Pro package. Both fast-forwards update the expected head. No force-push of `nightly` or `main`. An assigned `gh-pages` publication uses the force-push inside `tools/publish_pages.sh` and no other force-push.

## Owner decisions (2026-10-25)

The owner's current request overrides this skill.

Every change states a reason, a purpose, and the effect on the person using the device.

Each job is a new thread. Never resume an old thread. A subagent does the bounded work inside the thread and does not start the next thread. One worktree has one writer.

Agents run every applicable test in their own environment with every tool that machine has. GitHub is not a test runner. An Actions result, a billing lock, or an empty workflow is not a firmware result. A zero failure counter is not a pass unless the named tests printed ok.

The project is MIT. A missing distribution-license receipt, corresponding-source receipt, or native-build receipt does not block. A present receipt that is not PASS, a hash mismatch, a failed package check, or a compiler failure still blocks.

Firmware nightly fast-forwards when host checks passed and both affected development packages are `DEV CHECKED`. Host checks passed means the applicable host checks exited 0. Both affected development packages are the affected SMK-37 Elite and Pro development packages on that exact commit, and `DEV CHECKED` is the `tools/dev_validate.py` record for each of them. The fast-forward is a normal fast-forward of firmware `nightly` in `gobbajogger/X0X37-for-Loopy-Pro`. No force-push of `nightly` or `main`. If the remote refuses, record the refusal. Production `main`, production GitHub Pages, and a device flash are not this fast-forward. Independent human QA stays `QA_PENDING` when the author cannot supply it. Same-account approval is not that review. This decision assigns that nightly fast-forward. It does not assign a release gate, production GitHub Pages, or a device flash.

Firmware main fast-forwards only after a successful hardware validation of those exact bytes on the matching model. Success is an install record for the exact package and controller hashes, on the matching SMK-37 model, with backup, readback, and the running identity checked. `INSTALL BLOCKED` is not success. Elite evidence does not move a Pro package, and Pro evidence does not move an Elite package. The fast-forward is a normal fast-forward of firmware `main` in `gobbajogger/X0X37-for-Loopy-Pro`. No force-push of `nightly` or `main`. If the remote refuses, record the refusal. Production GitHub Pages and a release picker still follow their own publisher. This decision assigns that firmware `main` fast-forward. It does not flash a device, publish production GitHub Pages, or move a release picker. Recording this decision is not a hardware validation. A run with no successful hardware validation does not fast-forward `main`.

A cloud agent does not flash the keyboard. The Mac LaunchAgent `com.x0x37.unattended-install` may install a validated development package. Docs and skill edits do not require a firmware rebuild.

Do not restore the removed skill pack.

## Dependency graph

Source pushed, then source ready for review. Independent QA and `BUILD_ONLY` may proceed together on that immutable commit, in separate worktrees. Firmware nightly fast-forwards when host checks passed and both affected development packages are `DEV CHECKED`. No force-push of `nightly` or `main`. A release publication of this repository follows the section below and waits for the release gate. Live HTTP and hash verification comes after the `gh-pages` update. A Pages download is not the firmware main fast-forward. Firmware main fast-forwards only after a successful hardware validation of those exact bytes on the matching model. `INSTALL BLOCKED` is not success. Elite evidence does not move a Pro package, and Pro evidence does not move an Elite package. Production GitHub Pages and a release picker still follow their own publisher.

Browser and history source do not wait for a firmware package. A public selectable package waits for the applicable prehardware, independent, integrity, and data-safety checks. A missing distribution-license receipt, corresponding-source receipt, or native-build receipt does not block. A present receipt that is not PASS, a hash mismatch, a failed package check, or a compiler failure still blocks.

## NIGHTLY test candidate

This section is publication of a test candidate. It is not the firmware nightly fast-forward. Firmware nightly fast-forwards when host checks passed and both affected development packages are `DEV CHECKED`. No force-push of `nightly` or `main`.

A qualified nightly candidate is labeled `DEV / HARDWARE TEST — DEVICE NOT TESTED`. Prior physical pass is not one of its gates. The owner's publication assignment in this run is. A ChatGPT scheduler prompt does not authorize a Cursor run.

`tools/make_pages.py` accepts release identities only: `SMK-37 Pro_9` plus six digits, `SMK-37 Elite_9` plus six digits, `MKE-P37_9` plus six digits, and the FM-1 release form. Development identity `SMK-37 Pro_900` / `SMK-37 Elite_900` is rejected. Do not rename a dev package or edit its bytes so the generator will take it. Until a tested publisher path accepts a dev identity, a hardware-test candidate stays `PACKAGE_NOT_PUBLISHED`. `tools/publish_pages.sh --shell` builds an installer shell whose catalog can be empty. That shell is not a served candidate.

Eligible release publication, after the owner requests a release, uses the evidence below and the commands in `BUILDING.md`:

1. Affected behavior reviewed by someone who did not author the change, including negative and boundary paths. For SMC changes, cover the path that actually changed: BLE or HCI, decode, UI, feedback, forwarded origin, user mode, and mixer selection when the diff touches them. The authoring run records `QA_PENDING`.
2. Applicable static, host, and regression checks with exits. Mocks do not prove hardware. Agents run every applicable test in their own environment with every tool that machine has. GitHub is not a test runner. An Actions result, a billing lock, or an empty workflow is not a firmware result. A zero failure counter is not a pass unless the named tests printed ok.
3. Real Elite and Pro packages where firmware inputs changed, plus MKE when `tools/build_smk_release.sh` builds it. Record model identity, controller identity, SHA-256, CRC, and budgets. Static RAM free is nonnegative. Pool free is at least `pool_headroom_min` in `tools/locked_settings.json`. `tools/build.py` and `tools/dev_validate.py` use the same 8192-byte floor. If those three disagree, stop and change the source and the JSON together. Do not pick the lower number.
4. Package integrity: length, hash, source and config identity, sample preservation, and bootloader exclusions. Release packages ship with `LICENSE`, `LICENSING.md`, and `LICENSES/` as `BUILDING.md` requires. Those shipped files are package content. They are not a distribution-license receipt, a corresponding-source receipt, or a native-build receipt. A missing distribution-license receipt, corresponding-source receipt, or native-build receipt does not block. A present receipt that is not PASS, a hash mismatch, a failed package check, or a compiler failure still blocks.
5. Installer negatives: wrong-model refusal, backup before write, interrupted update, rollback, readback, and per-slot preservation. A changed installer page is exercised in a browser.
6. Every known failure disposed. Remaining items are physical observations only.

Keep the previous live per-model candidate when a new build fails or is unchanged. Report one of `NEW VERIFIED LIVE`, `PRIOR VERIFIED LIVE RETAINED`, `NO INSTALLABLE YET`, or `REVOKED/UNSAFE`, with URL and hash or the exact missing gate. `latest-built` and `latest-installable` are different. A malformed catalog fails closed.

## Pages update

Assigned publication of this repository uses `tools/publish_pages.sh` as `BUILDING.md` describes it. The script archives the current `gh-pages` tree, merges kept firmware into the new site, then `git push -f` an orphan `gh-pages` branch. That force-push is the maintained `gh-pages` publisher. It is not permission to force-push `main`, `nightly`, or the run's branch. The nightly and main fast-forwards do not use force. If the owner's current request forbids the `gh-pages` force-push itself, leave the stage `PACKAGE_NOT_PUBLISHED` and name the script. Do not replace the script with a fast-forward that it does not perform.

Before running it, record the fetched `gh-pages` SHA. After it exits 0, read the new remote SHA and fetch the live installer, catalog, and package bytes. Match those bytes to the qualified packages. A `gh-pages` SHA alone is `SITE BRANCH ONLY`. Matched downloads are `DOWNLOAD_HTTP_VERIFIED`. Device status stays `DEVICE NOT TESTED` until a hardware session exists.

Source branches still do not store `.fwsc` or `ctl.img`. The `gh-pages` tree produced by the assigned publisher does store the release packages it merged. The separate nightly repository, when used, stores neither.

The controller image is built for the package under test. Current SMK packages rewrite that block, so an empty controller is refused. Publish a `ctl.img` only when the destination's rules allow that file. The nightly status repository does not receive it.

The production picker accepts a package only with attributable same-byte, matching-model physical proof and a separate promotion approval. That approval is the release picker's own publisher. It is not a second gate on the firmware main fast-forward. `hardware_tested: true`, `dev-checked`, or a well-formed SHA string is not that proof. Re-read the current site source before claiming a catalog bug is already fixed.

## Hardware and production

A cloud agent does not flash the keyboard. The Mac LaunchAgent `com.x0x37.unattended-install` may install a validated development package. The owner tests a live candidate on the matching model, with slot backup, readback, and per-slot CRC. Record the model, build identity, connection, steps, expected and observed behavior, and logs that contain no private backups.

A reported failure becomes one regression on an owned branch, with a failing check before the repair and a passing check after, then returns through QA and build. The same hardware session does not certify a different model or different bytes. An unsafe candidate is removed from selection by the assigned publisher. Evidence and the previous safe version stay.

Firmware main fast-forwards only after a successful hardware validation of those exact bytes on the matching model. Success is an install record for the exact package and controller hashes, on the matching SMK-37 model, with backup, readback, and the running identity checked. `INSTALL BLOCKED` is not success. Elite evidence does not move a Pro package, and Pro evidence does not move an Elite package. The fast-forward is a normal fast-forward. No force-push of `nightly` or `main`. If the remote refuses, record the refusal. Production GitHub Pages and a release picker still follow their own publisher. A run with no successful hardware validation does not fast-forward `main`. A nightly or development package does not become the production selection on its own.

## Single-agent review limit

Independent QA requires a reviewer who did not write the change. When this run is the author, the review state is `QA_PENDING` even if every local check passed. Same-account approval does not satisfy a required human review. Say so in the pull request. Same-account approval does not replace the firmware nightly fast-forward or the firmware main fast-forward. Firmware nightly fast-forwards when host checks passed and both affected development packages are `DEV CHECKED`. Firmware main fast-forwards only after a successful hardware validation of those exact bytes on the matching model. `INSTALL BLOCKED` is not success. Elite evidence does not move a Pro package. No force-push of `nightly` or `main`.
