# Workflow handoff to stage 5

Receiving stage: **5 Merge** (new cloud thread; F12 after stage-4 BF-008 validate).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f12-stage4-bf008-1207`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-4 owner: `bc-eacdb970-e51c-553e-b0f8-fd49e9201626`.
Audited / validated source SHA (pre-BF-008): `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`.
Validated tip: stage-4 tip on `cursor/f12-stage4-bf008-1207` that includes this handoff and DEV CHECKED Elite/Pro records for commit `de11b6d7e3ac3322d2db689675d9ed27ed667785`.
BF-008 repair commit: `de11b6d7` (`fastmath.h` blob `a5c26c95…`).
Package validation commit: `de11b6d7e3ac3322d2db689675d9ed27ed667785`.
Content tree at validation commit (non-`.md`): `43e97f34ca00d32570c831ea4ec2bf56f697341cf8881018476cc67c9d76d5a9`.
Firmware tree: `9195dce5c4beb5fa8aa564504c8d1d114c8264cd`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007).
Package evidence: `evidence/f12-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Host test evidence: `evidence/f12-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f12-stage2/DEFERRED.md`.
Successor prompt: `evidence/f12-stage4/SUCCESSOR-STAGE5.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 5 id is recorded on this remote tip.

## Package summary

Elite and Pro development packages (`X0X_DEV_PADCAL=1`) are **DEV CHECKED**. Pool free 8324 B (>= 8192). Firmware tree changed for BF-008; package SHA-256 match F11 blank-slot bytes (defined unsigned cast yields identical Clang 4.0.1 object code). Controller SHA-256 unchanged. Host PASS alone was not treated as a package PASS.

- Elite: `x0x-smk37-elite.fwsc` sha256 `829bb6da6eb0042af0731a236d6238e50cc13581ce059ebbe02882534a94b056`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; identity SMK-37 Elite_900
- Pro: `x0x-smk37.fwsc` sha256 `d2186cb0f3fe74376651cb2804973c8255780532a8cd54c33d2bac09aeda60ec`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; identity SMK-37 Pro_900

## Scope for stage 5

Fast-forward firmware `nightly` to the validated tip when DEV CHECKED evidence applies. Save the local stage-6 install prompt with package and controller hashes. Do not flash from the cloud thread (owner advance-approved flash applies via Stage 6 local path). Do not implement deferred items. Do not merge production `main` or publish production Pages. Start exactly one owner-visible stage-7 Cloud Agent under the handoff rule after the successor prompt is on the remote.
