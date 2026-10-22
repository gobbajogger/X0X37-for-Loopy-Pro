# SMC filter correction and input investigation

Base: `53d5654c9bbaec10b64c55fb60d0e7d857ba8156` (the installed-build reference from the installation chat).
Lane: `lane/mb-smc-filter`. Receiving owner: the assigned firmware integration chat.

The owner reported a connected SMC with no response to faders, knobs or buttons in both matched DAW/DAW and User/User configurations. The owner requested fader 8 to behave like the X0X keyboard's filter fader.

The local change maps HOME/clip fader 8 to `T_MST/MST_DJF`, matching the keyboard's bank-B fader. The master mute light continues to follow master volume, independently of filter position. The change is not installed.

## Evidence

- Read-only `tools/ble_autotest.py --diag-only` reported CONNECTED, central state 6, step 14, no mailbox error and zero c2h drops. These diagnostics do not expose decoded SMC input. An empty INLOG does not establish that the SMC sent nothing.
- USB capture on both SINCO SMC-Mixer ports received 5,394 messages. The local raw capture is `evidence/smc-usb-capture.json` (not committed).
- Captured DAW messages include pitch bend on channels 1–8, channel-1 relative CC 16–23, and channel-1 Mackie note buttons. These match the existing DAW decoder.
- Later capture messages include CC 40–47 faders, CC buttons, and channel-2 absolute CC 30–37 knob values. The current User decoder expects relative CC 16–23 and MMC or note buttons, so it does not support this captured configuration. The exact button-to-CC correspondence needs a labelled capture before changing it. Do not assume this is an unmodified factory preset.
- Direct Mac BLE capture did not complete. The initial scanner waited for CoreBluetooth readiness; the run-loop fallback failed. No raw BLE packet evidence was obtained. The owner temporarily switched SMK-37 off for this attempt and was told it could be turned back on.

## Checks

- `python3 tests/host/smc_filter_test.py`: PASS; the same regression rejects the original master-volume destination.
- `sh tests/host/smc_clip_visibility_test.sh`: PASS for BLE DAW, BLE USER and host USER paths.
- `sh tests/host/smc_part_select_test.sh`: PASS.
- `tests/host/smc_protocol_test.c` and `tests/host/ble_midi_central_test.c`, compiled with `cc -std=c99 -Wall -Wextra -Werror` (central test linked with its production source): PASS.
- `git diff --check`: PASS before this note.
- Target build, formal gate, installation and device validation of the filter change: NOT RUN.

Next: obtain actual BLE notification bytes or add bounded SMC receive diagnostics through the integration owner. Test captured bytes against the BLE parser before proposing a Bluetooth fix. Integrate the filter correction separately from the unresolved transport diagnosis.

## Successful Bluetooth capture retry

The later Mac capture connected to `SMC-MackieMixer` and subscribed to MIDI characteristic handle 113. It received 1,271 raw BLE packets from owner-operated DAW controls. Replaying every packet through the production `blec_midi_packet` function produced all 2,711 expected MIDI messages. The local capture and reproducible replay script are `evidence/smc-raw-ble-retry.json` and `evidence/replay_capture.py`. This supersedes the earlier lack of raw BLE evidence. It verifies the MIDI payload parser, not the keyboard's HCI reception or UI dispatch.

After the keyboard returned on USB, repeated DIAG reads reported `plat_state=4` (READY), `h_state=3` (SCANNING), `step=6`, and zero controller-to-host mailbox drops. The owner reported SMC connected in host mode; the disagreement requires checking the exact on-screen STATUS value. Apple's documented `MIDIBluetoothDriverDisconnect` returned success for the Mac's SMC MIDI connection. No keyboard firmware was installed or changed during this retry.

## Owner confirms control response

The owner confirmed the X0X screen showed CONNECTED. A fresh USB DIAG then agreed: central state 6, step 14, no mailbox drops. During the control test, controller receive counts increased from 668 to 904 while connected. The owner answered “Controls now work” and added “and smc drives them too.” This confirms response on the existing installed firmware; it does not validate the uninstalled fader-8 correction.

The counter watch also recorded the link leaving CONNECTED later, entering CONNECTING and then SCANNING. The cause is unknown; continuous link stability is not established. No transport firmware change was necessary to obtain the confirmed control response. Do not attribute the original failure conclusively to the Mac connection.

## 23 October 2026 — owner follow-up: no visible SMC connection issues

The owner reports having tested the SMC connection to X0X sufficiently to
observe **no visible connection issues** in the currently tested setup. The
earlier symptom of the SMC not reaching X0X controls is not apparent in this
manual test. This supersedes the earlier *current-status concern* about
connection reachability; it does not erase the historical link-state transitions
recorded above.

**Scope of the observation:** owner-reported physical test of the connection
and basic reachability, not a claim that every SMC fader, knob, button, LED,
feedback route, both-controller configuration or long-duration link behavior has
passed exhaustive validation. No new independent hardware test or host regression
was run for this note.

**Repository reference:** GitHub `main` was
`c9718ae833eb64a5191c350922593df8a36017c9` when this report was recorded
on 2026-10-23. The installed firmware and BLE-controller image hashes were not
provided, so this is **not** proof that an artifact built from that exact commit
was flashed and hardware-validated. Do not treat this observation as a blanket
bug-free or release-gate PASS.

No firmware, MIDI routing, Bluetooth or installer behavior changed in recording
this observation.
