# Workflow handoff to stage 2

Receiving stage: **2 Implement** (new cloud thread; F10 after F09 nightly publish).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f10-bf006-stage1-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-1 owner: `bc-6b7eb25f-60be-5d9e-8fa7-d8b9111b5e63`.
Validated **source** SHA (audit baseline; packages authenticate this): `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`.
Content tree: `62339630bc6e133da464eed7924645c0bfc09e4de66a8bccafdc60fd2d660c83`.
Git source_tree: `e06299abf0f35f24c771d011d07c2e74de613369`.
Firmware tree: `32a36f68d1ce7a3c304b447475afe7e2bbb116cf`.
BF-005 already in tip: `94f4a012` (sequencer blob `fd3f6530…`) — do not reopen.
Selected defect: **BF-006** — `d8_exp2_cr` signed left-shift UB in `firmware/src/dsp/drum808.c` blob `4b873a79…`.
Evidence: `evidence/f10-stage1/AUDIT.md`, `CHECKS.md`, `live-verify.log`, `bf006-definition.md`, `bf006-witness.log`, `bf006-scratch-clear.log`.
Independent claim review: `bc-a4b5bfab-9e69-5adc-9677-9d2dfe23e20a` (12/12 PASS).
Stage-7 local review cite: Project store `internal/pr-local-reviews/pr-245.md` PASS.
Successor prompt: `evidence/f10-stage1/SUCCESSOR-STAGE2.txt`.
Successor agent id: none (pending CreateAgent — pod has no `CURSOR_API_KEY`; ask Project coordinator).
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 2 id is recorded on this remote tip.

## Scope for stage 2

Implement only BF-006 on a new `cursor/<slug>-stage2-fb90` branch from the pushed tip of `cursor/f10-bf006-stage1-fb90`. Replace the signed left shift in `d8_exp2_cr` with a defined form (scratch: `b.u += (uint32_t)(int32_t)n << 23;`); add host regressions; use `X0X_DEV_PADCAL=1`. Do not reopen BF-001–BF-005 or FP-001. Do not take BF-007. Do not flash. Leave production unchanged.

Deferred younger / excluded items: withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, delete Felucca-era `make_site.py` entirely, nightly-repo second qualification field, historical F06 Stage-7 “Installer FP” wording rewrite (`evidence/f09-stage2/DEFERRED.md`); younger BF-007.
