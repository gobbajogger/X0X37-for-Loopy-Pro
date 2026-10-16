# X0X37 roadmap

What we build, what is done, where we are, and what is left. Update this file when a step finishes or a plan changes. Preferences and rules: USER_PREFERENCES.md. Intended behaviour: x0x37/BEHAVIOR.md. Detailed history: x0x37-progress-log.md.

## The goal

X0X firmware for the M-Vave SMK-37 (Pro and Elite) that:

1. Has a clip mode that acts as an Akai APC40 mkII for Loopy Pro. Pads launch clips, and the pads and screen show the clip colours.
2. Keeps all the X0X features (sequencer, synth engines, BREAK mode) at full quality: 44.1 kHz, no length cuts.
3. Lets the user import their own loops for BREAK mode, and keeps them safe across updates.
4. Has Bluetooth MIDI (MIDI only), and stays freely shareable under an open license.
5. Installs from a web page with no toolchain needed for end users.

## Done

| Area | What | Where |
|---|---|---|
| Clip mode | APC40 mkII emulation for Loopy Pro: pad MIDI send and receive, clip colours on the pads, clip view on the screen, OCT- / OCT+ move the view down / up | smk37 branch |
| Clip mode | Fixes: notes are dropped only in clip mode, Loopy Start does not echo back, the screen clip view is not too dark (LCD floor) | 0fbfe07 ... be6fc1d |
| Docs | BEHAVIOR.md (intended behaviour, 200+ items), agent-rules.md, BREAK mode guide, README, licences | 4046fe9, 7269951, c24899f |
| Colours | Colour tool that calculates the real pad and screen colours from the APC40 palette | 07eb709 |
| Step 1 | The installer backs up the imported loop slots and restores them automatically | 39eafa9 ... bd6f9f3 |
| Step 2 | 909 ride, crash and hi-hat stored as IMA ADPCM at 44.1 kHz (no audible difference, about 166 KB freed) | 241bd15 ... 936f155 |
| Step 3 | Factory breaks: 2 bars at 44.1 kHz. Import tools use 44.1 kHz | d3f23c5 ... 25c87bf |
| Step 4 | New flash layout: one 64 KiB Bluetooth block, 3 loop slots (about 11 s of import space at 44.1 kHz) | 62fc644 ... 81d5f74 |
| Step 5 | Bluetooth MIDI code: the JieLi controller runs as a separate image on core 1, built on the computer of the user. GLOBAL > BLUETOOTH MIDI settings | 07159b6 ... 7972fbb |
| Installed | The keyboard ran 81d5f74 with all 5 user loops intact | 2026-10-15 |
| Safety | The installer refuses to add the Bluetooth image to old firmware (it would overwrite code) | ab44241 (flash_review, ble-stage3) |
| Safety | Bluetooth start guard: a failed Bluetooth start shows FAILED and the keyboard starts normally (no crash loop into safe mode). DIAG command reads crash and start details | bad7b95, 70a9a5f (flash_review, ble-stage3) |
| Bluetooth | Bluetooth starts: STATUS READY at power-on (6 of 6 automatic rounds) | f0bcb28 (a52) |
| Bluetooth | The Mac connects over BLE MIDI. MIDI goes in both directions | 79eb477 (a59), 4e6ae9b (a60) |
| Bluetooth | iPhone connects over BLE MIDI and the link holds | 94285b5 (a67) |
| Clip keys | Knobs 2-8 send CC 48-54 when no clip keys mode is on | 9984344 |

## Where we are now (2026-10-16 22:39)

