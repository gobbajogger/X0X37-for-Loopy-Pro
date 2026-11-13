# Contributing

Thanks for your interest in DnBOX / X0X37.

## Using the firmware

Install from the [web installer](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/). Pick the Elite or Pro package that matches your keyboard, back up sample slots first, and do not unplug USB while writing. See the [README](README.md) and the [manual](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/manual/).

## Reporting a problem or idea

Open a GitHub issue with:

- your keyboard model (SMK-37 Elite, SMK-37 Pro, or FM-1)
- the firmware version shown on the device or in the installer
- what you did, what you expected, and what happened
- whether you can repeat it

Do not attach private backups, credentials, or raw captures that you do not want public.

## Code and documentation changes

1. Fork the repository or push a branch, then open a pull request against `main` for docs or against the branch the maintainers are using for the current firmware work.
2. Keep changes focused. Do not commit firmware images (`.fwsc`, `.bin`, `.elf`, `.img`), logs, dumps, or zip archives.
3. Follow the licence in [LICENSE](LICENSE) and the third-party notices in [LICENSING.md](LICENSING.md).
4. Build and test guidance is in [BUILDING.md](BUILDING.md).

Please do not change files under `.github/` unless a maintainer asks you to.

## Security

See [SECURITY.md](SECURITY.md). Do not file a public issue for a vulnerability.
