# Workflow handoff to stage 3

Receiving stage: **3 Test** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f02-stage2-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-2 owner: `bc-37feb056-46e1-5377-8388-5fe9e46d3ca6`.
Audited SHA: `13d32e1dee37b6fb5a1e339aff499a75eecb97b5`.
Implement evidence: `evidence/f02-stage2/CHECKS.md`, `evidence/f02-stage2/DEFERRED.md`.
Successor prompt: `evidence/f02-stage2/SUCCESSOR-STAGE3.txt`.
Successor agent id: `bc-0d6b70b5-59b9-5d51-b9e2-c97ea486934b`.
Successor URL: https://cursor.com/agents/bc-0d6b70b5-59b9-5d51-b9e2-c97ea486934b
Successor source: `desktop` (CreateAgent). Name: `F02 stage3 test`. `startingRef`: `cursor/f02-stage2-fb90`.
Handoff status: **complete**. Owner-visible Stage 3 is recorded on this remote tip. Stage 2 ownership ends.

## Repair summary

F02 only. Snapshot loader (or running) ports before a commanded transition; `wait_for` / `waitFor` accept exactly one newly appeared matching port, otherwise fail closed and do not write. Slot-restore / ctl reconnect uses `reconnect_after` / `reconnectAfter` against the port the transition wait accepted. Initial first-running-keyboard scan unchanged. Unmerged data-safety identity lanes not imported.

## Scope for stage 3

Run the F02 regressions and the affected installer checks. Separate subagents for the test run and for failure investigation. Do not implement deferred items. Do not merge production `main`, publish production Pages, or flash a device.

When checks pass, write the stage-4 successor prompt, push it, and start exactly one owner-visible stage-4 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