- Branches: working branch integration f357a2e (code tree the same as 9a733fe; later commits are docs and the ledger only). It has ble-a75 (38af0a0), ble-pad-a76 (1c45b4b: violet and pad clear fixes, INLOG) and ble-arrows-a77 (59ddadd: < and > in the menus, GEN-56). Gate on 9a733fe: PASS 22:29 (tree 1d3c048d). The 22:24 run was VOID (setup error). Installed 22:25, before the PASS (ledger b1). Tag last-good-hw = 94285b5 (a67, iPhone PASS 19:18). Next batch: apc-colors-int (f175c30). Side branches ble-a68-opt (a1d8ea1), ble-a69-readback (75c42e1), ble-a70 (77fa067) and ble-a71 (9ef7bc2): built, not installed.
- Keyboard: integration 9a733fe (batch 1) installed 22:25 (ledger b1, b913735). fwsc CRC32 3a738997. ctl.img read-back eb742dd2. Slots 1 and 2 the same before and after. DIAG READY. Gate PASS 22:29 (tree 1d3c048d, 93 ok, 0 FAIL, 0 skip). The gate Elite fwsc CRC32 3a738997 is the same as the installed file. The install at 22:25 came before the gate PASS: a rule 7 break. Cause: Merge Integrator called a manual test run a gate PASS, and Firmware Main Builder accepted it without a check of index.md. No new rule. The owner tests batch 1 only after the gate PASS (now true). If the owner test fails, Firmware Main Builder installs 14fb352 (a74) again. Device test PENDING (checklist-batch1). Tag last-good-hw stays 94285b5.
- a74 (14fb352, branch ble-a74), installed 21:03. Owner test 1 at 21:18 (ledger 11d81a7): violet FAIL, pad clear FAIL. The other items are NOT TESTED. a74 adds to a73 the clock source setting INTERNAL / EXTERNAL / AUTO (MID-13) and the PLAY note (MID-14). Gate PASS (tree 2f1f79d2ca5c). ctl.img read back eb742dd2. Per-slot CRCs 05fb6e99 / 6bb165f0 before = after, slot 3 empty. DIAG READY. last-good-hw stays 94285b5 (a67).
- a73 (99faa0a) installed 20:26. Owner test 20:37 to 20:39 (ledger c3ee3ef): Loopy Start starts X0X (PASS). X0X PLAY does not start or stop Loopy Pro (FAIL). Pads, overdub and tempo are not tested yet. last-good-hw stays 94285b5 (a67). a73 (branch ble-a73) adds to a67: Bluetooth MIDI clock and transport fixes (a72), clip colour and feedback fixes (a71), the installer ctl.img read-back (a69), and live recording on the nearest step (REC-06). Slots 05fb6e99 / 6bb165f0 before = after, DIAG READY.
- a65 (ca1d4d2), installed 18:19, goes back to the BLE host in the app (from fb6b1b0, test 37), with no pairing and no bonds. The CTL_HOST host of a38 to a64 is not used. No connection parameter request (owner decision 18:18). Slots 961626d5 / 9a09fdb9 before = after. Slot 3 is empty, now 44 KiB at 0xB0000. DIAG: READY, no crash.
- iPhone: works on a67 (owner test PASS 19:18). On a73 the link carries the Loopy clock and Start (20:37 PASS). History: a65 iPhone test 1 (18:23): the iPhone connected (interval 7.5 ms, MTU 185), then the link ended with reason 0x08 (connection timeout). a62 and a62b: the link held about 2 min at 26.25 ms with 0 connection updates, reason 0x00. a63-15 (d9844a4): iOS dropped the link before any GATT step. Oct 15: Loopy used the keyboard as its APC40 over Bluetooth (test 30). But some colour notes made sounds, some pads stayed empty and colours were slow. A connection parameter request made the iPhone drop the link (test 31) and gave a large MIDI delay (test 34).
- Mac: the BLE GATT connection works and MIDI goes in both directions (a59 79eb477, a60). The a59 fix: the host must register its event handler after btstack_task_init, because btstack_task_init removes it.
- Bluetooth start: the supply must be the stock values (DCDC13, VDC14 1.60 V, SYSVDD 1.38 V) before the RF init (test 27, 56a7452). The controller init runs on core 0 before core 1 starts (83cd4dc). The clock stays 360 MHz.
- Release package 0b80994 (SMK-37 Elite only, a64 files: .fwsc CRC32 5ec8b21a, ctl.img CRC32 e9c05144). Pool free 4096 B, app code 144 B free, app RAM 0 B free. The Pro package is not built. Not pushed. Test only, out of date, not for release.
- Clip keys: clip-keys-a65 5a89b76. Its changes are in integration (cherry-picked). KEYS panel and SCALE, CHORD and ARP pages. Knobs 2-8 send CC 48-54. 24 scales, 30 chords, swing, Euclid rhythm, HUMAN, strum. Merged tree with a75: RAM .data+.bss 94500 of 98304 B (ledger row 75). Host tests pass. Not tested on hardware.
- Integration: f357a2e. integration-stale-02ae8b8 keeps the stale 17:34 merges.
- Pool limit: build.sh and the gate require at least 8192 B of pool free (enforced 2026-10-16 21:53). USER_PREFERENCES.md keeps the older 4096 B owner entry.
- Tools: tools/ble_autotest.py runs the Bluetooth test over USB with no user action. Attempt ledger: BLE_ATTEMPTS.md in the BLE branch (one row per attempt). JieLi SDK BLE design vs our host, with the step plan for the iPhone link: ~/Documents/x0x37/ble_split/SDK_DESIGN_COMPARE.md (Mac).

