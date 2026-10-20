# Hand back results

After required checks pass, record the result, exact revision, inputs, logs, artifacts, remaining work,
and originating classic ChatGPT thread in LoopyX0X37. Return that compact result through an authorized channel.
If no work remains, end the workflow. Do not automatically create another Codex thread for the next logical stage.
Use [chat routing](../../x0x37-task/references/routing.md) for the next task and its project destination.

If the owner explicitly requested a separate execution task and its tools are necessary:

1. Save its prompt and record before launch: unique task marker, source chat, project, lane, exact revision,
   tool requirement, relevant skill paths, approved scope and permission source, evidence, failures, and completion condition.
2. Check that no matching receiver exists, including idle and setup chats. Select the correct local project
   with `list_projects`; use `create_thread` only for the requested Codex task. Keep model defaults.
   Record its host and chat IDs. A pending client ID is not a chat ID.
3. Verify new assistant or tool activity that acknowledges the assignment or reads its inputs.
   Creation, title, queued input, or an active flag alone is insufficient. Record the turn and startup evidence.
   End the sender's task after verified startup; do not wait for the receiver's full task.
4. Keep uncertain launches pending. Inspect existing IDs before retrying; never duplicate an uncertain task.
   One transient retry is permitted only after confirming no receiver exists. Recover an unstarted receiver
   only through an authorized channel, with no active or queued input. Preserve attempts and errors.
   Respect tool permission requirements and policy refusals; do not change approval settings as a workaround.

Classic ChatGPT creation must retain the LoopyX0X37 project and classic mode.
If no supported tool can do that, provide the saved prompt and project link for the owner to launch.
Do not substitute a general chat, ChatGPT Work, cloud agent, or another Codex task.
Report `prompt ready; owner launch needed` rather than claiming startup or waiting for unavailable automation.

Failed checks need repair and affected checks before advancement. A missing device leaves installation pending;
do not skip it to push or report failed checks as success. Reuse valid evidence and finish already authorized work.
Existing action permission continues under [rule 14](../../x0x37-owner-decisions/SKILL.md).
Use [rule 2](../../x0x37-task/SKILL.md) for authorized context and completion notices.
