# Code workflow

### Rule 17: one fresh thread per stage

Use this order for an authorized code workflow. Start each stage in a fresh chat.
Subagents support a stage. They do not replace its chat.

A rules-only request does not start a firmware audit or release cycle.

1. **Audit.** Inspect the specified revision and intended behavior.
   Record defects, evidence, affected scope, proposed fixes, existing approvals, and missing decisions.
2. **Implement approved fixes.** Use the audit and recorded approvals.
   Hand off checked commits and deferred items. Ask only for decisions that are still missing.
3. **Test and investigate.** Test the complete candidate from stage 2.
   Use subagents with separate test and investigation scopes.

   Record passing checks and each failure's cause and disposition.
   A source repair requires a fresh stage 2, then a fresh stage 3 for affected checks.
4. **Validate builds and packages.** Use the tested candidate, target configurations, and dependencies.
   Record exact build and install-package identities, checks, and logs under [rule 11](integration.md) and [rule 6](install.md).
5. **Merge.** Wait for all contributing lanes to hand off their validated commits.
   The sole assigned integrator merges the complete set. Record the resulting revision and validation identity.
6. **Install.** Use the merged revision, matching artifacts, and existing owner permission under [rule 14](owner-messages.md).
   The designated installer records the install and data checks under [rules 5–7](install.md).

   Do not add another approval question for this stage.
7. **Push to Git.** Use the completed merge and install evidence and the approved repository, ref, and push scope.
   Verify that the remote ref matches the intended commit. End the workflow. Do not start another audit automatically.

Use [scoped checks and matching evidence](commit.md). Stage 4 uses development configuration by default.
Stage order does not request a release build. Apply [rule 11](integration.md) before release work or a full gate.

A public release package also needs the owner's device PASS for the same installed build.
Start that package task in a fresh chat when requested and when its evidence exists.

A Git push does not authorize public-release packaging or publication.

Before installation, compare the merged result with the inputs checked in stage 4.
A new commit ID alone does not invalidate matching evidence.

Changed checked source, configuration, tools, dependencies, or artifacts require a fresh chat for the earliest affected stage.
After those checks pass, use a fresh install chat. Do not repeat a completed merge without another merge to do.

Stage 5 receives sole firmware integration ownership through its handoff.
Check project activity and the target branch before shared changes. Do not compete with another integrator.

Another merge must pass the applicable validation and merge stages.
Preserve [rule 1](core.md). Carry valid owner permission forward under [rule 14](owner-messages.md).

Stage changes do not permit an unspecified external destination, publication, release, or new defect fix.

### Rule 18: verify the successor before finishing

After the current stage's work and required checks pass, its recorded owner creates and starts the next required stage's chat.
Create exactly one fresh successor chat.

Do not ask the owner to launch it. Do not create all stages in advance.
The receiving chat does its assigned stage. It does not create another copy of itself.

The owner permits automatic launch, recovery handoffs, startup checks, and completion notices within the requested workflow.
This includes a recovery turn by the recorded sender when its successor did not start.

Carry this permission and its source into each prompt. Do not ask for the same permission again.
For installation and other action scope, apply [rule 14](owner-messages.md).

1. **Save the handoff before creation.** Save the complete prompt and successor record in the lane's evidence folder.
   Include a unique workflow and stage marker, source chat ID, current rules entry, lane, repository, and exact revision.

   Include approved scope, actual missing decisions, commits, validation inputs, logs, artifacts, failures, and completion criteria.
   Identify the next stage and one receiving owner. Preserve every attempt and error.
2. **Check for a successor.** Check the record and project activity, including idle chats and chats in setup.
   Use `list_projects` to select the project. Use `create_thread` with local execution unless the owner selected another environment.

   Tell the chat to begin now, read the saved handoff, and apply rule 18. Keep model defaults.
   Record returned host and chat IDs immediately. A pending client ID is not a chat ID.

   Resolve pending setup with supported status tools before use.
3. **Check actual startup.** Use `read_thread` or `wait_threads`.
   Require a new assistant response or tool activity that acknowledges the assignment or reads its inputs.

   A completed successful turn also qualifies. Creation, a title, queued input, or an active flag alone does not qualify.
   A failed or interrupted turn, or a request to launch itself, does not qualify.

   Save the observed turn ID and evidence. Then mark `started`, report the receiving chat, and end the current stage.
   Do not wait for the next stage to finish.
4. **Resolve a failed launch.** Keep the handoff `pending` or `blocked` until you verify actual startup.
   Inspect saved IDs, activity, and matching chat contents before a retry. Use bounded status waits, not timer automations.

   For a transient service or setup error, permit one retry after a fresh check confirms that no successor exists.
   After a timeout or lost response, do not create another chat while the result is uncertain.

   If the existing successor failed before work began, verify that no turn or input is active or queued.
   Then recover that same chat with the saved prompt. The owner permits this handoff message.

   Do not resend to a running or completed stage. Preserve the original error and recovery result.
   Resolve permission denials, policy refusals, missing access, and unresolved setup. Do not bypass them.

   Change permission settings only with explicit owner permission, including existing permission for automatic handoff tools.
   If recovery remains blocked, report the exact dependency, saved prompt and record, successor ID, and next action.

A failed or blocked stage is incomplete. Finish its authorized work or start the required fresh repair stage with a concrete handoff.
Do not advance failed checks as a success. A missing device leaves installation pending. Do not silently skip to push.

Only a real missing permission pauses its dependent action. Do not invent a new approval requirement.
Reuse valid evidence. Do not restart completed work. Use [rule 2](core.md) for context and completion notices.

Report unresolved results truthfully. The startup checks cannot guarantee service availability.
