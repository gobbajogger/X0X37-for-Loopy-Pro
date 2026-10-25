# Workflow handoff to stage 2

Receiving stage: **2 Implement** (new cloud thread; F14 after F13 LIVE_PAGE_MATCH).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f14-bf013-stage1-237f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-1 owner: `bc-4c124351-a5c1-53ea-a243-b3b478f7bc15`.
Validated **source** SHA (audit / publish tip): `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`.
Content tree: `519ccdf21b46db9198993587cb9334960e67ce483641c744c5caf71b1c769141`.
Git source_tree: `a15afc439fc8b7bccd62d50403b925add45ae9e5`.
Firmware tree: `5facb3b4405d119a269a05224b9268adfa1baa13`.
Selected defect: **BF-013** (delay sync at exact 20 BPM in `firmware/src/dsp/fxbus.c`).
Live: **LIVE_PAGE_MATCH** `nightly-b42bcba4-dev`.
Elite page sha256: `82ee29d3ede1c17427f2689fbcdd2ab7e2eca7c72df73dcd7694078c9e313b14`.
Pro page sha256: `bc6c04229840c6342f6b8a696939bab7763a934b41d90a95f201250a42bfad34`.
Evidence: `evidence/f14-stage1/AUDIT.md`, `CHECKS.md`, `bf013-definition.md`, `live-verify.log`, `bf013-witness.log`, `bf013-scratch-clear.log`.
Successor prompt: `evidence/f14-stage1/SUCCESSOR-STAGE2.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 2 id is recorded on this remote tip. CreateAgent attempts recorded as `api-create-stage2-attempt{1,2}.json` (`NO_CURSOR_API_KEY`). Do not claim a Task subagent as Stage 2. Stage 1 keeps ownership until the id is pushed.

## Scope for stage 2

Implement BF-013 only: change tip `fxbus.c` `> 20.0f` gates to `>= 20.0f` and add stereo host regressions from PR #74. Branch `cursor/<slug>-stage2-237f` from this audit tip. Do not reopen BF-001–BF-008, BF-012, or FP-001. Do not steal Arp/Fx sole-owner leftovers. Do not flash. Leave production unchanged.
