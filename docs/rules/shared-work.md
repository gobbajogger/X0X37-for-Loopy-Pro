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

Each prompt gives the current entry point, [style](style.md), [skill status](../SKILLS.md),
both resolved writing skill paths, input commit/tree, task, owned files, and
done criteria. Each helper reads both skills. Report any missing file. Keep separate scopes. Rule repairs need one final independent review;
repeat only for a material new change or unresolved finding.
