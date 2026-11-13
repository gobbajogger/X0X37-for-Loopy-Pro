# Releasing DnBOX

This is the release standard for every public DnBOX package on the main repository and on the nightly repository. All bots, threads and Dot follow it. Do not invent a different tag, file name, or notes layout.

## Publish bar

A release may be published only when all of the following are true:

- Elite and Pro packages both built from the same source commit
- The full host test suite finished with 0 failures and 0 skips
- The emu360 heavy gate finished with 0 dropouts
- [CHANGELOG.md](../CHANGELOG.md) already has an entry for this version
- The build has not been withdrawn

Never publish a withdrawn build.

## Tag and target

- Main / stable: `vX.Y.Z` (for example `v0.0.25.1`)
- Nightly: `vX.Y.Z-nightly.YYYYMMDD` (or a close variant that still starts with `v`, includes the version, and includes a date)

The GitHub release **target must be the exact commit that produced the packages**. If the Releases UI cannot pick a SHA, create a `release/X.Y.Z` branch (or `release/X.Y.Z-nightly.YYYYMMDD`) that points at that commit, and target the release at that branch. Do not target a later tip or a mixed branch.

## Title, pre-release and Latest

- Title: `DnBOX X.Y.Z`. For a nightly, use `DnBOX X.Y.Z-nightly.YYYYMMDD`.
- Mark the release as **pre-release** if it is a nightly, or if it has not been signed off as a physical device test.
- Mark as **Latest** only for a main-repository stable release that is the current recommended install. Nightly releases and pre-releases are never Latest.

## Assets

Attach only:

- `dnbox-x0x37-X.Y.Z-elite.fwsc`
- `dnbox-x0x37-X.Y.Z-pro.fwsc`
- `SHA256SUMS.txt`, with each file's size in bytes, CRC32, and SHA-256

For a nightly, put the nightly tag in the version slot, for example `dnbox-x0x37-0.0.26-nightly.20261113-elite.fwsc`.

Do not attach internal or process notes, lane names, emulator output, raw captures, WAV files, debug images, or other junk.

## Notes

Public release notes are short and use these sections, in this order:

1. **Highlights**
2. **Fixes**
3. **Known issues**
4. **Supported models** (SMK-37 Elite, SMK-37 Pro, and FM-1 if this build supports it)
5. **Hardware-tested status** — an honest label, for example "software-checked only; not signed off as a physical device test" or "hardware-tested". Do not imply a device test that did not happen.
6. **Install** — the matching web installer:
   - Main: https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/
   - Nightly: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/
7. **Backup and restore** — back up sample slots before installing; do not unplug USB while writing; the installer or M-VAVE M-UPGRADE can restore stock firmware
8. **Checksums** — the size, CRC32 and SHA-256 of each package, or a clear pointer to `SHA256SUMS.txt`
9. **Credits** — a link to [CREDITS.md](../CREDITS.md) and [LICENSING.md](../LICENSING.md)

Do not put internal process chatter, lane names, or emulator/debug narrative in the public notes.

## Withdrawal

If a published package must be taken down (for example a black-screen boot):

1. Remove it from the installer catalog so it is no longer offered.
2. Delete the artifacts so the old package URLs return 404.
3. Record the withdrawal in [CHANGELOG.md](../CHANGELOG.md).
4. If a GitHub release exists for that build, mark it withdrawn or delete the release (keep the git tag unless a maintainer decides otherwise).
5. Never republish that same build.

Removing a catalog row without deleting the artifacts is not a complete withdrawal.
