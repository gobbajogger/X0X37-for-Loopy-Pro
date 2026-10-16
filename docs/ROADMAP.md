# X0X37 roadmap

This file tells what we build, what we finished, where we are and what we must still do. Update this file when a step ends or a plan changes. USER_PREFERENCES.md gives the preferences and rules. x0x37/BEHAVIOR.md gives the intended behavior. x0x37-progress-log.md gives the detailed history.

## The goal

We make X0X firmware for the M-Vave SMK-37 (Pro and Elite). The firmware has these properties:

1. It has a clip mode that acts as an Akai APC40 mkII for Loopy Pro. The pads start clips, and the pads and the screen show the clip colours.
2. It keeps all the X0X features (sequencer, synth engines, BREAK mode) at full quality: 44.1 kHz, with no length cuts.
3. End users can import their own loops for BREAK mode, and the loops stay safe across updates.
4. It has Bluetooth MIDI (MIDI only), and it stays freely shareable under an open license.
5. End users install it from a web page, with no toolchain.

## Done

| Area | What | Where |
|---|---|---|
| Clip mode | APC40 mkII emulation for Loopy Pro. The pads send and receive MIDI and show the clip colours. The screen shows the clip view. OCT- and OCT+ move the view down and up. | smk37 branch |
| Clip mode | Fixes: the firmware drops notes only in clip mode. Loopy Start does not echo back. The screen clip view is not too dark (LCD floor). | 0fbfe07 ... be6fc1d |
| Docs | BEHAVIOR.md (intended behavior, 200+ items), agent-rules.md, BREAK mode guide, README, licences | 4046fe9, 7269951, c24899f |
| Colours | Colour tool that calculates the real pad and screen colours from the APC40 palette | 07eb709 |
| Step 1 | The installer backs up the loop slots and restores them automatically | 39eafa9 ... bd6f9f3 |
| Step 2 | The firmware stores the 909 ride, crash and hi-hat as IMA ADPCM at 44.1 kHz (no audible difference, about 166 KB free after the change) | 241bd15 ... 936f155 |
| Step 3 | Factory breaks: 2 bars at 44.1 kHz. The import tools use 44.1 kHz | d3f23c5 ... 25c87bf |
| Step 4 | New flash layout: one 64 KiB Bluetooth block, 3 loop slots (about 11 s of import space at 44.1 kHz) | 62fc644 ... 81d5f74 |
| Step 5 | Bluetooth MIDI code: the JieLi controller runs as a separate image on core 1. The end user builds it on a computer. GLOBAL > BLUETOOTH MIDI settings | 07159b6 ... 7972fbb |
| Installed | The keyboard ran 81d5f74, and all 5 loops of the owner stayed intact | 2026-10-15 |
| Safety | The installer refuses to add the Bluetooth image to old firmware (the image can overwrite code) | ab44241 (flash_review, ble-stage3) |
| Safety | Bluetooth start guard: a failed Bluetooth start shows FAILED, and the keyboard starts normally (no crash loop into safe mode). The DIAG command reads the crash and start details | bad7b95, 70a9a5f (flash_review, ble-stage3) |
| Bluetooth | Bluetooth starts: STATUS shows READY at power-on (6 of 6 automatic rounds) | f0bcb28 (a52) |
| Bluetooth | The Mac connects over Bluetooth MIDI. MIDI goes in both directions | 79eb477 (a59), 4e6ae9b (a60) |
| Bluetooth | The iPhone connects over Bluetooth MIDI, and the link holds | 94285b5 (a67) |
| Clip keys | Knobs 2-8 send CC 48-54 when no clip keys mode is on | 9984344 |
| Bluetooth | BLE-16: when the Bluetooth link ends, X0X releases the notes that came from Bluetooth (host test tests/scenarios/blenotes.x0x). In the installed build 89f680e | 355f920 |
| Clip colours | The LCD clip colours are the plain Akai colours as RGB565, from the Akai CSV (CLP-05). In the installed build 89f680e | 453e9c8, 9a0b2c2 |
| Pad colours | Pad colour calibration (CLP-79, dev builds only). The build option X0X_DEV_PADCAL (default 1) removes it from a release build | d303e4e (in 89f680e), bd254a0 (integration be9a097) |
| Pad colours | The owner calibrated the pad colours (export of 2026-10-17, 89 entries with owner values). The clip pads use this table APC40_PAD_OWNER in the dev build and in the release build (CLP-13) | 48892fe, 4113ce0 (in the installed build c6bf44a) |
| Gate | The gate builds and tests the dev configuration and the release configuration. The install guard checks the configuration | 7cef9e4, 2012283 (in f9bcc46) |

