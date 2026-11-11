# SMC on first boot and saved Bluetooth roles (PR #709): hardware checklist

Status: DEV / HARDWARE TEST - DEVICE NOT TESTED

Plan only. Nothing here was run on a device. Do not flash a device until the owner
authorizes this exact candidate. Elite recovery and flash stay withdrawn until the owner
says otherwise. Stop at the first failure. Write down what you saw. Roll back (section 4).

Source: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/709 (wip/smc-first-boot-and-roles, main_fm1.c, project.c). The BLE controller is prepared at every boot. The saved OFF setting is applied after start and can be turned ON later without restart. With no saved device settings, the SMK-37 starts as SMC host with Bluetooth ON. Saved OFF and DEVICE roles are kept, also when other project data is missing.

## 1. What you need

- One SMK-37 with the DEV candidate of PR #709 installed by the authorized integrator.
- The last-known-good package for rollback.
- An SMC mixer. A BLE MIDI app on an iPad or Mac for the DEVICE role. A way for the integrator to clear device settings (first-boot state).
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

Task 1. First boot starts SMC discovery
- Start: ask the integrator to install the candidate with no saved device settings. Switch on the SMC. Do S-COLD.
1. Press GLO. Open BLUETOOTH MIDI. Read BLUETOOTH, BT ROLE and STATUS.
- Look for: BLUETOOTH is ON. BT ROLE is SMC HOST. STATUS goes to CONNECTED with the SMC.
- Stop if: BLUETOOTH is OFF, or the SMC does not connect within 30 seconds.

Task 2. Saved OFF stays OFF
- Start: S-BLE. Set BLUETOOTH to OFF. Press SAVE. Do S-COLD.
1. Read BLUETOOTH and STATUS.
- Look for: BLUETOOTH is OFF. STATUS is OFF. The SMC does not connect.
- Stop if: Bluetooth turns on by itself.

Task 3. Turn ON without restart
- Start: do task 2.
1. Set BLUETOOTH to ON. Do not restart.
2. Wait 30 seconds. Read STATUS.
- Look for: STATUS goes to READY or CONNECTED without a restart.
- Stop if: STATUS shows RESTART or FAILED, or the keyboard restarts.

Task 4. Saved DEVICE role stays
- Start: S-BLE. Set BT ROLE to DEVICE. Press SAVE. Do S-COLD.
1. Read BT ROLE. Connect the iPad or Mac app.
- Look for: BT ROLE is DEVICE. The app connects.
- Stop if: the role changed to SMC HOST.

Task 5. Role kept after a partial project load
- Start: do task 4. Ask the integrator for a project with missing sound or pattern data. Load it.
1. Do S-COLD. Read BT ROLE and BLUETOOTH.
- Look for: BT ROLE is still DEVICE. BLUETOOTH is still ON.
- Stop if: the role or the ON/OFF setting changed.

Task 6. Pad colours after boot with Bluetooth
- Start: S-SMC. Set GLO > LIGHTS to ON. Press PLAY.
1. Look at the pad step lights and the SMC button lights.
- Look for: pad step lights follow the steps. The SMC lights show the mute and select state.
- Stop if: lights are frozen, or the SMC lights stay off.

Task 7. No dropped or stuck notes
- Start: S-SMC.
1. Connect the MIDI monitor. Set NOTES OUT to ON in GLO.
2. Play 20 fast notes on the keys. Then hold 4 keys together for 3 seconds. Release them.
3. Repeat on a boot with Bluetooth OFF (task 2).
4. Count note on and note off messages in the monitor.
- Look for: each note on has one note off. No sound continues after you release all keys.
- Stop if: a note on is missing, a note off is missing, or a sound hangs.

Task 8. BLE link stays up during SMC use after first boot
- Start: Do task 1.
1. Look at GLO > BLUETOOTH MIDI > STATUS. It must show CONNECTED.
2. Move SMC faders and press PLAY and STOP.
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
