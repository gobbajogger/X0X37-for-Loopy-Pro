# F15 BF-034 Stage-3 host regression — test-run RESULT

- **git HEAD SHA:** `0f7f03b52e223e3a1f7ae3a21d46dc0ca62fefa4` (branch `cursor/f15-bf034-slotprobe-stage3-237f`; equals Stage-2 tip `0f7f03b5`)
- **env vars:** `X0X_DEV_PADCAL=1`, `X0X_TEST_JOBS=8`
- **scope:** host-only; no production tool edits; no flash; no GitHub Actions; no source patches

## Checks

| # | command | exit | result | log | summary |
|---|---------|------|--------|-----|---------|
| 1 | `python3 tests/install_f03_slot_probe_test.py` | 0 | PASS | `evidence/f15-stage3/f03-slotprobe.log` | F03 fail-closed probe/plan/run harness (timeout→exit 7, empty→install, `--no-slots` skip) |
| 2 | `python3 tests/install_test.py` | 0 | PASS | `evidence/f15-stage3/install-test.log` | Install suite PASS including missing/malformed SMP_INFO fail-closed before write |
| 3 | `python3 tests/x0x_slots_info_preflight_test.py` | 0 | PASS | `evidence/f15-stage3/slots-info-preflight.log` | 31 assertions, 0 failures/errors |
| 4 | `python3 tests/install_slots_test.py` | 0 | PASS | `evidence/f15-stage3/install-slots.log` | Slot backup/restore/install path suite PASS |

## FakeSMK mute / None / empty (read-only confirm from `tests/install_test.py`)

Matches Stage-2 claim:

- `answer_slots=False` or `slot_info=None` → no SMP_INFO reply (fail closed / TimeoutError path).
- Default `slot_info=(2, 80, 0, 0, 0, 0, 0, 0)` → successful empty layout parse (count/size > 0, all nz=0).

## ALL_PASS

**yes**
