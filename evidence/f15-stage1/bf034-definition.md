# BF-034 — F03 slot-probe fail-closed host negatives red on tip

## Defect

On validated source `092589b84fd455355216cb0de93c4782236f7524`, `python3 tests/install_f03_slot_probe_test.py` exits 1.

Recorded earlier as Stage-7 supporting `SUPPORTING_FAIL` / empty-FakeSMK `SlotError` (`evidence/f13-stage7-lx73/safety-gates-candidate.log`). Codex on PR #327 flagged the same installer fail-closed negatives. Local proof on this audited SHA confirms the tip FAIL; deferred non-FAIL items do not supersede it.

## Tip blobs (audited SHA)

| Path | Blob |
| --- | --- |
| `tests/install_test.py` (`FakeSMK`) | `4de1cc02719fd606aafce4c78537a844d925dbda` |
| `tests/install_f03_slot_probe_test.py` | `018d80e6fb871c337c680bc233de0d159391b95e` |
| `tools/fm1_install.py` | `3195c4b1249648074d15bd613db57067a16642e5` |
| `tools/x0x_slots.py` | `f05a978ce01641bf75d44be6320eae494a1f994b` |

## Cause (local)

`FakeSMK.rx` in `tests/install_test.py`:

1. Early branch (cmd byte 15 / `CMD_INFO`) always transmits payload `[0, 0]` when `answer_slots` is true. `parse_info` rejects count/size 0 → `SlotError("invalid SMP_INFO slot count or size")`. That is not a successful empty layout (empty slots need a positive count/size with empty descriptors, which default `slot_info=(2, 80, 0, 0, 0, 0, 0, 0)` already models).
2. `answer_slots=False` skips only that early branch; the later `slot_info` reply still answers, so mute never times out. F03 then sees a successful empty layout instead of `TimeoutError`.
3. `slot_info=None` (install_test “missing SMP_INFO”) is also shadowed by the early `[0, 0]` reply, so the timeout fail-closed path is never exercised; CLI still exits 7 but with the wrong “invalid SMP_INFO…” text instead of “SMP_INFO timed out”.

Executable witness: `evidence/f15-stage1/bf034-f03-slotprobe-witness.log` (exit 1). Supporting: `install-test-supporting.log` — nine installer negatives FAIL on the same tip, including missing/malformed SMP_INFO fail-closed cases.

## Expected after repair

- Mute / `answer_slots=False` / `slot_info=None`: no SMP_INFO answer → `slot_probe`/`plan_slots` fail closed via timeout (and `run` exit 7, nothing written, timed-out wording).
- Successful empty layout: valid empty descriptors → `plan_slots` returns `None` (no backup) and install may proceed without `--no-slots`.
- Malformed SMP_INFO still fail-closed before write.
- `python3 tests/install_f03_slot_probe_test.py` exits 0; affected `tests/install_test.py` fail-closed negatives pass. `X0X_TEST_JOBS≥8`.

## Out of scope

Production `main` / production Pages, device flash, reopening BF-001–BF-013 / FP-001, lengthening `SLOT_PROBE` budgets (still deferred optional), Chicago Arp/Fx sole-owner leftovers (`bc-8efb2f94…`), LX-73 package republish.
