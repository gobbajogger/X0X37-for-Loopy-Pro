# Workflow handoff to stage 7

Receiving stage: **7 Publish nightly** (new cloud thread). Stage 6 is a local install prompt only (no cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f03-slot-probe-stage5-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-5 owner: `bc-612598eb-d3fa-5382-8448-3bd7e18b0901`.
Audited SHA: `4ee8f37714b0892f2c70d157fda9328da393e987`.
Validated **source** SHA (publish / next audit / install commit): `a03cfab3a423ab5c65a6546afd4993fe51754ee7`.
Content tree: `1755a2dd6d8b79e7405ab58919cc4d60a69ca11a4ae6601030b0f5de963f170e`.
Evidence-bearing tip: this stage-5 branch tip (records/logs/handoff only). Do **not** treat it as the install or nightly FF target.
Repair commit: `4f18983a`.
Package evidence: `evidence/f03-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Merge evidence: `evidence/f03-stage5/CHECKS.md`.
Host test evidence: `evidence/f03-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f03-stage2/DEFERRED.md`.
Stage-6 local prompt: `evidence/f03-stage5/SUCCESSOR-STAGE6.txt` (also `stage6-install-prompt.json`).
Successor prompt: `evidence/f03-stage5/SUCCESSOR-STAGE7.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 7 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required.

## Nightly

Firmware `nightly` fast-forward to validated source SHA `a03cfab3…` was **REFUSED** (non-fast-forward vs `08d3164a…`). No force-push. Stage 7 still publishes from the validated source SHA and package hashes below.

## Packages (DEV CHECKED)

- Elite: `x0x-smk37-elite.fwsc` sha256 `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8324
- Pro: `x0x-smk37.fwsc` sha256 `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8324

## Scope for stage 7

Publish the valid Elite and Pro development packages for validated source SHA `a03cfab3…` to the nightly installer page. Leave production `main` and production GitHub Pages unchanged. Do not wait for stage 6. After the live page matches, start exactly one owner-visible stage-1 Cloud Agent under the handoff rule.
