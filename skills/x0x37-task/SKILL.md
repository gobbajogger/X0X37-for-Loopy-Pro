---
name: x0x37-task
description: "Start an x0x37 task or continuation with current ownership, isolated lanes, source protection, and verified results."
---

Initialize this skill when its description matches the current step. Read it once per task.
Load linked skills only when their described step applies. Current owner instructions take priority.

# Start an x0x37 task

Search relevant product decisions and behavior. Old plans, rosters, logs, and archives do not assign work or grant approval.

### Rule 1: protect data and sources

- Keep code on owner machines and approved local repositories.
- Push to an external repository only when the owner requests that destination.
- Never push, merge, or package `refs-oscuro`.
- Keep proprietary JieLi SDK and toolchain files out of commits and bundles.
- Preserve licensing, model guards, flash and RAM limits, features, and sample quality.

### Rule 2: process and edit ownership

- Start each task in a fresh chat. This includes work continued from another chat.
  Complete the task in its assigned fresh chat. Do not create another copy of that task.

  For a staged workflow, use [rules 17 and 18](../x0x37-workflow/SKILL.md).
- Use old chats for context and handoff details. The owner permits new agents to request that information.
  The owner permits requests for a completion notice from a working chat. The owner also permits that reply.

  Continue in the fresh chat when the notice arrives. Do not use polling or a timer automation.
  Do not assign new execution work to an old chat.
- At task start, check other project chats. Read their current status and scope before shared work.
  Check again before shared changes if that information can be out of date.

  Do not duplicate work or compete for ownership.
- Use a separate lane for each task. Preserve other work.
  Stop only your own processes by PID. Never use `pkill -f` or `killall` on shared scripts.

### Rule 6A: finish authorized work

- Use development builds and installs by default. Apply [rule 11](../x0x37-integrate/SKILL.md) before a release build or full gate.
- Complete the authorized work and its required checks. Carry existing permission forward under [rule 14](../x0x37-owner-decisions/SKILL.md).
  A missing device blocks only work that needs that device. Retired roles do not block work.

  State the real blocker and next action. Do not ask for the same permission again.
- Report only verified results. Identify source review, host tests, target builds, gates, installs, and device tests separately.
  Include relevant inputs and checks that did not run. A passing check does not create permission outside the owner's scope.
- Use the [strict STE default](../x0x37-writing/SKILL.md) for all output, including saved text.
  Give the result first. Keep facts, conditions, uncertainty, and evidence.

  Initialize [x0x37-writing](../x0x37-writing/SKILL.md) before output.
  Read its STE skill and strict references once per task.
  Load other skills only when their triggers apply.
