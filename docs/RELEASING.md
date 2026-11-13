# Releasing DnBOX

This is the release standard for every public DnBOX package on the main repository and on the nightly channel. Do not invent a different tag, file name, or notes layout.

## Publish bar

A release may be published only when all of the following are true:

- Elite and Pro packages both built from the same source commit
- The full host test suite finished with 0 failures and 0 skips
- The heavy-load timing test finished with 0 dropouts
- [CHANGELOG.md](../CHANGELOG.md) already has an entry for this version
- The build has not been withdrawn

Never publish a withdrawn build.

## Tag and target

- Main / stable GitHub releases: `vX.Y.Z` (for example `v0.0.25.1`)

The GitHub release **target must be the exact commit that produced the packages**. If the Releases UI cannot pick a SHA, create a `release/X.Y.Z` branch that points at that commit, and target the release at that branch. Do not target a later tip or a mixed branch.

## Nightly packages

Nightly / beta packages are published through the [nightly installer catalog](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/), not as GitHub Releases. Catalog IDs use the form `nightly-<sha>-dev`. Do not create GitHub releases or `vX.Y.Z-nightly.*` tags for these builds.

## Title, pre-release and Latest

- Title: `DnBOX X.Y.Z`.
- Mark the release as **pre-release** if it has not been signed off as a physical device test.
- Mark as **Latest** only for a main-repository stable release that is the current recommended install and has been signed off as a physical device test. Pre-releases are never Latest.

The 0.0.25.1 packages predate this standard. They were published as Latest on the main repository without a physical device-test sign-off, and the host suite that produced them recorded one skip. Later releases follow the publish bar and the pre-release rule above. Do not edit the live 0.0.25.1 release.

## Assets

Attach only these four files:

- `dnbox-x0x37-X.Y.Z-elite.fwsc`
- `dnbox-x0x37-X.Y.Z-pro.fwsc`
- `SHA256SUMS.txt` — standard `sha256sum -c` format (SHA-256 and filename)
- `SHA256SUMS-CRC32.txt` — filename, size in bytes, CRC32, and SHA-256

Do not attach internal or process notes, raw captures, WAV files, debug images, or other junk.

## Notes

Public release notes are short and follow this order, matching the published 0.0.25.1 notes:

1. Title: `# DnBOX X.Y.Z`
2. Supported models, in one line (for example: firmware for the **M-VAVE SMK-37 Elite** and **SMK-37 Pro**). Name the FM-1 only if this package actually supports it.
3. A short description of what changed in this version
4. **Install** — the matching web installer, with a reminder to pick the Elite or Pro package, back up sample slots, leave USB connected while the keyboard is being written, and that the installer can restore M-VAVE stock firmware
   - Main: https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/
   - Nightly catalog: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/
5. **Packages** — a table of each file's size, CRC32 and SHA-256, plus the source commit
6. **Test status** — an honest label, for example "Device not tested. These packages passed software checks. They have not been signed off as a physical device test." Then the beta/risk sentence and a pointer to [LICENSING.md](../LICENSING.md)

Do not put internal process chatter in the public notes.

## Withdrawal

If a published package must be taken down (for example a black-screen boot):

1. Remove it from the installer catalog so it is no longer offered.
2. Delete the artifacts so the old package URLs return 404.
3. Record the withdrawal in [CHANGELOG.md](../CHANGELOG.md).
4. If a GitHub release exists for that build, mark it withdrawn or delete the release (keep the git tag unless a maintainer decides otherwise).
5. Never republish that same build.

Removing a catalog row without deleting the artifacts is not a complete withdrawal.
