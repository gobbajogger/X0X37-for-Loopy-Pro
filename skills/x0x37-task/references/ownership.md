# Task ownership and shared work

- Start each task in a fresh chat. This includes work continued from another chat.
  Complete the task in its assigned fresh chat. Do not create another copy of that task.

  Apply [chat routing](routing.md) before choosing its destination.
  Logical workflow stages do not each need another chat; use the [current workflow](../../x0x37-workflow/SKILL.md) and its [handoff procedure](../../x0x37-workflow/references/handoff.md).
- Use old chats for context and handoff details. The owner permits new agents to request that information.
  The owner permits requests for a completion notice from a working chat. The owner also permits that reply.

  Continue in the fresh chat when the notice arrives. Do not use polling or a timer automation.
  Do not assign new execution work to an old chat.
- At task start, check other project chats. Read their current status and scope before shared work.
  Check again before shared changes if that information can be out of date.

  Do not duplicate work or compete for ownership.
- Use a separate lane for each task. Preserve other work.
  Stop only your own processes by PID. Never use `pkill -f` or `killall` on shared scripts.
