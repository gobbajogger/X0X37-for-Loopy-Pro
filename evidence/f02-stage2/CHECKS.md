# Stage 2 checks for F02

Branch: `cursor/f02-stage2-fb90`
Audited source SHA: `13d32e1dee37b6fb5a1e339aff499a75eecb97b5`
No real MIDI opened. No device written. Firmware / controller / package bytes unchanged.

## New regressions

| Check | Command | Result |
| --- | --- | --- |
| native wait_for | `python3 tests/install_f02_loader_bind_test.py` | PASS |
| web waitFor | `node web/test_f02_loader_bind.mjs` | PASS |

## Affected installer checks

| Check | Command | Result |
| --- | --- | --- |
| installer | `python3 tests/install_test.py` | PASS |
| installer-slots | `python3 tests/install_slots_test.py` | PASS |
| installer-ctl | `python3 tests/install_ctl_test.py` | PASS |
| install-slots-web | `X0X_TEST_TIME_SCALE=20 node web/test_install_slots.mjs` | PASS |
| install-guards | `sh tests/host/test_install_guards.sh` | PASS (11/11) |
| install-head | `sh tests/host/test_install_head.sh` | PASS (13/13) |
| fm1ota install smoke | synthetic FakeFM1 via `web/fm1ota.js` | PASS |

GitHub Actions were not used as tests.
