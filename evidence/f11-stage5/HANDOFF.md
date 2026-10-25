# Workflow handoff to stage 7

Receiving stage: **7 Publish nightly** (new cloud thread). Stage 6 is a local install prompt only (no cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f11-bf007-stage5-0e72`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-5 owner: `bc-432a157b-5c8e-5b0e-8adf-47cddc020e72`.
Audited / validated source SHA (pre-BF-007): `af4947846df7233c68a6be72c4d0c634052a2752`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Validated **source** SHA (publish / next audit / install commit): `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`.
Content tree: `f27c89e1ac8e11ab57c5ae7e49145ce19b964082bf3ca9a700a99bfa8a97091e`.
Firmware tree: `5b1a21f07563b0262ece6728051cb0436bd4bfa1`.
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Evidence-bearing tip: this stage-5 branch tip (records/logs/handoff only). Do **not** treat it as the install or nightly FF target.
BF-007 repair commit: `78a0452e`.
Package validation commit: `4f96b0d9`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007).
Package evidence: `evidence/f11-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Merge evidence: `evidence/f11-stage5/CHECKS.md`.
Host test evidence: `evidence/f11-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f11-stage2/DEFERRED.md`.
Stage-6 local prompt: `evidence/f11-stage5/SUCCESSOR-STAGE6.txt` (also `stage6-install-prompt.json`).
Successor prompt: `evidence/f11-stage5/SUCCESSOR-STAGE7.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 7 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY` (API create → `NO_CURSOR_API_KEY`); coordinator CreateAgent is required.

## Nightly

Firmware `nightly` fast-forward to validated source SHA `4f96b0d9…` was **REFUSED** (non-fast-forward vs `08d3164a…`). No force-push. Stage 7 still publishes from the validated source SHA and package hashes below.

## Packages (DEV CHECKED)

- Elite: `x0x-smk37-elite.fwsc` sha256 `829bb6da6eb0042af0731a236d6238e50cc13581ce059ebbe02882534a94b056`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8324; identity SMK-37 Elite_900
- Pro: `x0x-smk37.fwsc` sha256 `d2186cb0f3fe74376651cb2804973c8255780532a8cd54c33d2bac09aeda60ec`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8324; identity SMK-37 Pro_900

## Scope for stage 7

Publish the valid Elite and Pro development packages for validated source SHA `4f96b0d9…` to the nightly installer page. Leave production `main` and production GitHub Pages unchanged. Do not wait for stage 6. After the live page matches, start exactly one owner-visible stage-1 Cloud Agent under the handoff rule.
