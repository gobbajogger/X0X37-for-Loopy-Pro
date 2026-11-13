# Project context and execution

Use the owner's current rules for the **LoopyX0X37** project.

## Where work runs

- Builds and host tests run in classic ChatGPT Chat threads in the LoopyX0X37 project. Start a NEW classic thread for each stage. Never use ChatGPT Work or Codex for builds or tests.
- The ChatGPT Orchestrator only coordinates: plans, handoffs, QA and merge-readiness. It cannot start threads and it never builds.
- A big task goes first to the ChatGPT Orchestrator for a handoff. Then start a new classic thread at Extra High with that handoff. Use Pro only for hard builder tasks.
- If a long chat fails to load, start a new thread only after you verify the newest head on GitHub.
- Rule 1m: no git over the network in chats. Use the GitHub plugin or the GitHub API (zipball or tree) to get source, and use SDK V1.2.1 from the project files.
- `x0x37-capability-first` is a ChatGPT plugin, not a repository script.
- Merge bar: a full cloud pass (the full host suite plus the Elite and Pro builds, 0 fail and 0 skip), or the box result `Box host-suite check: PASS` plus a PR comment that the firmware builds pass.
- The Mac is only for hardware installs and device checks.
- Install only when all queued features are ready. Before the install, back up the loop slots. After the install, read back and compare the per-slot CRC, and check the slots.

## Handoffs

Give each new classic thread a compact handoff from the ChatGPT Orchestrator: project, lane, exact base revision on GitHub, requested operations, relevant skill paths, evidence and the result needed. Return the result and evidence to the ChatGPT Orchestrator for QA and merge-readiness. A saved prompt is not a started thread.

For code work, select the explicit tstack mode in [x0x37-workflow](../../x0x37-workflow/SKILL.md), then apply firmware proof where relevant. Device installation stays separate and needs current action-specific owner permission. Keep data safeguards, sole firmware integration ownership and existing action permissions.

This replaces the earlier routing that sent execution to Codex or Work.
