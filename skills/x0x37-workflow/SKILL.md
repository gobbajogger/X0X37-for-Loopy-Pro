---
name: x0x37-workflow
description: "Run bounded x0x37 code execution and hand results back to classic ChatGPT, preserving stage checks and ownership. Excludes docs-only tasks."
---

# Code workflow

### Rule 17: logical stages and bounded execution

Use this order: Audit → Implement → Test → Validate → Merge → Install → Push.
These are logical steps, not required Codex threads. Apply [chat routing](../x0x37-task/references/routing.md).
Preserve every applicable stage. Keep necessary authorized execution together where ownership permits. Reuse matching evidence.
Supported chains and subagents are permitted under routing and [helper scope](../x0x37-shared-work/SKILL.md).
A rules-only request does not start a firmware workflow.
Before the assigned stage or a repair, read [its procedure](references/stages.md).
It defines checked commits, development defaults, source changes, permissions, sole integration ownership, and exact install evidence.

### Rule 18: return results and verify necessary handoffs

For a handoff, read [the handoff procedure](references/handoff.md) after current required checks pass.
Return results to the originating classic project thread. Continue applicable reasoning stages there.
Do not launch another Codex task solely because the stage changed.
An explicitly requested execution receiver needs a concrete tool requirement and verified startup.
If classic chat creation is unavailable, save its prompt and report the owner launch step.
Do not advance failed checks or skip a blocked installation to push.
