x0x37 team: do not run the gh-pages step (agent rule 12).

# Building X0X

The build makes three files in `build/`:

| File | What |
| --- | --- |
| `x0x.bin` | the firmware app |
| `loader/ota.bin` | the update loader |
| `x0x.fwsc` | the installable package (app + loader) |

## Prerequisites (macOS)

- Python 3 with Pillow: `pip3 install Pillow`
- Docker Desktop. The JieLi toolchain is Linux x86-64 only. The build runs each tool in a
  `linux/amd64` `debian:bookworm-slim` container (Rosetta on Apple silicon). Keep the source
  tree in a folder that Docker can share, for example under `/Users`.
- The JieLi Linux toolchain (clang 4.0.1 for pi32v2, from JieLi's package server):

  ```
  tools/get_toolchain.sh            # installs to ~/.jieli/toolchain
  ```

- The JieLi AC79 SDK (Apache-2.0). The package uses three of its files
  (`cpu/wl82/tools/uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin`). They are not part of this tree.

  ```
  git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
      https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
  ```

- Node.js (optional, for the web tests).

On Linux x86-64, the toolchain runs natively and you do not need Docker.

## Locked settings (regression guard)

`tools/locked_settings.json` lists the hardware-relevant settings. They are the Bluetooth TX level and
crystal trim, advertising, ATT MTU, no connection parameter request and the controller config. They are also
the RAM canary default, the pool limit, the sample slots and the flash map.
`tools/check_locked_settings.py` compares the source with it.
`tools/build.py`, `ble/ctl/build_ctl.sh` and `tests/run_tests.sh` run it, and a difference stops the build.
To change a value, change the source and the JSON in one commit that names the setting. Add a
BLE_ATTEMPTS.md row.

## Build

```
./build.sh
```

`JIELI_TOOLCHAIN` and `AC79_SDK` change the default locations
(`~/.jieli/toolchain`, `~/fw-AC79_AIoT_SDK`).

`./build.sh --release 0.1-beta` makes a release build. The package is `build/x0x-0.1-beta.fwsc`.

On macOS with podman instead of Docker, put a `docker` script first on your PATH. The script runs
`exec podman "$@"`. `X0X_JOBS` (default 4) limits parallel compiles: a podman machine drops
connections when many containers start at once.

Build option: `X0X_CDC=1` adds Felucca's USB serial function (off by default: one plain
MIDI interface).

Build option: `X0X_DEV_PADCAL` (SMK-37). `1` (the default) makes a dev build with the pad calibration
mode (BEHAVIOR.md CLP-79). `0` makes a release build without it: no PAD COLOURS row, no pad calibration
code or RAM, and no INLOG args 2-5 export. The pads use the owner-calibrated table `APC40_PAD_OWNER`
(BEHAVIOR.md CLP-13) in the two builds. To make the release build, use this command:

```
X0X_DEV_PADCAL=0 MODEL=elite ./build.sh --board smk37
```

To get the calibration mode again, build with `X0X_DEV_PADCAL=1` (or with no value). The tag `padcal-v1`
marks the complete calibration mode (CLP-79, orientation page, reopen fix, ok dot).

The build also generates `build/gen/`. It contains the font and the 909's cymbal samples (IMA ADPCM,
44.1 kHz) and tables (`tools/gen_drum_samples.py`). It also contains the built-in break loops (two 2-bar
loops, IMA ADPCM at 44.1 kHz, 220,500 B). The 909 port renders them on the host
(`tools/gen_builtin_break.sh`, needs a host `cc`). Opt-in: `X0X_BREAK_BANK=DIR` builds the break bank
from one-bar WAVs instead. `tools/import_breaks.py` makes such a folder from BB Gen's samples, in
`build/breaks/` (README.md, Breaks). The build fails if it links a soft-double routine (a `double` crept
in). It also checks the image, RAM and pool sizes.

### M-Vave SMK-37 Pro

```
BOARD=smk37 ./build.sh          # or: ./build.sh --board smk37  ->  build/x0x-smk37.fwsc
```

This build uses the same sources for the M-Vave SMK-37 Pro (the same AC791N chip, SPL and update loader,
but a different panel). Each difference is behind `BOARD_SMK37` (C, default 0). Only this build sets it
(`-DBOARD_SMK37=1`, for the app and the update loader). The plain `./build.sh` does not change. Its
`build/x0x.bin`, `build/loader/ota.bin` and `build/x0x.fwsc` stay byte for byte as before the SMK port.
The SMK build keeps its objects, app and loader in `build/smk37/`. It writes the package
`build/x0x-smk37.fwsc` (identity `SMK-37 Pro_900`). `--release X.Y` gives `x0x-smk37-X.Y.fwsc`
(`SMK-37 Pro_9XXYYZZ`).

