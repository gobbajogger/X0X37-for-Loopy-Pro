# X0X licensing

X0X is free software under the GNU General Public License, version 3 only
(`GPL-3.0-only`, full text in `LICENSE`). It is a fork of Felucca and keeps Felucca's
licence. If you distribute X0X, or firmware derived from it, you must give your
recipients its complete corresponding source under the same licence.

## Where the code comes from

| What | Origin | Licence |
| --- | --- | --- |
| Platform: `firmware/hal/`, `firmware/loader/`, `firmware/src/{libc,lcd,gfx,usb,storage,ota}.c`, `tools/` (build, package, install, upload), `web/` (installer) | [Felucca](https://github.com/hugelton/Felucca), Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments; `usb.c` and `storage.c` changed for X0X | GPL-3.0-only |
| `firmware/src/app/{slots,editor,panel,main_fm1}.c` | adapted from Felucca's `eng_sample.c`, `editor.c`, `panel.c`, `main.c` / `audio.c` | GPL-3.0-only |
| 909 (`dsp/drum909*`, `dsp/fxbus*`) | ported from [9W9](https://github.com/athousanddetails/schwung-9W9) (athousanddetails), itself after [ER-99](https://github.com/matthewcieplak/er-99) (Matthew Cieplak) | GPL-3.0 |
| 909 hi-hat, ride and crash samples (`assets/909/`) | ER-99 via 9W9 | GPL-3.0 |
| 808 (`dsp/drum808*`) | ported from [8W8](https://github.com/athousanddetails/schwung-8W8) (athousanddetails); circuit models after the TR-808 service notes and Werner / Abel / Smith; rim shot after sc808 (Yoshinosuke Horiuchi / Sam Aaron) | GPL-3.0 |
| 303 (`dsp/bass303*`) | ported from [schwung-303](https://github.com/charlesvestal/schwung-303): Open303 by Robin Schmidt (MIT), Devilfish extensions after jc303 (midilab), RAT drive after dm-Rat (Dave Mollen) | GPL-3.0 (Open303 parts MIT) |
| TB-3PO (`seq/tb3po*`) | ported from [schwung-tb3po](https://github.com/charlesvestal/schwung-tb3po), itself a port of the Phazerville Hemisphere Suite `TB_3PO` applet (djphazer and contributors) | GPL-3.0 |
| Break generator (`dsp/breaks*`, `tests/host/breaks_test.c`) | ported from [schwung-breakbeat](https://github.com/mestela/schwung-breakbeat) (BB Gen) by mestela, **used with the author's permission** (that repository carries no licence file; see "Break generator provenance" below) | GPL-3.0-only here, by permission |
| The idea of a browser emulator for FM-1 firmware (`web/emu/`) | [Groove OS](https://www.groove-os.com/) by Peter Gombos, whose site runs its firmware in the browser; no code from it | credit only |
| Everything else in `firmware/src/{app,dsp,seq}`, `host/`, `tests/` | X0X | GPL-3.0-only |

No recorded breaks are in this tree or in the default firmware: the built-in break loops are
generated at build time by X0X's own 909 code. `tools/import_breaks.py`,
`tools/gen_break_bank.py` (`X0X_BREAK_BANK`) and `tools/upload_breaks.py` can put recordings
of your choosing into a build or onto a device; those recordings keep their own copyright.

## Break generator provenance

- Origin: <https://github.com/mestela/schwung-breakbeat> (BB Gen), by mestela.
- That repository has no licence file. GitHub shows no licence for it (checked 2026-10-15).
  Without a licence, the default copyright of the author applies to that code.
- X0X uses the generator code in `firmware/src/dsp/breaks.c`, `firmware/src/dsp/breaks.h`
  and `tests/host/breaks_test.c`. This use relies on permission that the author gave to the
  X0X / fm1-x0x authors. This tree has no copy or record of that permission.
- The built-in break loops (`tools/gen_builtin_break.c`, `tools/gen_builtin_break.sh`) are
  output of the X0X 909 code. They contain no BB Gen code and no BB Gen samples.
- `tools/import_breaks.py` and `tools/upload_breaks.py --bbgen` can read BB Gen's sample
  folder from a separate checkout. Those samples are not in this tree or in a package.

**TODO:** Get a written licence from mestela (for example, a licence file in the upstream
repository) or a written record of the permission. Keep that record with this tree.

## Felucca Assets

Felucca's non-GPL assets — the icon atlas, the panel photo and the Hügelton drum pack —
are **not** part of this fork and are not used by it. Felucca's section 7 additional
permission concerns those assets only, so it is not needed here.

## Third-party material

| What | Licence | Where |
| --- | --- | --- |
| Barlow Semi Condensed (The Barlow Project Authors), the default UI face | SIL OFL 1.1 | `assets/fonts/BarlowSemiCondensed-*.ttf`, `assets/fonts/Barlow-OFL.txt` |
| Terminus (Dimitar Toshkov Zhekov), the `terminus` font set | SIL OFL 1.1 | `assets/fonts/ter-u*.bdf`, `assets/fonts/Terminus-LICENSE.txt` |
| Fukiai icon font (Hügelton Instruments), web pages only | MIT | `web/fukiai.ttf`, `web/FUKIAI-LICENSE.txt` |
| JieLi AC79 SDK: `uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin` are read from your SDK checkout at build time and placed in the package; no SDK files are in this tree (see below) | Apache-2.0 | <https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK>, licence text in `LICENSES/Apache-2.0.txt` |

## JieLi SDK files in each package

Each `.fwsc` package contains three files from the JieLi AC79 SDK:

- `uboot.boot`
- `cfg_tool.bin`
- `cfg/eq_cfg_hw.bin`

The build reads these files from your AC79 SDK checkout. `tools/build.py` (`SDK_SHA256`)
pins them to the SHA-256 values of `AC79NN_SDK_V1.2.1_2023-12-13`. This tree does not
contain them.

These files are under the Apache License 2.0. They are separate programs. They are not
part of X0X and X0X does not link to them. The package only puts them next to the X0X
firmware on the same flash (an aggregate). The GPL of X0X does not apply to them.

You must include `LICENSES/Apache-2.0.txt` with each package that you distribute. This
applies to a `.fwsc` file, a release download and an installer ZIP. Also include `LICENSE`
(GPL-3.0) and this file.

## Trademarks

- "Felucca" and "Hügelton Instruments" are names of Hügelton Instruments.
- "M-VAVE", "FM-1", "SMK-37", "SMK-37 Pro", "SMK-37 Elite" and "MKE-P37" are trademarks of
  their respective owners.
- TR-808, TR-909 and TB-303 are trademarks of Roland Corporation.
- Akai, Akai Professional, APC, APC40 and APC40 mkII are trademarks of inMusic Brands, Inc.
- Loopy Pro is a product of A Tasty Pixel. Its name belongs to its owner.
- Novation and Launchpad are trademarks of the Focusrite group (Focusrite Audio Engineering
  Limited / Focusrite Group PLC).
- All other names are trademarks of their respective owners.

X0X uses these names only to describe what the parts emulate and which products X0X can
operate with. X0X is independent firmware. It is not affiliated with, endorsed by or
supported by any of these owners.

The SMK-37 firmware answers a MIDI Device Inquiry with the Akai manufacturer ID (`0x47`) and
the APC40 mkII model ID (`0x29`). It does this only for interoperability, so that host
software (for example Loopy Pro) can use its APC40 mkII mapping. This reply does not
show that the device is an Akai product.

## Radio

X0X never enables the Bluetooth / Wi-Fi radio of the hardware.
