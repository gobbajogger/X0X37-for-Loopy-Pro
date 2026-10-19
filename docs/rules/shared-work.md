# Shared work

Read before shared edits, hardware work, a hard to undo action, a wait, or a subagent handoff.

### Rule 3: shared resources and waits

Use isolated worktrees for independent edits. Do not edit a shared script while it runs. Serialize device writes and edits to the same live file. Never reset or overwrite another agent's changes.

While a check runs, do useful independent work. Use bounded waits and report a meaningful change. If a needed host is unavailable, make a bounded retry, then use an equivalent supported local environment or report the exact dependency. Do not wait five minutes as a ritual when the work can finish locally. A quiet or retired agent is not an approval gate.

### Rule 4: Decision check

Check evidence, damage, undo, cause, and scope before shared or hard to undo work. Prefer the smallest change that completes the task. Keep owner data and other agents' work safe. Investigate failures; do not bypass a real guard or report an unrun check as passed.

A word such as "probably" is a reason to state uncertainty and check the relevant fact. It does not require a new approval loop for a reversible task.

### Rule 16: subagents

Follow core rule 16. Each prompt includes the current rules entry point, task scope, allowed files, input commit or tree, done criteria, and [style.md](style.md). Tell the agent where to read both writing skills and how to report a missing skill. Separate independent audit scopes. Use one final independent review for rule changes; repair findings in the same task. Repeat review only for a material new change or unresolved finding.
