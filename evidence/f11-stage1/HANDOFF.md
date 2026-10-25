# Workflow handoff to stage 2

Receiving stage: **2 Implement** (new cloud thread; F11 after F10 nightly publish).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f11-bf007-stage1-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-1 owner: `bc-1751adc0-5254-5401-a54e-48e433f242b9`.
Validated **source** SHA (audit baseline; packages authenticate this): `af4947846df7233c68a6be72c4d0c634052a2752`.
Content tree: `1da3bbfbbf0df52256acceecbca2e064a1ef821478a33d78591e01813229643d`.
Git source_tree: `c12a7e747b79aa4619466764ecccc88285d4c10d`.
Firmware tree: `bcc21733ea77097a787cfa6cc617fbdf1dca5da1`.
BF-006 already in tip: `37bab7d1` (drum808 blob `90f46329…`) — do not reopen.
Selected defect: **BF-007** — legacy BD PAN motion migration order in `firmware/src/app/project.c` blob `1000a0d8…`.
Evidence: `evidence/f11-stage1/AUDIT.md`, `CHECKS.md`, `live-verify.log`, `bf007-definition.md`, `bf007-witness.log`, `bf007-scratch-clear.log`.
Independent claim review: `bc-b123ad99-27b7-5044-b4c6-efd6eccf2b4e` (8/8 PASS).
Stage-7 local review cite: Project store `internal/pr-local-reviews/pr-256.md`.
Package local review cite: Project store `internal/pr-local-reviews/pr-253.md` PASS.
Successor prompt: `evidence/f11-stage1/SUCCESSOR-STAGE2.txt`.
Successor agent id: none (pending CreateAgent — pod has no `CURSOR_API_KEY`; ask Project coordinator).
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 2 id is recorded on this remote tip.

## Scope for stage 2

Implement only BF-007 on a new `cursor/<slug>-stage2-fb90` branch from the pushed tip of `cursor/f11-bf007-stage1-fb90`. Migrate BD PAN motion lane indices only after arrangement/lanes load (scratch: move `sound_bd_sends` calls after stream/arrange load); add host regressions; use `X0X_DEV_PADCAL=1`. Do not reopen BF-001–BF-006 or FP-001. Do not fold PR #28. Do not flash. Leave production unchanged.

Deferred / excluded items: withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, delete Felucca-era `make_site.py` entirely, nightly-repo second qualification field, historical F06 Stage-7 “Installer FP” wording rewrite, open PR #28 fastmath (`evidence/f10-stage2/DEFERRED.md`).
