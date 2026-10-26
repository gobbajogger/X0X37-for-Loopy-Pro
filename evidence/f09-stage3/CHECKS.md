# Stage 3 checks for F09 (BF-005)

Branch: `cursor/f09-bf005-stage3-fb90`
Stage-3 owner: `bc-ae6fbd79-7084-5960-9b7c-1f7fce57cc2b`
Implement tip under test: `d0d00da23c3ce2268f06f31f8c062716d5a37ad0` (`cursor/f09-bf005-stage2-fb90`)
BF-005 repair commit: `94f4a012` — Fix BF-005 stale ext_pending after clock owner drop
Sequencer blob: `fd3f65306df166494d10facf8ddae058287cc17b` (matches implement tip)
Audited / validated source SHA: `8a74ff52cba842c3f2d8365600286f94e6bccdf0`
Content tree (pre-repair package baseline): `1a1e7a22d136fef6e30702c9d3207d22798a6dd382717acc5ab0f40d2f235acf`
Git tree (pre-repair): `33d6950b0a378525f2a8b592672275625893239f`
Firmware tree (pre-repair): `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f`
Audited firmware content tree (package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004)
Test run subagent: `bc-a4520fef-24a0-59c3-a24d-66631c79eb1d`
Failure investigation subagent: not started (no failing required check)
No real MIDI opened. No device written. No source patch by this stage.
No GitHub Actions used as tests.
No source repair found; do not return to stage 2.

## Required BF-005 regressions

| Check | Command | Exit | Result | Log |
| --- | --- | ---: | --- | --- |
| host unit (sequencer + clk-lock + clock-handover) | `X0X_TESTS="sequencer clk-lock clock-handover" sh tests/run_tests.sh` | 0 | PASS | `evidence/f09-stage3/host-unit.log` |
| sequencer | (suite subset) | 0 | PASS | `evidence/f09-stage3/sequencer.log` |
| clk-lock | (suite subset) | 0 | PASS | `evidence/f09-stage3/clk-lock.log` |
| clock-handover | (suite subset) | 0 | PASS | `evidence/f09-stage3/clock-handover.log` |
| seq_clk_drop intent review | code review + `git rev-parse HEAD:firmware/src/seq/sequencer.c` | 0 | PASS | `evidence/f09-stage3/seq-clk-drop-review.txt` |

Machine summary: `evidence/f09-stage3/test-run-summary.json`. Full run: Project store `internal/f09-stage3-test-run.md`.

## clock-handover coverage (both ok)

- `PASS old_pending=0`: step 2 followed the 7th fresh Bluetooth clock
- `PASS old_pending=3`: step 2 followed the 7th fresh Bluetooth clock (stale pending cleared on owner drop; no acceleration)

## seq_clk_drop intent

- Owner-matching drop clears `s->ext_pending` with `clk_lock`.
- Non-owning drop leaves pending unchanged (clk-lock also covers USB drop while BLE owns lock).
- Start / Continue / internal-clock paths unchanged in intent.

## Failures

None. Every required check met its expected exit. No cause/disposition rows.

## Overall

**ALL_PASS** — required BF-005 host regressions. Deferred items unchanged: `evidence/f09-stage2/DEFERRED.md`. BF-006 / BF-007 not taken.
