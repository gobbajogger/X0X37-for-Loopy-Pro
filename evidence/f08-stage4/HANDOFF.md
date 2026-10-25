# Workflow handoff to stage 5

Receiving stage: **5 Merge** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f08-bf004-stage4-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-4 owner: `bc-f37b5cb8-29ed-5ce4-b37a-e2398a39c492`.
Audited / validated source SHA (pre-BF-004): `6df0f81fbbe0adc3207f67d912d2fe0ab676be04`.
Validated tip: stage-4 tip on `cursor/f08-bf004-stage4-fb90` that includes this handoff and DEV CHECKED Elite/Pro records for commit `8a74ff52cba842c3f2d8365600286f94e6bccdf0`.
BF-004 repair commit: `ce690772` (engine blob `ab6d9667…`).
Package validation commit: `8a74ff52cba842c3f2d8365600286f94e6bccdf0`.
Content tree at validation commit (non-`.md`): `1a1e7a22d136fef6e30702c9d3207d22798a6dd382717acc5ab0f40d2f235acf`.
Firmware tree: `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001).
Package evidence: `evidence/f08-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Host test evidence: `evidence/f08-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f08-stage2/DEFERRED.md`.
Successor prompt: `evidence/f08-stage4/SUCCESSOR-STAGE5.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 5 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required.

## Package summary

Elite and Pro development packages (`X0X_DEV_PADCAL=1`) are **DEV CHECKED**. Pool free 8324 B (>= 8192). Firmware tree changed for BF-004; package SHA-256 differ from F07 attested bytes. Controller SHA-256 unchanged. Host PASS alone was not treated as a package PASS.

- Elite: `x0x-smk37-elite.fwsc` sha256 `aab3cd8e245dedaf561cd973b3ce05aac33eb84ba0077b9b81cb1d9aa3b260b4`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; identity SMK-37 Elite_900
- Pro: `x0x-smk37.fwsc` sha256 `066e2b1aeb6ed4abc8b3fba48606eaec1b42aa029a29ce07ffb0049ad7064a1e`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; identity SMK-37 Pro_900

## Scope for stage 5

Fast-forward firmware `nightly` to the validated tip when DEV CHECKED evidence applies. Save the local stage-6 install prompt with package and controller hashes. Do not flash. Do not implement deferred items. Do not merge production `main` or publish production Pages. Start exactly one owner-visible stage-7 Cloud Agent under the handoff rule after the successor prompt is on the remote.
