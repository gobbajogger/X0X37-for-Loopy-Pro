---
name: x0x37-workflow
description: "Develop x0x37 on dot's cloud machine with supported native delegation, staged checks, and one integration owner. Excludes docs-only tasks."
---

# Code workflow

### Rule 17: logical stages and bounded execution

Use this order: Audit → Implement → Test → Validate → Merge → Install → Push.
These are logical steps, not required Codex threads. Apply [development routing](../x0x37-task/references/routing.md).
Preserve every applicable stage. Keep necessary authorized execution together where ownership permits. Reuse matching evidence.
Supported chains and subagents are permitted under routing and [helper scope](../x0x37-shared-work/SKILL.md).
A rules-only request does not start a firmware workflow.
Before the assigned stage or a repair, read [its procedure](references/stages.md).
It defines checked commits, development defaults, source changes, permissions, sole integration ownership, and exact install evidence.

### Rule 18: return results and verify necessary handoffs

For a handoff, read [the handoff procedure](references/handoff.md) after current required checks pass.
Return results directly to the coordinating task or owner. No ordinary-chat relay is required.
Keep useful native delegation within the assigned scope; do not launch another task solely because the stage changed.
A necessary execution transfer needs a named receiver, verified inputs, and verified startup.
If the required tool or permission is unavailable, state the exact blocked action and continue independent work.
Do not advance failed checks or skip a blocked installation to push.
