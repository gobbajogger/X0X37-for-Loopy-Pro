# Workflow handoff to stage 7

Receiving stage: **7 Publish nightly** (new cloud thread). Stage 6 is a local install prompt only (no cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f13-bf012-stage5-237f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-5 owner: `bc-3275436b-e808-57c2-86c2-b8a3130dd94f`.
Stage-4 owner: `bc-952af67e-b6f6-5318-a3b3-ba4de860c800`.
Stage-3 owner: `bc-7c49b76a-97dd-57c9-9a5a-784be039d70a`.
Stage-2 owner: `bc-a0181730-1daf-5e94-826a-e4339ed37abf`.
Stage-1 owner: `bc-09959f6e-dc18-5575-baa6-444dfdf698cc`.
Audited / validated source SHA (pre-BF-012): `de11b6d7e3ac3322d2db689675d9ed27ed667785`.
Validated **source** SHA (publish / next audit / install commit): `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`.
Content tree: `519ccdf21b46db9198993587cb9334960e67ce483641c744c5caf71b1c769141`.
Firmware tree: `5facb3b4405d119a269a05224b9268adfa1baa13`.
Pre-repair content tree: `c143a9f8f5b5d0f6804995829ccbe515c77c6103`.
Pre-repair firmware tree: `9195dce5c4beb5fa8aa564504c8d1d114c8264cd`.
Evidence-bearing tip: this stage-5 branch tip (records/logs/handoff only). Do **not** treat it as the install or nightly FF target.
BF-012 repair commit: `6636cca1`.
Package validation commit: `b42bcba4`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008).
Package evidence: `evidence/f13-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Merge evidence: `evidence/f13-stage5/CHECKS.md`.
Host test evidence: `evidence/f13-stage3/CHECKS.md` (ALL_PASS).
Deferred (unchanged): `evidence/f13-stage2/DEFERRED.md` / `evidence/f13-stage3/DEFERRED.md`.
Stage-6 local prompt: `evidence/f13-stage5/SUCCESSOR-STAGE6.txt` (also `stage6-install-prompt.json`).
Successor prompt: `evidence/f13-stage5/SUCCESSOR-STAGE7.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 7 id is recorded on this remote tip. CreateAgent attempts recorded as `api-create-stage7-attempt{1,2}.json` (`NO_CURSOR_API_KEY`). Do not claim a Task subagent as Stage 7. Stage 5 keeps ownership until the id is pushed.

## Nightly

Firmware `nightly` fast-forward to validated source SHA `b42bcba4…` was **REFUSED** (non-fast-forward vs `dcd288a4…`). No force-push. Stage 7 still publishes from the validated source SHA and package hashes below.

## Packages (DEV CHECKED)

- Elite: `x0x-smk37-elite.fwsc` sha256 `d9a9ccf4698d9519316d90b36f6158101f7f1c5cd96ba50f6c1be6ef8693ebcc`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8324; identity SMK-37 Elite_900
- Pro: `x0x-smk37.fwsc` sha256 `a6a2387c712e95772ab1ff2f184f6b21357f71da08f0414f8d698b5f94309498`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8324; identity SMK-37 Pro_900

## Scope for stage 7

Publish the valid Elite and Pro development packages for validated source SHA `b42bcba4…` to the nightly installer page. Leave production `main` and production GitHub Pages unchanged. Do not wait for stage 6. After the live page matches, start exactly one owner-visible stage-1 Cloud Agent under the handoff rule.
