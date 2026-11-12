# Workflow handoff to stage 4

Receiving stage: **4 Validate** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f02-stage3-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-3 owner: `bc-0d6b70b5-59b9-5d51-b9e2-c97ea486934b`.
Audited SHA: `13d32e1dee37b6fb5a1e339aff499a75eecb97b5`.
Tested implement tip: `478419e62675a72a156f25072ccb9f59f3106661` plus this stage-3 evidence tip on `cursor/f02-stage3-fb90`.
Repair commit: `765695f`.
Test evidence: `evidence/f02-stage3/CHECKS.md`.
Deferred (unchanged): `evidence/f02-stage2/DEFERRED.md`.
Successor prompt: `evidence/f02-stage3/SUCCESSOR-STAGE4.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 4 id is recorded on this remote tip.

## Test summary

F02 only. Required loader-bind regressions and affected installer checks all PASS under a dedicated test-run subagent. No failing required check, so no failure-investigation subagent and no return to stage 2. Optional FakeFM1 web smoke NOT_RUN (missing `web/editor.html`); not treated as an F02 failure.

## Scope for stage 4

Build Elite and Pro development packages (`X0X_DEV_PADCAL=1`) from the tested candidate on `cursor/f02-stage3-fb90`. Record exact package and controller identities, budgets, checks, and logs. Host PASS is not a package PASS. Do not implement deferred items. Do not merge production `main`, publish production Pages, or flash a device.

When the affected development packages are DEV CHECKED, write the stage-5 successor prompt, push it, and start exactly one owner-visible stage-5 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
