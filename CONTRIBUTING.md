# Contributing

Thanks for your interest in DnBOX. By taking part in this repository you agree to follow the [Code of Conduct](CODE_OF_CONDUCT.md).

## Using the firmware

Find the owner-tested v0.0.26 packages in the [nightly installer](https://gobbajogger.github.io/DnBOX-nightly/install/) and [GitHub release](https://github.com/gobbajogger/DnBOX/releases/tag/v0.0.26). The [stable installer](https://gobbajogger.github.io/DnBOX/install/) lists its current build on the page. Pick the Elite or Pro package that matches your keyboard, back up sample slots first, and do not unplug USB while writing. See the [README](README.md) and the [manual](https://gobbajogger.github.io/DnBOX/manual/).

After an install attempt, record the build, package checksum and what happened on your keyboard. [Open an install report](https://github.com/gobbajogger/DnBOX/issues/new?template=install-report.yml).

## Reporting a problem or idea

Please use the issue forms:

- Bug report: keyboard model (Elite / Pro), firmware version, steps to reproduce, and whether you backed up sample slots
- [Feature request](https://github.com/gobbajogger/DnBOX/issues/new?template=feature_request.yml) — describe the change, the keyboard model and what it would let you do. To vote on a proposal, add 👍 to its opening post. GitHub keeps the votes with the issue.

Do not attach private backups, credentials, or raw captures that you do not want public.

## Code and documentation changes

1. Branch from `integration/dnbox-consolidated` and open a pull request into that branch (not `main`). `main` is reserved for the owner's final sign-off. Follow the pull request template.
2. Keep changes focused. Do not commit secrets, firmware images (`.fwsc`, `.bin`, `.elf`, `.img`), logs, dumps, or installer zip archives.
3. Follow the licence in [LICENSE](LICENSE) and the third-party notices in [LICENSING.md](LICENSING.md) and [CREDITS.md](CREDITS.md).
4. Run the local gate and paste its final line in the pull request:

   `scripts/gate.sh <lane>`

   It wraps `tools/dnbox-gate.sh` and prints `DNBOX-GATE: PASS <sha>` or `DNBOX-GATE: FAIL <sha> <failed checks>`. Name the lane (`sound`, `fwui`, or `integ`) and list the done checks the PR covers. Build and test guidance is also in [BUILDING.md](BUILDING.md).

   The gate is fail-closed and offline by default. It prints `DNBOX-GATE: PASS <sha>` only if every required inner step ran and reported success; otherwise the line is `FAIL` or `INCOMPLETE`. It does not `git fetch`, `clone` or `ls-remote` unless you pass `--online` (or set `DNBOX_GATE_ONLINE=1`). A skipped network step prints `SKIP`. Prove the parser with `scripts/gate.sh --self-test` (or `scripts/gate-selftest.sh`).

   After a local PASS, post the result as a GitHub commit status so the pull request can merge into `integration/dnbox-consolidated`:

   `scripts/gate.sh <lane> --post`

   That uses `gh api` to set context `dnbox-gate` (success, failure, or error) on the tested SHA, with the `DNBOX-GATE` line as the description. GitHub Actions are not used. The consolidated branch requires a successful `dnbox-gate` status and a pull request; do not push that branch directly. A docs- or meta-only change may still name the SHA and explain why the full product gate was not required, but a merge into consolidated still needs a posted `dnbox-gate` success on the pull-request head.
5. Do not change GitHub workflow files under `.github/workflows/` unless a maintainer asks you to. GitHub Actions are off on this repository; checks run locally.

## Branches

Max 15 branches per repo; every branch needs an open PR; delete the branch right after merge/close, after the archive/<branch> tag.

Protected branches (`main`, `gh-pages`, `nightly` where it exists, and `integration/dnbox-consolidated`) count toward the cap. Keep at most two `backup/*` branches. Work that is closed or merged is preserved by the `archive/<branch>` tag, then the branch is deleted.

## Releases

Integration-branch tags for finished delivery-plan moves are described in [docs/RELEASES.md](docs/RELEASES.md). Public package releases stay on `v*` tags and `main`.

## Security

See [SECURITY.md](SECURITY.md). Do not file a public issue for a vulnerability.
