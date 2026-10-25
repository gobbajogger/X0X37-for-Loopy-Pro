# Package and publication adapter

Use the [canonical contract](../../../../docs/engineering-delivery/SKILL.md) for
ownership, source-merge authority, acceptance and recovery. This file preserves
repository-specific technical facts, not a second owner-decision policy.

## Repositories and destinations

`gobbajogger/X0X37-for-Loopy-Pro` contains firmware and installer source. Read current
`nightly`, `main`, target branch and protections before an integration decision.
A feature PR base is not the firmware NIGHTLY integration target. Publication,
download hosting and picker selection remain distinct from firmware `nightly`/`main`.
`gobbajogger/x0x37-for-Loopy-Pro_nightly` is the separate browser/status repository;
confirm its current identity before writing. It receives links/hashes, not firmware
source, `.fwsc`, `ctl.img`, SDK/toolchain/DSP blobs, credentials, samples or backups.
The installer URL documented here is
https://gobbajogger.github.io/X0X37-for-Loopy-Pro/ from this repository's `gh-pages`.
Read current site/build source before assuming an old observation still applies.

## Publisher inputs

`tools/make_pages.py` accepts release identities only: `SMK-37 Pro_9` plus six digits, `SMK-37 Elite_9` plus six digits, `MKE-P37_9` plus six digits, and the FM-1 release form. Development identity `SMK-37 Pro_900` / `SMK-37 Elite_900` is rejected. Do not rename a dev package or edit its bytes so the generator will take it. Until a tested publisher path accepts a dev identity, a hardware-test candidate stays `PACKAGE_NOT_PUBLISHED`. `tools/publish_pages.sh --shell` builds an installer shell whose catalog can be empty. That shell is not a served candidate.

For a separately authorized release, follow `BUILDING.md` and actual license
files/headers. Keep notices and corresponding source required by applicable grants.
A redundant receipt is not required when genuine evidence is present; absence of
build bytes or rights evidence cannot be relabeled as a missing form. The repository
is not assumed wholly MIT merely because some components are MIT.

Build the affected models (MKE when the requested release script includes it).
Record model/controller identity, SHA-256, CRC and budgets. Static RAM free must be
nonnegative and pool headroom must meet `tools/locked_settings.json`; reconcile
source/config disagreement rather than choosing a lower value. Check affected
package integrity, source/config identity, sample preservation and bootloader
exclusions. Exercise installer wrong-model refusal before write, backup, interrupted
update, rollback, readback and per-slot preservation as applicable. SMC changes need
coverage of their actual BLE/HCI, decode, UI, feedback, forwarded-origin, user-mode
and mixer-selection paths. Tests of mocks do not prove hardware.

## Publication mechanism and readback

`tools/publish_pages.sh` historically archives the existing tree, merges kept
firmware and force-pushes an orphan `gh-pages` branch. That is a description of the
maintained script, not permission or a requirement to force-push. Inspect the current
script and specific publication authority before executing it. If authority excludes
that write, report the scoped blocker; do not replace it blindly or run it anyway.
Record the previous target ref and verified rollback/data compatibility first.

After an authorized publication, read remote SHA and fetch actual installer,
catalog and package bytes. Match lengths/model/hashes with qualified outputs. A
Pages commit alone is SITE BRANCH ONLY, not a served package. Keep previous safe
per-model candidates; malformed catalogs fail closed and unsafe candidates become
unselectable while evidence/history remain.

Current SMK packages rewrite the controller block, so an empty controller is
refused. Pair each package with its verified compatible controller; unchanged
controller reuse follows the canonical per-output closure rule. The separate status
repository stores neither package nor controller bytes. A production picker and
firmware `main` are separate destinations with separate action scope.
