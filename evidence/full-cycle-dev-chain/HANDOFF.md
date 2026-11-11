# Workflow handoff to stage 1

Receiving stage: **1 Audit** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/full-cycle-stage-workflow-d6ff`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent: `bc-01a16f1a-08bd-72eb-b558-6c498238d6ff`.
Prompt commit: `e23170227ca4ae4b6cc2700692f4d650fa1b506b`.
Successor agent id: none.
Successor URL: none.

`bc-ad31708d-5548-5785-bb62-75a54918e755` is not an owner-visible successor. Its source is internal. It did finish the stage-1 audit.

Audit branch: `cursor/f02-loader-stage1-e755` at `5293bcd8e9a4152c5dcdbd515390ee3a8e60f945`.
Audit record: `evidence/f02-loader-stage1/AUDIT.md`.
Selected defect: **F02**. After the installer sends the selected keyboard into its update loader, the next search can attach to a different keyboard and write the package there. The approved repair is fail-closed: snapshot the loader ports, accept exactly one new loader, and do not write otherwise.
Stage 2 agent id: none. Stage 2 has not started. This stage still owns the handoff.

## Scope

The cloud loop is Audit, Implement, Test, Validate, Merge, Publish, then Audit again. This successor is stage 1 only. It audits the tip that contains this file. It does not implement, merge, install, or publish in this thread.

Defect order: the oldest deferred defect from any cycle that is not already fixed in that tip, then a recorded failing check that is not already fixed in that tip, then the oldest open audit or bugfinder finding not already contained in that tip. Do not ask which defect to take.

If a defect is recorded, start one owner-visible stage-2 Cloud Agent. If none is recorded, start one owner-visible stage-1 Cloud Agent. Use `POST https://api.cursor.com/v1/agents` with the owner's API key. Push that agent's id and URL before stopping. An internal Task subagent does not complete the handoff.

Leave production `main` and production GitHub Pages unchanged. Do not flash a device. Do not authenticate the create call with the pod token.
