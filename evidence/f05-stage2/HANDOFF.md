# Workflow handoff to stage 3

Receiving stage: **3 Test** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f05-bf002-stage2-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-2 owner: `bc-9490d387-26c4-58f1-a59e-84ccb8145ef7`.
Audited / validated source SHA: `0d79d63485dc6585800a71ef716d095445f5a1fc`.
Content tree: `1b067cadd1e7887415354c4f384a51f93d6437c367574e2cee52b0b44d829479`.
Git tree: `603e2a2b1442a8214cd97b03d32bf57fc158feac`.
Repair commits already in tip: `d11b74fc` (BF-001), `4f18983a` (F03) — not re-opened.
BF-002 implement commit: see `git log` on this branch (`Fix BF-002 scoped quickcheck project suite routes`).
Implement evidence: `evidence/f05-stage2/CHECKS.md`, `evidence/f05-stage2/DEFERRED.md`.
Audit record: `evidence/f05-stage1/AUDIT.md`.
Stage-2 PR: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/195
Successor prompt: `evidence/f05-stage2/SUCCESSOR-STAGE3.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 3 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required.

## Repair summary

BF-002 only. `tools/quickcheck_plan.py` adds explicit project-suite routes for `web/x0x_projects.js`, `web/x0x_projects.html`, shared `web/x0x_breaks.js`, and project test entrypoints so scoped quickcheck selects `projects-web` / `projects-page`. Host fixture `tests/host/test_quickcheck_plan.py::test_project_web_routes_select_project_suites` advertises both project commands and asserts production-module edits select both suites while unrelated UI stays scoped. Local host/node proof recorded under `evidence/f05-stage2/`.

## Scope for stage 3

Run the BF-002 planner host tests and the project web/page suites on the implement tip. Separate subagents for the test run and for failure investigation. Do not implement deferred items (BF-003 needing retire-vs-restore, withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets). Do not merge production `main`, publish production Pages, or flash a device. Do not change firmware, controller, or package bytes.

When checks pass, write the stage-4 successor prompt, push it, and start exactly one owner-visible stage-4 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
