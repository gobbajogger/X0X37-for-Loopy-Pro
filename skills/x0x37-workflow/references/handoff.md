# Hand back results

After required checks pass, return a compact result to the coordinating task or owner:
exact revision, checked inputs, commands and logs, artifacts, reused evidence, gaps,
remaining work, and one receiving owner. No ordinary-chat manual relay is required.
Keep applicable stages with their assigned owner; a stage change alone needs no new task.
Use [development routing](../../x0x37-task/references/routing.md) and
[helper scope](../../x0x37-shared-work/SKILL.md) for independent native tasks.

For a necessary transfer to another executor or integration owner:

1. Record a unique task marker, source and receiving task, project, lane, exact revision,
   tool need, relevant skill paths, owned files, authorization, evidence, failures, and completion condition.
2. Check that no matching receiver exists, including idle or setup tasks. Use only the
   currently supported tool and verified destination. Keep model defaults. Record returned IDs.
   Do not assume inherited context transfers files or permissions; the receiver verifies local inputs.
3. Verify assistant or tool activity that acknowledges the assignment or reads its inputs.
   Creation, a title, queued input, or an active flag alone does not prove startup.
   Record startup evidence and explicit ownership transfer. The coordinator retains responsibility
   for dependent work and the final result; a helper launch is not task completion.
4. Keep uncertain launches pending. Inspect existing IDs before retrying; never duplicate
   an uncertain task. Retry a transient launch only after confirming no receiver exists.
   Recover an unstarted receiver only through an authorized channel without conflicting input.
   Respect refusals; do not change approval settings or switch routes to bypass them.

If a necessary tool or permission is missing, report the exact action, target, reason,
and pending status. Prepare a handoff only when it helps; do not invent a mandatory
owner paste step. Continue work that does not depend on the blocker.
Failed checks need repair and affected checks before advancement. A missing device leaves
installation pending; do not skip it to push. Reuse valid evidence and preserve
[existing permission](../../x0x37-owner-decisions/SKILL.md) and
[task ownership](../../x0x37-task/references/ownership.md).
