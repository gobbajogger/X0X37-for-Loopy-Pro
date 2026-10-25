# Stage 3 checks for F03

Branch: `cursor/f03-slot-probe-stage3-fb90`
Audited source SHA: `4ee8f37714b0892f2c70d157fda9328da393e987`
Content tree: `7ad3a50da8abac890215dae3c28197a2fa90041b8615e828b6b3c6e18060fb83`
Git tree: `c63af7972f3ca6e5d4e8ac3aac5e70abc8178265`
Implement tip under test: `a03cfab3a423ab5c65a6546afd4993fe51754ee7` (`cursor/f03-slot-probe-stage2-fb90`)
Repair commit: `4f18983a` — fail-closed sample-slot layout probe
Test run subagent: `bc-833de9db-27d0-5ba5-88a9-f5a30914f4ae`
Failure investigation subagent: not started (no failing required check)
No real MIDI opened. No device written. Firmware / controller / package bytes unchanged by this stage.
No GitHub Actions used as tests.
No source repair found; do not return to stage 2.

## Required F03 regressions

| Check | Command | Exit | Result |
| --- | --- | --- | --- |
| native slot probe | `python3 tests/install_f03_slot_probe_test.py` | 0 | PASS |
| web installKeepingSlots probe | `node web/test_f03_slot_probe.mjs` | 0 | PASS |

## Affected installer checks

| Check | Command | Exit | Result |
| --- | --- | --- | --- |
| installer | `python3 tests/install_test.py` | 0 | PASS |
| installer-f02-bind | `python3 tests/install_f02_loader_bind_test.py` | 0 | PASS |
| installer-f02-bind-web | `node web/test_f02_loader_bind.mjs` | 0 | PASS |
| installer-slots | `python3 tests/install_slots_test.py` | 0 | PASS |
| installer-ctl | `python3 tests/install_ctl_test.py` | 0 | PASS |
| install-slots-web | `X0X_TEST_TIME_SCALE=20 node web/test_install_slots.mjs` | 0 | PASS |
| install-guards | `sh tests/host/test_install_guards.sh` | 0 | PASS (11/11) |
| install-head | `sh tests/host/test_install_head.sh` | 0 | PASS (13/13) |

## Failures

None. Every required check exited 0. No cause/disposition rows.

## Overall

**ALL_PASS** for the F03 stage-3 scope. Ready for stage 4 validation of development packages from this tested tip.
