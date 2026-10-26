# Stage 2 checks for F04 BF-001

Branch: `cursor/f04-bf001-stage2-fb90`
Audited / validated source SHA (packages authenticate): `a03cfab3a423ab5c65a6546afd4993fe51754ee7`
Content tree (Stage-4 DEV CHECKED records): `1755a2dd6d8b79e7405ab58919cc4d60a69ca11a4ae6601030b0f5de963f170e`
Git tree (`source_tree`): `85e159c5365b14c48e9f3df7d2089e8f2eb560d4`
Repair commit already in tip (F03; not re-opened): `4f18983a`
No real MIDI opened. No device written. Firmware / controller / package bytes unchanged.
GitHub Actions were not used as tests.

## Baseline (unfixed tip `222ae1fa`)

Controlled Node interleaving of the tip’s `acceptStock` / `stockfile` handlers (slow Pro then fast Elite): **BUG REPRODUCED** — older Pro replaced newer Elite (exit 1). Matches PR #26 / audit model.

## New regression

| Check | Command | Result |
| --- | --- | --- |
| BF-001 local file / ctl race | `node web/test_install_local_file.mjs` | PASS (10/10) |

## Affected installer checks

| Check | Command | Result |
| --- | --- | --- |
| installer | `python3 tests/install_test.py` | PASS |
| installer-f02-bind | `python3 tests/install_f02_loader_bind_test.py` | PASS |
| installer-f02-bind-web | `node web/test_f02_loader_bind.mjs` | PASS |
| installer-f03-probe | `python3 tests/install_f03_slot_probe_test.py` | PASS |
| installer-f03-probe-web | `node web/test_f03_slot_probe.mjs` | PASS |
| installer-slots | `python3 tests/install_slots_test.py` | PASS |
| installer-ctl | `python3 tests/install_ctl_test.py` | PASS |
| install-slots-web | `X0X_TEST_TIME_SCALE=20 node web/test_install_slots.mjs` | PASS |
| install-guards | `sh tests/host/test_install_guards.sh` | PASS (11/11) |
| install-head | `sh tests/host/test_install_head.sh` | PASS (13/13) |

## Repair summary

- `web/x0x_installer.html`: `stockLoadSerial` / `ctlLoadSerial` before `arrayBuffer`; clear stock / uncheck+disable confirm / disable Install on new selection; apply digest state only when generation still matches; same for `ctlfile`.
- `web/test_install_local_file.mjs`: host race regression (no MIDI, no device write).
- `tests/run_tests.sh`: registers `installer-bf001-local-file`.
