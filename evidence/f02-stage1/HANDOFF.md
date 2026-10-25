# Workflow handoff to stage 2

Receiving stage: **2 Implement** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f02-stage1-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch (same as `origin/main` tip audited).
Parent / stage-1 owner: `bc-93d068c9-df43-525d-b826-6e57a67efb90`.
Audited SHA: `13d32e1dee37b6fb5a1e339aff499a75eecb97b5`.
Audit record: `evidence/f02-stage1/AUDIT.md`.
Successor prompt: `evidence/f02-stage1/SUCCESSOR-STAGE2.txt`.
Successor agent id: `bc-37feb056-46e1-5377-8388-5fe9e46d3ca6`.
Successor URL: https://cursor.com/agents/bc-37feb056-46e1-5377-8388-5fe9e46d3ca6
Successor source: `desktop` (CreateAgent). Name: `F02 stage2 implement`. `startingRef`: `cursor/f02-stage1-fb90`.
Handoff status: **complete**. Owner-visible Stage 2 is recorded on this remote tip. Stage 1 ownership ends. Ignore internal probe `bc-3afa217a-8ab0-53d0-a0bb-8f745a71c2b2` (`source=internal`); it is not the successor.

Selected defect: **F02**. After the installer sends the selected keyboard into its update loader, the next search can attach to a different keyboard and write the package there. The approved repair is fail-closed: snapshot the loader ports, accept exactly one new loader, and do not write otherwise.

## Scope

Stage 2 only. Apply the approved F02 fix. Do not implement F03 or BF-001–BF-003. Do not merge production `main`, publish production Pages, or flash a device.

When checked commits are on the remote, write the stage-3 successor prompt, push it, and start exactly one owner-visible stage-3 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