`MODEL=elite` or `MODEL=mke` builds the same firmware under the model name of the SMK-37 Elite or the
MKE-P37. The identity is `SMK-37 Elite_900` / `MKE-P37_900`. The objects go in `build/smk37-elite/` /
`build/smk37-mke/`. The packages are `build/x0x-smk37-elite.fwsc` / `build/x0x-smk37-mke.fwsc`. Thus
M-Vave's updater still takes the factory package of that unit. The Starrykey 37 Play uses the Pro build.

The SMK build differs from the FM-1 build in these items:

- The flash map (`firmware/hal/fm1_flash_map.h`): an explicit allow-list with compile-time checks. No
  window reaches the head (0x0-0x3FFF), BTIF / USRTRIM (0xC0000-0xC1FFF, the factory key and pad
  calibration), the stock resources (0xF4000-0xFDFFF) or key_mac (0xFF000-). The RAM flash driver has a
  second guard.
- The store (`storage.c`): seven objects A/B at 0x9D000-0xAAFFF (spare to 0xAEFFF), the store owner tag at
  0xAF000. The update staging at 0xBB000 (`ota.c`). Three user sample slots (`app/slots.c`): slot 1 80 KiB at
  0xC2000 and slot 2 120 KiB at 0xD6000 (to 0xF3FFF). Slot 3 44 KiB at 0xB0000 (to 0xBAFFF).
- The Bluetooth controller block: 64 KiB at flash 0x8B000-0x9AFFF (XIP 0x02087000), at the top of the
  encrypted app slot. The app code ends below it: `APP_CODE_MAX` 0x86EE0 (552,672 B). `build.py` links the
  app to that length and stops when the image is larger.
- The package layout (`tools/fm1pkg_make.py --board smk37`): PRCT 0x9D000, the stock region table and app
  slot 0x97400 (each board sets the XIP length of the linker script). It also has the slot map record at
  flash.bin 0x9CF00 ("X0X-SLOTMAP-1", per slot offset and size, CRC-32). The installers compare it with the
  slots on the device. They save and restore the slots that an install can lose.
- The input HAL (`firmware/hal/smk_input.h`), the LCD pins and the pad LEDs (`fm1_lcd_hw.h`, `lcd.c`), and
  the control mapping (`app/panel.c`, `app/plat_fm1.c`, `app/main_fm1.c`). X0X's UI (`app/ui.c`) does not
  change.
- No TRS MIDI IN (`X0X_TRS` is 0: PH8 is a key-matrix row on the SMK).
- The LCD's SPI clock is 12 MHz, the rate of the stock firmware. `X0X_SMK_LCD_BAUD=n` tries 60 MHz / (n + 1).
  The FM-1 build runs it at 60 MHz.

Store owner: several FM-1 firmwares have ports to the SMK-37 Pro with the same board layer and the same
store area. The SMK build tags the store with its family (`FELUCCA_DATA_FAMILY`, default `X0X`). At boot,
X0X clears a store with the tag of another firmware, or with no tag (`FELUCCA_DATA_UNTAGGED_OK`, default
`0`), before it reads it. X0X never touches the samples, the calibration, the Bluetooth pairing or key_mac.

The tests (below) include the SMK flash map, the store, the package layout and the update path on
SMK-layout packages. They check `build/x0x-smk37.fwsc` when it exists.

## The Bluetooth controller image (SMK-37, optional)

Bluetooth MIDI on the SMK-37 needs a controller image that you build yourself. X0X does not
ship it (LICENSING.md). With the toolchain above and Docker (or on Linux x86-64 with
`CTL_NATIVE=1`), do these steps:

```
ble/ctl/build_ctl.sh        # fetches JieLi's files (pinned, SHA-256 checked), builds ble/ctl/out/ctl.img
python3 tools/add_ctl.py build/x0x-smk37.fwsc ble/ctl/out/ctl.img -o build/x0x-smk37-ble.fwsc
python3 tools/add_ctl.py ble/ctl/out/ctl.img --check
```

`build_ctl.sh` also copies the image to `~/.config/x0x/ctl.img`. `tools/fm1_install.py` adds
that copy to each SMK-37 install (`--ctl FILE` for another one, `--no-ctl` for none). The web
installer has a file field for it. See ble/README.md for how the image and the app fit together.

## Tests

```
tests/run_tests.sh
```

The script runs these tests on the build machine:

- The maths library against libm.
- The sequencer and TB-3PO, against schwung-tb3po (from `../schwung-tb3po` or `TB3PO_REF`).
- It compares each engine with its original (9W9, 8W8, schwung-303, BB Gen). The scripts in `tests/host/` give
  the location of those sources.
- Flash storage.
- Felucca's update-path tests against `build/x0x.fwsc` (the loader test needs `AC79_SDK`).
- The Bluetooth mailbox, BLE-MIDI host, image check and package insertion (`tests/ble_test.c`,
  `tests/ble_pkg_test.py`).
