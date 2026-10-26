# Bluetooth device/host runtime switch

Reason: the role setting was read only at boot and the UI requested a restart.
Purpose: select either Bluetooth MIDI role while the instrument keeps running.
User effect: changing DEVICE/HOST tears down the current Bluetooth connection
and starts the selected role without restarting the instrument.

The TIMER5 owner stops the current host, waits for command/link completion,
both mailbox rings, and any deferred Bluetooth MIDI link-end event, then
reinitializes the shared host union. The next service tick starts the new
host with HCI Reset. The RF image/core is not restarted. Old queued MIDI,
SysEx and feedback state are discarded. Address and device name are retained.
A stop unresolved after three seconds fails closed without replacing the
union. Role changes while Bluetooth is OFF retain OFF. Enabling the controller
after an OFF boot still requires a restart; this change covers role switching.

## Local checks

- PASS: `X0X_DEV_PADCAL=1 sh tests/run_tests.sh host-build`.
- PASS: `X0X_DEV_PADCAL=1 sh tests/run_tests.sh ble ble-role-switch ble-midi-receive ble-capture-replay smk-clip-ble-clear smk-clip-ble-notify ble-central-diag smc-link smc-queue smc-protocol`.
- PASS: `git diff --check`.
- Independent helper review found no unsafe quiescence condition; its tests
  use the extracted production device/central service bodies, production role
  orchestrator, real HCI hosts and real mailbox rings under ASan/UBSan.
- Role cases: connected transitions both ways, pending startup command,
  deferred MIDI release with a full queue, rapid reversal, OFF, create/cancel
  race and late successful connection, mailbox backlog, rejected disconnect,
  and timeout across millisecond clock wraparound.
- The first receive-suite run exposed a missing role getter stub in existing
  extracted-service harnesses. Both harnesses were updated; final runs pass.

## Initial environment limitation (resolved during Full Send)

Before recovery, the native Elite build was attempted with `BOARD=smk37 MODEL=elite X0X_DEV_PADCAL=1 ./build.sh`:
blocked because the default toolchain and SDK were absent. Full Send subsequently
recovered pinned inputs and validated the Elite package and controller. See
[FULL-SEND.md](FULL-SEND.md) for current evidence and remaining limits.
