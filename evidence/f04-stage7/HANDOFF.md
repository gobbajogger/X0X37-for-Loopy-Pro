# Workflow handoff to stage 1 (next cycle)

Receiving stage: **1 Audit** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f04-bf001-stage7-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-7 owner: `bc-0d4e5c60-b783-50bf-aade-b445b509603c`.
Audited SHA: `a03cfab3a423ab5c65a6546afd4993fe51754ee7`.
Validated **source** SHA (published / next audit): `0d79d63485dc6585800a71ef716d095445f5a1fc`.
Content tree: `1b067cadd1e7887415354c4f384a51f93d6437c367574e2cee52b0b44d829479`.
Catalog git tree: `603e2a2b1442a8214cd97b03d32bf57fc158feac`.
Evidence-bearing tip: this stage-7 branch tip (records/logs/handoff only). Do **not** treat it as the package source tree.
Publish evidence: `evidence/f04-stage7/CHECKS.md`, `live-verify.log`.
Prior package evidence: `evidence/f04-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Merge evidence: `evidence/f04-stage5/CHECKS.md` (nightly FF REFUSED; no force-push).
Host tests: `evidence/f04-stage3/CHECKS.md` (ALL_PASS).
Deferred (next defect = oldest): `evidence/f04-stage2/DEFERRED.md`.
Successor prompt: `evidence/f04-stage7/SUCCESSOR-STAGE1.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 1 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required.

## Live nightly page (confirmed)

- Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/
- `latest`: `nightly-0d79d634-dev`
- Label: `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested`: false
- Elite bundled sha256: `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278`
- Pro bundled sha256: `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb`
- Installer FP: `2240be8702176544945a4b66171269e958b47259fb275654d2f6ff7125f637ee`
- Live marker: `d11b74fc` / selection generations
- Pages tip: `38b5d2a7d6380a4b9fd9ea1ff28d45b53d826569`
- Repair Mac: `bc-795342a0-d8f9-52d2-8485-20377d0a90f8`

## Scope for stage 1

Audit validated source SHA `0d79d634…` and the published page hashes. Take the oldest deferred defect not already fixed in that SHA (BF-001 already fixed). Start exactly one owner-visible successor under the handoff rule after the audit is recorded. Local host checks only — no GitHub Actions / Codex-on-GitHub.
