# Workflow handoff to stage 4

Receiving stage: **4 Validate** (new cloud thread; F13 after stage-3 BF-012 test).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f13-bf012-stage3-237f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-3 owner (CreateAgent / owner-visible): `bc-f7da628e-c002-56ce-ac69-5950bbe50dcf`.
First tip author (parallel worker): `bc-7c49b76a-97dd-57c9-9a5a-784be039d70a`.
Stage-2 owner: `bc-a0181730-1daf-5e94-826a-e4339ed37abf`.
Stage-1 owner: `bc-09959f6e-dc18-5575-baa6-444dfdf698cc`.
Audited / validated source SHA: `de11b6d7e3ac3322d2db689675d9ed27ed667785`.
Git `source_tree` (pre-repair package attribution): `c143a9f8f5b5d0f6804995829ccbe515c77c6103`.
Firmware tree (pre-repair): `9195dce5c4beb5fa8aa564504c8d1d114c8264cd`.
Implement tip: `465071f2faac226b760626f7ae0fb1f59fa81327` (`cursor/f13-bf012-stage2-237f`, PR #294).
Repair commit: `6636cca1778aa13dcb4f51f5f766a056c2b40698`.
BF-012 repair: `firmware/src/dsp/master.c` blob `f91a9473f0ae1cdd5e037e521f39fdfcc2022395` + `tests/host/master_test.c` blob `df2f13b35a6fc7b858b019a1950982c23f6386e3`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008).
Test evidence: `evidence/f13-stage3/CHECKS.md`, `DEFERRED.md`, `master-host.log`, `host-unit-reverify.log`, `bf012-red-witness.log`, `blob-confirm.txt`, `bf012-blob-review-reverify.txt`, `test-run-summary.json`, `test-run-summary-reverify.json`.
Implement evidence: `evidence/f13-stage2/CHECKS.md`, `DEFERRED.md`, `bf012-red.log`, `bf012-green.log`.
Audit record: `evidence/f13-stage1/AUDIT.md`, `bf012-definition.md`.
Successor prompt: `evidence/f13-stage3/SUCCESSOR-STAGE4.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 4 id is recorded on this remote tip.

## Test summary

BF-012 only. Required host regressions **ALL_PASS** on first tip and on owner-visible re-verify under test-run subagent `bc-381a7f1e-0cdc-501b-8f7c-a52fbca93763`: tip blobs match Stage-2 repair; `X0X_TESTS=master X0X_TEST_JOBS=8 X0X_DEV_PADCAL=1 sh tests/run_tests.sh` exit 0 (`ok master`). Optional red witness on unrepaired blob `b5557b6c…` FAIL_EXPECTED with stale peak `0.500000`. No failing required check, so no failure-investigation subagent and no return to stage 2.

## Scope for stage 4

Build Elite and Pro development packages (`X0X_DEV_PADCAL=1`) from the tested candidate on `cursor/f13-bf012-stage3-237f`. Record exact package and controller identities, budgets, checks, and logs. Host PASS is not a package PASS. Do not implement deferred items. Do not reopen BF-001–BF-008 or FP-001. Do not steal Arp/Fx #279/#281 leftovers. Do not merge production `main`, publish production Pages, or flash a device from this stage.

When the affected development packages are DEV CHECKED, write the stage-5 successor prompt, push it, and start exactly one owner-visible stage-5 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