## Where we are now (2026-10-17 02:45)

- Branches:
  - Installed = batch 10 (c6bf44a), release configuration (X0X_DEV_PADCAL=0). Batch 10 contains the batches before it. c6bf44a is the integration tip.
  - The gate on c6bf44a gave PASS at 2026-10-17 02:19 (tree b220466e91d0). It built and tested the dev and the release configuration (116 ok, 0 FAIL, 0 skip in each).
  - Tag last-good-hw = 94285b5 (a67, iPhone PASS 19:18).
  - The side branches ble-a68-opt (a1d8ea1), ble-a69-readback (75c42e1), ble-a70 (77fa067) and ble-a71 (9ef7bc2) have builds. Nobody installed them.
- Keyboard:
  - The keyboard runs integration c6bf44a (batch 10) in the release configuration. The install time is 2026-10-17 02:25 (INSTALLED.tsv). It has no PAD COLOURS page, and the owner pad colours are in the firmware.
  - The fwsc CRC32 is c48b5c6e. The ctl.img read-back is eb742dd2 (the same as the file). Slots 1 and 2 did not change, and slot 3 is empty. DIAG shows READY.
  - Free bytes of the Elite release build at c6bf44a: app code 3864 B, static RAM 5824 B, pool 8196 B.
  - Device test: PENDING (batch 10). Regression (owner 02:49): in clip mode, the knobs do nothing. Tag last-good-hw stays 94285b5.
- iPhone:
  - The iPhone works on a67 (owner test PASS 19:18). On a73, the link carries the Loopy clock and Start (20:37 PASS).
  - History: a65 iPhone test 1 (18:23): the iPhone connected (interval 7.5 ms, MTU 185). Then the link ended with reason 0x08 (connection timeout).
  - a62 and a62b: the link held for about 2 min at 26.25 ms with 0 connection updates, reason 0x00. a63-15 (d9844a4): iOS dropped the link before any GATT step.
  - Oct 15: Loopy used the keyboard as its APC40 over Bluetooth (test 30). But some colour notes made sounds, some pads stayed empty and the colours were slow.
  - A connection parameter request made the iPhone drop the link (test 31) and gave a large MIDI delay (test 34).
- Mac: the Bluetooth GATT connection works, and MIDI goes in both directions (a59 79eb477, a60). The a59 fix: the host must register its event handler after btstack_task_init, because btstack_task_init removes it.
- Bluetooth start: the supply must have the stock values (DCDC13, VDC14 1.60 V, SYSVDD 1.38 V) before the RF init (test 27, 56a7452). The controller init runs on core 0 before core 1 starts (83cd4dc). The clock stays at 360 MHz.
- Release package 0b80994 (SMK-37 Elite only, a64 files: .fwsc CRC32 5ec8b21a, ctl.img CRC32 e9c05144):
  - The free pool is 4096 B. The free app code is 144 B. The free app RAM is 0 B.
  - Nobody built the Pro package. Nobody pushed the package. It is for tests only, it is out of date, and it is not for release.
- Clip keys:
  - clip-keys-a65 is at 5a89b76. Its changes are in integration (cherry-picked) and in the installed build 89f680e.
  - It has the KEYS panel and the SCALE, CHORD and ARP pages. Knobs 2-8 send CC 48-54. It has 24 scales, 30 chords, swing, Euclid rhythm, HUMAN and strum.
  - The host tests pass. The owner did not test it on the hardware.
