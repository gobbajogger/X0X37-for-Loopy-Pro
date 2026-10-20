---
name: x0x37-workflow
description: "Run or hand off an authorized x0x37 code workflow across fresh stage threads, including successor startup and recovery. Excludes docs-only tasks."
---

# Code workflow

### Rule 17: one fresh thread per stage

Use this order: Audit → Implement → Test → Validate → Merge → Install → Push.
Use one fresh chat per stage. Subagents support that stage; they do not replace its chat.
A rules-only request does not start a firmware workflow.
Before the assigned stage or a repair, read [its procedure](references/stages.md).
It defines checked commits, development defaults, source changes, permissions, sole integration ownership, and exact install evidence.

### Rule 18: verify the successor before finishing

If a next stage is required, read [the handoff procedure](references/handoff.md) after current required checks pass.
It defines saved prompts, one successor, actual startup verification, automatic launch permission, recovery, and completion notices.
Creation or an active flag alone is insufficient.
Before finishing a stage with a required successor, verify that successor has started.
Do not advance failed checks or skip a blocked installation to push.
