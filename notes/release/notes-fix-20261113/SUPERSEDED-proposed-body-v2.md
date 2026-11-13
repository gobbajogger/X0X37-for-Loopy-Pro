# DnBOX 0.0.25.1

Firmware for the **M-VAVE SMK-37 Elite** and **SMK-37 Pro**.

A chain slot now lasts for the longest playing part in the current pattern (909, 808, both 303s and BREAK), counting length and rate, rounded up to a whole 909 bar. Muted, part-off and empty parts do not extend the slot. Chaining stays live-only and is not saved.

## Install

Use the web installer (Chrome or Edge, USB):

https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/

Pick the matching Elite or Pro package. **Back up your sample slots before you install.** Do not unplug the USB cable while the keyboard is being written. The installer can also put M-VAVE stock firmware back.

## Packages

| File | Size | CRC32 | SHA-256 |
| --- | ---: | --- | --- |
| `dnbox-x0x37-0.0.25.1-elite.fwsc` | 651,721 | `8ff3ba29` | `5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a` |
| `dnbox-x0x37-0.0.25.1-pro.fwsc` | 651,721 | `45560337` | `6f135a3d03bb1a2a4d09d63040c719313612b85590a6af69b293a928e22285bf` |

Source commit: `79280e763341d03ba43266d3e25dad66f12de860`. Firmware reports 0.0.25, GIT 79280e76.

## Test status

**Owner-reported use, not a full hardware test.** The project owner reported that he installed 0.0.25.1 to recover from a problem with a later test build, that 0.0.25.1 works, and that the update from 0.0.25.1 to 0.0.26 was smooth. The report did not say which board (Elite or Pro) was used, and it did not confirm the installed package hash. This is a single owner report. It is not a full hardware qualification of either model.

These packages also passed the software checks: the emu360 timing gate (Elite 11,548 µs, Pro 11,564 µs, 0 late, limit 11,609 µs) and the bundled Bluetooth controller check. The emulator result is not a hardware result.

DnBOX is beta software. Installing it is at your own risk. Licence: GPL-3.0. See LICENSING.md in the repository.
