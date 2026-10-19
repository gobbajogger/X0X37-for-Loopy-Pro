# Shared work

Read when: before hardware work, a shared-file edit or a hard-to-undo action, when you wait, and when you start a new change.

### Rule 3
Step: all steps. Rule owner: Orchestrator.

Run hardware work and edits to one shared file one at a time. Never edit a shared script while it runs. While you wait for a result, do the next task that does not need it. If you have no task, tell Orchestrator.

If the Mac does not answer, try again for 5 minutes. Report the Mac offline only after 5 minutes with no answer.

### Rule 4: Decision check
Step: each shared or hard-to-undo action. Rule owner: Orchestrator.

1. Evidence: Which fact shows that the action is correct? A fact against it wins.
2. Damage: If the device, the owner data or another agent can break, use the safe path.
3. Undo: If you cannot undo the action, obey the rule or ask the task agent.
4. Cause: Do not go around a failure. Find its cause.
5. Size: Make the smallest change.

Stop and check again if you think or write "probably", "should work" or "just this once". Do the same for "it works" with no build.

### Rule 16
Step: P2 start of a change. Rule owner: Orchestrator.

Start a new helper subagent for each new change. Give it only the task, the inputs (commit, branch, files), the rule files of its steps and the done criteria. Do not resume an old helper for a new change. Resume a helper only for a fix to its own change. Orchestrator also follows this rule.
