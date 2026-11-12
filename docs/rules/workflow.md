# Workflow routing

The maintained code and PR workflow is [x0x37-workflow](../../skills/x0x37-workflow/SKILL.md). Start with [AGENTS.md](../../AGENTS.md), then load the selected project skill. Sol Loop, Full Send, and Full Stop require distinct explicit requests. Current owner instructions and action-specific permission govern every step.

For where builds, host tests, coordination and installs run, and for the merge bar, follow the "Where work runs" section in [AGENTS.md](../../AGENTS.md) and [task routing](../../skills/x0x37-task/references/routing.md).

For recurring work, resolve the live `coordination/merge-queue` ref and read the
[automation runtime contract](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/coordination/merge-queue/coordination/automation/runtime-contract.md),
`coordination/automation/roles.json`, current requests and current delivery records
at that same revision. Keep the role bootstrap stable: new authorized work updates
the existing request and acceptance data. Task-specific proof, permissions and
existing owners still govern.

For firmware-specific proof, use [project stages](../../skills/x0x37-workflow/references/stages.md) and [local proof](local-proof.md). Device actions and protected merges have separate conditions. A docs-only skill edit does not start a firmware stage.

Earlier automatic stage, successor, install, and publication procedures in this file are historical Git revisions, not current instructions.

See [historical instruction boundaries](workflow-history.md) for retained plans,
thread records and legacy tooling that are not active routing or authorization.

`tools/merge_project_prs.py` is a read-only legacy planner. Its default, `--dry-run`
and `--json` routes perform no merge, retarget or auto-merge action. PR numbers and
full-head selectors are not proof or authority. The existing assigned integrator
consumes current candidates through the maintained queue and validates the exact
base, applicable proof, required checks/reviews and action permission. The planner
keeps failed/unknown items visible while reporting independent candidates.
