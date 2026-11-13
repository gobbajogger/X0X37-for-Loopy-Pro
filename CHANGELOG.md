# Changelog

Firmware version labels are applied at package time. The numbers below are the public package versions of DnBOX.

## 0.0.25.1 — 2026-11-12

Source: `79280e763341d03ba43266d3e25dad66f12de860`.

- A pattern-chain slot lasts for the longest playing part in the current pattern (909, 808, both 303s and BREAK), counting length and rate, rounded up to a whole 909 bar.
- Muted, part-off and empty parts do not extend the slot. Chaining stays live-only and is not saved.
- Packages: SMK-37 Elite and Pro, 651,721 bytes each. Software-checked; **not signed off as a physical device test**.

Install with the [web installer](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/) or from [GitHub Releases (latest)](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/latest) (v0.0.25.1).

## 0.0.25

Combined zero-dropout work (808, 909/Miami, sequencer drain, Chicago accent). Merged to `main` in pull request #864. Followed by 0.0.25.1 above.

## 0.0.26 (not published)

Work continues on the combined 0.0.26 branch (pull request #869): Drone, held-key chords, Juno and chorus, and related fixes. A 0.0.26 package was briefly offered on the nightly installer and was **withdrawn** after a black-screen boot. It is not a current public install target.

## Older development packages

Earlier 0.0.7–0.0.24 development packages exist in repository history and older catalogs. They are not the current recommended install.