- It runs the whole app in the simulator (`tests/scenarios/*.x0x`) and writes its screenshots and
  audio to `build/scenarios/`.

`host/build_host.sh` builds the simulator alone. `build/host/x0x_host SCRIPT OUTDIR` runs one
script (the command list is at the top of `host/x0x_host.c`).

## Install

X0X's web installer is <https://charlesvestal.github.io/fm1-x0x/install/> (Felucca's, at
hugelton.github.io, installs Felucca). To install your own build, use the command line, or a local
copy of the site (below).

From the command line (needs `pip3 install mido python-rtmidi`):

```
python3 tools/fm1_install.py build/x0x.fwsc
python3 tools/fm1_install.py --info          # identity of the connected FM-1
```

Or, to install your own build from the web installer, make a local copy of the site. Open it from
`localhost` (Web MIDI needs a secure context):

```
./build.sh --release 0.1-beta                     # the site wants a release build (identity FM-1_9XY)
python3 tools/make_pages.py build/x0x-0.1-beta.fwsc 0.1-beta build/pages
cd build/pages && python3 -m http.server 8000
# open http://localhost:8000/install/
```

You install firmware at your own risk. Sometimes the FM-1 does not start but still reaches the chip's
update mode (4C4A:8057 on USB). Then `tools/fm1_rescue.sh` puts stock V15 back from a Mac (docs/MANUAL.md,
Troubleshooting). Otherwise, recovery needs [FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter).

## Publishing a release

1. `./build.sh --release X.Y-beta` (the identity, FM-1_9XY, is what the installers check).
2. `tools/publish_pages.sh X.Y-beta`: builds the site (landing page, web installer, manual,
   firmware) with `tools/make_pages.py`. It pushes the site to the `gh-pages` branch. The site
   is then on GitHub Pages at <https://charlesvestal.github.io/fm1-x0x/>.
3. `gh release create vX.Y-beta build/x0x-X.Y-beta.fwsc --prerelease` for the command-line
   download.
4. Attach `LICENSE`, `LICENSING.md` and the `LICENSES/` folder to the release. Each package
   contains three Apache-2.0 files from the JieLi SDK (LICENSING.md). Thus
   `LICENSES/Apache-2.0.txt` must go with each package.

### SMK-37 APC40 mkII / Loopy Pro clip launcher

The SMK-37 Pro and Elite builds include an APC40 mkII-compatible MIDI identity handshake. They also include the 40-cell APC clip-note/LED protocol and the APC velocity color palette. Clip feedback on MIDI channels 0–15 carries the same color. The firmware trims empty outer margins but preserves internal empty cells. An 8-wide layout shows one local 8×2 view (two rows) at a time. OCTAVE− and OCTAVE+ move it down and up by two rows within the latched clip area. A layout of 3 or 4 rows and 4 or fewer columns folds onto one 8×2 view. Source rows 0–1 (the lowest APC rows) map to physical pads 5–8 and 13–16. Rows 2–3 map to pads 1–4 and 9–12. The order of the pad rows is upside down (vertical flip), and the columns stay left-to-right. Holding KNOB BANK for more than 1 s toggles clip mode. In clip mode, a short KNOB BANK tap does nothing. FADER BANK pins the SCALE / CHORD / ARP page, and `<` / `>` step its last field (BEHAVIOR.md CLP-76). The firmware sends no APC navigation messages. Change the Loopy page to reach clips outside its 8×5 APC box. The firmware stores APC feedback as it arrives (there is no staged feedback). It latches the layout 250 ms after the first burst. Seq PLAY and Seq REC send momentary APC40 Play and Record messages in clip mode. Their LEDs follow Loopy's Play and Record/overdub feedback. Configure the Loopy Record binding and overdub behavior in Loopy Pro. Loopy Pro stays the source of clip state and color.

After you install the JieLi toolchain and AC79 SDK (above), use this command. It builds the two model-specific firmware packages and two matching web installers:

```
tools/build_smk_release.sh 0.2-apc40
```

This writes the Pro and Elite release packages plus `build/pages-smk37-pro/` and `build/pages-smk37-elite/`. Each generated installer embeds the matching model-specific firmware. Each installer ZIP must contain `LICENSE`, `LICENSING.md` and the `LICENSES/` folder. The reason: each package contains Apache-2.0 files from the JieLi SDK (LICENSING.md). The script does not add these files at this time. Add them to each ZIP before you publish it. For SMK packages, the installer hides the FM-1-only stock-firmware restore and recovery controls. Do not use an FM-1 stock image to recover an SMK keyboard.

If Pillow has no optional `libraqm` support, the font generator puts proportional digits in the center of the widest digit cell. Thus numeric fields stay tabular.
