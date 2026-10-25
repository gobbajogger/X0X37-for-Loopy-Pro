# Stage 3 checks — F15 BF-034 FakeSMK / F03 slot-probe

Branch: `cursor/f15-bf034-slotprobe-stage3-237f`
Stage-3 owner: `bc-dfce289d-9e57-50f6-9219-39a84a11217e`
Implement tip under test: `0f7f03b52e223e3a1f7ae3a21d46dc0ca62fefa4` (`cursor/f15-bf034-slotprobe-stage2-237f` tip; BF-034 repair `7fb4b71d`)
Audited / validated source SHA: `092589b84fd455355216cb0de93c4782236f7524`
Content tree: `25ada1f02e2e289072fab8db464b715e09f9c0712bded561af008bc88d8c54af`
Git source_tree: `9382fb1276e320d9f9d290dbad187fa08cdd224c`
Firmware tree: `a5ba08046700be5cdf2751b7afc1197411b24121`
Env: `X0X_DEV_PADCAL=1`, `X0X_TEST_JOBS=8`
Policy: local host only — GitHub Actions / billing ≠ proof.
Test-run subagent: `bc-32baa265-d030-5c35-96a0-a96c0826c941`
Failure-investigation subagent: not started (no failing required check)
No device written. No source patch by this stage.
No production-path edit (`tools/fm1_install.py` / `tools/x0x_slots.py` untouched).
No source repair found; do not return to stage 2.

## Required BF-034 regressions

| Check | Command | Exit | Result | Log |
| --- | --- | ---: | --- | --- |
| F03 slot-probe harness | `python3 tests/install_f03_slot_probe_test.py` | 0 | PASS | `evidence/f15-stage3/f03-slotprobe.log` |
| install FakeSMK suite | `python3 tests/install_test.py` | 0 | PASS — missing/malformed SMP_INFO fail-closed + full suite | `evidence/f15-stage3/install-test.log` |
| slots INFO preflight | `python3 tests/x0x_slots_info_preflight_test.py` | 0 | PASS — 31/31 | `evidence/f15-stage3/slots-info-preflight.log` |
| install slots suite | `python3 tests/install_slots_test.py` | 0 | PASS | `evidence/f15-stage3/install-slots.log` |

Machine summary: `evidence/f15-stage3/test-run-RESULT.md`.

## FakeSMK semantics (read-only confirm)

- Mute (`answer_slots=False`) and `slot_info=None` → no SMP_INFO reply → fail closed.
- Default empty `slot_info=(2,80,0,0,0,0,0,0)` → successful empty parse (no backup).

## Failures

None. Every required check met its expected exit. No cause/disposition rows.

## Overall

**ALL_PASS** — required BF-034 host regressions. Deferred items unchanged: `evidence/f15-stage2/DEFERRED.md` (copied to `evidence/f15-stage3/DEFERRED.md`).
