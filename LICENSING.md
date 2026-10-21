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
| Clip chord recognition (`app/smk_chord_detect.h`) | adapted from [Polly](https://github.com/charlesvestal/fm1-polly/tree/62cd07ea7c95c30021c31e957cea8be448415fa7), `app/dsp/chords.c` and `chords.h`; extended chord shapes, compact note summaries and bounded text formatting for X0X | GPL-3.0-only |
| Break generator (`dsp/breaks*`, `tests/host/breaks_test.c`) | ported from [schwung-breakbeat](https://github.com/mestela/schwung-breakbeat) (BB Gen) by mestela, **used with the author's permission** (that repository carries no licence file; see "Break generator provenance" below) | GPL-3.0-only here, by permission |
| BLE-MIDI host (`firmware/src/ble/ble_midi_host.{c,h}`) | from [felucca-bsp](https://github.com/hugelton/felucca-bsp) `ble/ble_midi_host` (Leo Kuroshita, Hügelton Instruments), extended for X0X | MPL-2.0 (`LICENSES/MPL-2.0.txt`), compiled into the GPL app (MPL-2.0 section 3.3 allows it) |
| The app/controller interface (`firmware/hal/x0x_ble_abi.h`) and the controller shim (`ble/ctl/`: `src/`, `ctl.ld`, the build and fetch scripts, `mkimage.py`) | X0X | MPL-2.0 |
| Everything else in `firmware/src/{app,dsp,seq,ble}`, `host/`, `tests/` | X0X | GPL-3.0-only |

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

## The Bluetooth controller image (SMK-37)

Bluetooth MIDI on the SMK-37 needs JieLi's Bluetooth controller (`btctrler.a`, `wl_rf_common.a`,
the crypto libraries, the newlib and compiler runtime of the JieLi toolchain). These files are
closed or not GPL-compatible. The source repository does not contain those library files
and the GPL application does not link them. Controller images remain separate programs
under the applicable vendor terms.

- `ble/ctl/build_ctl.sh` runs on **your** computer. It fetches the files from JieLi's own
  repository (gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK, pinned to commit `d179b44`) and checks
  each SHA-256 (`btctrler.a`: `9ef42624...`). It then links them with the MPL-2.0 shim in
  `ble/ctl/src/` into a separate image, `ctl.img`.
- The image is a separate program. It runs from its own flash block (0x8B000) and its own RAM.
  The GPL app and the image share no code and call no function in each other except the image's
  two entry points (start and poll). They exchange only standard Bluetooth HCI packets through a
  shared-RAM mailbox, as an app processor and a separate Bluetooth chip do. The only file that
  both sides compile is `firmware/hal/x0x_ble_abi.h` (MPL-2.0).
- `tools/add_ctl.py`, `tools/fm1_install.py --ctl` and the web installer put your `ctl.img`
  into a package on the build or install computer. For an owner-authorized private development
  transfer, `tools/distribution/package_dev.py` makes an embedded matching controller the
  required user-facing package format. It retains the original raw DEV evidence internally.
  This private packaging policy does not grant public redistribution rights to vendor code;
  any other distribution still requires the applicable permissions and licensing review.
- The shim is MPL-2.0 so that you may link it with JieLi's files. The image that you build
  contains JieLi's files under JieLi's terms (the SDK's `LICENSE`, fetched with them). If you
  give that image to anyone else, those terms apply to it. X0X's GPL does not cover it.
- BlueKitchen BTstack is not used.

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

X0X enables the Bluetooth radio only on an SMK-37 that has a user-built controller image
(above) and only when GLOBAL > BLUETOOTH MIDI > BLUETOOTH is ON (it is OFF by default). Then it
runs Bluetooth Low Energy (BLE MIDI) only. X0X never enables Wi-Fi or Bluetooth Classic.
