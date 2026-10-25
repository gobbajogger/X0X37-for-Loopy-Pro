# Workflow handoff to stage 5

Receiving stage: **5 Merge** (new cloud thread; F09 after stage-4 BF-005 validate).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f09-bf005-stage4-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-4 owner: `bc-bb88a201-7b44-59ef-84e9-1f920f04e260`.
Audited / validated source SHA (pre-BF-005): `8a74ff52cba842c3f2d8365600286f94e6bccdf0`.
Validated tip: stage-4 tip on `cursor/f09-bf005-stage4-fb90` that includes this handoff and DEV CHECKED Elite/Pro records for commit `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`.
BF-005 repair commit: `94f4a012` (sequencer blob `fd3f6530…`).
Package validation commit: `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`.
Content tree at validation commit (non-`.md`): `62339630bc6e133da464eed7924645c0bfc09e4de66a8bccafdc60fd2d660c83`.
Firmware tree: `32a36f68d1ce7a3c304b447475afe7e2bbb116cf`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004).
Package evidence: `evidence/f09-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Host test evidence: `evidence/f09-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f09-stage2/DEFERRED.md`.
Successor prompt: `evidence/f09-stage4/SUCCESSOR-STAGE5.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 5 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required. Stage 4 retains ownership.

## Package summary

Elite and Pro development packages (`X0X_DEV_PADCAL=1`) are **DEV CHECKED**. Pool free 8324 B (>= 8192). Firmware tree changed for BF-005; package SHA-256 differ from F08 attested bytes. Controller SHA-256 unchanged. Host PASS alone was not treated as a package PASS.

- Elite: `x0x-smk37-elite.fwsc` sha256 `de0bb0773c0b6de17140b180f79ae0e157c1690fcc485e6ffba28ae299eebc77`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; identity SMK-37 Elite_900
- Pro: `x0x-smk37.fwsc` sha256 `25bebb261432e56b20f24b2100e05c369b9d3e8c277811ac300bae1050bd5e40`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; identity SMK-37 Pro_900

## Scope for stage 5

Fast-forward firmware `nightly` to the validated tip when DEV CHECKED evidence applies. Save the local stage-6 install prompt with package and controller hashes. Do not flash. Do not implement deferred items. Do not merge production `main` or publish production Pages. Start exactly one owner-visible stage-7 Cloud Agent under the handoff rule after the successor prompt is on the remote.
