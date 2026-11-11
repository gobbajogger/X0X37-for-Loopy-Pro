# Workflow handoff to stage 7

Receiving stage: **7 Publish nightly** (new cloud thread). Stage 6 is a local install prompt only (no cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f06-bf003-stage5-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-5 owner: `bc-79dd773c-ae89-569f-8782-477f0086ede5`.
Audited SHA: `ab90f47ccad0f213114d6a98c1bc6179be2ebb35`.
Validated **source** SHA (publish / next audit / install commit): `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Content tree: `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Evidence-bearing tip: this stage-5 branch tip (records/logs/handoff only). Do **not** treat it as the install or nightly FF target.
BF-003 repair commit: `1325f5d3`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03).
Package evidence: `evidence/f06-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Merge evidence: `evidence/f06-stage5/CHECKS.md`.
Host test evidence: `evidence/f06-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f06-stage2/DEFERRED.md`.
Stage-6 local prompt: `evidence/f06-stage5/SUCCESSOR-STAGE6.txt` (also `stage6-install-prompt.json`).
Successor prompt: `evidence/f06-stage5/SUCCESSOR-STAGE7.txt`.
Successor agent id: `bc-f7783059-a0d9-5c29-a373-763ffd739822`.
Successor URL: https://cursor.com/agents/bc-f7783059-a0d9-5c29-a373-763ffd739822
Successor source: `desktop` (owner-visible; visibility `user`).
Handoff status: **complete**. Stage 5 ownership ends. Pod had no `CURSOR_API_KEY` (API create ×2 → 401); Project coordinator CreateAgent launched Stage 7 (`F06 BF-003 publish`, startingRef `cursor/f06-bf003-stage5-fb90`).

## Nightly

Firmware `nightly` fast-forward to validated source SHA `1325f5d3…` was **REFUSED** (non-fast-forward vs `08d3164a…`). No force-push. Stage 7 still publishes from the validated source SHA and package hashes below.

## Packages (DEV CHECKED)

- Elite: `x0x-smk37-elite.fwsc` sha256 `d3b4cac3d7b7be7a72732d4dd00c3120c053dbffc9137ffb7c151a5fd5355f2b`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8324; identity SMK-37 Elite_900
- Pro: `x0x-smk37.fwsc` sha256 `0b09605f357cd0accd5c5109dfb61138eb300945ffe05e6ff46942f5c5e076b9`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8324; identity SMK-37 Pro_900

## Scope for stage 7

Publish the valid Elite and Pro development packages for validated source SHA `1325f5d3…` to the nightly installer page. Leave production `main` and production GitHub Pages unchanged. Do not wait for stage 6. After the live page matches, start exactly one owner-visible stage-1 Cloud Agent under the handoff rule.
