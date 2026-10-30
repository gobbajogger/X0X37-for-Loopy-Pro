# SMC mixer owns its own pages (PR #717): hardware checklist

Status: DEV / HARDWARE TEST - DEVICE NOT TESTED

Plan only. Nothing here was run on a device. Do not flash a device until the owner
authorizes this exact candidate. Elite recovery and flash stay withdrawn until the owner
says otherwise. Stop at the first failure. Write down what you saw. Roll back (section 4).

Source: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/717 (wip/smc-mixer-integration, smc_mixer.c). The mixer keeps its own page per view and part. Keyboard page changes do not move SMC faders, knobs or SELECT lights. When locked, mixer page buttons move lock_page only. Unlock adopts that page. Also includes the #714 held-button and FX caption work.

## 1. What you need

- One SMK-37 with the DEV candidate of PR #717 installed by the authorized integrator.
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

Task 1. Keyboard page change does not move the mixer
- Start: S-SMC. On the keyboard, select 303A and open its sound view.
1. On the SMC, press Right to go to the next page. Read the SMC screen.
2. On the keyboard, change to another page of the same view.
3. Turn SMC encoder 1. Read the SMC screen.
- Look for: the SMC screen still shows the mixer page you chose in step 1. Encoder 1 edits that page.
- Stop if: the mixer follows the keyboard page.

Task 2. Mixer pages are kept per part
- Start: S-SMC. Select 303A on the keyboard.
1. On the SMC, go to page 2 of the sound view.
2. On the keyboard, select 303B. On the SMC, go to page 1.
3. On the keyboard, select 303A again. Read the SMC screen.
- Look for: the SMC is on page 2 for 303A.
- Stop if: the page is lost or shared between parts.

Task 3. Page buttons while locked
- Start: S-SMC. Open a sound view with more than one page.
1. Press Pause once to lock. Release it.
2. On the SMC, press Right. Read the SMC screen. Move fader 1 and encoder 1.
3. Hold Pause for more than 1 second to unlock. Read the SMC screen.
- Look for: in step 2 the page label changes but fader 1 and encoder 1 keep their captured targets. After unlock the SMC stays on the page you chose.
- Stop if: the captured targets change, or unlock jumps to another page.

Task 4. Lock and 900 ms unlock
- Start: S-SMC. Press the SMC HOME button (R8).
1. Press Pause (the captured lock button) once. Release it.
2. Change pages on the keyboard. Move fader 1.
3. Press Pause and release it within half a second.
4. Press Pause and hold it for more than 1 second. Release it.
- Look for: after step 1 the feedback light is steady and fader 1 keeps its captured target. Step 3 does not unlock. Step 4 unlocks. The light returns to beat flashing.
- Stop if: a short press unlocks, a long press does not unlock, or the target moves while locked.

Task 5. SMC button lights and pad colours update
- Start: S-SMC. Open MIX. Use Left/Right to go to 909 MIX.
1. Press Mute on strip 2. Press it again.
2. Press SELECT on strip 3.
3. Hold Solo on strip 4. Release it.
4. Look at the SMK pad step lights after step 2.
- Look for: the Mute light goes on, then off. The SELECT light moves to strip 3. Solo lights only while held. The pad step lights show drum 3.
- Stop if: a light is wrong, late by more than a second, or stays on.

Task 6. FX captions
- Start: S-SMC. Press SELECT 6 on the SMC to open the reverb page.
1. Outside drum MIX/SEQ, hold SELECT 1. Read the SMC screen. Release it.
2. Hold SELECT 3. Read the screen. Release it.
- Look for: the caption names the effect you hear on that bus. The effect stops on release.
- Stop if: the caption does not match the sound, or the effect stays on.

Task 7. No dropped or stuck notes
- Start: S-SMC.
1. Connect the MIDI monitor. Set NOTES OUT to ON in GLO.
2. Play 20 fast notes on the keys. Then hold 4 keys together for 3 seconds. Release them.
3. While playing, press SMC Mute and Solo buttons and change SMC pages.
4. Count note on and note off messages in the monitor.
- Look for: each note on has one note off. No sound continues after you release all keys.
- Stop if: a note on is missing, a note off is missing, or a sound hangs.

Task 8. Two mixers keep their own pages
- Start: S-SMC. Connect the second SMC. Check that NAME lists two peers.
1. On SMC 1, choose page 2. On SMC 2, choose page 1.
2. Move fader 1 on each mixer.
- Look for: each mixer edits its own page target.
- Stop if: one mixer changes the other's page.

Task 9. BLE link stays up during mixer page changes
- Start: Do task 8 first.
1. Look at GLO > BLUETOOTH MIDI > STATUS. It must show CONNECTED.
2. Change SMC pages, lock and unlock, and move faders every few seconds.
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
