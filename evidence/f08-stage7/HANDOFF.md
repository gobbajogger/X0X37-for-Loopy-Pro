# Workflow handoff to stage 1

Receiving stage: **1 Audit** (new cloud thread; next cycle after F08 publish).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f08-bf004-stage7-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-7 owner: `bc-bcce5c32-d708-521d-8a09-7d4547fb5ca5`.
Validated **source** SHA (published / next audit): `8a74ff52cba842c3f2d8365600286f94e6bccdf0`.
Content tree: `1a1e7a22d136fef6e30702c9d3207d22798a6dd382717acc5ab0f40d2f235acf`.
Git source_tree: `33d6950b0a378525f2a8b592672275625893239f`.
Firmware tree: `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f`.
BF-004 repair commit: `ce690772`.
Live: **LIVE_PAGE_MATCH** `nightly-8a74ff52-dev` (Pages tip `e296e883…`).
Elite page sha256: `d7767bebf1413b5d6fb918ba3893335a7b9fae86078ce895aa3b72df48bedb93`.
Pro page sha256: `b0cecbcff42589cdc71231a16098b7cfc8301a8562aba25362d552b3dcdc5fa0`.
Blank Elite/Pro: `aab3cd8e…` / `066e2b1a…`.
Label: `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested` false.
Evidence: `evidence/f08-stage7/CHECKS.md`, `live-verify.log`, `independent-qa-local-review.md`.
Successor prompt: `evidence/f08-stage7/SUCCESSOR-STAGE1.txt`.
Successor agent id: none (pending coordinator CreateAgent — pod has no `CURSOR_API_KEY`).
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 1 id is recorded on this remote tip.

## Scope for stage 1

Audit validated source SHA `8a74ff52…` against the published nightly page hashes. Select the next defect per workflow (oldest deferred not fixed in that SHA, then failing check, then oldest open finding). Do not reopen BF-001–BF-004 or FP-001. Do not flash. Leave production unchanged.
