# Workflow handoff to stage 7

Receiving stage: **7 Publish nightly** (new cloud thread). Stage 6 is a local install prompt only (no cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f12-stage5-bf008-1207`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-5 owner: `bc-2ced8972-1537-5ad0-91cf-84177aa15c17`.
Audited / validated source SHA (pre-BF-008): `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Validated **source** SHA (publish / next audit / install commit): `de11b6d7e3ac3322d2db689675d9ed27ed667785`.
Content tree: `43e97f34ca00d32570c831ea4ec2bf56f697341cf8881018476cc67c9d76d5a9`.
Firmware tree: `9195dce5c4beb5fa8aa564504c8d1d114c8264cd`.
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Evidence-bearing tip: this stage-5 branch tip (records/logs/handoff only). Do **not** treat it as the install or nightly FF target.
BF-008 repair commit: `de11b6d7`.
Package validation commit: `de11b6d7`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008).
Package evidence: `evidence/f12-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Merge evidence: `evidence/f12-stage5/CHECKS.md`.
Host test evidence: `evidence/f12-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f12-stage2/DEFERRED.md`.
Stage-6 local prompt: `evidence/f12-stage5/SUCCESSOR-STAGE6.txt` (also `stage6-install-prompt.json`).
Successor prompt: `evidence/f12-stage5/SUCCESSOR-STAGE7.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 7 id is recorded on this remote tip.

## Nightly

Firmware `nightly` fast-forward to validated source SHA `de11b6d7…` was **REFUSED** (non-fast-forward vs `08d3164a…`). No force-push. Stage 7 still publishes from the validated source SHA and package hashes below.

## Packages (DEV CHECKED)

- Elite: `x0x-smk37-elite.fwsc` sha256 `829bb6da6eb0042af0731a236d6238e50cc13581ce059ebbe02882534a94b056`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8324; identity SMK-37 Elite_900
- Pro: `x0x-smk37.fwsc` sha256 `d2186cb0f3fe74376651cb2804973c8255780532a8cd54c33d2bac09aeda60ec`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8324; identity SMK-37 Pro_900

## Scope for stage 7

Publish the valid Elite and Pro development packages for validated source SHA `de11b6d7…` to the nightly installer page. Leave production `main` and production GitHub Pages unchanged. Do not wait for stage 6. After the live page matches, start exactly one owner-visible stage-1 Cloud Agent under the handoff rule.
