# Free Play pads with rate selection disabled (PR #715): hardware checklist

Status: DEV / HARDWARE TEST - DEVICE NOT TESTED

Plan only. Nothing here was run on a device. Do not flash a device until the owner
authorizes this exact candidate. Elite recovery and flash stay withdrawn until the owner
says otherwise. Stop at the first failure. Write down what you saw. Roll back (section 4).

Source: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/715 (fix/freeplay-pad-optout-regression). The PR changes only the host test. It checks that all 16 pads send normal notes when the rate selector is built disabled, and that no rate setting or save state changes. This checklist needs a DEV build made with the rate selector disabled (X0X_DEV_WOBBLE=0) and a second build with it enabled.

## 1. What you need

- One SMK-37 with the DEV candidate of PR #715 installed by the authorized integrator.
- The last-known-good package for rollback.
- Two DEV packages: rate selector disabled and rate selector enabled (the default).
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

Task 1. Disabled build: all 16 pads play
- Start: install the disabled build. Do S-FREE.
1. Press and release each of pads 1 to 16.
- Look for: every pad plays and stops. The screen shows no WOBBLE RATE message.
- Stop if: a pad makes no sound, or a WOBBLE RATE message appears.

Task 2. Disabled build: no hidden rate or save change
- Start: S-FREE on the disabled build. Press SAVE. Wait until UNSAVED is gone.
1. Press pads 1 to 8 twice each.
2. Look at the header.
3. Look at the 303A wobble RATE value on its sound page.
- Look for: UNSAVED does not appear because of the pads. The RATE value does not change.
- Stop if: UNSAVED appears, or RATE changes.

Task 3. Disabled build: pad colours and key lights
- Start: S-FREE on the disabled build. Set GLO > LIGHTS to ON.
1. Press pads 1, 5 and 8. Look at the key lights after each press.
- Look for: the lights follow the notes you play. There is no dim row of rate lights with one bright light.
- Stop if: the rate light pattern appears, or a light stays on after release.

Task 4. No dropped or stuck notes
- Start: S-FREE on the disabled build.
1. Connect the MIDI monitor. Set NOTES OUT to ON in GLO.
2. Play 20 fast notes on the keys. Then hold 4 keys together for 3 seconds. Release them.
3. Press pads 1 to 16 fast in order, twice. Hold pads 1 and 9 together for 3 seconds, then release.
4. Count note on and note off messages in the monitor.
- Look for: each note on has one note off. No sound continues after you release all keys.
- Stop if: a note on is missing, a note off is missing, or a sound hangs.

Task 5. Enabled build: rate pads 1 to 8, notes on 9 to 16
- Start: install the enabled build. Do S-FREE.
1. Press pads 1 to 8. Read the screen.
2. Press pads 9 to 16.
- Look for: pads 1 to 8 show WOBBLE RATE 1/2 to 1/24 and play no note. Pads 9 to 16 play.
- Stop if: the split is wrong.

Task 6. Pad colours and key lights follow the selected rate
- Start: S-FREE on the enabled build. Set GLO > LIGHTS to ON.
1. Press rate pad 1. Look at the key lights.
2. Press rate pad 5. Look at the key lights.
3. Press rate pad 8. Look at the key lights.
- Look for: the 8 white key lights for the rates glow dim. Only the selected rate glows bright. The light moves at once with each press. No key light stays from a fake keybed note.
- Stop if: more than one rate light is bright, the light does not move, or a light stays on after you leave Free Play.

Task 7. BLE link stays up during Free Play pad use
- Start: S-BLE. Connect a BLE MIDI app. Then do the S-FREE steps without a power cycle.
1. Look at GLO > BLUETOOTH MIDI > STATUS. It must show CONNECTED.
2. Press pads 1 to 16 and play keys. Watch the app for the notes.
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
