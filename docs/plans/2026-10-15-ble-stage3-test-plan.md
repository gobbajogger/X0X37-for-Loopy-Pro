# BLE MIDI Stage 3: hardware test plan (SMK-37)

Status: plan only. Nothing here was run. Run it only after the user approves Stage 3.
Each step names what to look for and when to stop. Stop at the first failure, write down what
you saw, and roll back (section 5).

## 1. What you need

- One SMK-37 Pro (or Elite) with X0X at commit 81d5f74 or later, working.
- A Mac with the X0X tree at the Stage 2 commits, the JieLi toolchain, Docker, `mido` and
  `python-rtmidi`.
- A BLE MIDI central: an iPad or iPhone with a BLE MIDI app (for example "MIDI Wrench" or
  Loopy Pro), or a Mac's Audio MIDI Setup > Bluetooth Configuration.
- Optional: a second Mac or phone with a BLE scanner (for example nRF Connect) to read the
  advertising data and the address.
- The previous release package (for example `x0x-smk37-<last>.fwsc`) for the rollback.

## 2. Before any write

1. Build: `BOARD=smk37 ./build.sh` and `BOARD=smk37 MODEL=elite ./build.sh`. Expect 0 warnings
   and the size lines of the Stage 2 report.
2. `tests/run_tests.sh`. Expect all ok, except drum909, drum808 and bass303 when their reference
   sources are missing.
3. Build the image: `ble/ctl/build_ctl.sh`. Expect `ctl.img: 64504 B of 65536` (or close) and
   `python3 tools/add_ctl.py ble/ctl/out/ctl.img --check` = ok.
4. Back up the slots: `python3 tools/x0x_slots.py backup slots-before-ble.x0xslots`. Keep the
   file. The installs below do not write the slots (BEHAVIOR.md FLS-15), but keep a backup.
5. Note the unit's state: `python3 tools/fm1_install.py --info` (identity), GLO > ABOUT (version),
   the saved project (take a photo of GLO and one pattern).

## 3. Steps

### 3.1 The Stage 2 app without an image (regression)

1. `python3 tools/fm1_install.py --no-ctl build/x0x-smk37.fwsc`.
2. Observe: X0X starts as before. GLO > BLUETOOTH MIDI > STATUS = NO IMAGE. NAME = X0X-xxxx.
   The project is still there.
3. Play all parts with a dense pattern for 5 minutes: no new xruns compared with the last release
   (GLO > PERFORMANCE). SAVE PROJECT works. A sample upload (break loops page) works.
4. Set BLUETOOTH ON: the screen shows "RESTART TO TURN ON". Power off and on: STATUS = NO IMAGE.
   Set BLUETOOTH OFF again.
5. Stop if anything differs from the last release.

### 3.2 The image installed, Bluetooth OFF

1. `python3 tools/fm1_install.py --ctl ble/ctl/out/ctl.img build/x0x-smk37.fwsc`.
   Expect "Bluetooth controller image: ... (added to the package)" and a normal install.
2. Observe: STATUS = OFF. Everything in 3.1 steps 2-3 again (core 1 starts as before: the
   controller does not run when BLUETOOTH is OFF).
3. A scanner sees no advertising from the unit.

### 3.3 First start

