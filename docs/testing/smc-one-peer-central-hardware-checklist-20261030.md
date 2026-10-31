# One-peer SMC BLE central and guarded boot (PR #700): hardware checklist

Status: DEV / HARDWARE TEST - DEVICE NOT TESTED

Plan only. Nothing here was run on a device. Do not flash a device until the owner
authorizes this exact candidate. Elite recovery and flash stay withdrawn until the owner
says otherwise. Stop at the first failure. Write down what you saw. Roll back (section 4).

Source: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/700 (smc-a-port-20261029, base dnbox-base-smc-20261029). It replaces dual-peer central scheduling with the working one-peer HCI/ATT discovery and credit handling. Controller: 1 central link, 5 ACL TX credits, multilink off. SMC output is slot 0 only. The controller starts at power-on. Saved OFF stays idle and can turn ON without restart. Missing settings default to SMC host and ON.

## 1. What you need

- One SMK-37 with the DEV candidate of PR #700 installed by the authorized integrator.
- The last-known-good package for rollback.
- One SMC mixer. A second SMC or BLE central to check that only one link is accepted.
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

Task 1. SMC connects and finds the service
- Start: S-SMC.
1. Move fader 1. Press Mute on strip 1.
- Look for: STATUS is CONNECTED. The fader and button act at once.
- Stop if: the SMC does not connect within 30 seconds.

Task 2. Stop and restart the SMC
- Start: S-SMC.
1. Switch off the SMC. Wait 10 seconds. Switch it on.
2. Do this 5 times. Move fader 1 after each reconnect.
- Look for: it reconnects each time. Fader 1 works each time.
- Stop if: a reconnect fails, or the keyboard restarts.

Task 3. Only one peer
- Start: S-SMC.
1. Try to connect a second SMC or central.
- Look for: the first SMC stays connected and works. NAME shows one peer.
- Stop if: the first SMC drops or stops responding.

Task 4. Saved OFF stays idle, ON without restart
- Start: S-BLE. Set BLUETOOTH to OFF. Press SAVE. Do S-COLD.
1. Read STATUS. Set BLUETOOTH to ON. Wait 30 seconds.
- Look for: STATUS is OFF after boot. It goes to READY or CONNECTED without a restart.
- Stop if: Bluetooth starts by itself, or needs a restart.

Task 5. Missing settings default to SMC host
- Start: ask the integrator to install with no saved device settings. Switch on the SMC. Do S-COLD.
1. Read BLUETOOTH, BT ROLE and STATUS.
- Look for: ON, SMC HOST, CONNECTED.
- Stop if: any value differs.

Task 6. Command credits under fast feedback
- Start: S-SMC. Open 909 MIX. Press PLAY.
1. Press Mute on all 8 strips fast, twice.
- Look for: each SMC light follows its mute state. No light is missed.
- Stop if: lights stop updating, or the link drops.

Task 7. SMC button lights and pad colours update
- Start: S-SMC. Open MIX. Use Left/Right to go to 909 MIX.
1. Press Mute on strip 2. Press it again.
2. Press SELECT on strip 3.
3. Hold Solo on strip 4. Release it.
4. Look at the SMK pad step lights after step 2.
- Look for: the Mute light goes on, then off. The SELECT light moves to strip 3. Solo lights only while held. The pad step lights show drum 3.
- Stop if: a light is wrong, late by more than a second, or stays on.

Task 8. No dropped or stuck notes
- Start: S-SMC.
1. Connect the MIDI monitor. Set NOTES OUT to ON in GLO.
2. Play 20 fast notes on the keys. Then hold 4 keys together for 3 seconds. Release them.
3. Move SMC faders during playing.
4. Count note on and note off messages in the monitor.
- Look for: each note on has one note off. No sound continues after you release all keys.
- Stop if: a note on is missing, a note off is missing, or a sound hangs.

Task 9. BLE link stays up during long SMC use
- Start: S-SMC.
1. Look at GLO > BLUETOOTH MIDI > STATUS. It must show CONNECTED.
2. Move faders and press buttons every few seconds.
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
