# Workflow handoff to stage 7

Receiving stage: **7 Publish nightly** (new cloud thread). Stage 6 is a local install prompt only (no cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f13-bf012-stage5-land-f385`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Stage-5 land worker: `bc-4b1093ae-a911-50c0-a776-f140463cf385`.
Nightly base before land: `029c3c5f63849ba71196e43902ccdf531c169843`.
Validated **source** SHA (publish / next audit / install): `092589b84fd455355216cb0de93c4782236f7524`.
Build DEV CHECKED commit (identical trees): `e4b9b28578e0f1734dd5fe2d8c39f4c7d40a3a0d`.
Content tree: `25ada1f02e2e289072fab8db464b715e09f9c0712bded561af008bc88d8c54af`.
Firmware tree: `a5ba08046700be5cdf2751b7afc1197411b24121`.
Git tree: `9382fb1276e320d9f9d290dbad187fa08cdd224c`.
Evidence-bearing tip: this stage-5 branch tip (records/logs/handoff only). Do **not** treat it as the install commit.
Land PR: [#308](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/308) MERGED (rebase).
Package evidence: `evidence/f13-stage5/elite/record.json`, `pro/record.json`, `CHECKS.md`.
Stage-6 local prompt: `evidence/f13-stage5/SUCCESSOR-STAGE6.txt` + `stage6-install-prompt.json`.
Successor prompt: `evidence/f13-stage5/SUCCESSOR-STAGE7.txt`.
Successor agent id: _pending CreateAgent_
Successor URL: _pending_
Successor source: _pending_
Handoff status: **incomplete until owner-visible Stage 7 id is pushed**.

## Nightly

Firmware `nightly` = `092589b8…` via PR #308 rebase-merge. Direct push refused by ruleset (PR-only); no force-push.

## Packages (DEV CHECKED)

- Elite: `x0x-smk37-elite.fwsc` sha256 `9c1168cf83d298e02863441df63f09a0508c00e8a908f04dbae79d532dfdd1e5`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8308; code_free 292; identity SMK-37 Elite_900
- Pro: `x0x-smk37.fwsc` sha256 `3a29723eb62024eccd7b010d97912e21eabad4ec1a933b90b428ad8f63e87a07`; `ctl.img` sha256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`; pool_free 8308; code_free 292; identity SMK-37 Pro_900

## Scope for stage 7

Publish these Elite and Pro development packages for validated source SHA `092589b8…` to the nightly installer page. Leave production `main` and production GitHub Pages unchanged. Do not wait for stage 6. After the live page matches, start exactly one owner-visible stage-1 Cloud Agent under the handoff rule.