1. Set BLUETOOTH ON, SAVE PROJECT, power off and on.
2. Observe: the start takes up to about 1.5 s longer (the controller's init, before the audio).
   STATUS goes to STARTING, then READY within a few seconds.
3. The scanner sees "X0X-xxxx" (the same 4 characters as NAME), a random static address
   (top bits 11), the BLE-MIDI service UUID 03B80E5A-EDE8-4B33-A751-6CE34EC4C700.
4. Power off and on twice: the same name and address each time.
5. Stop and roll back if: the unit does not start, STATUS = FAILED, the audio has new dropouts,
   or the screen shows a crash report. Write down STATUS and, on a crash, the crash screen.

### 3.4 Connection and MIDI

1. Connect from the central. STATUS = CONNECTED. No pairing dialog appears (if the central asks
   for pairing, X0X refuses it; the connection must still work for MIDI).
2. MIDI in: send notes on channel 10 (909) and channel 2 (303A): they play. Send a program change
   and pitch bend: nothing breaks. Send clock at 120 BPM: X0X follows it (top line). Start and
   Stop start and stop the patterns.
3. MIDI out: CLOCK OUT ON: the central receives clock (24 per beat) and Start/Stop. NOTES OUT ON:
   it receives the pattern's notes on each part's channel. Compare the timing with USB MIDI out
   (a DAW that records both): the BLE notes may come later by up to one connection interval
   (7.5-45 ms) and by up to one ms of timestamp, but no notes are lost at 120 BPM with all parts.
4. Running status and SysEx from the central: send a SysEx message: X0X ignores it and keeps
   working.
5. Both together: USB and BLE connected, clock from one only: the tempo is right. Clock from both:
   the tempo doubles (expected, LIM-10).

### 3.5 Load and flash operations while connected

1. Play the densest pattern while BLE MIDI out runs for 10 minutes. Watch GLO > PERFORMANCE:
   the xruns stay at the 3.1 level. The connection stays up.
2. SAVE PROJECT while connected (stopped). Expect: it saves; the connection stays up or drops and
   the central reconnects. Core 1 must not be held (BLUETOOTH STATUS stays READY or CONNECTED,
   not FAILED).
3. Upload a break loop over USB while BLE is connected. Expect the same as step 2. A long sample
   erase may drop the BLE link (parking); it must reconnect after.
4. Stop and roll back if the unit crashes or STATUS goes to FAILED.

### 3.6 OFF, ON, disconnect

1. Connected: set BLUETOOTH OFF. The central sees the disconnect at once. The scanner sees no
   advertising.
2. Set BLUETOOTH ON (no restart): advertising starts again, the central can connect.
3. Disconnect from the central: STATUS = READY, advertising again.
4. Walk away until the link drops, come back: the central reconnects.

### 3.7 Update without the image

1. `python3 tools/fm1_install.py --no-ctl build/x0x-smk37.fwsc`.
2. Observe: STATUS = NO IMAGE, BLUETOOTH still ON in GLO, the unit works as in 3.1.
3. Install again with the image (3.2 step 1). STATUS = READY after a restart.

### 3.8 A damaged image

1. Make a damaged copy: flip one byte in the middle of `ctl.img`. `tools/add_ctl.py` refuses it
   (CRC). That is the expected result. Do not force a damaged image onto the unit.

## 4. What to record

For each step: pass or fail, STATUS, the time to READY, the xrun count, and anything on the
screen. On a failure with a debug build (`X0X_DEBUG=1`), `tools/fm1_debug.py` reads the mailbox
(state, err, polls, c2h_drops, irqs_routed) and core 1's poll_max and park state.

## 5. Rollback

1. Bluetooth misbehaves but X0X starts: set BLUETOOTH OFF (the controller then never starts), or
   install without the image: `python3 tools/fm1_install.py --no-ctl build/x0x-smk37.fwsc`.
2. X0X does not start but the update loader answers (`fm1_install.py --info` shows the update
   mode): `python3 tools/fm1_install.py --no-ctl <previous release>.fwsc`. The install finishes the
   write.
3. The unit crashes at power-on with BLUETOOTH ON and the image installed: the crash guard
   (bootguard) stops repeated early crashes; then install without the image as in step 2. The
   BLUETOOTH setting stays ON, but without an image nothing starts.
4. Back to stock: M-VAVE's own updater with the stock SMK-37 package. The X0X store and slot 3
   can be erased by stock (FLS-17); restore the slots from `slots-before-ble.x0xslots` after
   X0X is installed again.
5. Restore the slots if any were lost: `python3 tools/x0x_slots.py restore slots-before-ble.x0xslots`.

## 6. Open points to check on hardware

- The core-1 interrupt bank (0x1EEF300) and the shared vector table: the controller's interrupts
  must arrive on core 1 only.
- The clocks given to the image (360 / 180 / 60 MHz) on an SMK-37.
- The RF calibration (SDK demo values): range and link quality.
- `poll_max` against the 8 ms audio job limit, and the time core 1 needs to park.
- The start time (settle 800 ms) and whether READY comes reliably.
