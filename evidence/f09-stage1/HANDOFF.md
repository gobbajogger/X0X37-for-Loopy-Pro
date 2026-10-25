# Workflow handoff to stage 2

Receiving stage: **2 Implement** (new cloud thread; F09 after F08 nightly publish).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f09-bf005-stage1-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-1 owner: `bc-2ac9852a-4eac-553f-906c-d7ecf1a4d46e`.
Validated **source** SHA (audit / repair base): `8a74ff52cba842c3f2d8365600286f94e6bccdf0`.
Content tree: `1a1e7a22d136fef6e30702c9d3207d22798a6dd382717acc5ab0f40d2f235acf`.
Git source_tree: `33d6950b0a378525f2a8b592672275625893239f`.
Firmware tree: `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f`.
Selected defect: **BF-005** — `seq_clk_drop` clears `clk_lock` not `ext_pending` (stale clocks accelerate new source).
Definition: `evidence/f09-stage1/bf005-definition.md` (from PR #26 / `docs/audit/bugfinder-clock-handover-20261023.md`).
Tip `sequencer.c` blob: `83b03f73139d8e679c94526c66b390b6e5bc2f39`.
Live: **LIVE_PAGE_MATCH** `nightly-8a74ff52-dev` (Pages tip `e296e883…`); packages unchanged by this audit.
Evidence: `evidence/f09-stage1/AUDIT.md`, `CHECKS.md`, `live-verify.log`, `bf005-witness.log`.
Successor prompt: `evidence/f09-stage1/SUCCESSOR-STAGE2.txt`.
Successor agent id: none (pending coordinator CreateAgent — pod has no `CURSOR_API_KEY`).
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 2 id is recorded on this remote tip.

## Scope for stage 2

Implement only BF-005 on a new `cursor/<slug>-stage2-fb90` branch from the pushed tip of `cursor/f09-bf005-stage1-fb90`. Clear `ext_pending` when owner-matching `seq_clk_drop` clears `clk_lock`; add host regressions; use `X0X_DEV_PADCAL=1`. Do not reopen BF-001–BF-004 or FP-001. Do not take BF-006/BF-007. Do not flash. Leave production unchanged.

Deferred younger / excluded items: withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, delete Felucca-era `make_site.py` entirely, nightly-repo second qualification field, historical F06 Stage-7 “Installer FP” wording rewrite (`evidence/f08-stage2/DEFERRED.md`); younger BF-006/BF-007.
