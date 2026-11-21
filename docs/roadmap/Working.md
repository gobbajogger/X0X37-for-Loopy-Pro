# Working

Generated from `CONSOLIDATED-STATE.md` by `tools/generate_roadmap.py`. Do not edit.

| Item | Move | Status | Owner | Merge | Dependencies | Next action |
|---|---|---|---|---|---|---|
| M0: Start files and green gates | M0 | working | integ |  |  | Reconcile the 23 host failures recorded at firmware 76040daa and qualify the current consolidated firmware and nightly baselines. |
| M1: Sound engines | M1 | working | sound |  | M0 | Complete exact-head checks and integrate; no move completion asserted |
| M2: Firmware UI | M2 | working | fwui |  | M0 | FWUI #975 merged. PR #988 closed the FWUI checklist after all four integrated F5 controls passed. Integ will complete the host-suite accounting under I2. |
| M3: Remote | M3 | working | remote |  | M0 | Complete exact-head checks and integrate; no move completion asserted |
| M4: Website | M4 | working | web |  | M0 | Use M2 render inputs as they arrive; require final same-SHA render parity before completion |
| M5: BREAK samples, LSan and roadmap | M5 | working | integ |  | M0 | Compact-program LeakSanitizer passed again at 2643d690. Embed the five original BREAK samples in both stock models and verify a signed-in opening-post vote through inbox refresh. |
