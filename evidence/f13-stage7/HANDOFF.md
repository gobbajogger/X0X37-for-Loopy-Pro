# Workflow handoff to stage 1

Receiving stage: **1 Audit** (new cloud thread; next cycle after F13 publish).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f13-bf012-stage7-237f`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-7 owner: `bc-5bd25afb-db89-51d1-90c1-0c0a985ff2eb`.
Validated **source** SHA (published / next audit): `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`.
Content tree: `519ccdf21b46db9198993587cb9334960e67ce483641c744c5caf71b1c769141`.
Git source_tree: `a15afc439fc8b7bccd62d50403b925add45ae9e5`.
Firmware tree: `5facb3b4405d119a269a05224b9268adfa1baa13`.
BF-012 repair commit: `6636cca1`.
Live: **LIVE_PAGE_MATCH** `nightly-b42bcba4-dev` (Pages tip `0bfcbdf8fe3b…`).
Elite page sha256: `82ee29d3ede1c17427f2689fbcdd2ab7e2eca7c72df73dcd7694078c9e313b14`.
Pro page sha256: `bc6c04229840c6342f6b8a696939bab7763a934b41d90a95f201250a42bfad34`.
Blank Elite/Pro: `d9a9ccf4698d…` / `a6a2387c712e…`.
Label: `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested` false.
Evidence: `evidence/f13-stage7/CHECKS.md`, `live-verify.log`, `independent-qa-local-review.md`, `safety-gates-note.md`.
Successor prompt: `evidence/f13-stage7/SUCCESSOR-STAGE1.txt`.
Successor agent id: none.
Successor URL: none.
Successor source: pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
Handoff status: **incomplete** until an owner-visible Stage 1 id is recorded on this remote tip. CreateAgent attempts recorded as `api-create-stage1-attempt{1,2}.json` (`NO_CURSOR_API_KEY`). Do not claim a Task subagent as Stage 1. Stage 7 keeps ownership until the id is pushed.

## Scope for stage 1

Audit validated source SHA `b42bcba46ee7…` against the published nightly page hashes. Select the next defect per workflow (oldest deferred not fixed in that SHA, then failing check, then oldest open finding). Do not reopen BF-001–BF-008, BF-012, or FP-001. Do not flash. Leave production unchanged. Stage 6 flash remains Mac LaunchAgent after DEV OK.
