# Workflow handoff to stage 7

Receiving stage: **7 Publish nightly** (new cloud thread). Stage 6 is a local install prompt only (no cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f15-bf034-stage5-9e97` (rebased onto post-#336/#337 tip `cb676e85`).
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-5 owner: `bc-0d5534ed-7263-5bd3-9495-f40a0f179e97`.
Audited / validated source SHA: `092589b84fd455355216cb0de93c4782236f7524`.
Package validation commit: `d16cdb678c2328406473a82494e91d8bd23e450e`.
Content tree at validation: `be4039c0887a4e49e9baac55e216edf458c1815c9f4108f2af4986a8c4447c67`.
Firmware tree (packages): `a5ba08046700be5cdf2751b7afc1197411b24121`.
BF-034 repair: `7fb4b71d` (already on nightly).
Stage-5 disposition: **Safe Merge / ALREADY_ON_NIGHTLY** (FF refused; no force-push).
Package evidence: `evidence/f15-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Merge evidence: `evidence/f15-stage5/CHECKS.md`.
Host tip proof: `evidence/f15-stage5/host-bf034-green.log`.
Stage-6 local prompt: `evidence/f15-stage5/SUCCESSOR-STAGE6.txt` (also `stage6-install-prompt.json`).
Successor prompt: `evidence/f15-stage5/SUCCESSOR-STAGE7.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Tip-wait Stage-1 already live (not this handoff's successor): `bc-deca3b7b-4de7-5203-b277-bd4f80284629` — do not CreateAgent another Stage-1 from Stage 5.
Handoff status: **incomplete** until an owner-visible Stage 7 id is recorded on this remote tip.

## Nightly

Direct FF of validated / Stage-4 tips **REFUSED**. Safe Merge evidence PR lands onto current tip without firmware rewind. Stage 7 still publishes from validated package hashes below.

## Packages (DEV CHECKED)

- Elite: `x0x-smk37-elite.fwsc` sha256 `9c1168cf83d298e02863441df63f09a0508c00e8a908f04dbae79d532dfdd1e5`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8308; identity SMK-37 Elite_900
- Pro: `x0x-smk37.fwsc` sha256 `3a29723eb62024eccd7b010d97912e21eabad4ec1a933b90b428ad8f63e87a07`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8308; identity SMK-37 Pro_900

## Scope for stage 7

Publish or confirm the valid Elite and Pro development packages for validated source SHA `092589b8…` / Stage-4 hashes on the nightly installer page. Leave production `main` and production GitHub Pages unchanged. Do not wait for stage 6. After the live page matches, start exactly one owner-visible stage-1 Cloud Agent under the handoff rule.
