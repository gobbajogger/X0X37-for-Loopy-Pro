# Workflow handoff to stage 4

Receiving stage: **4 Validate** (new cloud thread; F14 after stage-3 BF-013 test).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f14-bf013-stage3-237f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-3 owner: `bc-ffda0814-f56c-5ba4-bf4c-d31ecf7a4cf1`.
Stage-2 owner: `bc-129a2454-b858-5423-9331-22fb792ae11f`.
Stage-1 owner: `bc-4c124351-a5c1-53ea-a243-b3b478f7bc15`.
Audited / validated source SHA: `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`.
Git `source_tree` (pre-repair package attribution): `a15afc439fc8b7bccd62d50403b925add45ae9e5`.
Firmware tree (pre-repair): `5facb3b4405d119a269a05224b9268adfa1baa13`.
Content tree (non-`.md`, Stage-4/7 DEV CHECKED pre-repair): `519ccdf21b46db9198993587cb9334960e67ce483641c744c5caf71b1c769141`.
Implement tip: `4c01165a8391d12a2ca2842bb139671e89b1fc8b` (`cursor/f14-bf013-stage2-237f`, PR #316).
Repair commit: `13b831e8a018137a64fa3d0c03cb5d520d8be8fd`.
BF-013 repair: `firmware/src/dsp/fxbus.c` blob `49a89483e1c777a46758ed370973afb85f15274d` + `tests/host/stereo_test.c` blob `c8bc1092319f38a46d4f8d41ab5b2a4ef560ff39`.
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008), `6636cca1`/`b42bcba4` (BF-012).
Test evidence: `evidence/f14-stage3/CHECKS.md`, `DEFERRED.md`, `blob-confirm.txt`, `stereo-green.log`, `stereo-green-reconfirm.log`, `stereo-red-witness.log`, `stereo-red-detail.log`, `test-run-summary.json`.
Implement evidence: `evidence/f14-stage2/CHECKS.md`, `DEFERRED.md`, `bf013-red.log`, `bf013-green.log`, `host-unit.log`.
Audit record: `evidence/f14-stage1/AUDIT.md`, `bf013-definition.md`.
Successor prompt: `evidence/f14-stage3/SUCCESSOR-STAGE4.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 4 id is recorded on this remote tip. CreateAgent attempts recorded as `api-create-stage4-skip.json` / `api-create-stage4-note.txt` (`NO_CURSOR_API_KEY`). Do not claim a Task subagent as Stage 4. Stage 3 keeps ownership until the id is pushed.

## Test summary

BF-013 only. Required host regressions **ALL_PASS**: tip blobs match Stage-2 repair; `X0X_TESTS=stereo X0X_TEST_JOBS=8 X0X_DEV_PADCAL=1 sh tests/run_tests.sh` exit 0 (`ok stereo`). Optional red witness on unrepaired blob `4c748539…` FAIL_EXPECTED with four 20 BPM assertions. No failing required check, so no failure-investigation subagent and no return to stage 2. Task subagent namespace was unavailable in this runtime; Stage-3 owner ran the tests directly.

## Scope for stage 4

Build Elite and Pro development packages (`X0X_DEV_PADCAL=1`) from the tested candidate on `cursor/f14-bf013-stage3-237f`. Record exact package and controller identities, budgets, checks, and logs. Host PASS is not a package PASS. Do not implement deferred items. Do not reopen BF-001–BF-008, BF-012, or FP-001. Do not steal Arp/Fx #279/#281 leftovers. Do not merge production `main`, publish production Pages, or flash a device from this stage.

When the affected development packages are DEV CHECKED, write the stage-5 successor prompt, push it, and start exactly one owner-visible stage-5 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
