# Approved DnBOX UI with faster Chicago and Miami (PR #712): hardware checklist

Status: DEV / HARDWARE TEST - DEVICE NOT TESTED

Plan only. Nothing here was run on a device. Do not flash a device until the owner
authorizes this exact candidate. Elite recovery and flash stay withdrawn until the owner
says otherwise. Stop at the first failure. Write down what you saw. Roll back (section 4).

Source: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/712 (wip/engines-ui-one-build). It carries the optimized Chicago and Miami DSP (clock span, chorus span, Miami idle clock and zero stores) into the one-build UI. The sound must stay the same. The one-build UI, rate pads, SMC, BREAK and stored revision 13 are not changed.

## 1. What you need

- One SMK-37 with the DEV candidate of PR #712 installed by the authorized integrator.
- The last-known-good package for rollback.
- Recordings of the same project made on the last-known-good build, for a listening compare. A USB audio recorder on the Mac.
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

Task 1. Screens and engine picker
- Start: S-COLD.
1. Open HOME, a 303 sound page, the engine/preset picker and GLO.
- Look for: the approved DnBOX screens and glyphs. The picker shows the Chicago and Miami sounds.
- Stop if: a screen is blank, broken or from an older UI.

Task 2. Chicago sound is unchanged
- Start: S-COLD. Load the compare project. Choose a Chicago sound for 303A.
1. Press PLAY. Record 30 seconds over USB audio.
2. Compare with the old recording.
- Look for: the same sound, chorus and timing.
- Stop if: you hear a difference, a click or a dropout.

Task 3. Miami sound is unchanged
- Start: S-COLD. Load the compare project. Choose a Miami sound for 303B.
1. Repeat task 2 steps 1 and 2.
- Look for: the same sound and effects.
- Stop if: you hear a difference, a click or a dropout.

Task 4. Idle load is lower
- Start: S-COLD. Do not press PLAY.
1. Open GLO > ABOUT X0X. Read the audio load.
2. Press PLAY. Read it again.
- Look for: the idle load is lower than on the last-known-good build. The playing load is not higher.
- Stop if: the load is higher, or audio breaks up.

Task 5. Silence after notes end
- Start: S-FREE.
1. Play a long note. Release it. Wait 10 seconds.
- Look for: the sound fades and ends in silence with no noise or hum.
- Stop if: noise or a tone stays after the release.

Task 6. Pad colours and key lights follow the selected rate
- Start: S-FREE. Set GLO > LIGHTS to ON.
1. Press rate pad 1. Look at the key lights.
2. Press rate pad 5. Look at the key lights.
3. Press rate pad 8. Look at the key lights.
- Look for: the 8 white key lights for the rates glow dim. Only the selected rate glows bright. The light moves at once with each press. No key light stays from a fake keybed note.
- Stop if: more than one rate light is bright, the light does not move, or a light stays on after you leave Free Play.

Task 7. No dropped or stuck notes
- Start: S-FREE.
1. Connect the MIDI monitor. Set NOTES OUT to ON in GLO.
2. Play 20 fast notes on the keys. Then hold 4 keys together for 3 seconds. Release them.
3. Play 8-note chords fast on Chicago, then on Miami.
4. Count note on and note off messages in the monitor.
- Look for: each note on has one note off. No sound continues after you release all keys.
- Stop if: a note on is missing, a note off is missing, or a sound hangs.

Task 8. BLE link stays up during synth playback
- Start: S-BLE with a BLE MIDI app.
1. Look at GLO > BLUETOOTH MIDI > STATUS. It must show CONNECTED.
2. Play both synths from the app and the keys.
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
