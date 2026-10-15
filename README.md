# X0X for the M-VAVE FM-1

X0X turns the M-VAVE FM-1 into a groovebox: a **909**, an **808**, two **303s** with an acid line
generator, and a **breakbeat player**, all playing at once. Each part can play its own pattern,
you can string patterns into a song, and you can record knob moves. Everything runs in stereo,
with a shared reverb and tape delay and a punchy master compressor. Over USB, the FM-1 is both a
MIDI device and an audio input, with no driver needed.

The factory sound is set up for big beat, so it should make noise the moment you press PLAY.

[![X0X running on the FM-1 (watch on YouTube)](https://img.youtube.com/vi/cZLYrZaLDUk/maxresdefault.jpg)](https://www.youtube.com/watch?v=cZLYrZaLDUk)

- **Try it in your browser** (no FM-1 needed): <https://charlesvestal.github.io/fm1-x0x/emu/>
- **Install it** (Chrome or Edge, with the FM-1 plugged in): <https://charlesvestal.github.io/fm1-x0x/install/>
- **Read the manual**: <https://charlesvestal.github.io/fm1-x0x/manual/>
- **Download the firmware file**: [releases](https://github.com/charlesvestal/fm1-x0x/releases)
- **M-Vave SMK-37:** see [SMK-37 instructions](SMK37.md).

> **X0X is in beta.** It's solid for everyday playing, but a very busy pattern can push the FM-1
> to its limit. When that happens, X0X trades a little sound quality to keep playing rather than
> drop out. Installing it is at your own risk, though it's hard to get stuck: the web installer
> can put M-VAVE's firmware back, an FM-1 that keeps crashing starts in a safe mode, and the
> manual's Troubleshooting section covers the rest.

## What's inside

| Part | What it is | Based on |
|---|---|---|
| 909 | 11 voices: modelled kick, snare, toms, rim and clap, with sampled hats and cymbals | [9W9](https://github.com/athousanddetails/schwung-9W9) (samples from ER-99) |
| 808 | 11 tracks, including the 808's tom/conga, rim/claves and clap/maracas switches | [8W8](https://github.com/athousanddetails/schwung-8W8) |
| 303 A and B | Open303 with Devilfish-style ranges, each with a TB-3PO line generator | [schwung-303](https://github.com/charlesvestal/schwung-303), [schwung-tb3po](https://github.com/charlesvestal/schwung-tb3po) |
| Break | A loop cut into eight slices and rearranged as it plays | [BB Gen](https://github.com/mestela/schwung-breakbeat) |

Each part has its own step length (up to 64 steps) and rate, so parts can drift against each
other. Pattern changes wait for the end of the bar, and patterns can be chained.

X0X is a fork of [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita (Hügelton
Instruments). It keeps Felucca's foundations (the hardware layer, USB, the update loader, the web
installer and the storage) and replaces the instrument on top.

## Your own breaks

X0X doesn't ship with any recorded breaks, only two loops played by its own 909. You can load
your own into the FM-1's three sample slots, which hold about nine one-bar loops in total.
The [break loops page](https://charlesvestal.github.io/fm1-x0x/breaks/) does it from Chrome or
Edge (drop the files, press Upload); from the command line:

```
tools/upload_breaks.py --bars amen.wav think.wav ...   # take one bar of each and upload them
tools/upload_breaks.py --dry-run OUTDIR ...            # see what fits, without the FM-1
tools/x0x_slots.py backup my.x0xslots                  # save every slot (read-only)
tools/x0x_slots.py restore my.x0xslots                 # write a backup back (asks first)
```

Recorded breaks belong to whoever made them and aren't covered by this project's licence.

## Building and testing

[BUILDING.md](BUILDING.md) explains the toolchain. In short:

```
tests/run_tests.sh      # the maths, the sequencer, every engine against its original, the app in a simulator
./build.sh              # build/x0x.fwsc
```

`host/x0x_host` runs the whole app on a computer from a script (`tests/scenarios/*.x0x`). Button
presses, keys, knob turns and MIDI go in; audio and screenshots come out. That's how most of
X0X is tested before it reaches the FM-1.

## Licence and credits

X0X is GPL-3.0-only, like Felucca, 9W9, 8W8 and schwung-tb3po. Open303 (by Robin Schmidt) is
MIT. BB Gen is ported from mestela's schwung-breakbeat with the author's permission. The 909's
hi-hat, ride and crash samples come from ER-99 (Matthew Cieplak, GPL-3.0). The fonts are Barlow
Semi Condensed and Terminus (SIL OFL 1.1).

Felucca's own assets that aren't under its GPL (the icon set, the panel photo and the Hügelton
drum pack) aren't part of this fork.

Felucca and Hügelton Instruments are names of Hügelton Instruments. M-VAVE, FM-1, SMK-37 and MKE-P37 are trademarks of their respective owners.
TR-808, TR-909 and TB-303 are trademarks of Roland Corporation. Akai, APC40 and APC40 mkII are trademarks of inMusic
Brands, Inc. Loopy Pro is a product of A Tasty Pixel. Novation and Launchpad are trademarks of the
Focusrite group. All other names belong to their respective owners. X0X uses these names only to
describe what the parts emulate and which products it can operate with. X0X is not affiliated with
or endorsed by any of these owners.

The SMK-37 firmware answers a MIDI Device Inquiry with the Akai and APC40 mkII IDs. It does this
only for interoperability with host software such as Loopy Pro. See [LICENSING.md](LICENSING.md)
for detail.
