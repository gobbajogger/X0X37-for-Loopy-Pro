---
name: x0x37-shared-work
description: "Coordinate x0x37 concurrent edits, shared processes, device resources, hard-to-undo actions, or delegated independent review."
---

# Shared work

### Rule 3: resources and waits

Use separate worktrees for independent edits. Coordinate overlapping files before editing.
Do not edit a shared script while it runs. Do not reset another agent's work or run concurrent device writes.

Do independent work while a check runs. Use bounded waits. Report meaningful changes.
After one bounded host retry, use an equivalent supported environment or state the missing dependency.

Quiet or retired agents do not create approval requirements.

### Rule 4: Decision check

Before shared work or an action that is difficult to undo, check the evidence and possible damage.
Check the undo method, cause, and scope.

Make the smallest complete repair. Investigate failures. Never bypass guards or claim checks that did not run.
When uncertain, check the relevant facts. Do not ask again for authorized reversible work.

### Rule 16: subagents

The owner permits retaining useful chains and subagents where supported; do not require them for every stage.
Use helpers for bounded independent work when justified by the authorized task, explicitly requested, or required for independent review.
Verify the current interface exposes delegation tools. If unavailable, work sequentially or prepare a handoff.
Do not switch classic chat to Work or Codex solely to obtain helpers; apply [chat routing](../x0x37-task/references/routing.md).
Use the fewest helpers and smallest relevant context. Account for their added usage; do not promise a quota saving.
Give each helper the current skill entry point, input commit or tree, task, owned files, relevant skill paths, and completion criteria.

Keep scopes separate. Apply [rule 2](../x0x37-task/SKILL.md) to chat ownership.
The task agent remains responsible for completion.

A rule repair needs one final independent review. Repeat it only after a material change or for an unresolved finding.
