# Stage-3 handoff → stage 4 validate (post-repair)

Receiving stage: **4 Validate** (fresh cloud thread).
Audit: [docs/audit/smc-mixer-ui-popup-2026-10-22.md](../../docs/audit/smc-mixer-ui-popup-2026-10-22.md).
Stage-2 repair: `cursor/smc-mixer-ui-popup-stage2-repair-5686` @ `828298d9db1a403919e7e4c8176db83512281bb2`.
Stage-3 branch: `cursor/smc-mixer-ui-popup-stage3-repair-5686`.
Stage-3 owner: `bc-8182b9ce-4a8d-5e10-a8e3-195f08d5e305`.
Prior Stage-4 FAIL: `evidence/smc-mixer-ui-popup-stage4/DEFECT.txt` (ad4f311 XIP).

## Candidate

Firmware SHA: `828298d9db1a403919e7e4c8176db83512281bb2` (identical tree tip as Stage-2 repair at independent Stage-3 retest).
Fetch `cursor/smc-mixer-ui-popup-stage3-repair-5686` or `cursor/smc-mixer-ui-popup-stage2-repair-5686` and validate that SHA. Do not validate `ad4f311` / `5109ef6`.

## Stage-3 results

- Focused SMC host suite: **PASS** (10/10). Evidence: `host-suite.log` and per-test logs.
- SMC-05 behavior confirmed via `smc_ui_popup_test` + source review.
- Optional Elite/Pro DEV spot-check on this machine: both **56 B code free**, pool_free **8364** (informational; Stage 4 owns formal `dev_validate`).

## Do (stage 4)

1. Check out candidate `828298d9db1a403919e7e4c8176db83512281bb2`.
2. Run formal Elite and Pro DEV package validation (`tools/dev_validate.py` / BUILDING.md) with `X0X_DEV_PADCAL=1`.
3. Require pool_free >= 8192 and positive code/RAM free; record identities.
4. On PASS → exactly one Stage 5 Merge thread. On FAIL → exactly one Stage 2 repair with DEFECT.
5. Do **not** install. Do **not** merge to main. Do **not** implement page-lock/multi-SMC.

## Evidence

`evidence/smc-mixer-ui-popup-stage3-repair/` — RESULT.txt, host-suite.log, size-*-spotcheck.log, stage4-prompt.txt.
