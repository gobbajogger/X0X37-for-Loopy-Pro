# Workflow handoff to stage 1 (next cycle)

Receiving stage: **1 Audit** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f03-slot-probe-stage7-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-7 owner: `bc-b413a138-39ce-508e-bee4-893900ed93d1`.
Audited SHA: `4ee8f37714b0892f2c70d157fda9328da393e987`.
Validated **source** SHA (published / next audit): `a03cfab3a423ab5c65a6546afd4993fe51754ee7`.
Content tree: `1755a2dd6d8b79e7405ab58919cc4d60a69ca11a4ae6601030b0f5de963f170e`.
Catalog git tree: `85e159c5365b14c48e9f3df7d2089e8f2eb560d4`.
Evidence-bearing tip: this stage-7 branch tip (records/logs/handoff only). Do **not** treat it as the package source tree.
Publish evidence: `evidence/f03-stage7/CHECKS.md`, `live-verify.log`.
Prior package evidence: `evidence/f03-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Merge evidence: `evidence/f03-stage5/CHECKS.md` (nightly FF REFUSED; no force-push).
Host tests: `evidence/f03-stage3/CHECKS.md` (ALL_PASS).
Deferred (next defect = oldest): `evidence/f03-stage2/DEFERRED.md`.
Successor prompt: `evidence/f03-stage7/SUCCESSOR-STAGE1.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 1 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required.

## Live nightly page (confirmed)

- Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/
- `latest`: `nightly-a03cfab3-dev`
- Label: `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested`: false
- Elite bundled sha256: `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278`
- Pro bundled sha256: `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb`
- Pages tip: `4b3c5994eabd14aa1a1a575b2211800a478112b4`
- Package host: `artifacts/nightly-dev` @ `f917b636…`

## Scope for stage 1

Audit validated source SHA `a03cfab3…` and the published page hashes. Take the oldest deferred defect not already fixed in that SHA. Start exactly one owner-visible successor under the handoff rule after the audit is recorded.
