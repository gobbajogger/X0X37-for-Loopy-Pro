# DnBOX / X0X37 for Loopy Pro

DnBOX (X0X37) is firmware that turns an **M-VAVE SMK-37 Elite** or **SMK-37 Pro** into a groovebox you can play with [Loopy Pro](https://loopypro.com/) and as a stand-alone instrument. It also runs on the M-VAVE FM-1.

It gives you a **909**, an **808**, two **303s** with an acid line generator, and a **breakbeat player**, all running at once. Each part can use its own pattern length and rate. You can chain patterns, record knob moves, and play everything in stereo through a shared reverb, tape delay and master compressor. Over USB the keyboard is a MIDI device and an audio interface, with no driver needed.

The factory sound is set up for big beat, so it should make noise the moment you press PLAY.

[![X0X running on the FM-1 (watch on YouTube)](https://img.youtube.com/vi/cZLYrZaLDUk/maxresdefault.jpg)](https://www.youtube.com/watch?v=cZLYrZaLDUk)

## Install

Use Chrome or Edge on a computer, with the keyboard on USB:

**[Open the installer](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/)**

- Pick the package that matches your model (**Elite** or **Pro**). Starrykey 37 Play uses the SMK-37 Pro package.
- **Back up your sample slots before you install.** The installer can do this for you.
- Do not unplug the USB cable while firmware is being written.
- You can also go back to M-VAVE stock firmware from the installer, or with [M-VAVE M-UPGRADE](https://www.m-vave.com/download).

See the [manual](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/manual/) and the [SMK-37 control map](SMK37.md). To load your own break loops, use the [sample importer](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/breaks/).

> **This firmware is in beta.** It is meant for everyday playing, but a very busy pattern can push the hardware to its limit. When that happens, X0X trades a little sound quality to keep playing rather than drop out. Installing it is at your own risk. Current packages are software-checked and **have not been signed off as a physical device test**. If something goes wrong, the installer can restore official firmware, and an FM-1 that keeps crashing starts in a safe mode. The manual's Troubleshooting section covers the rest.

## What's inside

| Part | What it is | Based on |
|---|---|---|
| 909 | 11 voices: modelled kick, snare, toms, rim and clap, with sampled hats and cymbals | [9W9](https://github.com/athousanddetails/schwung-9W9) (samples from ER-99) |
| 808 | 11 tracks, including the 808's tom/conga, rim/claves and clap/maracas switches | [8W8](https://github.com/athousanddetails/schwung-8W8) |
| 303 A and B | Open303 with Devilfish-style ranges, each with a TB-3PO line generator | [schwung-303](https://github.com/charlesvestal/schwung-303), [schwung-tb3po](https://github.com/charlesvestal/schwung-tb3po) |
| Break | A loop cut into eight slices and rearranged as it plays | [BB Gen](https://github.com/mestela/schwung-breakbeat) |

Each part has its own step length (up to 64 steps) and rate, so parts can drift against each other. Pattern changes wait for the end of the bar, and patterns can be chained.

On the SMK-37, clip mode can act as an Akai APC40 mkII for Loopy Pro. Bluetooth MIDI is available if you build a matching controller image on your own computer (it cannot be shipped with X0X): see the manual's "Bluetooth MIDI" section and [ble/README.md](ble/README.md).

X0X is a fork of [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita (Hügelton Instruments). It keeps Felucca's foundations (the hardware layer, USB, the update loader, the web installer and the storage) and replaces the instrument on top.

## Your own breaks

X0X doesn't ship with any recorded breaks, only two loops played by its own 909. You can load your own into the sample slots at 44.1 kHz. The SMK-37 has three slots of 80, 120 and 32 KiB (about 3.7, 5.5 and 1.5 seconds, 10.7 seconds in all); the FM-1 has two of 80 KiB (about 3.7 seconds each). Ask for 22.05 kHz to fit twice as much at lower quality. An install that would lose loops saves them first and writes them back after.

The [sample importer](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/breaks/) does this from Chrome or Edge. From the command line:

```
tools/upload_breaks.py --bars amen.wav think.wav ...   # take one bar of each and upload them
tools/upload_breaks.py --rate 22050 --bars ...          # half the rate: twice the room, duller
tools/upload_breaks.py --dry-run OUTDIR ...            # see what fits, without a device
tools/x0x_slots.py backup my.x0xslots                  # save every slot (read-only)
tools/x0x_slots.py restore my.x0xslots                 # write a backup back (asks first)
```

Recorded breaks belong to whoever made them and aren't covered by this project's licence.

## Building and testing

[BUILDING.md](BUILDING.md) explains the toolchain. In short:

```
tests/run_tests.sh      # maths, sequencer, engines, and the app in a simulator
./build.sh              # build/x0x.fwsc
```

`host/x0x_host` runs the whole app on a computer from a script (`tests/scenarios/*.x0x`). Button presses, keys, knob turns and MIDI go in; audio and screenshots come out.

## Licence and credits

X0X is GPL-3.0-only, like Felucca, 9W9, 8W8 and schwung-tb3po. Open303 (by Robin Schmidt) is MIT. BB Gen is ported from mestela's schwung-breakbeat with the author's permission. The 909's hi-hat, ride and crash samples come from ER-99 (Matthew Cieplak, GPL-3.0). The fonts are Barlow Semi Condensed and Terminus (SIL OFL 1.1). The BLE-MIDI host (from felucca-bsp) and the Bluetooth controller interface are MPL-2.0. JieLi's Bluetooth files are never part of X0X. See [LICENSING.md](LICENSING.md) and [LICENSE](LICENSE) for the full terms.

Felucca's own assets that aren't under its GPL (the icon set, the panel photo and the Hügelton drum pack) aren't part of this fork.

Felucca and Hügelton Instruments are names of Hügelton Instruments. M-VAVE, FM-1, SMK-37 and MKE-P37 are trademarks of their respective owners. TR-808, TR-909 and TB-303 are trademarks of Roland Corporation. Akai, APC40 and APC40 mkII are trademarks of inMusic Brands, Inc. Loopy Pro is a product of A Tasty Pixel. Novation and Launchpad are trademarks of the Focusrite group. All other names belong to their respective owners. X0X uses these names only to describe what the parts emulate and which products it can operate with. X0X is not affiliated with or endorsed by any of these owners.

The SMK-37 firmware answers a MIDI Device Inquiry with the Akai and APC40 mkII IDs only for interoperability with host software such as Loopy Pro.

## Changelog, contributing and security

- [CHANGELOG.md](CHANGELOG.md) — recent firmware changes
- [CONTRIBUTING.md](CONTRIBUTING.md) — how to report issues and propose changes
- [SECURITY.md](SECURITY.md) — how to report a vulnerability

People working in this repository should also read [BUILDING.md](BUILDING.md).
