# Workflow handoff to stage 5

Receiving stage: **5 Merge** (new cloud thread; F11 after stage-4 BF-007 validate).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f11-bf007-stage4-4dfc`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-4 owner: `bc-283a6d11-1c65-5746-9fd3-3d5878784dfc`.
Audited / validated source SHA (pre-BF-007): `af4947846df7233c68a6be72c4d0c634052a2752`.
Validated tip: stage-4 tip on `cursor/f11-bf007-stage4-4dfc` that includes this handoff and DEV CHECKED Elite/Pro records for commit `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`.
BF-007 repair commit: `78a0452e` (`project.c` blob `240a4b17…`).
Package validation commit: `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`.
Content tree at validation commit (non-`.md`): `f27c89e1ac8e11ab57c5ae7e49145ce19b964082bf3ca9a700a99bfa8a97091e`.
Firmware tree: `5b1a21f07563b0262ece6728051cb0436bd4bfa1`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006).
Package evidence: `evidence/f11-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Host test evidence: `evidence/f11-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f11-stage2/DEFERRED.md`.
Successor prompt: `evidence/f11-stage4/SUCCESSOR-STAGE5.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 5 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY` (API create expected → 401); coordinator CreateAgent is required.

## Package summary

Elite and Pro development packages (`X0X_DEV_PADCAL=1`) are **DEV CHECKED**. Pool free 8324 B (>= 8192). Firmware tree changed for BF-007; package SHA-256 differ from F10 as expected. Controller SHA-256 unchanged. Host PASS alone was not treated as a package PASS.

- Elite: `x0x-smk37-elite.fwsc` sha256 `829bb6da6eb0042af0731a236d6238e50cc13581ce059ebbe02882534a94b056`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; identity SMK-37 Elite_900
- Pro: `x0x-smk37.fwsc` sha256 `d2186cb0f3fe74376651cb2804973c8255780532a8cd54c33d2bac09aeda60ec`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; identity SMK-37 Pro_900

## Scope for stage 5

Fast-forward firmware `nightly` to the validated tip when DEV CHECKED evidence applies. Save the local stage-6 install prompt with package and controller hashes. Do not flash from the cloud thread (owner advance-approved flash applies via Stage 6 local path). Do not implement deferred items. Do not merge production `main` or publish production Pages. Start exactly one owner-visible stage-7 Cloud Agent under the handoff rule after the successor prompt is on the remote.
