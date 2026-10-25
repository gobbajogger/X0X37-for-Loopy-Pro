# Workflow handoff to stage 5

Receiving stage: **5 Merge** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f03-slot-probe-stage4-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-4 owner: `bc-16eff7cf-f278-524a-8fd3-c45a34488945`.
Audited SHA: `4ee8f37714b0892f2c70d157fda9328da393e987`.
Validated tip: stage-4 tip on `cursor/f03-slot-probe-stage4-fb90` that includes this handoff and DEV CHECKED Elite/Pro records for commit `a03cfab3a423ab5c65a6546afd4993fe51754ee7`.
Repair commit: `4f18983a`.
Package evidence: `evidence/f03-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Host test evidence: `evidence/f03-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f03-stage2/DEFERRED.md`.
Successor prompt: `evidence/f03-stage4/SUCCESSOR-STAGE5.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 5 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required.

## Package summary

Elite and Pro development packages (`X0X_DEV_PADCAL=1`) are **DEV CHECKED**. Pool free 8324 B (>= 8192). Firmware paths unchanged vs audited tip; package/controller SHA-256 match prior F02 attested bytes. Host PASS alone was not treated as a package PASS.

## Scope for stage 5

Fast-forward firmware `nightly` to the validated tip when DEV CHECKED evidence applies. Save the local stage-6 install prompt with package and controller hashes. Do not flash. Do not implement deferred items. Do not merge production `main` or publish production Pages. Start exactly one owner-visible stage-7 Cloud Agent under the handoff rule after the successor prompt is on the remote.
