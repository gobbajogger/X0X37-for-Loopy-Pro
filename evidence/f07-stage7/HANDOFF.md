# Workflow handoff to stage 1 (next cycle)

Receiving stage: **1 Audit** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f07-fp-pin-stage7-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-7 owner: `bc-327a66e2-7dfc-58c2-af70-8bb1ea80452d`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Validated **source** SHA (published / next audit): `6df0f81fbbe0adc3207f67d912d2fe0ab676be04`.
Content tree: `69ccb3d77a718d32191cf17a75bce6454dad807ef9bb76f6050e6532a0bff3ad`.
Catalog git tree: `4dc57f74141fd1f01728a0001025b6e76065bf19`.
Evidence-bearing tip: this stage-7 branch tip (records/logs/handoff only). Do **not** treat it as the package source tree.
Publish evidence: `evidence/f07-stage7/CHECKS.md`, `live-verify.log`, `independent-qa-local-review.md`.
Prior package evidence: `evidence/f07-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Merge evidence: `evidence/f07-stage5/CHECKS.md` (nightly FF REFUSED; no force-push).
Host tests: `evidence/f07-stage3/CHECKS.md` (ALL_PASS; not used alone for independent-qa).
Independent QA: local-env `internal/pr-local-reviews/pr-222.md` (PASS; unchanged package bytes) — copy on tip.
Deferred (next defect = oldest): `evidence/f07-stage2/DEFERRED.md`.
Successor prompt: `evidence/f07-stage7/SUCCESSOR-STAGE1.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 1 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required after the prompt is pushed.

## Live nightly page (confirmed)

- Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/
- `latest`: `nightly-6df0f81f-dev`
- Label: `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested`: false
- Elite bundled sha256: `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278`
- Pro bundled sha256: `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb`
- Installer FP: `8caf87247afb79dda2107f3074d73da7f2512f5d14e55db32b9285baadc38157`
- Live marker: `6df0f81f` / FP-001 / `nightly-6df0f81f-dev`
- Pages tip: `d6a237c43cb6830d8cf0cb51e567181821630a28`
- Mac: `bc-36495868-6d21-55f1-9274-4c6b065bdc08`

## Scope for stage 1

Audit validated source SHA `6df0f81f…` and the published page hashes. Take the oldest deferred defect not already fixed in that SHA (BF-001, BF-002, BF-003, and FP-001 already fixed). Start exactly one owner-visible successor under the handoff rule after the audit is recorded. Local host checks only — no GitHub Actions / Codex-on-GitHub.
