# Workflow handoff to stage 5

Receiving stage: **5 Merge** (new cloud thread; F10 after stage-4 BF-006 validate).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f10-bf006-stage4-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-4 owner: `bc-515ee598-6baf-5740-a8c9-bf951aa1d0ea`.
Audited / validated source SHA (pre-BF-006): `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`.
Validated tip: stage-4 tip on `cursor/f10-bf006-stage4-fb90` that includes this handoff and DEV CHECKED Elite/Pro records for commit `af4947846df7233c68a6be72c4d0c634052a2752`.
BF-006 repair commit: `37bab7d1` (drum808 blob `90f46329…`).
Package validation commit: `af4947846df7233c68a6be72c4d0c634052a2752`.
Content tree at validation commit (non-`.md`): `1da3bbfbbf0df52256acceecbca2e064a1ef821478a33d78591e01813229643d`.
Firmware tree: `bcc21733ea77097a787cfa6cc617fbdf1dca5da1`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005).
Package evidence: `evidence/f10-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Host test evidence: `evidence/f10-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f10-stage2/DEFERRED.md`.
Successor prompt: `evidence/f10-stage4/SUCCESSOR-STAGE5.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 5 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; CreateAgent attempts 1–2 recorded as `api-create-stage5-attempt{1,2}.json` (`NO_CURSOR_API_KEY`). Coordinator CreateAgent is required. Stage 4 retains ownership.

## Package summary

Elite and Pro development packages (`X0X_DEV_PADCAL=1`) are **DEV CHECKED**. Pool free 8324 B (>= 8192). Firmware tree changed for BF-006; package SHA-256 match F09 attested bytes (identical object code for the defined cast). Controller SHA-256 unchanged. Host PASS alone was not treated as a package PASS.

- Elite: `x0x-smk37-elite.fwsc` sha256 `de0bb0773c0b6de17140b180f79ae0e157c1690fcc485e6ffba28ae299eebc77`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; identity SMK-37 Elite_900
- Pro: `x0x-smk37.fwsc` sha256 `25bebb261432e56b20f24b2100e05c369b9d3e8c277811ac300bae1050bd5e40`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; identity SMK-37 Pro_900

## Scope for stage 5

Fast-forward firmware `nightly` to the validated tip when DEV CHECKED evidence applies. Save the local stage-6 install prompt with package and controller hashes. Do not flash. Do not implement deferred items. Do not merge production `main` or publish production Pages. Start exactly one owner-visible stage-7 Cloud Agent under the handoff rule after the successor prompt is on the remote.
