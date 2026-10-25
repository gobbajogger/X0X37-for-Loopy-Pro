# Workflow handoff to stage 1 (next cycle)

Receiving stage: **1 Audit** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f02-stage7-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-7 owner: `bc-aaf1fbac-fbdf-50d7-8f6f-a71e7bc55c3b`.
Audited SHA: `13d32e1dee37b6fb5a1e339aff499a75eecb97b5`.
Validated **source** SHA (published / next audit): `4ee8f37714b0892f2c70d157fda9328da393e987`.
Content tree: `7ad3a50da8abac890215dae3c28197a2fa90041b8615e828b6b3c6e18060fb83`.
Evidence-bearing tip: this stage-7 branch tip (records/logs/handoff only). Do **not** treat it as the package source tree.
Publish evidence: `evidence/f02-stage7/CHECKS.md`.
Prior package evidence: `evidence/f02-stage4/CHECKS.md`, `elite/record.json`, `pro/record.json`.
Merge evidence: `evidence/f02-stage5/CHECKS.md` (nightly FF REFUSED; no force-push).
Host tests: `evidence/f02-stage3/CHECKS.md` (ALL_PASS).
Deferred (next defect = oldest): `evidence/f02-stage2/DEFERRED.md` → **F03**.
Successor prompt: `evidence/f02-stage7/SUCCESSOR-STAGE1.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 1 id is recorded on this remote tip.

## Live nightly page (confirmed)

- Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/
- `latest`: `nightly-4ee8f377-dev`
- Label: `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested`: false
- Elite bundled sha256: `8ba23a89f50c70b631927077ec4cfe7c98593870035455261842f1b245b82278`
- Pro bundled sha256: `d90932115ff47cf421b0ef5caab3b93e730da5810e5b26d4c3ecd2c61c58fecb`
- Pages tip: `364c55092b8a40fcb683c3008074c508a6d75bb4`
- Package host: `artifacts/nightly-dev` @ `f917b636…`

## Scope for stage 1

Audit validated source SHA `4ee8f377…` and the published page hashes. Take the oldest deferred defect (F03) not already fixed in that SHA. Start exactly one owner-visible successor under the handoff rule after the audit is recorded.
