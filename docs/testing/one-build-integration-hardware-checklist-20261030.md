# One-build integration (PR #698): hardware checklist

Status: DEV / HARDWARE TEST - DEVICE NOT TESTED

Plan only. Nothing here was run on a device. Do not flash a device until the owner
authorizes this exact candidate. Elite recovery and flash stay withdrawn until the owner
says otherwise. Stop at the first failure. Write down what you saw. Roll back (section 4).

Source: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/698 (wip/one-build at cb5423c). Changes covered: Miami rate pads on by default in Free Play (1eb56ce), unused pads stay playable (9ca647b), BREAK transient bend (c32f927), wobble DSP isolated from Miami (f62fa65), SMC command-credit startup fix (33df71b), two-peer BLE controller heap (dcfb9e5), reused audio output windows (f0444ae).

## 1. What you need

- One SMK-37 with the DEV candidate of PR #698 installed by the authorized integrator.
- The last-known-good package for rollback.
- An SMC mixer, and a second BLE MIDI central (iPad or Mac) for the two-peer check.
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

Task 1. Boot and version
- Start: S-COLD.
1. Look at the boot screen and the HOME screen.
2. Press GLO. Open ABOUT X0X. Read the version.
- Look for: HOME appears. The version matches the candidate. The audio load looks normal.
- Stop if: the screen stays blank, the keyboard restarts, or the version is wrong.

Task 2. Eight rate pads select the Miami rate
- Start: S-FREE.
1. Press pad 1. Read the screen.
2. Press pads 2 to 8, one at a time. Read the screen after each press.
- Look for: the screen says WOBBLE RATE 1/2, 1/4, 1/8, 1/16, 1/3, 1/6, 1/12, 1/24 in pad order. A rate pad makes no note.
- Stop if: a rate pad plays a note, or the rate label is out of order.

Task 3. The other eight pads still play
- Start: S-FREE.
1. Press each of pads 9 to 16. Release each pad.
2. Hold pad 2 (a rate pad). While you hold it, press and release pad 12.
- Look for: pads 9 to 16 play and stop normally. Pad 12 plays while pad 2 is held.
- Stop if: a pad 9 to 16 makes no sound, or its sound does not stop on release.

Task 4. Pad colours and key lights follow the selected rate
- Start: S-FREE. Set GLO > LIGHTS to ON.
1. Press rate pad 1. Look at the key lights.
2. Press rate pad 5. Look at the key lights.
3. Press rate pad 8. Look at the key lights.
- Look for: the 8 white key lights for the rates glow dim. Only the selected rate glows bright. The light moves at once with each press. No key light stays from a fake keybed note.
- Stop if: more than one rate light is bright, the light does not move, or a light stays on after you leave Free Play.

Task 5. Rate pads give way outside Miami Free Play
- Start: S-FREE.
1. Set FREE PLAY to OFF. Press pads 1 to 8.
2. Set FREE PLAY to ON. Choose a Chicago sound for 303A. Press pads 1 to 8.
- Look for: in both cases the pads do their normal job and the screen shows no WOBBLE RATE message.
- Stop if: a WOBBLE RATE message appears, or a pad does nothing.

Task 6. BREAK transient bend
- Start: S-COLD. Load a project with a BREAK pattern. Press PLAY.
1. Use the BREAK pitch bend control that the BREAK pages show.
2. Release it.
- Look for: the BREAK pitch bends and returns. The drums and 303 stay in time.
- Stop if: the BREAK stays bent after release, or audio clicks or drops out.

Task 7. No dropped or stuck notes
- Start: S-FREE.
1. Connect the MIDI monitor. Set NOTES OUT to ON in GLO.
2. Play 20 fast notes on the keys. Then hold 4 keys together for 3 seconds. Release them.
3. Press PLAY. Play 20 notes over the running pattern. Press STOP while you hold a key, then release it.
4. Count note on and note off messages in the monitor.
- Look for: each note on has one note off. No sound continues after you release all keys.
- Stop if: a note on is missing, a note off is missing, or a sound hangs.

Task 8. SMC connects at start-up and two peers stay up
- Start: S-SMC.
1. Connect the second central (iPad or Mac) to the keyboard NAME.
2. Read GLO > BLUETOOTH MIDI > NAME.
3. Move fader 1 on the SMC. Play notes from the second central.
- Look for: NAME lists both peers, separated by a comma. The fader moves the 909 level. Notes from the second central play.
- Stop if: STATUS shows FAILED, a peer drops, or the keyboard restarts.

Task 9. BLE link stays up during playback with two peers
- Start: Do task 8 first.
1. Look at GLO > BLUETOOTH MIDI > STATUS. It must show CONNECTED.
2. Press PLAY. Move SMC faders and press SMC buttons every few seconds.
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