## Left to do

1. Done (a52, f0bcb28): Bluetooth starts. STATUS READY at power-on.
2. Mac: the connection works (a59, a60). Open on the Mac: test clock and Start/Stop over Bluetooth, and check for audio dropouts. iPhone: done, the link works on a67 (94285b5, owner test 19:18).
   Open: CLP-50 FAIL. X0X PLAY does not start or stop Loopy Pro (a73, owner test 20:37-20:39). Also test the a73 pads, overdub and tempo with Loopy Pro.
   Open: CLP-13 FAIL (a74 owner test 1, 21:18). A violet clip from Loopy Pro shows blue on the pads. Fix in 9a733fe (a76), installed 22:25, device test pending.
   Open: CLP-18 FAIL (a74 owner test 1, 21:18). When Loopy Pro disconnects, the pads go dark, but the clip colours come back at once. Fix in 9a733fe (a76), installed 22:25, device test pending.
   Required before a colour fix (agent rule 11): the owner records a playing blue and a playing violet clip from Loopy with INLOG on. The builder makes a host replay test from that log.
   Open: owner test of batch 1 (9a733fe, checklist-batch1). If it fails, Firmware Main Builder installs 14fb352 again.
   Open: a74 clock source and PLAY note items are not tested yet.
   Open: BLE-16 (All Notes Off when the BLE link ends) is not in code.
3. Turn Bluetooth on and off in GLOBAL without a restart. OFF: stop advertising and disconnect. ON: start the controller while the keyboard runs, with no audio dropouts (the radio calibration is the hard part).
4. Bring integration (f357a2e or later) into the main tree (x0x37, smk37) after an owner test PASS of an installed build.
5. Screen clip colours: the approved table (apc40_lcd565.h, CLP-05) is on apc-colors-int (f175c30), the next batch. Install after approval.
6. User checks by ear and eye: the ADPCM cymbals, the 44.1 kHz factory breaks and idle clip colours on the screen. Also check that patterns and settings survived.
7. Web installer: let the page fetch the JieLi files and build the Bluetooth image, or guide the user through it. If possible, end users need no toolchain.
8. Release: build the Pro and Elite packages, check the installer hashes, and publish (only when the user asks).
9. Lane files: Firmware Main Builder and Second Builder agree on the split of the lane files after the batch 1 owner test.

## Open decisions

- Whether to keep spare flash for growth, or give all of it to the loop slots.

## Not planned

- FM-1 build (not maintained, and its overflow is accepted).
- Bluetooth audio.
- Shipping any JieLi Bluetooth files with the firmware.

Last update: 2026-10-16 23:13.
