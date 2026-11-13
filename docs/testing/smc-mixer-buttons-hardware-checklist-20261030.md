# Reliable SMC mixer buttons (PR #714): hardware checklist

Status: DEV / HARDWARE TEST - DEVICE NOT TESTED

Plan only. Nothing here was run on a device. Do not flash a device until the owner
authorizes this exact candidate. Elite recovery and flash stay withdrawn until the owner
says otherwise. Stop at the first failure. Write down what you saw. Roll back (section 4).

Source: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/714 (wip/mixer-ux-one-build, smc_mixer.c, smc_fx_caption.h). Pattern arrows use a pending cue and repeat presses add up. A press back to the playing pattern cancels the cue. Held buttons are owned per peer. A repeated NoteOn is not another REC/MUTE toggle. Held feedback stays until NoteOff. Lost releases and reconnects clear stale holds. FX SELECT captions show the real effect. FX audio is unchanged.

## 1. What you need

- One SMK-37 with the DEV candidate of PR #714 installed by the authorized integrator.
- The last-known-good package for rollback.
- An SMC mixer in DAW preset, and a second SMC or BLE central for two-peer checks.
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

Task 1. Pattern cue adds up and cancels
- Start: S-SMC. Load a project with 4 or more patterns. Press PLAY on pattern 1.
1. Press the SMC pattern Right arrow twice. Read the screen.
2. Press the pattern Left arrow twice. Read the screen.
3. Press Right once. Wait for the pattern end.
- Look for: step 1 cues pattern 3. Step 2 cancels the cue. Step 3 changes to pattern 2 at the pattern end.
- Stop if: the cue is wrong, or the pattern changes at once.

Task 2. Held REC and MUTE toggle once
- Start: S-SMC. Open 909 MIX.
1. Hold Mute on strip 1 for 3 seconds. Release it.
2. Hold Record on strip 2 for 3 seconds. Release it.
- Look for: each hold toggles once. The held feedback stays on screen until release.
- Stop if: a hold toggles more than once, or the feedback goes before release.

Task 3. Lost release after a disconnect
- Start: S-SMC. Open 909 MIX.
1. Hold Solo on strip 3.
2. While holding, switch off the SMC.
3. Switch on the SMC. Wait for CONNECTED.
- Look for: Solo clears when the link drops. Drum 3 is not soloed after reconnect.
- Stop if: Solo stays stuck after reconnect.

Task 4. SMC button lights and pad colours update
- Start: S-SMC. Open MIX. Use Left/Right to go to 909 MIX.
1. Press Mute on strip 2. Press it again.
2. Press SELECT on strip 3.
3. Hold Solo on strip 4. Release it.
4. Look at the SMK pad step lights after step 2.
- Look for: the Mute light goes on, then off. The SELECT light moves to strip 3. Solo lights only while held. The pad step lights show drum 3.
- Stop if: a light is wrong, late by more than a second, or stays on.

Task 5. FX caption shows the real effect
- Start: S-SMC. Outside drum MIX/SEQ.
1. Hold SELECT 1. Read the caption. Listen. Release it.
2. Hold SELECT 2, then SELECT 5, the same way.
3. Hold a main FX bus (reverb or delay) and read its caption.
- Look for: each caption names the effect you hear. The sound is the same as before this PR.
- Stop if: a caption is wrong, or the effect sound changed.

Task 6. No dropped or stuck notes
- Start: S-SMC.
1. Connect the MIDI monitor. Set NOTES OUT to ON in GLO.
2. Play 20 fast notes on the keys. Then hold 4 keys together for 3 seconds. Release them.
3. Press PLAY. Hold and release SMC Solo and Mute buttons fast for 30 seconds.
4. Count note on and note off messages in the monitor.
- Look for: each note on has one note off. No sound continues after you release all keys.
- Stop if: a note on is missing, a note off is missing, or a sound hangs.

Task 7. BLE link stays up during fast button use
- Start: S-SMC.
1. Look at GLO > BLUETOOTH MIDI > STATUS. It must show CONNECTED.
2. Press SMC buttons as fast as you can for 10 seconds, then rest 10 seconds.
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
