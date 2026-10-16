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

## Where we are now (2026-10-16 23:27)

- Branches:
  - The working branch integration is at f357a2e. Its code tree is the same as 9a733fe. The later commits change only docs and the ledger.
  - It has ble-a75 (38af0a0), ble-pad-a76 (1c45b4b: violet and pad clear fixes, INLOG) and ble-arrows-a77 (59ddadd: < and > in the menus, GEN-56).
  - The gate on 9a733fe gave PASS at 22:29 (tree 1d3c048d). The gate at 22:24 was VOID (setup error). The install at 22:25 came before the PASS (ledger b1).
  - Tag last-good-hw = 94285b5 (a67, iPhone PASS 19:18). Next batch: apc-colors-int (f175c30).
  - The side branches ble-a68-opt (a1d8ea1), ble-a69-readback (75c42e1), ble-a70 (77fa067) and ble-a71 (9ef7bc2) have builds. Nobody installed them.
- Keyboard:
  - The keyboard runs integration 9a733fe (batch 1). Firmware Main Builder installed it at 22:25 (ledger b1, b913735).
  - The fwsc CRC32 is 3a738997. The ctl.img read-back is eb742dd2. Slots 1 and 2 did not change. DIAG shows READY.
  - The gate gave PASS at 22:29 (tree 1d3c048d, 93 ok, 0 FAIL, 0 skip). The gate Elite fwsc CRC32 3a738997 is the same as the installed file.
  - The install at 22:25 came before the gate PASS. This is a rule 7 break. Cause: Merge Integrator called a manual test run a gate PASS, and Firmware Main Builder accepted it without a check of index.md. We made no new rule.
  - The owner tests batch 1 only after the gate PASS (this is now true). If the owner test fails, Firmware Main Builder installs 14fb352 (a74) again.
  - Device test: PENDING (checklist-batch1). Tag last-good-hw stays 94285b5.
- a74 (14fb352, branch ble-a74):
  - Firmware Main Builder installed it at 21:03. Owner test 1 at 21:18 (ledger 11d81a7): violet FAIL, pad clear FAIL. The owner did not test the other items (NOT TESTED).
  - a74 adds two items to a73: the clock source setting INTERNAL / EXTERNAL / AUTO (MID-13) and the PLAY note (MID-14).
  - The gate gave PASS (tree 2f1f79d2ca5c). The ctl.img read-back is eb742dd2. The per-slot CRCs 05fb6e99 / 6bb165f0 did not change, and slot 3 is empty. DIAG shows READY. last-good-hw stays 94285b5 (a67).
- a73 (99faa0a):
  - Firmware Main Builder installed it at 20:26. Owner test 20:37 to 20:39 (ledger c3ee3ef): Loopy Start starts X0X (PASS). X0X PLAY does not start or stop Loopy Pro (FAIL).
  - The owner did not test the pads, the overdub and the tempo yet. last-good-hw stays 94285b5 (a67).
  - a73 (branch ble-a73) adds four changes to a67: the Bluetooth MIDI clock and transport fixes (a72) and the clip colour and feedback fixes (a71). It also adds the installer ctl.img read-back (a69) and live recording on the nearest step (REC-06).
  - The slot CRCs 05fb6e99 / 6bb165f0 did not change. DIAG shows READY.
- a65 (ca1d4d2):
  - Firmware Main Builder installed it at 18:19. It goes back to the Bluetooth host in the app (from fb6b1b0, test 37), with no pairing.
  - a65 does not use the CTL_HOST host of a38 to a64. It sends no connection parameter request (owner decision 18:18).
  - The slot CRCs 961626d5 / 9a09fdb9 did not change. Slot 3 is empty and is now 44 KiB at 0xB0000. DIAG shows READY, with no crash.
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
  - clip-keys-a65 is at 5a89b76. Its changes are in integration (cherry-picked).
  - It has the KEYS panel and the SCALE, CHORD and ARP pages. Knobs 2-8 send CC 48-54. It has 24 scales, 30 chords, swing, Euclid rhythm, HUMAN and strum.
  - Merged tree with a75: RAM .data+.bss is 94500 of 98304 B (ledger row 75). The host tests pass. The owner did not test it on the hardware.
- Integration: f357a2e. integration-stale-02ae8b8 keeps the stale 17:34 merges.
- Pool limit: build.sh and the gate require at least 8192 B of free pool (enforced 2026-10-16 21:53). USER_PREFERENCES.md keeps the older 4096 B owner entry.
- Tools: tools/ble_autotest.py runs the Bluetooth test over USB with no user action. The attempt ledger is BLE_ATTEMPTS.md in the Bluetooth branch (one row for each attempt). The JieLi SDK Bluetooth design compared with our host, with the step plan for the iPhone link, is in ~/Documents/x0x37/ble_split/SDK_DESIGN_COMPARE.md (Mac).

## Left to do

1. Done (a52, f0bcb28): Bluetooth starts. STATUS shows READY at power-on.
2. Mac: the connection works (a59, a60). Open on the Mac: test the clock and Start/Stop over Bluetooth, and check for audio dropouts. iPhone: done, the link works on a67 (94285b5, owner test 19:18).
   - Open: CLP-50 FAIL. X0X PLAY does not start or stop Loopy Pro (a73, owner test 20:37-20:39). Also test the a73 pads, overdub and tempo with Loopy Pro.
   - Open: CLP-13 FAIL (a74 owner test 1, 21:18). A violet clip from Loopy Pro shows blue on the pads. The fix is in 9a733fe (a76). Firmware Main Builder installed it at 22:25. Device test: pending.
   - Open: CLP-18 FAIL (a74 owner test 1, 21:18). When Loopy Pro disconnects, the pads go dark, but the clip colours come back immediately. The fix is in 9a733fe (a76). Firmware Main Builder installed it at 22:25. Device test: pending.
   - Required before a colour fix (agent rule 11): the owner records a playing blue clip and a playing violet clip from Loopy with INLOG on. The builder makes a host replay test from that log.
   - Open: owner test of batch 1 (9a733fe, checklist-batch1). If it fails, Firmware Main Builder installs 14fb352 again.
   - Open: the owner did not test the a74 clock source and PLAY note items yet.
   - Open: BLE-16 (All Notes Off when the Bluetooth link ends) is not in the code.
3. Turn Bluetooth on and off in GLOBAL without a restart. OFF: stop advertising and disconnect. ON: start the controller while the keyboard runs, with no audio dropouts (the radio calibration is the hard part).
4. Bring integration (f357a2e or later) into the main tree (x0x37, smk37) after an owner test PASS of an installed build.
5. Clip colours: the LCD colours are the plain Akai colours as RGB565, from the Akai CSV (CLP-05, on integration). The plan puts pad colour calibration (CLP-79) in batch 3.
6. Owner checks by ear and eye: the ADPCM cymbals, the 44.1 kHz factory breaks and the idle clip colours on the screen. Also check that the patterns and settings stayed the same.
7. Web installer: let the page get the JieLi files and build the Bluetooth image. Or let it guide the end user. If possible, end users need no toolchain.
8. Release: Merge Integrator builds the Pro and Elite packages from a hash with a gate PASS and a device PASS. It checks the installer hashes and publishes only when the owner asks.
9. Lane files: Firmware Main Builder and Second Builder agree on the split of the lane files after the batch 1 owner test.

## Open decisions

- Keep spare flash for growth, or give all of it to the loop slots.

## Not planned

- FM-1 build. We do not maintain it, and we accept its overflow.
- Bluetooth audio.
- Ship JieLi Bluetooth files with the firmware.

Last update: 2026-10-16 23:35.
