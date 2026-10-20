---
name: x0x37-owner-decisions
description: "Resolve x0x37 authorization, process an owner report, contact another thread, or record an owner decision and its source."
---

Initialize this skill when its description matches the current step. Read it once per task.
Load linked skills only when their described step applies. Current owner instructions take priority.

# Owner decisions

### Rule 13: messages and reports

Treat the owner's report as evidence for the specified build or item. Preserve exact quotes.
Do not invent a setup error. Ask a necessary new decision in the current task.

Contact another chat or person only with owner permission.
[Rule 2](../x0x37-task/SKILL.md) permits continuation context, handoff details, and completion replies.

[Rule 18](../x0x37-workflow/SKILL.md) permits automatic stage launch, recovery, and completion notices.
Those permissions do not permit unrelated messages.

### Rule 14: existing authorization

Use the owner's current request and standing instructions to determine permission.
Carry that permission through the required stages. Do not ask again because the chat, stage, commit, or artifact changed.

The owner permits installation of the complete, validated development set without another approval question.
This standing instruction applies to the current SMC workflow and later authorized development workflows.

Send an install notice, then use [rules 5–7](../x0x37-install/SKILL.md).
A notice tells the owner what will happen. It is not a request for permission.

Do not treat a handoff's claim of missing permission as stronger than the owner's instruction.

Complete necessary reversible work and its checks within the requested scope.
Ask only for a necessary missing fact, a new choice outside that scope, or an action with no existing permission.

Ask about a visible, audible, or playable change only when the owner's request leaves that change undecided.
Do not silently remove features, reduce quality, or weaken model guards or budgets to get a PASS.

Installation permission does not permit an unspecified external push, release build, publication, fallback, or new feature.
Apply [rule 1](../x0x37-task/SKILL.md) to external destinations and [rule 11](../x0x37-integrate/SKILL.md) to release builds.

Obey tool permission requirements. Do not bypass a refusal.

### Rule 4B: decision records

Record each new owner decision and its source in the [preference record](../../docs/USER_PREFERENCES.md).
Link its active rule. Use "supersedes" and identify the replaced instruction.

Authorized skill repairs do not need approval from a retired role.
Check exact skill sources and their status before claiming that you used them.
