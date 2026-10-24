# Workflow handoff to stage 2

Receiving stage: **2 Implement** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f02-loader-stage1-e755`.
Audited SHA: `e23170227ca4ae4b6cc2700692f4d650fa1b506b`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Audit: `evidence/f02-loader-stage1/AUDIT.md`.
Successor prompt: `evidence/f02-loader-stage1/SUCCESSOR-STAGE2.txt`.
Stage-1 thread: `bc-ad31708d-5548-5785-bb62-75a54918e755`.
Successor agent id: pending until the new thread starts and this file is pushed again.

This push prepares the handoff. It is not complete until the successor agent id is on the remote.

## Scope

Selected defect: **F02**. A loader transition can attach to a different keyboard. The approved repair and the exclusions are in the audit and in the successor prompt.

Stage 2 implements that repair only. It does not audit a different defect, merge production `main`, publish production Pages, or flash a device.
