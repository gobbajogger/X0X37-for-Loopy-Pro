# MIDI queue order when queues fill (PR #705): hardware checklist

Status: DEV / HARDWARE TEST - DEVICE NOT TESTED

Plan only. Nothing here was run on a device. Do not flash a device until the owner
authorizes this exact candidate. Elite recovery and flash stay withdrawn until the owner
says otherwise. Stop at the first failure. Write down what you saw. Roll back (section 4).

Source: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/705 (fix/midi-queue-order, usb.c, ble.c, plat_fm1.c). When an output queue is full, one discardable controller message is removed and the protected note or transport message is added at the end. Order is kept. TIMER5 is masked during queue changes. The BLE ring is now 64 slots so a cursor wrap keeps the index.

## 1. What you need

- One SMK-37 with the DEV candidate of PR #705 installed by the authorized integrator.
- The last-known-good package for rollback.
- A Mac with a MIDI monitor that shows message order, over USB and over BLE.
- A Mac or iPad with a MIDI monitor app, for note on/off checks over USB or Bluetooth.
- Headphones on the line or headphone output.

## 2. Start states (each task names one)

- **S-COLD:** Switch off the keyboard. Wait 5 seconds. Switch it on. Wait for the HOME screen.
- **S-BLE:** Do S-COLD. Press GLO. Open BLUETOOTH MIDI. Make sure BLUETOOTH is ON. If you
  changed it, do S-COLD again. Wait until STATUS shows READY or CONNECTED.
- **S-SMC:** Do S-BLE with BT ROLE set to SMC HOST. Switch on the SMC mixer. Wait until
  STATUS shows CONNECTED and NAME shows the mixer.
- **S-FREE:** Do S-COLD. Select 303A. Choose a Miami sound for 303A. Press SEQ. Turn SELECT
  to the KEYS page. Set FREE PLAY to ON. Make sure the top line shows FREE PLAY.

## 3. Tasks

Task 1. Flood CCs, then play notes over USB
- Start: S-COLD. Connect USB to the Mac. Set CLOCK OUT and NOTES OUT to ON.
1. Press PLAY.
2. Turn 4 knobs fast for 20 seconds while you play keys.
3. Press STOP while you hold a key. Release it.
- Look for: in the monitor every note on is before its note off. Every note has a note off. Start and Stop are in order.
- Stop if: a note off comes before its note on, a note off is missing, or Stop comes before Start.

Task 2. Same flood over BLE
- Start: S-BLE. Connect the Mac Bluetooth MIDI. Set NOTES OUT to ON.
1. Repeat task 1 steps 1 to 3.
- Look for: the same order and no missing note off over BLE.
- Stop if: any order error or missing note off.

Task 3. Long run for cursor wrap
- Start: S-BLE. Connect the Mac. Set CLOCK OUT and NOTES OUT to ON.
1. Press PLAY at 180 BPM. Turn knobs now and then.
2. Let it run for 30 minutes.
- Look for: messages stay in order for the full run. No sudden wrong notes.
- Stop if: notes go wrong, out of order, or stop.

Task 4. No dropped or stuck notes
- Start: S-COLD with USB to the Mac.
1. Connect the MIDI monitor. Set NOTES OUT to ON in GLO.
2. Play 20 fast notes on the keys. Then hold 4 keys together for 3 seconds. Release them.
3. Send a fast CC stream into the keyboard from the Mac at the same time.
4. Count note on and note off messages in the monitor.
- Look for: each note on has one note off. No sound continues after you release all keys.
- Stop if: a note on is missing, a note off is missing, or a sound hangs.

Task 5. Pad colours update under load
- Start: S-COLD. Set GLO > LIGHTS to ON. Press PLAY.
1. Turn knobs fast for 20 seconds.
2. Look at the pad step lights and button lights.
- Look for: lights follow the steps with no freeze.
- Stop if: lights freeze or lag by more than one step.

Task 6. Input during heavy output
- Start: S-COLD with USB to the Mac.
1. From the Mac, send a 20-note sequence with note off messages to the 303A channel.
2. At the same time, turn knobs on the keyboard.
- Look for: all 20 notes play and stop.
- Stop if: a received note hangs or is lost.

Task 7. BLE link stays up during heavy MIDI traffic
- Start: S-BLE with the Mac connected.
1. Look at GLO > BLUETOOTH MIDI > STATUS. It must show CONNECTED.
2. Turn knobs fast and play keys.
3. Do this for 5 minutes.
4. Look at STATUS again.
- Look for: STATUS stays CONNECTED. The app does not show a disconnect. Controls still respond after 5 minutes.
- Stop if: STATUS shows READY, FAILED or STARTING during the test, or the app loses the keyboard.

## 4. Rollback

1. Write down the task number, the step and what you saw.
2. Stop playback. Switch off the keyboard.
3. Ask the authorized integrator to reinstall the last-known-good package.
4. Do S-COLD. Make sure the HOME screen and the version in ABOUT X0X are the old ones.
5. Record the result in the PR as FAIL. Do not mark the candidate hardware-tested.