- Pool limit: build.sh and the gate require at least 8192 B of free pool (enforced 2026-10-16 21:53). USER_PREFERENCES.md keeps the older 4096 B owner entry.
- Tools: tools/ble_autotest.py runs the Bluetooth test over USB with no user action. The attempt ledger is BLE_ATTEMPTS.md in the Bluetooth branch (one row for each attempt). The JieLi SDK Bluetooth design compared with our host, with the step plan for the iPhone link, is in ~/Documents/x0x37/ble_split/SDK_DESIGN_COMPARE.md (Mac).

## Left to do

1. Mac: test the clock and Start/Stop over Bluetooth, and check for audio dropouts.
2. Clip mode with Loopy Pro:
   - Open: CLP-50 FAIL. X0X PLAY does not start or stop Loopy Pro (a73, owner test 20:37-20:39). Also test the pads, overdub and tempo with Loopy Pro.
   - Open: CLP-13 FAIL (a74 owner test 1, 21:18). A violet clip from Loopy Pro shows blue on the pads. The installed build c6bf44a has the owner pad table (48892fe). In c6bf44a, an idle pad sends the full owner table level (4113ce0). Device test: pending.
   - Open: CLP-18 FAIL (a74 owner test 1, 21:18). When Loopy Pro disconnects, the pads go dark, but the clip colours come back immediately. The fix (a76) is in the installed build c6bf44a. In c6bf44a, a link end clears only the clip cells of that link (30a08fc, 79de009). Device test: pending.
   - Open: the owner test of batch 10 (c6bf44a).
   - Open: regression on c6bf44a (owner 02:49). In clip mode, the knobs do nothing. Firmware Main Builder records a knob turn on the device, then finds the cause. The fix goes in batch 11. At 03:20, the owner said "Just fix them". After the fix, knobs 2 to 8 edit ARP, CHORD and SCALE again.
   - Batch 11 (owner 02:49, Second Builder): in clip mode, a tap on MODE latches the MODE UI open, and a second tap closes it. A hold works as before.
   - Batch 11 (owner 02:50, Second Builder): show a "waiting for Loopy Pro to connect" screen instead of the 8x5 grid.
   - Batch 11 (owner 02:52, Second Builder): a tap on the stage latches it open, and a tap closes it. FADER BANK also closes it. The screen has no top bar, and it shows a help line at the bottom. Nobody confirmed the link to the 02:49 MODE item. Second Builder confirms it.
   - Open: the owner did not test the a74 clock source (MID-13) and PLAY note (MID-14) items yet.
3. Turn Bluetooth on and off in GLOBAL without a restart. OFF: stop advertising and disconnect. ON: start the controller while the keyboard runs, with no audio dropouts (the radio calibration is the hard part).
4. Bring integration (c6bf44a or later) into the main tree (x0x37, smk37) after an owner test PASS of an installed build.
5. Owner checks by ear and eye: the ADPCM cymbals, the 44.1 kHz factory breaks and the idle clip colours on the screen. Also check that the patterns and settings stayed the same.
6. Web installer: let the page get the JieLi files and build the Bluetooth image. Or let it guide the end user. If possible, end users need no toolchain.
7. Release: Merge Integrator builds the Pro and Elite packages from a hash with a gate PASS and a device PASS. It checks the installer hashes and publishes only when the owner asks. The release build has X0X_DEV_PADCAL=0. It has no PAD COLOURS row, and it uses the pad table that the owner calibrated. Pad calibration is only for development (owner, 2026-10-17 00:01). The build option is in the code (bd254a0, BUILDING.md c3cdb5a).

## Open decisions

- Keep spare flash for growth, or give all of it to the loop slots.
- Pairing: keep no pairing, or add pairing and a "forget pairing" row (BEHAVIOR.md OQ-11).
- Installer: add ctl.img to an SMK-37 install by itself, or only with --ctl (BEHAVIOR.md OQ-12).

## Not planned

- FM-1 build. We do not maintain it, and we accept its overflow.
- Bluetooth audio.
- Ship JieLi Bluetooth files with the firmware.

Last update: 2026-10-17 03:24.
