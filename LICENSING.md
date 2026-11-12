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
| TB-3PO (`seq/tb3po*`) | ported from [schwung-tb3po](https://github.com/charlesvestal/schwung-tb3po), itself a port of the Phazerville Hemisphere Suite `TB_3PO` applet (djphazer and contributors) | GPL-3.0 |
| Clip chord recognition (`app/smk_chord_detect.h`) | adapted from [Polly](https://github.com/charlesvestal/fm1-polly/tree/62cd07ea7c95c30021c31e957cea8be448415fa7), `app/dsp/chords.c` and `chords.h`; extended chord shapes, compact note summaries and bounded text formatting for X0X | GPL-3.0-only |
| Break generator (`dsp/breaks*`, `tests/host/breaks_test.c`) | ported from [schwung-breakbeat](https://github.com/mestela/schwung-breakbeat) (BB Gen) by mestela; upstream declares MIT in `src/module.json`; the inherited port records the author's permission (see "Break generator provenance" below) | GPL-3.0-only port; upstream MIT terms and notice retained |
| BLE-MIDI host (`firmware/src/ble/ble_midi_host.{c,h}`) | from [felucca-bsp](https://github.com/hugelton/felucca-bsp) `ble/ble_midi_host` (Leo Kuroshita, Hügelton Instruments), extended for X0X | MPL-2.0 (`LICENSES/MPL-2.0.txt`), compiled into the GPL app (MPL-2.0 section 3.3 allows it) |
| The app/controller interface (`firmware/hal/x0x_ble_abi.h`) and the controller shim (`ble/ctl/`: `src/`, `ctl.ld`, the build and fetch scripts, `mkimage.py`) | X0X | MPL-2.0 |
| Everything else in `firmware/src/{app,dsp,seq,ble}`, `host/`, `tests/` | X0X | GPL-3.0-only |

No recorded breaks are in this tree or in the default firmware: the built-in break loops are
generated at build time by X0X's own 909 code. `tools/import_breaks.py`,
`tools/gen_break_bank.py` (`X0X_BREAK_BANK`) and `tools/upload_breaks.py` can put recordings
of your choosing into a build or onto a device; those recordings keep their own copyright.

## Break generator provenance

- Origin: <https://github.com/mestela/schwung-breakbeat> (BB Gen), by mestela.
- Upstream explicitly declares `"author": "mestela"` and `"license": "MIT"` in
  `src/module.json`, lines 7–8: [initial commit](https://github.com/mestela/schwung-breakbeat/blob/cb205ada577c4b4d54e6ce28ad83d32c75ad08b9/src/module.json#L7-L8),
  [matching historical implementation](https://github.com/mestela/schwung-breakbeat/blob/c3ea85da9025a3694c167e096cb09d0efb60420c/src/module.json#L7-L8),
  and [checked upstream head](https://github.com/mestela/schwung-breakbeat/blob/c6e5b94dad1c0c5644c6a68052ba110d6a5a2c85/src/module.json#L7-L8).
  The absence of a standalone licence file does not negate this declaration.
- X0X retains adapted generator logic in `firmware/src/dsp/breaks.c`, with its interface
  in `firmware/src/dsp/breaks.h` and tests in `tests/host/breaks_test.c`. The anchor table,
  fill equations, MOVE/STAY branches and random draw order match the historical
  [slice selector](https://github.com/mestela/schwung-breakbeat/blob/c3ea85da9025a3694c167e096cb09d0efb60420c/src/dsp/slice_select.c).
  This identifies matching source, not the exact upstream checkout used by the original porter.
- The [initial fm1-x0x licensing record](https://github.com/charlesvestal/fm1-x0x/blob/2377537582b8d9a236edde8c2bf4e9ae83f4792c/LICENSING.md)
  labels the port GPL-3.0-only here, by permission. Its
  [source header](https://github.com/charlesvestal/fm1-x0x/blob/2377537582b8d9a236edde8c2bf4e9ae83f4792c/firmware/src/dsp/breaks.c#L1-L38)
  and [README credits](https://github.com/charlesvestal/fm1-x0x/blob/54194c54bb01e9d9b3fe9d641b3ddfa512ac04a3/README.md#L67-L72)
  retain mestela's attribution and record permission. Those inherited notices remain.
- Include [the BB Gen MIT notice](docs/notices/schwung-breakbeat-MIT.txt) with copies or
  substantial portions of the adapted generator, including distributed firmware and its
  corresponding source. X0X and the port remain GPL-3.0-only; complete corresponding-source
  and other component licence requirements still apply. This corrects only the former
  requirement to obtain a separate BB Gen permission record: the explicit upstream MIT
  declaration supplies the licence evidence. It does not clear QA, OTA, hosting or publication
  gates, or clear the controller distribution questions below.
- The built-in break loops (`tools/gen_builtin_break.c`, `tools/gen_builtin_break.sh`) are
  output of the X0X 909 code. They contain no BB Gen code and no BB Gen samples.
- `tools/import_breaks.py` and `tools/upload_breaks.py --bbgen` can read BB Gen's sample
  folder from a separate checkout. Those samples are not in this tree or in a package.

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

Qualified new NIGHTLY packages must include their exact matching Bluetooth controller.
The existing published Elite/Pro packages have erased controller blocks and still require
the exact local `ctl.img`; do not label those bytes bundled or remove their working local
composition path. Source support for combined packages is not publication approval.

The controller links six SDK archives with the MPL-2.0 shim in `ble/ctl/`. The SDK fetch
pin is `d179b4484759423312073f5fbb232501aa491047`; its exact 11,357-byte `LICENSE`
has SHA256 `c71d239df91726fc519c6eb72d318ec65820627232b2f796219e87dcf35d0ab4`
and contains Apache License 2.0. The official later README also describes the SDK as
Apache-2.0. This establishes SDK-level evidence, not every linked component's origin.

The former blanket statement that these archives are closed or not GPL compatible was
not established by component-specific evidence. The earlier no-shipping wording was
project policy, not an independently verified vendor prohibition. Equally, do not assume
that every archive is freely redistributable from the root license alone. Related crypto
headers contain BSD-2-Clause, public-domain and GPL-2.0-or-later notices; exact retained
archive members and their sources need mapping. Newlib has per-file notices; the exact
compiler runtime version, origin and terms remain unverified. See the
[dated evidence and unresolved conditions](docs/controller-licensing-evidence-20261025.md).

The app and controller remain separate executable images, communicating through standard
Bluetooth HCI packets and the MPL-2.0 ABI/mailbox interface. This technical layout does not
by itself settle license compatibility or conveyance obligations. BlueKitchen BTstack is
not used by this build.

`tools/package_nightly.py` reuses the existing `tools/add_ctl.py` composition and atomic
output path, requires an explicit reviewed controller hash/size, and verifies the exact
embedded bytes and model/layout/CRCs. It creates no legal, independent-QA or publication
approval. Owner-local composition can proceed. Public distribution remains **undecided**
until component grants, notices and applicable source obligations are established for the
exact output. Private transfer to another recipient also requires applicable conditions;
privacy alone is not an exemption.

Where applicable, retain Apache license/NOTICE and modification notices, provide MPL
covered-source availability and notices, preserve runtime-specific notices/disclaimers,
and provide the GPL app's complete corresponding source. Before any combined artifact is
hosted, the existing rights/release owner must review that exact artifact's source and
notice package. Do not commit SDK archives, toolchain files or controller binaries here.

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

X0X enables the Bluetooth radio only on an SMK-37 that has a matching validated controller image
(above) and only when GLOBAL > BLUETOOTH MIDI > BLUETOOTH is ON (it is OFF by default). Then it
runs Bluetooth Low Energy (BLE MIDI) only. X0X never enables Wi-Fi or Bluetooth Classic.
