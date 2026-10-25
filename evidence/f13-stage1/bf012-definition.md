# BF-012 — Master limiter replays stale look-ahead after OFF→ON

**Finding id:** BF-012 (open PR #68 `lane/bugfinder-master-limiter-stale-ring-20261023-2024`).
**Audited source SHA:** `de11b6d7e3ac3322d2db689675d9ed27ed667785` (F12 published BF-008 tip).
**Tip `master.c` blob:** `b5557b6c41bdf45275547a3b6c5431d5252f62f0` (identical on `origin/nightly` at audit time).
**Repair lane tip:** `99543b84c4a785c6bd279721250d99db977b4da7`.
**Repair `master.c` blob:** `f91a9473f0ae1cdd5e037e521f39fdfcc2022395`.
**Repair `master_test.c` blob:** `df2f13b35a6fc7b858b019a1950982c23f6386e3`.

## Defect

When MASTER LIMIT is disabled, `master_process` / `master_process_st` bypass the limiter and do not advance the 64-sample look-ahead rings. Re-enabling LIMIT later emits audio that was captured in those rings before the bypass, even when the live input is silence. A performer can hear a spurious click or burst when toggling LIMIT back on.

In tip `firmware/src/dsp/master.c` `master_set`, there is no clear of `la` / `la_r` / `la_pos` / hold / env when transitioning LIMIT from off to on.

## Expected

OFF→ON with silent input after bypass yields peak 0.0 from the look-ahead window. A fresh signal still emerges after the normal look-ahead. Writing LIMIT=ON while already ON must not wipe a live ring.

## Witness (this Stage 1)

- Tip baseline `master` (existing tests, no BF-012 case): PASS (`master-tip-baseline.log`).
- Tip sources + PR #68 regression overlay: FAIL mono/stereo stale peak `0.500000` (`bf012-witness.log`). Scratch only; tip tree restored.
- Scratch PR #68 `master.c` + regression: PASS (`bf012-scratch-clear.log`). Not a committed fix.

## Out of scope for this repair

BF-001–BF-008, FP-001, deferred F02–F12 non-FAIL items, Arp/Fx sole-owner leftovers, production `main`, force-push `nightly`, device flash.
