# Shared work

### Rule 3: resources and waits

Use separate worktrees for independent edits. Coordinate overlapping files
before editing. Do not edit a running shared script, reset another agent's
work, or run concurrent device writes.

While a check runs, do independent work. Use bounded waits; report meaningful
changes. After a bounded host retry, use an equivalent supported environment
or name the dependency. Quiet or retired agents are not approval gates.

### Rule 4: Decision check

Before shared or hard to undo work, check evidence, damage, undo, cause, and
scope. Make the smallest complete repair. Investigate failures; never bypass
guards or claim unrun checks. Uncertainty calls for a relevant fact check,
not repeated approval for authorized reversible work.

### Rule 16: subagents

Use helpers when requested or required for independent review. Give the current
entry point, input commit/tree, task, owned files, relevant skill paths, and done
criteria. Keep scopes separate and reuse helpers for follow-up; the task agent
owns completion. Rule repairs need one final independent review; repeat only
for a material new change or unresolved finding.
