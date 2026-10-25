# Workflow handoff to stage 7

Receiving stage: **7 Publish nightly** (new cloud thread). Stage 6 is a local install prompt only (no cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f02-stage5-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-5 owner: `bc-94bdcfbc-3579-555b-8b14-0f4431db7cd7`.
Audited SHA: `13d32e1dee37b6fb5a1e339aff499a75eecb97b5`.
Validated **source** SHA (publish / next audit / install commit): `4ee8f37714b0892f2c70d157fda9328da393e987`.
Content tree: `7ad3a50da8abac890215dae3c28197a2fa90041b8615e828b6b3c6e18060fb83`.
Evidence-bearing tip: this stage-5 branch tip (records/logs/handoff only). Do **not** treat it as the install or nightly FF target.
Repair commit: `765695f`.
Package evidence: `evidence/f02-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Merge evidence: `evidence/f02-stage5/CHECKS.md`.
Host test evidence: `evidence/f02-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f02-stage2/DEFERRED.md`.
Stage-6 local prompt: `evidence/f02-stage5/SUCCESSOR-STAGE6.txt` (also `stage6-install-prompt.json`).
Successor prompt: `evidence/f02-stage5/SUCCESSOR-STAGE7.txt`.
Successor agent id: `bc-aaf1fbac-fbdf-50d7-8f6f-a71e7bc55c3b`.
Successor URL: https://cursor.com/agents/bc-aaf1fbac-fbdf-50d7-8f6f-a71e7bc55c3b
Successor source: `desktop` (CreateAgent). Name: `F02 stage7 publish`. `startingRef`: `cursor/f02-stage5-fb90`.
Handoff status: **complete**. Owner-visible Stage 7 is recorded on this remote tip. Stage 5 ownership ends.

## Nightly

Firmware `nightly` fast-forward to validated source SHA `4ee8f377…` was **REFUSED** (non-fast-forward vs `58e2c52…`). No force-push. Stage 7 still publishes from the validated source SHA and package hashes below.

## Packages (DEV CHECKED)

- Elite: `x0x-smk37-elite.fwsc` sha256 `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8324
- Pro: `x0x-smk37.fwsc` sha256 `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8324

## Scope for stage 7

Publish the valid Elite and Pro development packages for validated source SHA `4ee8f377…` to the nightly installer page. Leave production `main` and production GitHub Pages unchanged. Do not wait for stage 6. After the live page matches, start exactly one owner-visible stage-1 Cloud Agent under the handoff rule.
