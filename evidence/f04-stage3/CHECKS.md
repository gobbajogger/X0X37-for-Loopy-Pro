# Stage 3 checks for F04 BF-001

Branch: `cursor/f04-bf001-stage3-fb90`
Audited / validated source SHA (packages authenticate): `a03cfab3a423ab5c65a6546afd4993fe51754ee7`
Content tree (Stage-4 DEV CHECKED records): `1755a2dd6d8b79e7405ab58919cc4d60a69ca11a4ae6601030b0f5de963f170e`
Git tree (`source_tree`): `85e159c5365b14c48e9f3df7d2089e8f2eb560d4`
Implement tip under test: `0d79d63485dc6585800a71ef716d095445f5a1fc` (`cursor/f04-bf001-stage2-fb90`)
BF-001 repair commit: `d11b74fc` — local `.fwsc` / `ctl.img` selection-generation race
Repair commit already in tip (F03; not re-opened): `4f18983a`
Test run subagent: `bc-ef042592-7de9-5b6d-8f3d-848569f55437`
Failure investigation subagent: not started (no failing required check)
No real MIDI opened. No device written. Firmware / controller / package bytes unchanged by this stage.
No GitHub Actions used as tests.
No source repair found; do not return to stage 2.

## Required BF-001 regressions

| Check | Command | Exit | Result |
| --- | --- | --- | --- |
| installer-bf001-local-file | `node web/test_install_local_file.mjs` | 0 | PASS (10/10) |

## Affected installer checks

| Check | Command | Exit | Result |
| --- | --- | --- | --- |
| installer | `python3 tests/install_test.py` | 0 | PASS |
| installer-f02-bind | `python3 tests/install_f02_loader_bind_test.py` | 0 | PASS |
| installer-f02-bind-web | `node web/test_f02_loader_bind.mjs` | 0 | PASS |
| installer-f03-probe | `python3 tests/install_f03_slot_probe_test.py` | 0 | PASS |
| installer-f03-probe-web | `node web/test_f03_slot_probe.mjs` | 0 | PASS |
| installer-slots | `python3 tests/install_slots_test.py` | 0 | PASS |
| installer-ctl | `python3 tests/install_ctl_test.py` | 0 | PASS |
| install-slots-web | `X0X_TEST_TIME_SCALE=20 node web/test_install_slots.mjs` | 0 | PASS |
| install-guards | `sh tests/host/test_install_guards.sh` | 0 | PASS (11/11) |
| install-head | `sh tests/host/test_install_head.sh` | 0 | PASS (13/13) |

## Failures

None. Every required check exited 0. No cause/disposition rows.

## Overall

**ALL_PASS** — 11/11. Full run log: Project store `internal/f04-stage3-test-run.md` (subagent `bc-ef042592-7de9-5b6d-8f3d-848569f55437`).
Deferred items unchanged: `evidence/f04-stage2/DEFERRED.md` (BF-002, BF-003 retire-vs-restore, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets).
