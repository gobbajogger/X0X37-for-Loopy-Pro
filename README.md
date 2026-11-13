# DnBOX

DnBOX is firmware for the **M-VAVE SMK-37 Elite** and the **SMK-37 Pro**.
It turns the keyboard into a groovebox.
You can play it with [Loopy Pro](https://loopypro.com/) or as a stand-alone instrument.

Current public packages support the SMK-37 Elite and the SMK-37 Pro only.
Starrykey 37 Play uses the SMK-37 Pro package.
FM-1 builds exist in the source tree.
They are not in the current public packages.

[![Upstream X0X on the M-VAVE FM-1 (historical reference, watch on YouTube)](https://img.youtube.com/vi/cZLYrZaLDUk/maxresdefault.jpg)](https://www.youtube.com/watch?v=cZLYrZaLDUk)

## What it does

DnBOX runs a **909**, an **808**, two bass slots, and a **breakbeat player** at the same time.

| Part | What it is | Based on |
|---|---|---|
| 909 | 11 voices: modelled kick, snare, toms, rim and clap, with sampled hats and cymbals | [9W9](https://github.com/athousanddetails/schwung-9W9) (samples from ER-99) |
| 808 | 11 tracks, with the 808's tom/conga, rim/claves and clap/maracas switches | [8W8](https://github.com/athousanddetails/schwung-8W8) |
| Chicago | The 303 engine, with Open303 Devilfish-style ranges and a TB-3PO line generator | [schwung-303](https://github.com/charlesvestal/schwung-303), [schwung-tb3po](https://github.com/charlesvestal/schwung-tb3po) |
| Miami | A separate bass engine. Miami is not a 303 | — |
| Break | A loop cut into eight slices and rearranged as it plays | [BB Gen](https://github.com/mestela/schwung-breakbeat) |

Each bass slot can run Chicago or Miami.

In **0.0.26**, each bass slot can also run **Juno** or **Drone**.
Juno is a six-voice subtractive engine with chorus.
Drone is a four-voice string-machine engine.
Juno and Drone are not 303 engines.
Juno and Drone can record a held-key chord.
These engines and held-key chords are not in **0.0.25.1**.

Each part can use its own step length (up to 64 steps) and rate.
Parts can drift against each other.
Pattern changes wait for the end of the bar.
You can chain patterns and record knob moves.

Playback is stereo, through a shared reverb, tape delay and master compressor.
Over USB the keyboard is a MIDI device and an audio interface.
The keyboard needs no driver.

On the SMK-37, clip mode can act as an Akai APC40 mkII for Loopy Pro.
See [ble/README.md](ble/README.md) for Bluetooth MIDI.

## Versions

**Recommended published release (hardware qualification incomplete): DnBOX 0.0.25.1**, on [GitHub Releases](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/tag/v0.0.25.1). The release is published, but does **not** have complete device qualification for Elite or Pro. These are the downloadable files for the physical model printed on your keyboard:

- [SMK-37 Elite: `dnbox-x0x37-0.0.25.1-elite.fwsc`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/download/v0.0.25.1/dnbox-x0x37-0.0.25.1-elite.fwsc)
- [SMK-37 Pro: `dnbox-x0x37-0.0.25.1-pro.fwsc`](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/download/v0.0.25.1/dnbox-x0x37-0.0.25.1-pro.fwsc)

The [nightly installer](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/) also lists **0.0.26 from source `94197fc8`**, but marks it **hardware test: boot/restart not yet verified**. An earlier 0.0.26 package was withdrawn after a boot concern. This is **not a verified production upgrade**; use it only for deliberate hardware testing with a retained published 0.0.25.1 package and backups.

The owner reported testing 0.0.25.1 and an earlier 0.0.26 build on an SMK-37 Elite, **without confirming the installed package hash**. That does not validate startup or recovery of the current 0.0.26 candidate. The Pro package has no reported physical test. Neither model has completed full hardware qualification.

## Install

On a computer, open [the published DnBOX installer](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/) in Chrome or Edge. Use a USB data cable, click **Connect**, and allow **MIDI and SysEx** access. If permission is denied, enable it for the site and retry Connect; close other MIDI applications. Check the **physical Elite/Pro label**, not only the USB device name. Starrykey 37 Play uses the Pro package.

Choose the **visible model and an admitted published 0.0.25.1 entry** in the firmware list, review the package warning, and follow the confirmation. If the installer cannot identify and admit a suitable package, **stop**. The separate **local `.fwsc` / stock chooser is hidden and disabled** on the published main installer. The [GitHub Release downloads](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/tag/v0.0.25.1) above are real files, **not** an upload method on that page. The [nightly installer](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/) is for deliberately testing unqualified candidates, including the boot-unverified 0.0.26.

**First installation from M-VAVE factory firmware:** There is no DnBOX app or DnBOX project yet, so its `.x0xslots` sample backup and `.x0xproject` export cannot be created. Preserve any factory data using M-VAVE-supported methods, if available. Choose only a visible, admitted package for the physical model; if the main installer cannot complete its required sample-layout preflight and refuses the write, **stop rather than bypassing it**.

**Updating or reinstalling an already-running DnBOX:** Stop playback, make a `.x0xslots` backup via [Samples](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/breaks/), and separately export a portable `.x0xproject` (with samples) via [Projects](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/projects/). Confirm **both files exist** before writing. The main installer blocks a firmware write if a required at-risk sample backup fails; **nightly may warn and continue without a backup**, permanently losing samples. Do not unplug USB during an active write.
## Manual and help

- **[SMK-37 keyboard map](SMK37.md)** for the older port; the [0.0.26 control manual (draft)](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/docs/manual-94197fc8/docs/MANUAL.md) explains candidate-only screens, glyphs, chords and presets, but does **not** describe the published 0.0.25.1 firmware. The [published web manual](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/manual/) still describes the FM-1.
- **[Sample importer](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/breaks/)** — import your own break loops
- **[Projects](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/projects/)** — save, load and export projects

Repository and Pages URLs still use the previous name, because the GitHub repository has not been renamed.

## Restore

The next step depends on what the computer can **actually detect**, not on a momentary disconnect:

1. **Already-running DnBOX app (update or reinstall).** Create and check **both** DnBOX backups above, then choose the **listed, admitted** 0.0.25.1 package for the printed model in the [main installer](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/). Stop if admission fails; a downloaded `.fwsc` cannot be dropped into its hidden local chooser.
2. **Only the update loader responds after an interrupted write.** The app's sample reader is not running, so **you cannot create a new `.x0xslots` or project backup**. The main installer source supports resuming an interrupted write from the loader with a **listed, admitted and physical-model-matched package**, without an app-side backup. Existing unsaved samples may be lost. If that exact selection is unavailable, stop rather than force a file or guess a hardware recovery gesture.
3. **Restart is uncertain or no device appears.** Keep USB connected while a write is active. Once the write has ended, reconnect and use **Connect / Rescan** to distinguish the running app from the update loader. Check browser MIDI/SysEx permission and other MIDI applications before deciding; never assume a disconnect proves an install succeeded.
4. **Return to factory firmware.** Obtain the exact **Elite or Pro stock package** from [M-VAVE](https://www.m-vave.com/download) and follow the official **M-UPGRADE** instructions. The main DnBOX site's stock-file chooser is **not available**. The web installer does not bundle a factory image; stock firmware may not read DnBOX sample slots.

**After any write**, the browser's `Done` means a fresh application MIDI identity matched the selected package; it is not proof that the keyboard booted reliably, makes sound or recovered its data. Check the running UI and sound, then the project and sample slots. A partial restore or missing read-back is **not complete**. Keep the `.x0xslots` and `.x0xproject` files until checked. No special SMK-37 recovery button gesture is documented here.
## Samples, projects and presets

DnBOX does not ship with recorded breaks, only two loops from its own 909.
You can import your own loops into the sample slots at 44.1 kHz.
The SMK-37 has three slots of 80, 120 and 32 KiB (about 3.7, 5.5 and 1.5 seconds, 10.7 seconds in all).
You can ask for 22.05 kHz to fit twice as much at lower quality.

The [sample importer](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/breaks/) does this from Chrome or Edge.
While the normal DnBOX app responds, the Samples page can back up the slots; after an update, the installer attempts to restore compatible slots when a recoverable backup exists. A loader-only recovery cannot make a new backup.
From the command line:

```
tools/upload_breaks.py --bars amen.wav think.wav ...   # take one bar of each and upload them
tools/upload_breaks.py --rate 22050 --bars ...          # half the rate: twice the room, duller
tools/upload_breaks.py --dry-run OUTDIR ...            # see what fits, without a device
tools/x0x_slots.py backup my.x0xslots                  # save every slot (read-only)
tools/x0x_slots.py restore my.x0xslots                 # write a backup back (asks first)
```

You can save, load and reset a project on the keyboard.
You can export or import a portable `.x0xproject` file from the [Projects](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/projects/) page.

The device can recall factory Chicago and Miami presets.
User preset file import and export is not in the current packages.

This project's licence does not cover recorded breaks.
Those recordings belong to whoever made them.

## Building

[BUILDING.md](BUILDING.md) explains the toolchain. In short:

```
tests/run_tests.sh      # maths, sequencer, engines, and the app in a simulator
./build.sh              # build/x0x.fwsc
```

`host/x0x_host` runs the whole app on a computer from a script (`tests/scenarios/*.x0x`).
Button presses, keys, knob turns and MIDI go in.
Audio and screenshots come out.

## Licence and credits

DnBOX comes from [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita (Hügelton Instruments).
[Charles Vestal](https://github.com/charlesvestal) then made [X0X](https://github.com/charlesvestal/fm1-x0x), groovebox firmware for the M-VAVE FM-1.
[genkigenki](https://github.com/genkigenki) then made the [X0X37 SMK-37 port](https://github.com/genkigenki/fm1-x0x).
DnBOX keeps Felucca's foundations: the hardware layer, USB, the update loader, the web installer and the storage.
It replaces the instrument on top.

DnBOX is GPL-3.0-only, like Felucca, 9W9, 8W8 and schwung-tb3po.
Open303 (by Robin Schmidt) is MIT.
The project ports BB Gen from mestela's schwung-breakbeat.
Upstream declares MIT.
The inherited port uses GPL-3.0-only and retains the MIT notice.

The 909's hi-hat, ride and crash samples come from ER-99 (Matthew Cieplak, GPL-3.0).
The fonts are Barlow Semi Condensed and Terminus (SIL OFL 1.1).
The BLE-MIDI host (from felucca-bsp) and the Bluetooth controller interface are MPL-2.0.
JieLi's Bluetooth files are never part of this firmware tree.
Packages include three JieLi SDK files (Apache-2.0).
See [LICENSING.md](LICENSING.md), [CREDITS.md](CREDITS.md) and [LICENSE](LICENSE) for the full terms.

Felucca's own assets that are not under its GPL are not part of this fork.
Those assets are the icon set, the panel photo and the Hügelton drum pack.

Felucca and Hügelton Instruments are names of Hügelton Instruments.
M-VAVE, FM-1, SMK-37 and MKE-P37 are trademarks of their respective owners.
TR-808, TR-909 and TB-303 are trademarks of Roland Corporation.
Akai, APC40 and APC40 mkII are trademarks of inMusic Brands, Inc.
Loopy Pro is a product of A Tasty Pixel.

Novation and Launchpad are trademarks of the Focusrite group.
All other names belong to their respective owners.
DnBOX uses these names only to describe what the parts emulate and which products it can operate with.
DnBOX has no affiliation with these owners, and these owners do not endorse DnBOX.

The SMK-37 firmware answers a MIDI Device Inquiry with the Akai and APC40 mkII IDs only for interoperability with host software such as Loopy Pro.

## Support the original creators

DnBOX does not take donations.
It builds on other people's work.
If you want to support it, support the original creators directly:

- Leo Kuroshita / Hügelton Instruments (Felucca): [GitHub Sponsors](https://github.com/sponsors/hugelton) or [Felucca on itch.io](https://hugelton.itch.io/felucca)
- Charles Vestal (X0X, schwung-303, TB-3PO, Polly): [X0X](https://github.com/charlesvestal/fm1-x0x) or [GitHub](https://github.com/charlesvestal)
- genkigenki (X0X37 SMK-37 port): [SMK37](https://genkigenki.github.io/smk37/) or [the port](https://github.com/genkigenki/fm1-x0x)
- athousanddetails (9W9, 8W8): [GitHub](https://github.com/athousanddetails)
- Matthew Cieplak (ER-99): [ER-99](https://github.com/matthewcieplak/er-99)
- Robin Schmidt (Open303): [Open303](https://github.com/RobinSchmidt/Open303)
- mestela (BB Gen): [schwung-breakbeat](https://github.com/mestela/schwung-breakbeat)
- midilab (jc303): [Support midilab](https://midilab.co/#support)
- Dave Mollen (dm-Rat): [GitHub Sponsors](https://github.com/sponsors/davemollen)
- djphazer (Hemisphere Suite TB-3PO): [GitHub](https://github.com/djphazer)

## Changelog, contributing and security

- [CHANGELOG.md](CHANGELOG.md) — recent firmware changes
- [docs/RELEASING.md](docs/RELEASING.md) — how the project makes a public release
- [CONTRIBUTING.md](CONTRIBUTING.md) — how to report issues and propose changes
- [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) — Contributor Covenant 2.1
- [SECURITY.md](SECURITY.md) — how to report a vulnerability
- [CREDITS.md](CREDITS.md) — third-party authors and licences
