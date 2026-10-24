# Workflow handoff to stage 1

Receiving stage: **1 Audit** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/full-cycle-stage-workflow-d6ff`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent: `bc-01a16f1a-08bd-72eb-b558-6c498238d6ff`.
Prompt commit: `e23170227ca4ae4b6cc2700692f4d650fa1b506b`.
Successor agent id: `bc-ad31708d-5548-5785-bb62-75a54918e755`.
Successor URL: https://cursor.com/agents/bc-ad31708d-5548-5785-bb62-75a54918e755

The handoff is complete when this id is on the remote.

## Scope

The cloud loop is Audit, Implement, Test, Validate, Merge, Publish, then Audit again. This successor is stage 1 only. It audits the tip that contains this file. It does not implement, merge, install, or publish in this thread.

Defect order: the oldest deferred defect from any cycle that is not already fixed in that tip, then a recorded failing check that is not already fixed in that tip, then the oldest open audit or bugfinder finding not already contained in that tip. Do not ask which defect to take.

If a defect is recorded, start one new stage-2 cloud thread. If none is recorded, start one new stage-1 cloud thread. Push that thread's id before stopping. Do not resume this parent or any earlier review thread.

Leave production `main` and production GitHub Pages unchanged. Do not flash a device. Do not call the Cloud Agents HTTP API.
