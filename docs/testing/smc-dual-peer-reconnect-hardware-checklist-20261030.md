# SMC dual-peer reconnect and overflow safeguards (PR #687): hardware checklist

Status: DEV / HARDWARE TEST - DEVICE NOT TESTED

Plan only. Nothing here was run on a device. Do not flash a device until the owner
authorizes this exact candidate. Elite recovery and flash stay withdrawn until the owner
says otherwise. Stop at the first failure. Write down what you saw. Roll back (section 4).

Source: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/687 (wip/smc-port). Per-peer link epochs and lost-release boundaries keep a new session's Solo and Pause safe from old releases. Disabled or disconnected BLE cannot clear hosted Solo. MMC STOP overflow stays in order. Two-peer ownership and connected names are kept. The PR also carries the Chicago/Miami composition from its base.

## 1. What you need

- One SMK-37 with the DEV candidate of PR #687 installed by the authorized integrator.
- The last-known-good package for rollback.
- Two SMC mixers (or one SMC and one BLE central). Matching two-link controller image.
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

Task 1. Two peers connect and show names
- Start: S-SMC. Switch on the second SMC.
1. Read GLO > BLUETOOTH MIDI > NAME.
- Look for: both names in slot order, separated by a comma.
- Stop if: one peer is missing, or a name is cut badly.

Task 2. Solo is the union of both mixers
- Start: do task 1. Open HOME.
1. Hold S on strip 1 on SMC 1. Hold S on strip 2 on SMC 2.
2. Release S on SMC 1.
- Look for: both parts solo in step 1. Part 2 stays soloed after step 2.
- Stop if: one release clears the other mixer's Solo.

Task 3. Reconnect does not replay old releases
- Start: do task 1.
1. On SMC 2, press Pause to lock.
2. Switch off SMC 2. Switch it on. Wait for CONNECTED.
3. Hold S on strip 3 on SMC 2.
- Look for: Solo works on the new session. No old release clears it.
- Stop if: Solo or lock state is cleared by itself.

Task 4. Pause locks only its own mixer
- Start: do task 1.
1. Press Pause on SMC 1.
2. Change pages on the keyboard. Move fader 1 on each mixer.
- Look for: SMC 1 keeps its captured target. SMC 2 follows normally.
- Stop if: SMC 2 is locked too.

Task 5. MMC STOP under load
- Start: S-SMC with the SMC in USER preset. Press PLAY.
1. Move all faders fast. Press STOP on the SMC during the movement.
2. Do this 5 times.
- Look for: transport stops each time. Fader moves after STOP still apply.
- Stop if: STOP is lost, or play restarts.

Task 6. SMC button lights and pad colours update
- Start: S-SMC. Open MIX. Use Left/Right to go to 909 MIX.
1. Press Mute on strip 2. Press it again.
2. Press SELECT on strip 3.
3. Hold Solo on strip 4. Release it.
4. Look at the SMK pad step lights after step 2.
- Look for: the Mute light goes on, then off. The SELECT light moves to strip 3. Solo lights only while held. The pad step lights show drum 3.
- Stop if: a light is wrong, late by more than a second, or stays on.

Task 7. No dropped or stuck notes
- Start: Do task 1.
1. Connect the MIDI monitor. Set NOTES OUT to ON in GLO.
2. Play 20 fast notes on the keys. Then hold 4 keys together for 3 seconds. Release them.
3. While playing, move faders on both mixers fast.
4. Count note on and note off messages in the monitor.
- Look for: each note on has one note off. No sound continues after you release all keys.
- Stop if: a note on is missing, a note off is missing, or a sound hangs.

Task 8. BLE link stays up during two-peer use
- Start: Do task 1.
1. Look at GLO > BLUETOOTH MIDI > STATUS. It must show CONNECTED.
2. Use both mixers in turn.
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
