# Workflow handoff to stage 5

Receiving stage: **5 Merge** (new cloud thread; F14 after stage-4 BF-013 validate).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f14-bf013-stage4-237f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-4 owner: `bc-3789dd85-75ee-53a1-9be7-ddf256b127a9`.
Stage-3 owner: `bc-ffda0814-f56c-5ba4-bf4c-d31ecf7a4cf1`.
Stage-2 owner: `bc-129a2454-b858-5423-9331-22fb792ae11f`.
Stage-1 owner: `bc-4c124351-a5c1-53ea-a243-b3b478f7bc15`.
Audited / validated source SHA: `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`.
Validated tip: stage-4 tip on `cursor/f14-bf013-stage4-237f` that includes this handoff and DEV CHECKED Elite/Pro records for commit `656d70e9f0dbcffb346c59112d6ae582f2d93fe7`.
BF-013 repair commit: `13b831e8` (`fxbus.c` blob `49a89483…`, `stereo_test.c` blob `c8bc1092…`).
Package validation commit: `656d70e9f0dbcffb346c59112d6ae582f2d93fe7`.
Content tree at validation commit: `1ce1c2155db1e1e90d47284899db8726e1686b2aa86891f750e709503cbd1407`.
Firmware tree: `3b10e0c6a5954846a354b1dda5d457828b0a7731`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008), `6636cca1`/`b42bcba4` (BF-012).
Package evidence: `evidence/f14-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Host test evidence: `evidence/f14-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f14-stage2/DEFERRED.md` / `evidence/f14-stage3/DEFERRED.md`.
Successor prompt: `evidence/f14-stage4/SUCCESSOR-STAGE5.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 5 id is recorded on this remote tip. CreateAgent attempts recorded as `api-create-stage5-skip.json` / `api-create-stage5-note.txt` (`NO_CURSOR_API_KEY`). Do not claim a Task subagent as Stage 5. Stage 4 keeps ownership until the id is pushed.

## Package summary

Elite and Pro development packages (`X0X_DEV_PADCAL=1`) are **DEV CHECKED**. Pool free 8324 B (>= 8192). Firmware tree changed for BF-013; package SHA-256 differ from the live nightly as expected. Controller SHA-256 unchanged. Host PASS alone was not treated as a package PASS.

- Elite: `x0x-smk37-elite.fwsc` sha256 `af5cd709fb9ad235cb814b667e0eb5e99e01f9ced3d427c17a1704f628345247`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; identity SMK-37 Elite_900
- Pro: `x0x-smk37.fwsc` sha256 `eba70777632b959a339d2f8a326a3d874c938627aeef96648b83d52091379161`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; identity SMK-37 Pro_900

## Scope for stage 5

Fast-forward firmware `nightly` to the validated tip when DEV CHECKED evidence applies. Save the local stage-6 install prompt with package and controller hashes. Do not flash from the cloud thread. Do not implement deferred items. Do not reopen BF-001–BF-008, BF-012, or FP-001. Do not steal Arp/Fx #279/#281 leftovers. Do not merge production `main` or publish production Pages. Start exactly one owner-visible stage-7 Cloud Agent under the handoff rule after the successor prompt is on the remote.
