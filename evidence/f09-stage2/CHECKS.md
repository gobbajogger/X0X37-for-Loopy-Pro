# Stage 2 checks for F09 (BF-005)

Branch: `cursor/f09-bf005-stage2-fb90`
Stage-2 owner: `bc-42c7795e-c06a-577e-b9f6-2b9f2b368ed0`
Base audit tip: `168a1352755226453c5beaeddbe1dac033add88d` (`cursor/f09-bf005-stage1-fb90`)
Audited / validated source SHA: `8a74ff52cba842c3f2d8365600286f94e6bccdf0`
Content tree (pre-repair package baseline): `1a1e7a22d136fef6e30702c9d3207d22798a6dd382717acc5ab0f40d2f235acf`
Git tree (pre-repair): `33d6950b0a378525f2a8b592672275625893239f`
Firmware tree (pre-repair): `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f`
Audited firmware content tree (package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Repair: **BF-005 only** — owner-matching `seq_clk_drop` clears `ext_pending` with `clk_lock` in `firmware/src/seq/sequencer.c` + host regression `tests/host/bugfinder_clock_handover_test.c` wired as `clock-handover` in `tests/run_tests.sh`.
Sequencer blob before: `83b03f73139d8e679c94526c66b390b6e5bc2f39`. After: `fd3f65306df166494d10facf8ddae058287cc17b`.
No production `main` / production Pages update. No device flash. GitHub Actions were not used as tests.
BF-001 / BF-002 / BF-003 / FP-001 / BF-004 not reopened. BF-006 / BF-007 not taken.
Development configuration for later firmware rebuilds: `X0X_DEV_PADCAL=1`.

## Local proof (this head)

| Check | Result | Log |
| --- | --- | --- |
| `bugfinder_clock_handover_test` on unfixed tip (`83b03f73…`) | **FAIL** exit 1 — `old_pending=3` accelerates new BLE owner (`observed=2` vs expected 1 on 4th fresh clock); `old_pending=0` PASS | `evidence/f09-stage2/bf005-red.log` |
| `bugfinder_clock_handover_test` after repair | **PASS** exit 0 — both `old_pending=0` and `old_pending=3` | `evidence/f09-stage2/bf005-green.log` |
| Non-owning `seq_clk_drop` keeps `ext_pending`; owning drop clears | **PASS** exit 0 | `evidence/f09-stage2/nonowning-drop.log` |
| `clk-lock` after repair | **PASS** exit 0 | `evidence/f09-stage2/clk-lock.log` |
| `X0X_TESTS="sequencer clk-lock clock-handover" sh tests/run_tests.sh` | **PASS** — all three ok | `evidence/f09-stage2/host-unit.log` |

Host builds used `cc` with the suite `W` flags (`-O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion -Werror`). UBsan runtime was absent on this VM (`libclang_rt.ubsan_standalone` missing); red/green still match Stage-1 witness outcomes without sanitizers. Pillow/ASan optional gaps were not required for these sequencer unit checks.

## Repair summary

- When `seq_clk_drop` clears `clk_lock` for the ending (owner-matching) source, also clear `s->ext_pending`.
- A drop of a non-owning source leaves `ext_pending` unchanged.
- Start / Continue / internal-clock paths unchanged in intent.
- Promoted PR #26 lane harness into `tests/host/bugfinder_clock_handover_test.c` and wired `clock-handover` into `tests/run_tests.sh`.

## Deferred

See `evidence/f09-stage2/DEFERRED.md`.

## Overall

**IMPLEMENT CHECKED.** Ready for Stage 3.
