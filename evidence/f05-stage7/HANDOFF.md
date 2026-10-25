# Workflow handoff to stage 1 (next cycle)

Receiving stage: **1 Audit** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f05-bf002-stage7-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-7 owner: `bc-1db0b238-d8dd-5769-acf7-2a9237155a9f`.
Audited SHA: `0d79d63485dc6585800a71ef716d095445f5a1fc`.
Validated **source** SHA (published / next audit): `ab90f47ccad0f213114d6a98c1bc6179be2ebb35`.
Content tree: `0ee03318a0b64a1e535b36fad284558529ff7e3f95ff069efed86bbcf308518c`.
Catalog git tree: `50c381c528cae9692deff4a712071f7dc6acb020`.
Evidence-bearing tip: this stage-7 branch tip (records/logs/handoff only). Do **not** treat it as the package source tree.
Publish evidence: `evidence/f05-stage7/CHECKS.md`, `live-verify.log`, `independent-qa-local-review.md`.
Prior package evidence: `evidence/f05-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Merge evidence: `evidence/f05-stage5/CHECKS.md` (nightly FF REFUSED; no force-push).
Host tests: `evidence/f05-stage3/CHECKS.md` (ALL_PASS; not used alone for independent-qa).
Independent QA: local-env `internal/pr-local-reviews/pr-198.md` (PASS; unchanged package bytes) — copy on tip.
Deferred (next defect = oldest): `evidence/f05-stage2/DEFERRED.md`.
Successor prompt: `evidence/f05-stage7/SUCCESSOR-STAGE1.txt`.
Successor agent id: `bc-a76a9628-9a50-5b04-a7a4-ba134f41d9e1`.
Successor URL: https://cursor.com/agents/bc-a76a9628-9a50-5b04-a7a4-ba134f41d9e1
Successor name: `F06 stage1 audit`.
Successor source: `desktop` (owner-visible; visibility `user`).
Handoff status: **complete**. Stage 7 ownership ends. Pod had no `CURSOR_API_KEY`; Project coordinator CreateAgent launched Stage 1 (`startingRef` `cursor/f05-bf002-stage7-fb90`).

## Live nightly page (confirmed)

- Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/
- `latest`: `nightly-ab90f47c-dev`
- Label: `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested`: false
- Elite bundled sha256: `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278`
- Pro bundled sha256: `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb`
- Installer FP: `2240be8702176544945a4b66171269e958b47259fb275654d2f6ff7125f637ee`
- Live marker: `d11b74fc` / BF-001 / `nightly-ab90f47c-dev`
- Pages tip: `e10823e38ec9e56f5ec381822b1597f7364a95d9`
- Mac: `bc-c2a20c4f-164b-5375-b042-99a914c47c5f`

## Scope for stage 1

Audit validated source SHA `ab90f47c…` and the published page hashes. Take the oldest deferred defect not already fixed in that SHA (BF-001 and BF-002 already fixed). Start exactly one owner-visible successor under the handoff rule after the audit is recorded. Local host checks only — no GitHub Actions / Codex-on-GitHub.
