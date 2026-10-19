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
