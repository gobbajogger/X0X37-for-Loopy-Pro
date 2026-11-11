# Stage-2 repair handoff → stage 3 test

Receiving stage: **3 Test and investigate** (fresh cloud thread).
Audit: [docs/audit/smc-mixer-ui-popup-2026-10-22.md](../../docs/audit/smc-mixer-ui-popup-2026-10-22.md).
Stage-4 FAIL: `evidence/smc-mixer-ui-popup-stage4/DEFECT.txt`.
Repair branch: `cursor/smc-mixer-ui-popup-stage2-repair-5686`.
Stage-2 repair owner: `bc-a9596c35-cd3d-534c-b570-4a5800957829`.

## Candidate

Tip of `cursor/smc-mixer-ui-popup-stage2-repair-5686` after push (repair of failed
`5109ef6` / `ad4f311`). Stage 3 must `git fetch` and test that tip.

## What changed in the repair

SMC-05 peek/hold/restore kept; XIP fit recovered for Elite and Pro DEV:

- Table-driven strip→(view,page) with MASTER = `1+NKIT` (no `page=-1` resolve).
- Peek state in `smc_peek_raw[]` accessed via `smc_peek` pointer (avoids IPA-SRA
  field splitting that inflated abs loads).
- Slimmer guards; restore tick only in `smc_frame`.

Link proof on this machine: Elite/Pro DEV both **56 B code free**, pool_free **8364**.

## Do (stage 3)

1. Check out the repair tip on `cursor/smc-mixer-ui-popup-stage2-repair-5686`.
2. Re-run the focused SMC host suite (commands below). Dispose failures.
3. Record under `evidence/smc-mixer-ui-popup-stage3/` (or a repair-stage3 dir).
4. Do **not** install. Do **not** merge to main.
5. On PASS, start exactly one fresh **stage 4 Validate** thread with the tip SHA.

## Test commands

```sh
python3 tests/host/smc_fader_test.py
python3 tests/host/smc_buttons_test.py
python3 tests/host/smc_mapping_test.py
python3 tests/host/smc_filter_test.py
sh tests/host/smc_clip_visibility_test.sh
sh tests/host/smc_part_select_test.sh
sh tests/host/smc_drum_mutes_test.sh
sh tests/host/smc_performance_abort_test.sh
sh tests/host/smc_reachability_test.sh
sh tests/host/smc_ui_popup_test.sh
```
