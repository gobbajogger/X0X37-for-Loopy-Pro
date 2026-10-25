# Workflow handoff to stage 7

Receiving stage: **7 Publish nightly** (new cloud thread). Stage 6 is a local install prompt only (no cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f14-bf013-stage5-237f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-5 owner: `bc-b08d7670-f066-5496-8972-62767b638eb4`.
Stage-4 owner: `bc-3789dd85-75ee-53a1-9be7-ddf256b127a9`.
Stage-3 owner: `bc-ffda0814-f56c-5ba4-bf4c-d31ecf7a4cf1`.
Stage-2 owner: `bc-129a2454-b858-5423-9331-22fb792ae11f`.
Stage-1 owner: `bc-4c124351-a5c1-53ea-a243-b3b478f7bc15`.
Audited / validated source SHA (pre-BF-013): `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`.
Validated **source** SHA (publish / next audit / install commit): `656d70e9f0dbcffb346c59112d6ae582f2d93fe7`.
Content tree: `1ce1c2155db1e1e90d47284899db8726e1686b2aa86891f750e709503cbd1407`.
Firmware tree: `3b10e0c6a5954846a354b1dda5d457828b0a7731`.
Pre-repair content tree: `a15afc439fc8b7bccd62d50403b925add45ae9e5`.
Pre-repair firmware tree: `5facb3b4405d119a269a05224b9268adfa1baa13`.
Evidence-bearing tip: this stage-5 branch tip (records/logs/handoff only). Do **not** treat it as the install or nightly FF target.
BF-013 repair commit: `13b831e8`.
Package validation commit: `656d70e9`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008), `6636cca1`/`b42bcba4` (BF-012).
Package evidence: `evidence/f14-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Merge evidence: `evidence/f14-stage5/CHECKS.md`.
Host test evidence: `evidence/f14-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f14-stage2/DEFERRED.md` / `evidence/f14-stage3/DEFERRED.md`.
Stage-6 local prompt: `evidence/f14-stage5/SUCCESSOR-STAGE6.txt` (also `stage6-install-prompt.json`).
Successor prompt: `evidence/f14-stage5/SUCCESSOR-STAGE7.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 7 id is recorded on this remote tip. CreateAgent attempts recorded as `api-create-stage7-attempt{1,2}.json` (`NO_CURSOR_API_KEY`). Do not claim a Task subagent as Stage 7. Stage 5 keeps ownership until the id is pushed.

## Nightly

Firmware `nightly` fast-forward to validated source SHA `656d70e9…` was **REFUSED** (non-fast-forward vs `811d7dd2…`). No force-push. Stage 7 still publishes from the validated source SHA and package hashes below.

## Packages (DEV CHECKED)

- Elite: `x0x-smk37-elite.fwsc` sha256 `af5cd709fb9ad235cb814b667e0eb5e99e01f9ced3d427c17a1704f628345247`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8324; identity SMK-37 Elite_900
- Pro: `x0x-smk37.fwsc` sha256 `eba70777632b959a339d2f8a326a3d874c938627aeef96648b83d52091379161`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8324; identity SMK-37 Pro_900

## Scope for stage 7

Publish the valid Elite and Pro development packages for validated source SHA `656d70e9…` to the nightly installer page. Leave production `main` and production GitHub Pages unchanged. Do not wait for stage 6. After the live page matches, start exactly one owner-visible stage-1 Cloud Agent under the handoff rule.
