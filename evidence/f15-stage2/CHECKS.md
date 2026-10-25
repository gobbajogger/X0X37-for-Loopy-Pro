# Stage 2 checks — F15 BF-034 FakeSMK / F03 slot-probe

Branch: `cursor/f15-bf034-slotprobe-stage2-237f`
Parent audit tip: `22d9d7d4` (`cursor/f15-f03-slotprobe-stage1-237f`)
Audited source SHA: `092589b84fd455355216cb0de93c4782236f7524`
Env: `X0X_DEV_PADCAL=1`, `X0X_TEST_JOBS=8`
Policy: local host only — GitHub Actions / billing ≠ proof.

## Repair

| Path | Change |
| --- | --- |
| `tests/install_test.py` | Removed FakeSMK early CMD_INFO `[0,0]` shortcut. INFO reply only when `answer_slots` (default True) and `slot_info is not None`. Mute / `slot_info=None` → no reply → timeout. Default `slot_info=(2,80,0,0,0,0,0,0)` remains successful empty layout. |
| `tests/install_f03_slot_probe_test.py` | Assertion alignment only: mute raise may be `SlotError("…timed out…")` (production `slot_probe` wrap); CLI wording matches `slot backup cannot start` / timed out (not legacy `"layout could not be read"`). |
| `tools/fm1_install.py` / `tools/x0x_slots.py` | **Untouched** — no remaining production-path gap after harness fix. |

## Host proof (this VM)

| Check | Result | Log |
| --- | --- | --- |
| `python3 tests/install_f03_slot_probe_test.py` | **PASS** exit 0 | `f03-slotprobe-green.log` |
| `python3 tests/install_test.py` | **PASS** (fail-closed missing/malformed SMP_INFO + full suite) | `install-test-green.log` |
| `python3 tests/x0x_slots_info_preflight_test.py` | **PASS** 31/31 | `slots-info-preflight-green.log` |
| `python3 tests/install_slots_test.py` | **PASS** | `install-slots-green.log` |

## Not run / N/A

| Item | Note |
| --- | --- |
| Firmware Elite/Pro package rebuild | Host-only harness repair; Stage 4 owns packages. |
| Device flash | Cloud does not flash. |
| GitHub Actions | Never proof. |
