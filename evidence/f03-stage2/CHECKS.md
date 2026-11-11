# Stage 2 checks for F03

Branch: `cursor/f03-slot-probe-stage2-fb90`
Audited source SHA: `4ee8f37714b0892f2c70d157fda9328da393e987`
Content tree: `7ad3a50da8abac890215dae3c28197a2fa90041b8615e828b6b3c6e18060fb83`
Git tree: `c63af7972f3ca6e5d4e8ac3aac5e70abc8178265`
No real MIDI opened. No device written. Firmware / controller / package bytes unchanged.

## New regressions

| Check | Command | Result |
| --- | --- | --- |
| native slot probe | `python3 tests/install_f03_slot_probe_test.py` | PASS |
| web installKeepingSlots probe | `node web/test_f03_slot_probe.mjs` | PASS |

## Affected installer checks

| Check | Command | Result |
| --- | --- | --- |
| installer | `python3 tests/install_test.py` | PASS |
| installer-f02-bind | `python3 tests/install_f02_loader_bind_test.py` | PASS |
| installer-f02-bind-web | `node web/test_f02_loader_bind.mjs` | PASS |
| installer-slots | `python3 tests/install_slots_test.py` | PASS |
| installer-ctl | `python3 tests/install_ctl_test.py` | PASS |
| install-slots-web | `X0X_TEST_TIME_SCALE=20 node web/test_install_slots.mjs` | PASS |
| install-guards | `sh tests/host/test_install_guards.sh` | PASS (11/11) |
| install-head | `sh tests/host/test_install_head.sh` | PASS (13/13) |

GitHub Actions were not used as tests.
