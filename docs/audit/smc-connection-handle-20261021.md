# SMC connection handle repair — 21 October 2026

## Report and installed source

The owner reports that Bluetooth never connects and asks to use the last installation record. `evidence/latest-install/install-record.json` identifies Elite development firmware `a5cb24dd08b40b80e802e03ff0d9c848315a9751`, installed at 07:40 Vladivostok time on 21 October. Later candidates, including integration base `7537a46ab49419920428299db8f3321d01e6fd16`, have build evidence but no later installation record.

The installed and integration-base `ble_midi_central.c` files are identical. The earlier working-source reference is `53d5654c9bbaec10b64c55fb60d0e7d857ba8156`; its previous physical report included both working controls and a later disconnect. These records do not establish a deterministic failure from every fresh boot.

## Reproduced regression

HCI connection handle zero is valid. The installed code also uses zero to mean no connection. During a stop/cancel race, a successful connection with handle zero is disconnected, but the host rejects the matching successful Disconnection Complete because it tests the handle as a Boolean. The original deadline then leaves the host in FAILED with stop intent retained. Restart is refused.

The same public HCI event sequence leaves the old `53d5654` host OFF and allows a fresh Reset. It leaves installed `a5cb24d` STOPPING, then FAILED, and a restart sends no command. The precise intervening introduction commit is not established by the locally available shallow history.

The new regression in `tests/host/ble_midi_stop_test.c`, invoked with `handles`, fails on the unchanged integration-base transport at the assertion that successful disconnection completes stop. The expected failure is retained in `evidence/smc-connect-20261021/baseline-handles.log`. HCI events, the mailbox, peer behavior and time are simulated.

## Repair and validation scope

Use `BLEC_NO_CONN = 0xffff` for absence, outside the valid wire handle range. Initialize and clear the handle explicitly. Use presence checks for stop, failure, late connection, disconnect and completed-packet handling. Do not send ACL data without a current handle. This does not add a field to the central state structure or change radio/controller settings.

The regression covers handles zero, `0x50` and `0x0eff`; ordinary and enhanced connection events; timer wrap; connection winning cancellation; an established link stopped under transport pressure; discovery timeout followed by rescan; and unrelated handle-zero events while no connection exists. Existing abort/deadline fixtures use the explicit absence value.

`evidence/smc-connect-20261021/dev-plan.json` selects the existing sanitized BLE suite and SMC input reachability check, plus an Elite development target and explicit controller artifact. The exact validation record and logs accompany the candidate handoff. The base integration's unchanged application, project, drum-control and storage evidence is reused. No release gate is selected.

## Physical limit and receiving owner

This is a reproduced connection-lifecycle defect. No retained hardware record establishes that the current device used handle zero or hit this race. Do not report the owner's connection failure fixed on hardware until the selected build is installed and its connection behavior is verified.

Firmware Main Builder remains the integration and device-write owner. Hand off this lane commit and its exact development artifacts to that owner. The cloud lane does not claim an installation or device PASS.

Protocol reference: [STMicroelectronics HCI event documentation](https://dev.st.com/stm32cube-docs/mw-bluenrg2/4.0.0/en/docs/markup/Host/bluenrg2_event_callbacks.html), `hci_le_connection_complete_event`, `hci_le_enhanced_connection_complete_event` and `hci_disconnection_complete_event`: valid handles include `0x0000` through `0x0eff`.
