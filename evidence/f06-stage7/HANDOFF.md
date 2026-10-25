# Workflow handoff to stage 1 (next cycle)

Receiving stage: **1 Audit** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f06-bf003-stage7-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-7 owner: `bc-f7783059-a0d9-5c29-a373-763ffd739822`.
Audited SHA: `ab90f47ccad0f213114d6a98c1bc6179be2ebb35`.
Validated **source** SHA (published / next audit): `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Content tree: `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Catalog git tree: `6b6401f5c57ed4142bce1adbc3ceab2f99a80b6e`.
Evidence-bearing tip: this stage-7 branch tip (records/logs/handoff only). Do **not** treat it as the package source tree.
Publish evidence: `evidence/f06-stage7/CHECKS.md`, `live-verify.log`, `independent-qa-local-review.md`.
Prior package evidence: `evidence/f06-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Merge evidence: `evidence/f06-stage5/CHECKS.md` (nightly FF REFUSED; no force-push).
Host tests: `evidence/f06-stage3/CHECKS.md` (ALL_PASS; not used alone for independent-qa).
Independent QA: local-env `internal/pr-local-reviews/pr-210.md` (PASS; unchanged package bytes) — copy on tip.
Deferred (next defect = oldest): `evidence/f06-stage2/DEFERRED.md`.
Successor prompt: `evidence/f06-stage7/SUCCESSOR-STAGE1.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 1 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required.

## Live nightly page (confirmed)

- Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/
- `latest`: `nightly-1325f5d3-dev`
- Label: `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested`: false
- Elite bundled sha256: `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278`
- Pro bundled sha256: `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb`
- Installer FP: `8caf87247afb79dda2107f3074d73da7f2512f5d14e55db32b9285baadc38157`
- Live marker: `1325f5d3` / BF-003 / `nightly-1325f5d3-dev`
- Pages tip: `cff694f3792d196267c2dc8ce2d86c6aef7d5e9e`
- Mac: `bc-3161dbe9-ac70-502b-9032-a3314f404283`

## Scope for stage 1

Audit validated source SHA `1325f5d3…` and the published page hashes. Take the oldest deferred defect not already fixed in that SHA (BF-001, BF-002, and BF-003 already fixed). Start exactly one owner-visible successor under the handoff rule after the audit is recorded. Local host checks only — no GitHub Actions / Codex-on-GitHub.
