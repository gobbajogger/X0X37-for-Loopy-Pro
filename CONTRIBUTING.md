# Contributing

Thanks for your interest in DnBOX. By taking part in this repository you agree to follow the [Code of Conduct](CODE_OF_CONDUCT.md).

## Using the firmware

Install from the [web installer](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/) or the [latest GitHub release](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/latest) (v0.0.25.1). Pick the Elite or Pro package that matches your keyboard, back up sample slots first, and do not unplug USB while writing. See the [README](README.md) and the [manual](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/manual/).

## Reporting a problem or idea

Please use the issue forms:

- Bug report: keyboard model (Elite / Pro), firmware version, steps to reproduce, and whether you backed up sample slots
- Feature request

Do not attach private backups, credentials, or raw captures that you do not want public.

## Code and documentation changes

1. Fork the repository or push a branch, then open a pull request against `main` for docs or against the branch the maintainers are using for the current firmware work. Follow the pull request template.
2. Keep changes focused. Do not commit firmware images (`.fwsc`, `.bin`, `.elf`, `.img`), logs, dumps, or zip archives.
3. Follow the licence in [LICENSE](LICENSE) and the third-party notices in [LICENSING.md](LICENSING.md) and [CREDITS.md](CREDITS.md).
4. Build and test guidance is in [BUILDING.md](BUILDING.md).
5. Do not change GitHub workflow files under `.github/workflows/` unless a maintainer asks you to.

## Security

See [SECURITY.md](SECURITY.md). Do not file a public issue for a vulnerability.
