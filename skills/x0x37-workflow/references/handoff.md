# Launch and verify the next stage

If no stage remains, end the workflow. Otherwise, the recorded owner starts one fresh chat after current required checks pass.
Create exactly one successor for the next required stage.
Do not ask the owner to launch it or create all stages in advance. The receiver does its assignment; it does not create itself again.
The owner permits automatic launch, recovery handoffs, startup checks, and completion notices within the requested workflow.
Carry that permission and its source in every prompt. Use [rule 14](../../x0x37-owner-decisions/SKILL.md) for action scope.

1. Save the complete prompt and successor record in the lane's evidence folder before creation.
   Include:
   - A unique workflow/stage marker, source chat, and current skill entry.
   - Required skill names and paths, lane, repository, and exact revision.
   - Approved scope, missing decisions, commits, validation inputs, logs, artifacts, failures, and completion criteria.
   Name the next stage and one receiver. Preserve every attempt and error.
2. Check the saved record and project activity, including idle and setup chats, for an existing successor.
   Use `list_projects`, then `create_thread`. Use local execution unless the owner selected another environment. Keep model defaults.
   Tell the receiver to begin now, read the handoff, and initialize `x0x37-task`, `x0x37-workflow`, and its stage skills.
   Record host and chat IDs immediately. A pending client ID is not a chat ID; resolve setup with supported status tools.
3. Verify startup with `read_thread` or `wait_threads`.
   Require a new assistant response or tool activity that acknowledges the assignment or reads its inputs. A successful completed turn qualifies.
   Creation, title, queued input, or an active flag alone is insufficient. Failed/interrupted turns and self-launch requests do not qualify.
   Save the observed turn ID and evidence. Mark `started`, report the receiver, and end this stage without waiting for its completion.
4. Until startup is verified, keep the handoff `pending` or `blocked`.
   Before a retry, inspect IDs, activity, and matching contents. Use bounded status waits, not timer automations.
   A transient service/setup error permits one retry only after a fresh check confirms no successor exists.
   After a timeout or lost response, do not create another chat while the outcome is uncertain.
   If an existing successor failed before work began, verify no active or queued turn/input. Recover that same chat with the saved prompt.
   The recorded sender may make that recovery turn. Do not resend to a running or completed stage. Preserve the error and recovery result.
   Resolve access, setup, permission denials, and policy refusals; do not bypass them.
   Permission-setting changes need explicit owner permission, including any existing permission for automatic handoff tools.
   If blocked, report the dependency, saved prompt/record, successor ID, and next action.

A failed or blocked stage is incomplete. Finish authorized work or launch its required fresh repair stage with a concrete handoff.
A missing device leaves installation pending; do not skip to push or report failed checks as success.
Only a real missing permission pauses dependent work. Use valid evidence again; do not restart completed work.
Use [rule 2](../../x0x37-task/SKILL.md) for context and completion notices.
Report unresolved outcomes truthfully. These checks cannot guarantee service availability.
