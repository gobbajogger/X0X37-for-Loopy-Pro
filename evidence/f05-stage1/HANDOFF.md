# Workflow handoff to stage 2

Receiving stage: **2 Implement** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f05-bf002-stage1-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-1 owner: `bc-bd9cb2ca-4784-5d48-968d-bee42f593ff1`.
Audited / validated source SHA: `0d79d63485dc6585800a71ef716d095445f5a1fc`.
Content tree: `1b067cadd1e7887415354c4f384a51f93d6437c367574e2cee52b0b44d829479`.
Git tree: `603e2a2b1442a8214cd97b03d32bf57fc158feac`.
Repair commits already in tip: `d11b74fc` (BF-001), `4f18983a` (F03) — not re-opened.
Audit record: `evidence/f05-stage1/AUDIT.md`.
Checks: `evidence/f05-stage1/CHECKS.md`.
Prior publish evidence: `evidence/f04-stage7/CHECKS.md`, `HANDOFF.md`, `live-verify.log`.
Deferred younger items: BF-003 (retire-vs-restore), withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets (`evidence/f04-stage2/DEFERRED.md`).
Successor prompt: `evidence/f05-stage1/SUCCESSOR-STAGE2.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 2 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required.

Selected defect: **BF-002**. Scoped quickcheck omits project web regressions (`projects-web` / `projects-page`). Approved repair: add explicit project-suite routes for project JS/HTML and shared `x0x_breaks.js` as needed; extend `tests/host/test_quickcheck_plan.py`; run planner + project suites locally.

## Scope

Stage 2 only. Apply the approved BF-002 fix in `tools/quickcheck_plan.py` with host planner coverage. Do not implement BF-003 or other deferred items. Do not merge production `main`, publish production Pages, or flash a device. Do not change firmware, controller, or package bytes.

When checked commits are on the remote, write the stage-3 successor prompt, push it, and start exactly one owner-visible stage-3 Cloud Agent under the same handoff rule (coordinator CreateAgent if no API key). An internal Task subagent does not complete the handoff.
